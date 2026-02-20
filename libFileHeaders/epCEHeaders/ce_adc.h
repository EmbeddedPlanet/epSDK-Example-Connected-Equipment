/****************************************************************************                                                                     *
 * Copyright (c) 2026 Embedded Planet, Inc.                                 *
 * SPDX-License-Identifier: Apache-2.0                                      *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 ****************************************************************************/
/**
 * Created on: Sept 1, 2022
 * Created by: golobmichael
 * 
 * Copyright (c) Embedded Planet, Inc - All rights reserved
 *
 * This source file is private and confidential.
 * Unauthorized copying of this file is strictly prohibited.
 * 
 * Version 1.0 - 01SEPT22  Initial
 */
#ifndef __CE_ADC_H__
#define __CE_ADC_H__

#include "ads124s08.h"
#include "mcp23008.h"

/* Number of ADC channels present on CE */
#define ADC_CHANNEL_COUNT 10

typedef struct Adc_Channel_Config{
    bool enabled;
    bool output_active_low;
    float low_cal;
    float high_cal;
} adc_channel_config;

extern adc_channel_config adc_channels[ADC_CHANNEL_COUNT];

typedef enum{
    CE_ADC_4_20_CH1 =   ADS_AIN0,
    CE_ADC_4_20_CH2 =   ADS_AIN1,
    CE_ADC_4_20_CH3 =   ADS_AIN2,
    CE_ADC_4_20_CH4 =   ADS_AIN3,
    CE_ADC_0_5_CH1  =   ADS_AIN9,
    CE_ADC_0_5_CH2  =   ADS_AIN8,
    CE_ADC_BATT     =   ADS_AIN6,
    CE_ADC_0_5_CH3  =   ADS_AIN5,
    CE_ADC_0_5_CH4  =   ADS_AIN4
} CE_ADC_CHANNEL;

nrfx_err_t adc_init(MCP23008 *mcp);

/**
 * @brief   Deinitialize ADCs
 * 
 */
void adc_deinit();

/**
 * @brief   Initialize ADC values
 * 
 */
void adc_config();

/**
 * @brief   Enable power to ADC chip
 * 
 * @param   enabled     0 - disable power, 1 - enable power
 * 
 */
void adc_power_enable(bool enabled);

/**
 * @brief   Configure VCH2 - AIN8(GPIO0) or VCH1 - AIN9(GPIO1) as either
 *          analog inputs or digital outputs. Default is analog inputs.
 * 
 * @param   channel Channel for ADC to be read  
 *   
 * @return float    Value for mapped ADC channel
 * 
 */
float adc_sample(CE_ADC_CHANNEL channel);

/**
 * @brief   Configure VCH2 - AIN8(GPIO0) or VCH1 - AIN9(GPIO1) as either
 *          analog inputs or digital outputs. Default is analog inputs.
 * 
 * @param gpio      Which GPIO to set as an input or output.
 *                  Can only be ADS_GPIO0 or ADS_GPIO1   
 * @param enable    true to set as a GPIO output, false to set as analog inputs
 * 
 */
void adc_set_gpio_config (ADS_GPIO gpio, bool enable);

/**
 * @brief   Check active channels 
 *   
 * @return uint8_t   Channel mask
 */
uint8_t adc_active_channels();

#endif