#ifndef __SIXTOP_H
#define __SIXTOP_H

/**
\addtogroup MAChigh
\{
\addtogroup sixtop
\{
*/

#include "opentimers.h"
#include "processIE.h"
#include "openwsn.h"

//=========================== define ==========================================

enum sixtop_CommandID_num{
   SIXTOP_SOFT_CELL_REQ                = 0x00,
   SIXTOP_SOFT_CELL_RESPONSE           = 0x01,
   SIXTOP_REMOVE_SOFT_CELL_REQUEST     = 0x02,
   SIXTOP_HARD_CELL_REQ                = 0x03,
   SIXTOP_REMOVE_HARD_CELL             = 0x04,
};

// the different states of the reservation state machine
typedef enum {
   S_IDLE                              = 0x00,   // ready for next event
   // send,receive and sendDone state of resLinkRequest
   S_SIXTOP_LINKREQUEST_SEND           = 0x01,   // generating resLinkRequest command packet
   S_WAIT_SIXTOP_LINKREQUEST_SENDDONE  = 0x02,   // waiting for SendDone confirmation
   S_SIXTOP_LINKREQUEST_RECEIVE        = 0x03,   // 
   // wait respone command
   S_WAIT_FORRESPONSE                  = 0x04,   // waiting for response from the neighbor
   // send,receive and sendDone state of resLinkRespone
   S_SIXTOP_LINKRESPONSE_SEND          = 0x05,   // generating resLinkRespone command packet       
   S_WAIT_SIXTOP_LINKRESPONSE_SENDDONE = 0x06,   // waiting for SendDone confirmation
   S_SIXTOP_LINKRESPONSE_RECEIVE       = 0x07,
   // send,receive and sendDone state of removeLinkRequest
   S_REMOVELINKREQUEST_SEND            = 0x08,   // generating resLinkRespone command packet  
   S_WAIT_REMOVELINKREQUEST_SENDDONE   = 0x09,   // waiting for SendDone confirmation
   S_REMOVELINKREQUEST_RECEIVE         = 0x0a
} sixtop_state_t;

//=========================== typedef =========================================

#define SIXTOP2SIXTOP_TIMEOUT_MS 2000

//=========================== module variables ================================

typedef struct {
   uint16_t             periodMaintenance;
   bool                 busySendingKa;        // TRUE when busy sending a keep-alive
   bool                 busySendingAdv;       // TRUE when busy sending an advertisement
   uint8_t              dsn;                  // current data sequence number
   uint8_t              MacMgtTaskCounter;    // counter to determine what management task to do
   opentimer_id_t       timerId;
   opentimer_id_t       TOtimerId;            // TimeOut timer id
   uint16_t             kaPeriod;             // period of sending KA
   sixtop_state_t       State;
   uint8_t              commandID;
} sixtop_vars_t;

//=========================== prototypes ======================================

// admin
void      sixtop_init(void);
// from upper layer
owerror_t sixtop_send(OpenQueueEntry_t *msg);
void      sixtop_linkRequest(open_addr_t* sixtopNeighAddr,uint16_t bandwidth);
void      sixtop_linkResponse(bool success,open_addr_t* tempNeighbor,uint8_t bandwidth,sixtop_generalschedule_subIE_t* schedule_ie);
void      sixtop_removeLinkRequest(open_addr_t*  sixtopNeighAddr);
// from lower layer
void      task_sixtopNotifSendDone(void);
void      task_sixtopNotifReceive(void);
// helper functions
void      sixtop_setKaPeriod(uint16_t kaPeriod);
// debugging
bool      debugPrint_myDAGrank(void);

/**
\}
\}
*/

#endif
