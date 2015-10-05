#ifndef __BOARD_H
#define __BOARD_H

/**
\addtogroup BSP
\{
\addtogroup board
\{

\brief Cross-platform declaration "board" bsp module.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012.
*/

#include "board_info.h"
#include "toolchain_defs.h"

//=========================== define ==========================================

typedef enum {
   DO_NOT_KICK_SCHEDULER,
   KICK_SCHEDULER,
} kick_scheduler_t;

#ifdef AT86RF212B
/* This sleep modes are common for all MCU types 
   Depending on the MAC state sleep modes must be selected
   If particular sleep modes are  not exist, it can be combined 
   with other possible sleep modes 
*/
typedef enum {
	BOARD_SLEEP_MODE0,
	BOARD_SLEEP_MODE1,
	BOARD_SLEEP_MODE2,
	BOARD_SLEEP_MODE3,
	BOARD_SLEEP_MODE4,
	BOARD_SLEEP_MODE5,
}board_sleepmode_t;

/* This sleep modes are common for all MCU types 
   Depending on the MAC state sleep modes must be selected 
*/
typedef enum {
	RADIO_SLEEP_MODE0,
	RADIO_SLEEP_MODE1,
	RADIO_SLEEP_MODE2,
	RADIO_SLEEP_MODE3,
	RADIO_SLEEP_MODE4,
	RADIO_SLEEP_MODE5,
}radio_sleepmode_t;
#endif
//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void board_init(void);
#ifdef AT86RF212B

#if defined MCU_SMART_SLEEP
void mcu_smart_sleep(void);
#endif

void board_sleep(board_sleepmode_t sleep_mode);
#else
void board_sleep(void);
#endif
void board_reset(void);

/**
\}
\}
*/

#endif
