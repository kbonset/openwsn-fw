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

#ifndef samd21_timer_h__
#define samd21_timer_h__

#include "cm0plus_interrupt.h"
#include "sam.h"

/* MCU Related */
#define CLK_DOMAIN_SYNC_DELAY_READ  			(12)
#define CLK_DOMAIN_SYNC_DELAY_WRITE				(0)

/**
 * \brief TC module count direction.
 *
 * Timer/Counter count direction.
 */
enum tc_count_direction {
	/** Timer should count upward from zero to MAX. */
	TC_COUNT_DIRECTION_UP,

	/** Timer should count downward to zero from MAX. */
	TC_COUNT_DIRECTION_DOWN,
};


/**
 * \brief Enable tc3 interrupt vector
 *
 * \param[in] None
 */
static inline void tc3_irq_enable(void)
{
	nvic_irq_enable(TC3_IRQn);
}

/**
 * \brief Disable tc3 interrupt vector
 *
 * \param[in] None
 */
static inline void tc3_irq_disable(void)
{
	nvic_irq_disable(TC3_IRQn);
}

/**
 * \brief Enable tc4 interrupt vector
 *
 * \param[in] None
 */
static inline void tc4_irq_enable(void)
{
	nvic_irq_enable(TC4_IRQn);
}

/**
 * \brief Disable tc4 interrupt vector
 *
 * \param[in] None
 */
static inline void tc4_irq_disable(void)
{
	nvic_irq_disable(TC4_IRQn);
}

/** \brief tc4_enable_compare0_isr This function enables the compare0 interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_enable_compare0_isr(void)
{
 TC4->COUNT16.INTENSET.reg = TC_INTFLAG_MC0;
}

/** \brief tc4_disable_compare0_isr This function disables the compare0 interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_disable_compare0_isr(void)
{
 TC4->COUNT16.INTENCLR.reg = TC_INTFLAG_MC0;
}

/** \brief tc4_enable_compare1_isr This function enables the compare1 interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_enable_compare1_isr(void)
{
 TC4->COUNT16.INTENSET.reg = TC_INTFLAG_MC1;
}

/** \brief tc4_disable_compare1_isr This function disables the compare1 interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_disable_compare1_isr(void)
{
 TC4->COUNT16.INTENCLR.reg = TC_INTFLAG_MC1;
}

/** \brief tc4_enable_overflow_isr This function enables the overflow interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_enable_overflow_isr(void)
{
 TC4->COUNT16.INTENSET.reg = TC_INTFLAG_OVF;
}

/** \brief tc4_disable_overflow_isr This function disables the overflow interrupt for Timer-4

     
    \param [in] None
    \return None
 */
static inline void tc4_disable_overflow_isr(void)
{
 TC4->COUNT16.INTENCLR.reg = TC_INTFLAG_OVF;
}

/** \brief tc4_set_compare0 This function sets the compare0 value for Timer-3

     
    \param [in] None
    \return None @2us
 */
static inline void tc4_set_compare0(uint32_t value)
{
 while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }	
 TC4->COUNT16.CC[0].reg = (uint16_t)value /*- CLK_DOMAIN_SYNC_DELAY_WRITE*/;
}

/** \brief tc4_set_compare1 This function sets the compare1 value for Timer-3

     
    \param [in] None
    \return None @var
 */
static inline void tc4_set_compare1(uint32_t value, uint8_t delay)
{
 while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 if (value > delay)
 {
	 TC4->COUNT16.CC[1].reg = (uint16_t)(value - delay);
 } 
 else
 {
	 TC4->COUNT16.CC[1].reg = (uint16_t)(delay - value);
 } 
}

/** \brief tc3_set_compare0 This function sets the compare0 value for Timer-3

     
    \param [in] None
    \return None @2us
 */
static inline void tc3_set_compare0(uint32_t value)
{
 
 while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }	
 if (value > CLK_DOMAIN_SYNC_DELAY_WRITE)
 {
	 TC3->COUNT16.CC[0].reg = (uint16_t)(value - CLK_DOMAIN_SYNC_DELAY_WRITE);
 } 
 else
 {
	 TC3->COUNT16.CC[0].reg = (uint16_t)(CLK_DOMAIN_SYNC_DELAY_WRITE - (uint16_t)value);
 } 
}

/** \brief tc3_set_compare1 This function sets the compare1 value for Timer-3

     
    \param [in] None
    \return None @u
 */
static inline void tc3_set_compare1(uint32_t value)
{
  //dbg_pin2_set();
 while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }	
 if (value > CLK_DOMAIN_SYNC_DELAY_WRITE)
 {
	 TC3->COUNT16.CC[1].reg = (uint16_t)(value - CLK_DOMAIN_SYNC_DELAY_WRITE);
 } 
 else
 {
	 TC3->COUNT16.CC[1].reg = (uint16_t)(CLK_DOMAIN_SYNC_DELAY_WRITE - (uint16_t)value);
 }
 
}



/** \brief tc3_enable_compare0_isr This function enables the compare0 interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_enable_compare0_isr(void)
{
 TC3->COUNT16.INTENSET.reg = TC_INTFLAG_MC0;
}

/** \brief tc3_enable_compare0_isr This function enables Trigger the Interrupt 
          manually for Timer-3 compare-0 

     
    \param [in] None
    \return None
 */
static inline void tc3_trigger_compare0_isr(void)
{
 TC3->COUNT16.INTFLAG.reg |= TC_INTFLAG_MC0;
}


/** \brief tc3_disable_compare0_isr This function disables the compare0 interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_disable_compare0_isr(void)
{
 TC3->COUNT16.INTENCLR.reg = TC_INTFLAG_MC0;
}

/** \brief tc3_enable_compare1_isr This function enables the compare1 interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_enable_compare1_isr(void)
{
 TC3->COUNT16.INTENSET.reg = TC_INTFLAG_MC1;
}

/** \brief tc3_disable_compare1_isr This function disables the compare1 interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_disable_compare1_isr(void)
{
 TC3->COUNT16.INTENCLR.reg = TC_INTFLAG_MC1;
}

/** \brief tc3_enable_overflow_isr This function enables the overflow interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_enable_overflow_isr(void)
{
 TC3->COUNT16.INTENSET.reg = TC_INTFLAG_OVF;
}

/** \brief tc3_disable_overflow_isr This function disables the overflow interrupt for Timer-3

     
    \param [in] None
    \return None
 */
static inline void tc3_disable_overflow_isr(void)
{
 TC3->COUNT16.INTENCLR.reg = TC_INTFLAG_OVF;
}

/** \brief tc3_get_count This function reads the current counter value from the counter register

     

    \param [in] None
    \return count value @160us
 */
static inline uint32_t tc3_get_count(void)
{
 TC3->COUNT16.READREQ.reg |= TC_READREQ_RREQ;	
 while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 return(((uint32_t)(TC3->COUNT16.COUNT.reg)+CLK_DOMAIN_SYNC_DELAY_READ));
}

/** \brief tc3_set_count This function sets the current counter value to counter register

     

    \param [in] count
    \return None @u
 */
static inline void tc3_set_count(uint32_t count)
{
 while(TC3->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 if (count > CLK_DOMAIN_SYNC_DELAY_WRITE)
 {
	 TC3->COUNT16.COUNT.reg = (uint16_t)(count - CLK_DOMAIN_SYNC_DELAY_WRITE);
 } 
 else
 {
	 TC3->COUNT16.COUNT.reg = (uint16_t)(CLK_DOMAIN_SYNC_DELAY_WRITE - count);
 }
}

/** \brief tc4_get_count This function reads the current counter value from the counter register

     

    \param [in] None
    \return count value @160
 */
static inline uint32_t tc4_get_count(void)
{
  TC4->COUNT16.READREQ.reg |= TC_READREQ_RREQ;	
 
  while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 return(((uint32_t)(TC4->COUNT16.COUNT.reg)+CLK_DOMAIN_SYNC_DELAY_READ));
}

/** \brief tc4_get_capture This function reads the current capture value from the capture register
     

    \param [in] None
    \return capture value @#182us->u
 */
static inline uint32_t tc4_get_capture0_val(void)
{
 while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 return((uint32_t)TC4->COUNT16.CC[0].reg);
}

/** \brief tc4_set_count This function sets the current counter value to counter register
    

    \param [in] count
    \return None
 */
static inline void tc4_set_count(uint32_t count)
{
 while(TC4->COUNT16.STATUS.reg & TC_STATUS_SYNCBUSY){
	 /* wait for Sync */
 }
 if (count > CLK_DOMAIN_SYNC_DELAY_WRITE)
 {
	 TC4->COUNT16.COUNT.reg = (uint16_t)(count - CLK_DOMAIN_SYNC_DELAY_WRITE);
 } 
 else
 {
	 TC4->COUNT16.COUNT.reg = (uint16_t)(CLK_DOMAIN_SYNC_DELAY_WRITE - count);
 }
}

/* Initialize the timer3 peripheral */
void timer3_init(void);

/* Initialize the timer4 peripheral */
void timer4_init(void);

/* capture compare0 callback for timer3 handler */
void tc3_cca0_callback(void);

/* capture compare1 callback for timer3 handler */
void tc3_cca1_callback(void);

/* capture compare0 callback for timer4 handler */
void tc4_cca0_callback(void);

/* capture compare1 callback for timer4 handler */
void tc4_cca1_callback(void);

/* timer4 overflow callback handler */
void tc4_ovf_callback(void);


#endif // samd21_timer_h__
