/*
 * Copyright (c) 2014, Ken Bannister
 * All rights reserved. 
 *  
 * Released under the Mozilla Public License 2.0, as published at the link below.
 * http://opensource.org/licenses/MPL-2.0
 */
#ifndef __CNETHEAD_H
#define __CNETHEAD_H

/**
\addtogroup AppCoAP
\{
\defgroup nethead Nethead

\brief Management and monitoring server

\author Ken Bannister <kb2ma@runbox.com>, December 2014.
\{
*/

#include "opendefs.h"

//=========================== define ==========================================

/**
\brief Address of the nethead client for phone home 
*/
extern const uint8_t nethead_home_addr[];

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void cnethead_init(void);

void cnethead_notify_registered(void);

/**
\}
\}
*/

#endif
