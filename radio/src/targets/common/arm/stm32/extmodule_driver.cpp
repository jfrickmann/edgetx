/*
 * Copyright (C) EdgeTX
 *
 * Based on code named
 *   opentx - https://github.com/opentx/opentx
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "extmodule_driver.h"
#include "stm32_pulse_driver.h"

#include "hal.h"
#include "timers_driver.h"

#if defined(CROSSFIRE)
#include "pulses/crossfire.h"
#endif

#define STOP_TIMER_ON_LAST_UPDATE

static const stm32_pulse_timer_t extmoduleTimer = {
  .GPIOx = EXTMODULE_TX_GPIO,
  .GPIO_Pin = EXTMODULE_TX_GPIO_PIN,
  .GPIO_Alternate = EXTMODULE_TIMER_TX_GPIO_AF,
  .TIMx = EXTMODULE_TIMER,
  .TIM_Prescaler = __LL_TIM_CALC_PSC(EXTMODULE_TIMER_FREQ, 2000000),
  .TIM_Channel = EXTMODULE_TIMER_Channel,
  .TIM_IRQn = EXTMODULE_TIMER_IRQn,
  .DMAx = EXTMODULE_TIMER_DMA,
  .DMA_Stream = EXTMODULE_TIMER_DMA_STREAM_LL,
  .DMA_Channel = EXTMODULE_TIMER_DMA_CHANNEL,
  .DMA_IRQn = EXTMODULE_TIMER_DMA_STREAM_IRQn,
};

#if defined(STOP_TIMER_ON_LAST_UPDATE)

static_assert(EXTMODULE_TIMER_Channel == LL_TIM_CHANNEL_CH1
              || EXTMODULE_TIMER_Channel == LL_TIM_CHANNEL_CH1N
              || EXTMODULE_TIMER_Channel == LL_TIM_CHANNEL_CH3,
              "Unsupported timer channel");

static void set_compare_reg(uint32_t val)
{
  switch(EXTMODULE_TIMER_Channel){
  case LL_TIM_CHANNEL_CH1:
  case LL_TIM_CHANNEL_CH1N:
    LL_TIM_OC_SetCompareCH1(EXTMODULE_TIMER, val);
    break;
  case LL_TIM_CHANNEL_CH3:
    LL_TIM_OC_SetCompareCH3(EXTMODULE_TIMER, val);
    break;
  }
}

extern "C" void EXTMODULE_TIMER_DMA_IRQHandler()
{
  if (!LL_DMA_IsActiveFlag_TC5(EXTMODULE_TIMER_DMA))
    return;
  
  LL_DMA_ClearFlag_TC5(EXTMODULE_TIMER_DMA);

  LL_TIM_ClearFlag_UPDATE(EXTMODULE_TIMER);
  LL_TIM_EnableIT_UPDATE(EXTMODULE_TIMER);

  set_compare_reg(0);
  LL_TIM_OC_SetMode(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, LL_TIM_OCMODE_PWM1);
}

extern "C" void EXTMODULE_TIMER_IRQHandler()
{
  if (!LL_TIM_IsActiveFlag_UPDATE(EXTMODULE_TIMER))
    return;

  LL_TIM_ClearFlag_UPDATE(EXTMODULE_TIMER);
  LL_TIM_DisableIT_UPDATE(EXTMODULE_TIMER);

  // Halt pulses and force to inactive level
  LL_TIM_OC_SetMode(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel,
                    LL_TIM_OCMODE_FORCED_INACTIVE);
}
#endif

void extmoduleStop()
{
  EXTERNAL_MODULE_OFF();
  stm32_pulse_deinit(&extmoduleTimer);
}

static void config_ppm_output(uint16_t ppm_delay, bool polarity)
{
  // PPM generation principle:
  //
  // Hardware timer in PWM mode is used for PPM generation
  // Output is OFF if CNT<CCR1(delay) and ON if bigger
  // CCR1 register defines duration of pulse length and is constant
  // AAR register defines duration of each pulse, it is
  // updated after every pulse in Update interrupt handler.

  LL_TIM_OC_InitTypeDef ocInit;
  LL_TIM_OC_StructInit(&ocInit);

  ocInit.OCMode = LL_TIM_OCMODE_PWM1;
  ocInit.OCState = LL_TIM_OCSTATE_ENABLE;

  if (polarity) {
    ocInit.OCPolarity = LL_TIM_OCPOLARITY_LOW;
  } else {
    ocInit.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  }

  ocInit.CompareValue = ppm_delay * 2;
  stm32_pulse_config_output(&extmoduleTimer, &ocInit);  
}

void extmodulePpmStart(uint16_t ppm_delay, bool polarity)
{
  EXTERNAL_MODULE_ON();
  stm32_pulse_init(&extmoduleTimer);
  config_ppm_output(ppm_delay, polarity);
}

void extmoduleSendNextFramePpm(void* pulses, uint16_t length,
                               uint16_t ppm_delay, bool polarity)
{
  if (!stm32_pulse_stop_if_running(&extmoduleTimer))
    return;

  // Config output
  set_compare_reg(ppm_delay * 2);
 
  uint32_t ll_polarity;
  if (polarity) {
    ll_polarity = LL_TIM_OCPOLARITY_LOW;
  } else {
    ll_polarity = LL_TIM_OCPOLARITY_HIGH;
  }
  LL_TIM_OC_SetPolarity(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, ll_polarity);
  LL_TIM_OC_SetMode(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, LL_TIM_OCMODE_PWM1);

  // Start DMA request and re-enable timer
  stm32_pulse_start_dma_req(&extmoduleTimer, pulses, length);
}

#if defined(PXX1)
void extmodulePxx1PulsesStart()
{
  EXTERNAL_MODULE_ON();

  LL_TIM_OC_InitTypeDef ocInit;
  LL_TIM_OC_StructInit(&ocInit);

  ocInit.OCMode = LL_TIM_OCMODE_PWM1;
  ocInit.OCState = LL_TIM_OCSTATE_ENABLE;
  ocInit.OCPolarity = LL_TIM_OCPOLARITY_LOW;
  ocInit.CompareValue = 9 * 2; // 9 uS

  stm32_pulse_config_output(&extmoduleTimer, &ocInit);

  stm32_pulse_init(&extmoduleTimer);
}

void extmoduleSendNextFramePxx1(const void* pulses, uint16_t length)
{
  if (!stm32_pulse_stop_if_running(&extmoduleTimer))
    return;

  set_compare_reg(9 * 2);
  LL_TIM_OC_SetMode(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, LL_TIM_OCMODE_PWM1);

  // Start DMA request and re-enable timer
  stm32_pulse_start_dma_req(&extmoduleTimer, pulses, length);
}
#endif

// TODO: polarity?
void extmoduleSerialStart()
{
  EXTERNAL_MODULE_ON();
  stm32_pulse_init(&extmoduleTimer);

  LL_TIM_OC_InitTypeDef ocInit;
  LL_TIM_OC_StructInit(&ocInit);

  ocInit.OCMode = LL_TIM_OCMODE_TOGGLE;
  ocInit.OCState = LL_TIM_OCSTATE_ENABLE;
  ocInit.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
  ocInit.CompareValue = 0;

  stm32_pulse_config_output(&extmoduleTimer, &ocInit);
}

#if defined(AFHDS3) && !(defined(EXTMODULE_USART) && defined(EXTMODULE_TX_INVERT_GPIO))
void extmoduleSendNextFrameAFHDS3(const void* pulses, uint16_t length)
{
  if (!stm32_pulse_stop_if_running(&extmoduleTimer))
    return;

  // Start DMA request and re-enable timer
  stm32_pulse_start_dma_req(&extmoduleTimer, pulses, length);
}
#endif

void extmoduleSendNextFrameSoftSerial100kbit(const void* pulses, uint16_t length,
                                             bool polarity)
{
  if (!stm32_pulse_stop_if_running(&extmoduleTimer))
    return;

  uint32_t ll_polarity;
  if (polarity) {
    ll_polarity = LL_TIM_OCPOLARITY_HIGH;
  } else {
    ll_polarity = LL_TIM_OCPOLARITY_LOW;
  }
  LL_TIM_OC_SetPolarity(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, ll_polarity);
  LL_TIM_OC_SetMode(EXTMODULE_TIMER, EXTMODULE_TIMER_Channel, LL_TIM_OCMODE_TOGGLE);

  // Start DMA request and re-enable timer
  stm32_pulse_start_dma_req(&extmoduleTimer, pulses, length);
}

// Delay based byte sending @ 57600 bps
void extmoduleSendInvertedByte(uint8_t byte)
{
  uint16_t time;
  uint32_t i;

  __disable_irq();
  time = getTmr2MHz();
  LL_GPIO_SetOutputPin(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
  while ((uint16_t) (getTmr2MHz() - time) < 34)	{
    // wait
  }
  time += 34;
  for (i = 0; i < 8; i++) {
    if (byte & 1) {
      LL_GPIO_ResetOutputPin(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    }
    else {
      LL_GPIO_SetOutputPin(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
    }
    byte >>= 1 ;
    while ((uint16_t) (getTmr2MHz() - time) < 35) {
      // wait
    }
    time += 35 ;
  }
  LL_GPIO_ResetOutputPin(EXTMODULE_TX_GPIO, EXTMODULE_TX_GPIO_PIN);
  __enable_irq();	// No need to wait for the stop bit to complete
  while ((uint16_t) (getTmr2MHz() - time) < 34) {
    // wait
  }
}
