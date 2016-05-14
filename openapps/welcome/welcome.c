/*
 * Copyright (c) 2016, Onset Computer Corp.
 * All rights reserved. 
 */
#include "opendefs.h"
#include "welcome.h"
#include "opentimers.h"
#include "opencoap.h"
#include "openqueue.h"
#include "idmanager.h"
#include "packetfunctions.h"
#include "openserial.h"
#include "openrandom.h"
#include "IEEE802154E.h"
#include "scheduler.h"
#include "eui64.h"
#include "icmpv6rpl.h"

//=========================== defines =========================================

// Delay before sending POST (in mseconds)
#define INIT_DELAY     10000
// Delay if initial send fails
#define RETRY_DELAY    20000

const uint8_t welcome_path0[]  = "wc";

//=========================== variables =======================================

welcome_vars_t welcome_vars;

//=========================== prototypes ======================================

void      welcome_task(void);
void      welcome_timer(opentimer_id_t id);
void      welcome_sendDone(OpenQueueEntry_t* msg,
                         owerror_t error);
owerror_t welcome_receive(OpenQueueEntry_t* msg,
                        coap_header_iht* coap_header,
                        coap_option_iht* coap_options);
void      welcome_conRetry(void);
void      welcome_conFail(OpenQueueEntry_t* msg);
void      welcome_start_timer(uint32_t delay);

//=========================== public ==========================================

void welcome_init() {
   // prepare the resource descriptor for the /nh/welcome path
   welcome_vars.desc.path0len             = sizeof(welcome_path0)-1;
   welcome_vars.desc.path0val             = (uint8_t*)(&welcome_path0);
   welcome_vars.desc.path1len             = 0;
   welcome_vars.desc.path1val             = NULL;
   welcome_vars.desc.componentID          = COMPONENT_WELCOME;
   welcome_vars.desc.callbackRx           = &welcome_receive;
   welcome_vars.desc.callbackSendDone     = &welcome_sendDone;
   welcome_vars.desc.callbackConRetry     = &welcome_conRetry;
   welcome_vars.desc.callbackConFail      = &welcome_conFail;

   memset(&welcome_vars.dest_addr,0,sizeof(open_addr_t));
   welcome_vars.dest_addr.type            = ADDR_NONE;
   // initialize to sentinel value to know if running
   welcome_vars.timerId                   = MAX_NUM_TIMERS;
   
   opencoap_register(&welcome_vars.desc);
}

//=========================== private =========================================

owerror_t welcome_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   // Only useful for non-confirmable messaging
   //opentimers_stop(welcome_vars.timerId);

   // Set response code if received a request
   if (
      coap_header->Code>=COAP_CODE_REQ_GET &&
      coap_header->Code<=COAP_CODE_REQ_DELETE
   ) {
      coap_header->Code = COAP_CODE_RESP_VALID;
      // Must reset payload since we reuse packet for response
      msg->payload      = &(msg->packet[127]);
      msg->length       = 0;

   } else {
      openserial_printInfo(COMPONENT_WELCOME, ERR_DEBUG, 2, 0);
   }
   return E_SUCCESS;
}

/**
\brief Sends the welcome message to a mote.

\param[in] destination for message
*/
void welcome_send(open_addr_t* destination) {
   memcpy(&welcome_vars.dest_addr, destination, sizeof(open_addr_t));
   welcome_start_timer(INIT_DELAY);
}

void welcome_timer(opentimer_id_t id) {
   welcome_vars.timerId = MAX_NUM_TIMERS;
   // Defer task execution from this ISR until CoAP priority.
   scheduler_push_task(welcome_task,TASKPRIO_COAP);
}

void welcome_start_timer(uint32_t delay) {
   if (
      welcome_vars.timerId==MAX_NUM_TIMERS && 
      welcome_vars.desc.confirmable.msg == NULL
   ) {
      openserial_printInfo(COMPONENT_WELCOME, ERR_DEBUG, 0, 0);
      welcome_vars.timerId = opentimers_start(delay,
                                              TIMER_ONESHOT,TIME_MS,
                                              welcome_timer);
   }
}

void welcome_task() {    
   OpenQueueEntry_t* pkt;
   owerror_t         outcome;

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) {
      welcome_start_timer(RETRY_DELAY);
      return;
   }

   // don't run on leaf, and sanity check destination
   if (idmanager_getIsDAGroot()==FALSE || welcome_vars.dest_addr.type==ADDR_NONE) {
      return;
   }

   if (icmpv6rpl_getOpStatus() != RPL_STATUS_RUNOK) {
      welcome_start_timer(RETRY_DELAY);
      return;
   }
   
   // create a CoAP packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_WELCOME);
   if (pkt==NULL) {
      openserial_printError(COMPONENT_WELCOME,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      return;
   }

   // take ownership over that packet
   pkt->creator    = COMPONENT_WELCOME;
   pkt->owner      = COMPONENT_WELCOME;

   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(welcome_path0)-1);
   memcpy(&pkt->payload[0],&welcome_path0,sizeof(welcome_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_OPTION_NUM_URIPATH << 4 | (sizeof(welcome_path0)-1);
   
   // metadata
   pkt->l4_destination_port    = WKP_UDP_COAP;
   // Send to designated mote
   pkt->l3_destinationAdd.type = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&welcome_vars.dest_addr.addr_128b[0],16);
   // send
   outcome = opencoap_send(pkt,
                           COAP_TYPE_CON,
                           COAP_CODE_REQ_POST,
                           2,
                           &welcome_vars.desc);
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      welcome_start_timer(RETRY_DELAY);
   }
}

void welcome_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   // Only useful for server-side (non-confirmable) ACK
   if (welcome_vars.desc.confirmable.msg==NULL) {
      openqueue_freePacketBuffer(msg);
   }
}

// Boilerplate to resend a confirmable
void welcome_conRetry() {
   if (welcome_vars.desc.confirmable.msg!=NULL) {
      opencoap_resend(&welcome_vars.desc.confirmable);
   }
}

// If confirmable fails, give up for now.
void welcome_conFail(OpenQueueEntry_t* msg) {
   openqueue_freePacketBuffer(msg);
   openserial_printInfo(COMPONENT_WELCOME, ERR_DEBUG, 3, 0);
}
