/*
 * Copyright (c) 2016, Onset Computer Corp.
 * All rights reserved. 
 */
#include "opendefs.h"
#include "hello.h"
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
#include "welcome.h"

//=========================== defines =========================================

// Delay before sending POST (in mseconds)
#define INIT_DELAY     30000

const uint8_t hello_path0[]  = "lo";

//=========================== variables =======================================

hello_vars_t hello_vars;

//=========================== prototypes ======================================

void      hello_task(void);
void      hello_timer(opentimer_id_t id);
void      hello_sendDone(OpenQueueEntry_t* msg,
                         owerror_t error);
owerror_t hello_receive(OpenQueueEntry_t* msg,
                        coap_header_iht* coap_header,
                        coap_option_iht* coap_options);
void      hello_conRetry(void);
void      hello_conFail(OpenQueueEntry_t* msg);
void      hello_start_timer(void);

//=========================== public ==========================================

void hello_init() {
   // prepare the resource descriptor for the /nh/hello path
   hello_vars.desc.path0len             = sizeof(hello_path0)-1;
   hello_vars.desc.path0val             = (uint8_t*)(&hello_path0);
   hello_vars.desc.path1len             = 0;
   hello_vars.desc.path1val             = NULL;
   hello_vars.desc.componentID          = COMPONENT_HELLO;
   hello_vars.desc.callbackRx           = &hello_receive;
   hello_vars.desc.callbackSendDone     = &hello_sendDone;
   hello_vars.desc.callbackConRetry     = &hello_conRetry;
   hello_vars.desc.callbackConFail      = &hello_conFail;
   // initialize to sentinel value to know if running
   hello_vars.timerId                   = MAX_NUM_TIMERS;
   
   hello_start_timer();
   opencoap_register(&hello_vars.desc);
}

//=========================== private =========================================

owerror_t hello_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   // Only useful for non-confirmable messaging
   //opentimers_stop(hello_vars.timerId);

   // Set response code if received a *request*, and schedule welcome message.
   if (
      coap_header->Code>=COAP_CODE_REQ_GET &&
      coap_header->Code<=COAP_CODE_REQ_DELETE
   ) {
      coap_header->Code = COAP_CODE_RESP_VALID;
      if (idmanager_getIsDAGroot()==TRUE) {
         welcome_send(&msg->l3_sourceAdd);
      }
   }
   return E_SUCCESS;
}

void hello_timer(opentimer_id_t id) {
   hello_vars.timerId = MAX_NUM_TIMERS;
   // Defer task execution from this ISR until CoAP priority.
   scheduler_push_task(hello_task,TASKPRIO_COAP);
}

void hello_start_timer() {
   if (
      hello_vars.timerId==MAX_NUM_TIMERS && 
      hello_vars.desc.confirmable.msg == NULL
   ) {
      openserial_printInfo(COMPONENT_HELLO, ERR_DEBUG, 0, 0);
      hello_vars.timerId = opentimers_start(INIT_DELAY,
                                            TIMER_ONESHOT,TIME_MS,
                                            hello_timer);
   }
}

void hello_task() {    
   OpenQueueEntry_t* pkt;
   owerror_t         outcome;
   uint8_t           dagroot[16];

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE) {
      hello_start_timer();
      return;
   }

   // don't run on dagroot
   if (idmanager_getIsDAGroot()==TRUE) {
      return;
   }

   if (icmpv6rpl_getOpStatus() != RPL_STATUS_RUNOK) {
      hello_start_timer();
      return;
   }
   
   // create a CoAP packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_HELLO);
   if (pkt==NULL) {
      openserial_printError(COMPONENT_HELLO,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      return;
   }

   // take ownership over that packet
   pkt->creator    = COMPONENT_HELLO;
   pkt->owner      = COMPONENT_HELLO;

   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(hello_path0)-1);
   memcpy(&pkt->payload[0],&hello_path0,sizeof(hello_path0)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = COAP_OPTION_NUM_URIPATH << 4 | (sizeof(hello_path0)-1);
   
   // metadata
   pkt->l4_destination_port    = WKP_UDP_COAP;
   // Send to DAGroot
   icmpv6rpl_getRPLDODAGid(&dagroot[0]);
   pkt->l3_destinationAdd.type = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&dagroot[0],16);
   // send
   outcome = opencoap_send(pkt,
                           COAP_TYPE_CON,
                           COAP_CODE_REQ_POST,
                           2,
                           &hello_vars.desc);
   // avoid overflowing the queue if fails
   if (outcome==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
      hello_start_timer();
   }
}

void hello_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   // Only useful for server-side (non-confirmable) ACK
   if (hello_vars.desc.confirmable.msg==NULL) {
      openqueue_freePacketBuffer(msg);
   }
}

// Boilerplate to resend a confirmable
void hello_conRetry() {
   if (hello_vars.desc.confirmable.msg!=NULL) {
      opencoap_resend(&hello_vars.desc.confirmable);
   }
}

// If confirmable fails, wait and try again.
void hello_conFail(OpenQueueEntry_t* msg) {
   openqueue_freePacketBuffer(msg);
   hello_start_timer();
}
