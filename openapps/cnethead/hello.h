/*
 * Copyright (c) 2014, Ken Bannister
 * All rights reserved. 
 *  
 * Released under the Mozilla Public License 2.0, as published at the link below.
 * http://opensource.org/licenses/MPL-2.0
 */
#ifndef __HELLO_H
#define __HELLO_H

#include "opencoap.h"
#include "opentimers.h"

/**
\addtogroup AppCoAP
\{
\addtogroup nethead
\{
\defgroup hello hello
\brief Boostraps the management and monitoring server implementation.

When the mote boots, 'hello' schedules POSTing `nh/lo` to an internally designated 
host (nethead_home_addr[]), that is a client for monitoring and management. 

\author Ken Bannister <kb2ma@runbox.com>, December 2014.
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
} nhlo_vars_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

void hello_init(void);

/**
\}
\}
\}
*/

#endif
