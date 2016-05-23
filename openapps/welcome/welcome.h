/*
 * Copyright (c) 2016, Onset Computer Corp.
 * All rights reserved. 
 */
#ifndef __WELCOME_H
#define __WELCOME_H

#include "opencoap.h"
#include "opentimers.h"
#include "opendefs.h"

/**
\addtogroup AppCoAP
\{
\addtogroup welcome
\{
*/

//=========================== define ==========================================

//=========================== typedef =========================================

/**
\brief Package for hello CoAP endpoint variables, as well as internal state.
*/
typedef struct {
   opentimer_id_t       timerId;
   coap_resource_desc_t desc;
   open_addr_t          dest_addr;
   bool                 isConfirmable;
} welcome_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void welcome_init(void);

void welcome_send(open_addr_t* destination);

/**
\}
\}
*/

#endif
