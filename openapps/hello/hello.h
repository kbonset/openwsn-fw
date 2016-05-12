/*
 * Copyright (c) 2016, Onset Computer Corp.
 * All rights reserved. 
 */
#ifndef __HELLO_H
#define __HELLO_H

#include "opencoap.h"
#include "opentimers.h"

/**
\addtogroup AppCoAP
\{
\addtogroup hello
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
} hello_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void hello_init(void);

/**
\}
\}
*/

#endif
