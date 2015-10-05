/**
* Copyright (c) 2014 Atmel Corporation. All rights reserved. 
*  
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions are met:
* 
* 1. Redistributions of source code must retain the above copyright notice, this
* list of conditions and the following disclaimer.
* 
* 2. Redistributions in binary form must reproduce the above copyright notice, 
* this list of conditions and the following disclaimer in the documentation 
* and/or other materials provided with the distribution.
* 
* 3. The name of Atmel may not be used to endorse or promote products derived 
* from this software without specific prior written permission.  
* 
* 4. This software may only be redistributed and used in connection with an 
* Atmel microcontroller product.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* 
* 
* 
*/

/* === INCLUDES ============================================================ */
#include "sam.h"
#include "samd21_extint.h"
#include "samd21_flash.h"
#include "samd21_gclk.h"
#include "samd21_gpio.h"
#include "samd21_radio.h"
#include "samd21_spi.h"
#include "samd21_sysctrl.h"
#include "samd21_timer.h"
#include "samd21_uart.h"
#include "cm0plus_interrupt.h"
#include "base_type.h"
#include "board.h"
#include "uart.h"
#include "opentimers.h"
#include "radiotimer.h"
#include "radio.h"
#include "bsp_timer.h"
#include "spi.h"
#include "debugpins.h"
#include "leds.h"
#include "delay.h"



/* === MACROS ============================================================== */

/* TRX Parameter: t10 */
#define RST_PULSE_WIDTH_US                          (10)

/* TRX Parameter: tTR1 typical value */
#define P_ON_TO_CLKM_AVAILABLE_TYP_US               (330)

#define TRX_EXT_INT_CH	(4)

/* === GLOBALS ============================================================= */

extern int mote_main();

void rf_interface_init(void);

void buttons_init(void);
static void at86rfx_intc_init(void);
static void button_init(void);

/*
 * @brief Main entry functions. 
 *
 * @param returns the value of returns by mote_main
 *
 */ 
int main(void)
{
	SystemInit();
	return mote_main();
}

/*
 * @brief board_init will initialize the delay functions,
 *        debug pins in the board, Radio External Interrupt functions,
 *        SPI Related, UART and timers with default value.
 * @param None
 *
 */
void board_init(void)
{
 /* Disable the irq before initialization */
 cpu_irq_disable();
 
 /*  Initialize the Clock and event system and more */
 sys_clock_init(); 
 
 /* Configure the Debug Pins */
 debugpins_init(); 
 
 /* Configure the Radio Interrupt */	
 at86rfx_intc_init();
 /* Clear the Radio Interrupt */
 extint_flag_clear(TRX_EXT_INT_CH);

/*  Initialize board hardware
	SPI Init  Configure the Radio Pins and  Like RST, SLP_TR, EXTI(IRQ on Rising Edge)	 
 */
 rf_interface_init();
 
 /* Initialize the LED Output */
 leds_init();
 
 /* Initialize the Button Input */
 button_init();
 
 /* Radio Init */
 radio_init();
  
 /* UART Init */
#if defined BRIDGED_MODE 
 uart_init();
#endif /* BRIDGED_MODE */
 
 /* BSP Timer Init  --Init the RTC before BSP Timer*/
 bsp_timer_init(); 
 
 
 /* Radio Timer Init */
 radiotimer_init();
 
 /* Clear the Radio Interrupt */
 extint_enable_irq(TRX_EXT_INT_CH);
 /* Enable the IRQ */
 cpu_irq_enable();
 
  /* Disable the Power Consuming Sources Like USB */
  PM->APBBMASK.reg &= ~(PM_APBBMASK_USB);
  PM->AHBMASK.reg &= ~(PM_AHBMASK_USB);
  	
  PM->AHBMASK.reg &= ~(PM_AHBMASK_USB | PM_AHBMASK_DMAC | PM_AHBMASK_DSU);
  
   /* Disable the Power Consuming Sources */
   PM->APBCMASK.reg &= ~(PM_APBCMASK_EVSYS | PM_APBCMASK_ADC);
   PM->APBBMASK.reg &= ~(PM_APBBMASK_DSU | PM_APBBMASK_DMAC);
   PM->APBAMASK.reg &= ~(PM_APBAMASK_RTC | PM_APBAMASK_WDT);
   
  /* Disable the BOD33 */
  SYSCTRL->BOD33.reg &= ~(SYSCTRL_BOD33_ENABLE);
}

/*
 * @brief rf_interface_init will initialize the SPI and RFCTRL,
 *        This function will also Reset and initialize the Radio 
 *        to default state
 *
 * @param None
 *
 */
void rf_interface_init(void)
{
	pinmux_t pinmux;
		/* Configure the RF233 SPI Interface */
	pinmux.dir = PORT_PIN_DIR_OUTPUT;
	pinmux.mux_loc = SYSTEM_PINMUX_GPIO;
	pinmux.pull = PORT_PIN_PULLUP;
	
	pinmux_config(AT86RFX_SPI_SCK, &pinmux);
	pinmux_config(AT86RFX_SPI_MOSI, &pinmux);
	pinmux_config(AT86RFX_SPI_CS, &pinmux);
	pinmux_config(AT86RFX_RST_PIN, &pinmux);
	pinmux_config(AT86RFX_SLP_PIN, &pinmux);
	
	pinmux.dir = PORT_PIN_DIR_INPUT;
	pinmux_config(AT86RFX_SPI_MISO, &pinmux);
	
	port_pin_set_level(AT86RFX_SPI_SCK, SET_HIGH);
	port_pin_set_level(AT86RFX_SPI_MOSI, SET_HIGH);
	port_pin_set_level(AT86RFX_SPI_CS, SET_HIGH);
	port_pin_set_level(AT86RFX_RST_PIN, SET_HIGH);
	port_pin_set_level(AT86RFX_SLP_PIN, SET_HIGH);
		
	spi_init();
		
	/* Wait typical time of timer TR1. */
	cpu_delay_us(P_ON_TO_CLKM_AVAILABLE_TYP_US);
		
	/* Initialize the transceiver */
	PORT_PIN_RADIO_SLP_TR_CNTL_LOW();

	/* Wait typical time of timer TR1. */
	cpu_delay_us(P_ON_TO_CLKM_AVAILABLE_TYP_US+RST_PULSE_WIDTH_US);

	/* Apply reset pulse */
	//RST_LOW();
	cpu_delay_us(RST_PULSE_WIDTH_US);
	//RST_HIGH();
}

/*
 * @brief board_sleep This function will prepare the board to sleep
 *        Before entering to sleep the MCU will enable the wakeup source
 *        and set the appropriate possible power saving mode in the MCU.
 *
 * @param None
 *
 */
void board_sleep(board_sleepmode_t sleep_mode)
{
 /* Enter into sleep mode and disable the MCU and other peripherals */
 cpu_irq_enable();
 
 /*
 CPU_MODE	CPU	 AHB  APB
 Idle 0		Stop Run  Run
 Idle 1		Stop Stop Run
 Idle 2		Stop Stop Stop // Non Run in standby peripheral will run
 Standby	Stop Stop Stop // Run in standby peripheral will only can RUN
 */
 
 /* Set sleep mode */
 switch(sleep_mode)
 {
	 case BOARD_SLEEP_MODE0:
	 system_set_sleepmode(SYSTEM_SLEEPMODE_IDLE_0);
	 break;
	 
	 case BOARD_SLEEP_MODE1:
	 system_set_sleepmode(SYSTEM_SLEEPMODE_IDLE_1);
	 break;
	 
	 case BOARD_SLEEP_MODE2:
	 system_set_sleepmode(SYSTEM_SLEEPMODE_IDLE_2);
	 break;
	 
	 case BOARD_SLEEP_MODE3:
	 case BOARD_SLEEP_MODE4:
	 case BOARD_SLEEP_MODE5:
	 system_set_sleepmode(SYSTEM_SLEEPMODE_STANDBY);
	 break;
 } 
}

/* 
 * @brief This will reset the MCU and board to default state
 *
 * @param None
 *
 */
void board_reset(void)
{
 /* No Handlers added */
}

/* 
 * @brief TRX END and other Transceiver Interrupt 
 *        Handler for AT86RFX
 */
void ext_int4_callback(void)
{
	debugpins_isr_set();
	 extint_flag_clear(TRX_EXT_INT_CH);	
	radio_isr();			
	debugpins_isr_clr();
}

extern void udp_trigger(void);
extern void tcp_trigger(void);
extern void c2m_req(void);
/* 
 * @brief Button press Interrupt 
 *        Handler for Button press
 */
void ext_int15_callback(void)
{
	extint_flag_clear(15);
	//udp_trigger();
	//tcp_trigger();
	#if defined SC6TISCH_NODE
	c2m_req();
	#endif
}

/** \brief at86rfx_intc_init This function will configure the Transceiver interrupt

     No Pull Up, Wake if sleeps, Interrupt detection on both edges 

    \param [in] None
    \return None
 */
static void at86rfx_intc_init(void)
{
	
 pinmux_t pinmux;
 pinmux.dir = PORT_PIN_DIR_INPUT;
 pinmux.mux_loc = MUX_PB04A_EIC_EXTINT4;
 pinmux.pull = PORT_PIN_PULLNONE;
 extint_t extint;
 extint.channel = 4;
 extint.detect = EXTINT_DETECT_BOTH;
 eic_irq_init(PIN_PB04, &pinmux, &extint);
}


static void button_init(void)
{
	pinmux_t pinmux;
	pinmux.dir = PORT_PIN_DIR_INPUT;
	pinmux.mux_loc = MUX_PA15A_EIC_EXTINT15;
	pinmux.pull = PORT_PIN_PULLUP;
	extint_t extint;
	extint.channel = 15;
	extint.detect = EXTINT_DETECT_FALLING;
	eic_irq_init(PIN_PA15, &pinmux, &extint);
}


