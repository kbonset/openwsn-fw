/*
 * Copyright (c) 2014, Ken Bannister
 * All rights reserved. 
 *  
 * Released under the Mozilla Public License 2.0, as published at the link below.
 * http://opensource.org/licenses/MPL-2.0
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
#include "cnethead.h"

//=========================== defines =========================================

// Delay before sending POST (in mseconds)
#define INIT_DELAY     30000

const uint8_t hello_path0[]  = "nh";
const uint8_t hello_path1[]  = "lo";

//=========================== variables =======================================

nhlo_vars_t hello_vars;

//=========================== prototypes ======================================

void      hello_task(void);
void      hello_timer(void);
void      hello_sendDone(OpenQueueEntry_t* msg,
                         owerror_t error);
owerror_t hello_receive(OpenQueueEntry_t* msg,
                        coap_header_iht* coap_header,
                        coap_option_iht* coap_options);

//=========================== public ==========================================

void hello_init() {
   // prepare the resource descriptor for the /nh/hello path
   hello_vars.desc.path0len             = sizeof(hello_path0)-1;
   hello_vars.desc.path0val             = (uint8_t*)(&hello_path0);
   hello_vars.desc.path1len             = sizeof(hello_path1)-1;
   hello_vars.desc.path1val             = (uint8_t*)(&hello_path1);
   hello_vars.desc.componentID          = COMPONENT_CNETHEAD;
   hello_vars.desc.callbackRx           = &hello_receive;
   hello_vars.desc.callbackSendDone     = &hello_sendDone;
   

   hello_vars.timerId    = opentimers_start(INIT_DELAY,
                                          TIMER_PERIODIC,TIME_MS,
                                          hello_timer);
   opencoap_register(&hello_vars.desc);
}

//=========================== private =========================================

owerror_t hello_receive(OpenQueueEntry_t* msg,
                      coap_header_iht* coap_header,
                      coap_option_iht* coap_options) {
   
   opentimers_stop(hello_vars.timerId);
   cnethead_notify_registered();
   return E_SUCCESS;
}

void hello_timer() {
   // Defer task execution from this ISR until CoAP priority.
   scheduler_push_task(hello_task,TASKPRIO_COAP);
}

void hello_task() {    
   OpenQueueEntry_t* pkt;
   owerror_t         outcome;
   uint8_t           dagroot[16];

   // don't run if not synch
   if (ieee154e_isSynch() == FALSE)
      return;
   
   // don't run on dagroot
   if (idmanager_getIsDAGroot()) {
      opentimers_stop(hello_vars.timerId);
      return;
   }

   if (icmpv6rpl_getOpStatus() != RPL_STATUS_RUNOK)
      return;
   
   // create a CoAP packet
   pkt = openqueue_getFreePacketBuffer(COMPONENT_CNETHEAD);
   if (pkt==NULL) {
      openserial_printError(COMPONENT_CNETHEAD,ERR_NO_FREE_PACKET_BUFFER,
                            (errorparameter_t)0,
                            (errorparameter_t)0);
      openqueue_freePacketBuffer(pkt);
      return;
   }
   // take ownership over that packet
   pkt->creator    = COMPONENT_CNETHEAD;
   pkt->owner      = COMPONENT_CNETHEAD;

   // location-path option
   packetfunctions_reserveHeaderSize(pkt,sizeof(hello_path1)-1);
   memcpy(&pkt->payload[0],&hello_path1,sizeof(hello_path1)-1);
   packetfunctions_reserveHeaderSize(pkt,1);
   pkt->payload[0] = 0 << 4 | (sizeof(hello_path1)-1);
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
   }
   
   return;
}

void hello_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
