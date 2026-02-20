/****************************************************************************                                                                     *
 * Copyright (c) 2022 Embedded Planet, Inc.                                 *
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
 * @file    ads124s08.h
 * @version 0.0.1
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @date    06 JUN 2023
 * 
 * @brief Contains functions for the configuration and control of the TI ADS124S08 ADC.
 * versions:
 *  0.1.0 - Initial release

 * Built for use with the nRF SDK 17.1
 * 
 * @see Data sheet:     https://www.ti.com/lit/ds/symlink/ads124s08.pdf?ts=1685980725614&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FADS124S08
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 * 
 */

#ifndef ADS124S08_H
#define ADS124S08_H

#include "nrfx_spim.h"
#include "mcp23008.h"

/**
 * @brief used to hold the configuration of a single ADS124S08
 */
typedef struct{
    nrfx_spim_t spi;        /** < SPIM interface used. SPI mode MUST be set to NRF_SPIM_MODE_1 */
    bool start_pin_active;  /** < If set to true via ads124s08_pin_control_enable, the driver uses the registered start GPIO to trigger
                                    conversion start/stop instead of the SPI command */
    bool reset_pin_active;  /** < If set to true via ads124s08_pin_control_enable, the driver uses the registered reset GPIO to trigger
                                    conversion start/stop instead of the SPI command */

    MCP23008 *mcp;          /** < An MCP23008 I/O expander can be used to control the reset, start, and cs pins on the ADS124. This is optional,
                                but is configured with ads124s08_mcp_init if needed */
    uint8_t mcp_reset_gpio; /** < MCP GPIO pin to be used for ADS124 reset. Pass an element from the mcp23008_pins struct */
    uint8_t mcp_start_gpio; /** < MCP GPIO pin to be used for ADS124 start/stop. Pass an element from the mcp23008_pins struct */
    uint8_t mcp_cs_gpio;    /** < MCP GPIO pin to be used for ADS124 chip-select. Pass an element from the mcp23008_pins struct */
} ADS124S08;

/**
 * @brief masks for the desired ADC channel 
 */
typedef enum{
    ADS_AIN0 = 0x00,
    ADS_AIN1 = 0x01,
    ADS_AIN2 = 0x02,
    ADS_AIN3 = 0x03,
    ADS_AIN4 = 0x04,
    ADS_AIN5 = 0x05,
    ADS_AIN6 = 0x06,
    ADS_AIN7 = 0x07,
    ADS_AIN8 = 0x08,
    ADS_AIN9 = 0x09,
    ADS_AIN10 = 0x0A,
    ADS_AIN11 = 0x0B,
    ADS_AINCOM = 0x0C,
    ADS_DISCONNECTED = 0x0F
} ADS_CHANNEL;

/**
 * @brief Used to request/read flags from the STATUS register
 * 
 * Register 0x01 (STATUS)
 */
typedef enum{
    ADS_REF_L0_FLAG     =   0x01,   /** < Reference voltage monitor flag, level 0. Indicates the external reference voltage is lower than 0.3 V. 
                                            Can be used to indicate a missing or floating external reference voltage */
    ADS_REF_L1_FLAG     =   0x02,   /** < Reference voltage monitor flag, level 1. Indicates the external reference voltage is lower than 1/3 of 
                                            the analog supply voltage. Can be used to detect an open-excitation lead in a 3-wire RTD application */
    ADS_N_RAILN_FLAG    =   0x04,   /** < Negative PGA output at negative rail flag. Indicates the negative PGA output is within 150 mV of AVSS */
    ADS_N_RAILP_FLAG    =   0x08,   /** < Negative PGA output at positive rail flag. Indicates the negative PGA output is within 150 mV of AVDD */
    ADS_P_RAILN_FLAG    =   0x10,   /** < Positive PGA output at negative rail flag. Indicates the positive PGA output is within 150 mV of AVSS */
    ADS_P_RAILP_FLAG    =   0x20,   /** < Positive PGA output at positive rail flag. Indicates the positive PGA output is within 150 mV of AVDD */
    ADS_RDY_FLAG        =   0x40,   /** < Device ready flag. Indicates the device has started up and is ready for communication */
    ADS_POR_FLAG        =   0x80    /** < POR flag. Indicates a power-on reset (POR) event has occurred */
} ADS_STATUS_FLAG;

/**
 * @brief Used to set the conversion delay for the first conversion after a WREG when a configuration change 
 * resets of the digital filter and triggers a new conversion.
 * 
 * Register 0x03 (PGA)
 */
typedef enum{
    ADS_DELAY_14    =   0x00,   /** < 14 * tMOD (DEFAULT)*/
    ADS_DELAY_25    =   0x20,   /** < 25 * tMOD */
    ADS_DELAY_64    =   0x40,   /** < 64 * tMOD */
    ADS_DELAY_256   =   0x60,   /** < 256 * tMOD */
    ADS_DELAY_1024  =   0x80,   /** < 1024 * tMOD */
    ADS_DELAY_2048  =   0xA0,   /** < 2048 * tMOD */
    ADS_DELAY_4096  =   0xC0,   /** < 4096 * tMOD */
    ADS_DELAY_1     =   0xE0    /** < 1 * tMOD */
} ADS_CONVERSION_DELAY;

/**
 * @brief Used to set the PGA gain.
 * 
 * Register 0x03 (PGA)
 */
typedef enum{
    ADS_GAIN_1  =   0x00,   /** < (DEFAULT) */
    ADS_GAIN_2  =   0x01,
    ADS_GAIN_4  =   0x02,
    ADS_GAIN_8  =   0x03,
    ADS_GAIN_16 =   0x04,
    ADS_GAIN_32 =   0x05,
    ADS_GAIN_64 =   0x06,
    ADS_GAIN_128 =  0x07
} ADS_GAIN;

/**
 * @brief Used to set the output data rate.
 * 
 * Register 0x04 (DATARATE)
 */
typedef enum{
    ADS_DATARATE_2_5_SPS    =   0x00,
    ADS_DATARATE_5_SPS      =   0x01,
    ADS_DATARATE_10_SPS     =   0x02,
    ADS_DATARATE_16_6_SPS   =   0x03,
    ADS_DATARATE_20_SPS     =   0x04,   /** < (DEFAULT)*/
    ADS_DATARATE_50_SPS     =   0x05,
    ADS_DATARATE_60_SPS     =   0x06,
    ADS_DATARATE_100_SPS    =   0x07,
    ADS_DATARATE_200_SPS    =   0x08,
    ADS_DATARATE_400_SPS    =   0x09,
    ADS_DATARATE_800_SPS    =   0x0A,
    ADS_DATARATE_1000_SPS   =   0x0B,
    ADS_DATARATE_2000_SPS   =   0x0C,
    ADS_DATARATE_4000_SPS   =   0x0D
} ADS_DATARATE;

/**
 * @brief Used to set the reference monitor configuration
 * 
 * Register 0x05 (REF)
 */
typedef enum{
    ADS_REF_MON_DISABLED    =   0x00,   /** < Reference monitor disabled (DEFAULT)*/
    ADS_FL_REF_L0           =   0x10,   /** < Monitor enabled, threshold 0.3V */
    ADS_FL_REF_L0_L1        =   0x20,   /** < Monitors enabled, threshold 0.3V and 1/3 * (AVDD - AVSS) */
    ADS_FL_REF_L0_10M       =   0x30    /** < Monitor and 10-M ohm pull-together enabled, threshold 0.3V */
} ADS_REF_MON_CONFIG;

/**
 * @brief Used to set reference input source for the ADC
 * 
 * Register 0x05 (REF)
 */
typedef enum{
    ADS_REFP0_REFN0     =   0x00,   /** < REFP0, REFN0 (DEFAULT) */
    ADS_REFP1_REFN1     =   0x04,   /** < REFP1, REFN1 */
    ADS_REF_INTERNAL    =   0x08    /** < Internal 2.5-V reference */
} ADS_REF_INPUT;

/**
 * @brief Used to configure the behavior of the internal voltage reference
 * 
 * Register 0x05 (REF)
 */
typedef enum{
    ADS_REF_INTERNAL_OFF        =   0x00,   /** < Internal reference off (DEFAULT) */
    ADS_REF_INTERNAL_ON         =   0x01,   /** < Internal reference on, but powers down in power-down mode */
    ADS_REF_INTERNAL_ALWAYS_ON  =   0x02    /** < Internal reference is always on, even when in power-down mode */
} ADS_REF_INTERNAL_CONFIG;

/**
 * @brief Used to control the low-side power switch
 * 
 * Register 0x06 (IDACMAG)
 */
typedef enum{
    ADS_PWR_SW_OPEN     =   0x00,   /** < Low-side power switch is open (DEFAULT) */
    ADS_PWR_SW_CLOSED   =   0x40    /** < Low-side power switch is closed */
} ADS_PWR_SW_CONFIG;

/**
 * @brief Used to set the value of the excitation current sources. Sets IDAC1 and IDAC2 to the same value.
 * 
 * Register 0x06 (IDACMAG)
 */
typedef enum{
    ADS_IDAC_OFF    =   0x00,
    ADS_IDAC_10uA   =   0x01,
    ADS_IDAC_50uA   =   0x02,
    ADS_IDAC_100uA  =   0x03,
    ADS_IDAC_250uA  =   0x04,
    ADS_IDAC_500uA  =   0x05,
    ADS_IDAC_750uA  =   0x06,
    ADS_IDAC_1000uA =   0x07,
    ADS_IDAC_1500uA =   0x08,
    ADS_IDAC_2000uA =   0x09
} ADS_IDAC_MAGNITUDE;

/**
 * @brief Used to set the VBIAS output voltage level
 * 
 * Register 0x08 (VBIAS)
 */
typedef enum{
    ADS_VBIAS_LVL_2     =   0x00,   /** < (AVDD + AVSS) / 2 (DEFAULT) */
    ADS_VBIAS_LVL_12    =   0x80    /** < (AVDD + AVSS / 12) */
} ADS_VBIAS_LEVEL;

/**
 * @brief Used to connect VBIAS to a AIN pin
 * 
 * Register 0x08 (VBIAS)
 */
typedef enum{
    ADS_VBIAS_OFF   =   0x00,   /** < VBIAS on all pins disabled (DEFAULT) */
    ADS_VBIAS_AIN0  =   0x01,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AIN1  =   0x02,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AIN2  =   0x04,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AIN3  =   0x08,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AIN4  =   0x10,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AIN5  =   0x20,   /** < VBIAS connected to AI0 */
    ADS_VBIAS_AINC  =   0x40    /** < VBIAS connected to AI0 */
} ADS_VBIAS_PINS;

/**
 * @brief Used to enable system monitor measurements using the ADC
 * 
 * Register 0x08 (SYS)
 */
typedef enum{
    ADS_SYS_MON_CONFIG_DISABLED =   0x00,   /** < Disables system monitor measurements (DEFAULT) */
    ADS_SYS_MON_CONFIG_1        =   0x20,   /** < PGA inputs shorted to (AVDD + AVSS) / 2 and disconnected from
                                                AINx and the multiplexer; gain set by user */
    ADS_SYS_MON_CONFIG_2        =   0x40,   /** < Internal temperature sensor measurement; PGA must be enabled;
                                                gain set by user  */
    ADS_SYS_MON_CONFIG_3        =   0x60,   /** < (AVDD - AVSS) / 4 measurement; gain set to 1 */
    ADS_SYS_MON_CONFIG_4        =   0x80,   /** < DVDD / 4 measurement; gain set to 1 */
    ADS_SYS_MON_CONFIG_5        =   0xA0,   /** < Burn-out current sources enabled, 0.2-uA setting */
    ADS_SYS_MON_CONFIG_6        =   0xC0,   /** < Burn-out current sources enabled, 1-uA setting */
    ADS_SYS_MON_CONFIG_7        =   0xE0    /** < Burn-out current sources enabled, 10-uA setting */
} ADS_SYS_MONITOR_CONFIG;

/**
 * @brief Used to configure the number of samples averaged for self and system offset and system gain calibration
 * 
 * Register 0x08 (SYS)
 */
typedef enum{
    ADS_CAL_1_SAMPLE    =   0x00,
    ADS_CAL_4_SAMPLES   =   0x08,
    ADS_CAL_8_SAMPLES   =   0x10,
    ADS_CAL_16_SAMPLES  =   0x18
} ADS_CAL_SAMPLES;

/**
 * @brief Used to enable/disable GPIO, set GPIO direction, read GPIO data or set GPIO level
 * 
 * Register 0x10 - 0x11 (GPIODAT - GPIOCON)
 */
typedef enum{

    ADS_GPIO0       =   0x01,
    ADS_GPIO1       =   0x02,
    ADS_GPIO2       =   0x04,
    ADS_GPIO3       =   0x08,
    ADS_GPIO_NONE   =   0x00,
    ADS_GPIO_ALL    =   0x0F,
    ADS_GPIO_ALL_OUTPUT = 0x00,
    ADS_GPIO_ALL_INPUT  = 0x0F 
} ADS_GPIO;

/**
 * @brief Used to init an ADS124S08 struct from the main application as well as the SPIM peripheral used.
 * 
 * @param ads124s08 Pointer to an ADS124S08 struct
 * @param spi       SPIM interface to be used
 * 
 * @note    SPI mode must be set to NRF_SPIM_MODE_1
 */
void ads124s08_init(ADS124S08 *ads124s08, nrfx_spim_t spi);

/**
 * @brief Used after ads124s08_init() to register an MCP23008 I/O expander and its corresponding GPIO pins to an ADS124. In this way, all
 * chip-select, start, stop, and reset pin manipulation is performed internally by the driver. 
 * 
 * @note Initialize the MCP23008 struct prior to passing to this function. This function DOES take care of pin direction setting on the MCP without
 * modifying the direction of other pins on the MCP
 * 
 * @note If the chip-select pin is onboard the micro, use the ss_pin setting in the nrfx_spim_config_t struct while initializing the SPIM peripheral
 * 
 * @note Even if the start and reset pins are registered with this function, they will not be used unless enabled with ads124s08_pin_control_enable().
 * By default, start/stop/reset commands are triggered by SPI commands.
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param mcp           Pointer to an MCP23008 struct
 * @param reset_gpio    GPIO of the MCP that is connect to the ADS124 reset pin
 * @param start_gpio    GPIO of the MCP that is connect to the ADS124 start/sync pin
 * @param cs_gpio       GPIO of the MCP that is connect to the ADS124 chip-select pin
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_mcp_init(ADS124S08 *ads124s08, MCP23008 *mcp, mcp23008_pins reset_gpio, mcp23008_pins start_gpio, mcp23008_pins cs_gpio);

/**
 * @brief Used after ads124s08_mcp_init() to enable usage of the start and reset pins. By default these are not needed; start/stop/reset 
 * are triggered by SPI commands.
 * 
 * @param ads124s08          Pointer to an ADS124S08 struct
 * @param start_pin_enabled  true if the driver shall use the start pin instead of the start SPI command
 * @param reset_pin_enabled  true if the driver shall use the reset pin instead of the reset SPI command
 */
void ads124s08_pin_control_enable(ADS124S08 *ads124s08, bool start_pin_enabled, bool reset_pin_enabled);

/**
 * @brief Starts an ADC conversion on the channels configured with ads124s08_select_pos_ch() and ads124s08_select_neg_ch().
 * This is accomplished by a start SPI command or pin manipulation if start_pin_enabled is set to true
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param continuous    true if the conversions shall continue until stopped with ads124s08_stop_conversions()
 *                      false if the conversion shall be single-shot
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_start_conversions(ADS124S08 *ads124s08, bool continuous);

/**
 * @brief Stops ADC conversions taking place in continuous mode. This function call is not needed if in single-shot mode.
 * This is accomplished by a stop SPI command or pin manipulation if start_pin_enabled is set to true
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_stop_conversions(ADS124S08 *ads124s08);

/**
 * @brief Resets the ADS124 either by a reset SPI command or pin manipulation if reset_pin_enabled is set to true
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_reset(ADS124S08 *ads124s08);

/**
 * @brief Sends the sleep command to the device
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_sleep(ADS124S08 *ads124s08);

/**
 * @brief Sends the wake command to the device
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_wake(ADS124S08 *ads124s08);

/**
 * @brief Retrieves the last converted data. Called after ads124s08_start_conversions()
 * 
 * @note This driver implementation retrieves converted data from the data-holding register, not direct out of the shift register. 
 * In this way, the CRC functionality of the device is not needed and should not be used.
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * @return uint32_t     24-bit raw converted data in binary two's complement
 */
uint32_t ads124s08_get_data(ADS124S08 *ads124s08, nrfx_err_t *ret_code);

/**
 * @brief Retrieves and returns the ID of the device
 * 
 * @note The ADS124S08 ID is 0x00, which should not be used to confirm that the device is connected.
 * 
 * @note Register 0x00 (ID) bits [2:0]
 * 
 * @param ads124s08 Pointer to an ADS124S08 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * @return uint8_t  000 = ADS124S08
 *                  001 = ADS124S06
 */
uint8_t ads124s08_get_dev_ID(ADS124S08 *ads124s08, nrfx_err_t *ret_code);

//Register 0x01 Device Status (STATUS)
/**
 * @brief Returns the full STATUS register byte.
 * 
 * @see ADS_STATUS_FLAG for bit-positions of the flags
 * 
 * @note Register 0x01 (STATUS) byte [7:0]
 * 
 * @param ads124s08 Pointer to an ADS124S08 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * @return uint8_t  STATUS register byte
 */
uint8_t ads124s08_get_status(ADS124S08 *ads124s08, nrfx_err_t *ret_code);

/**
 * @brief Returns the status of a specified flag.
 * 
 * @note Register 0x01 (STATUS)
 * 
 * @param ads124s08 Pointer to an ADS124S08 struct
 * @param flag      Flag to read
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * @return true     if the flag is set
 * @return false    if the flag is clear
 */
bool ads124s08_get_flag(ADS124S08 *ads124s08, ADS_STATUS_FLAG flag, nrfx_err_t *ret_code);

/**
 * @brief Used to select the ADS positive input channel to measure when ads124s08_start_conversions() is called
 * 
 * @note DEFAULT is AIN0
 * 
 * @note Register 0x02 (INPMUX) [7:4]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param pos_channel   Channel to measure
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t ads124s08_select_pos_ch(ADS124S08 *ads124s08, ADS_CHANNEL pos_channel);

/**
 * @brief Used to select the ADS negative input channel to measure when ads124s08_start_conversions() is called
 * 
 * @note DEFAULT is AIN1
 * 
 * @note Register 0x02 (INPMUX) [3:0]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param neg_channel   Channel to measure
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_select_neg_ch(ADS124S08 *ads124s08, ADS_CHANNEL neg_channel);

/**
 * @brief Sets the conversion delay time for the first conversion after a WREG when a configuration change resets of 
 * the digital filter and triggers a new conversion
 * 
 * @note Register 0x03 Gain (PGA) bits [7:5]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param delay         Conversion delay (DEFAULT 14)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_conversion_delay(ADS124S08 *ads124s08, ADS_CONVERSION_DELAY delay);

/**
 * @brief Enabled/disables the PGA
 * 
 * @note Register 0x03 Gain (PGA) bits [4:3]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param enabled       false if PGA should be bypassed (DEFAULT)
 *                      true if PGA should be enabled
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_pga_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Configures the PGA gain
 * 
 * @note Register 0x03 Gain (PGA) bits [2:0]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param gain          Gain to be set (DEFAULT 1)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_gain(ADS124S08 *ads124s08, ADS_GAIN gain);

/**
 * @brief Enables/disables the global chop function. When enabled, the device automatically swaps the inputs and takes the 
 * average of two consecutive readings to cancel the offset voltage.
 * 
 * @note Register 0x04 Data Rate (DATARATE) bits [7]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param enabled       false to disable (DEFAULT)
 *                      true to enable
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_chop_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Enables the external or internal clock
 * 
 * @note Register 0x04 Data Rate (DATARATE) bits [6]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param enabled       false to enable the internal 4.096-MHz oscillator (DEFAULT)
 *                      true to enable the external clock
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_external_clock_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Selects the conversion mode - continuous or single-shot
 * 
 * @note Register 0x04 Data Rate (DATARATE) bits [5]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param enabled       false for continuous conversion mode (DEFAULT)
 *                      true for single-shot conversion mode
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_single_shot_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Used to choose between Sinc filter and low-latency filter
 * 
 * @note Register 0x04 Data Rate (DATARATE) bits [4]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param enabled       false for Sinc filter
 *                      true for Low-latency filter (DEFAULT)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_low_latency_filter_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Used to set the data rate (samples per second)
 * 
 * @note Register 0x04 Data Rate (DATARATE) bits [3:0]
 * 
 * @param ads124s08     Pointer to an ADS124S08 struct
 * @param rate          Output data rate (DEFAULT 20 SPS)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_datarate(ADS124S08 *ads124s08, ADS_DATARATE rate);

/**
 * @brief Enables and configures the reference monitor
 * 
 * @note Register 0x05 Reference Control (REF) bits [7:6]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param config        Reference monitor configuration (DEFAULT disabled)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_ref_monitor(ADS124S08 *ads124s08, ADS_REF_MON_CONFIG config);

/**
 * @brief Enables/disables the positive reference buffer. Recommended when V(REFPx) is close to AVDD.
 * 
 * @note Register 0x05 Reference Control (REF) bits [5]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false enables the positive reference buffer
 *                      true disables the positive reference buffer (DEFAULT)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_p_buffer_ref_bypass(ADS124S08 *ads124s08, bool disabled);

/**
 * @brief Disables the negative reference buffer
 * 
 * @note Register 0x05 Reference Control (REF) bits [4]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false enables the negative reference buffer
 *                      true disables the negative reference buffer (DEFAULT)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_n_buffer_ref_bypass(ADS124S08 *ads124s08, bool disabled);

/**
 * @brief Selects the reference input source for the ADC
 * 
 * @note Register 0x05 Reference Control (REF) bits [3:2]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param input         Reference input source for the ADC (DEFAULT REFP0, REFN0)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_reference_input(ADS124S08 *ads124s08, ADS_REF_INPUT input);

/**
 * @brief Configures the behavior of the internal voltage reference
 * 
 * @note Register 0x05 Reference Control (REF) bits [1:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param config        Internal voltage reference configuration (DEFAULT internal reference off)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_reference_config(ADS124S08 *ads124s08, ADS_REF_INTERNAL_CONFIG config);

/**
 * @brief Enables/disables the PGA output voltage rail monitor circuit
 * 
 * @note Register 0x06 Excitation Current Register 1 (IDACMAG) bits [7]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false to disable the circuit (DEFAULT)
 *                      true to enable the circuit
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_rail_flag_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Configures the low-side power switch. The low-side power switch opens automatically in power-down mode.
 * 
 * @note Register 0x06 Excitation Current Register 1 (IDACMAG) bits [6]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param config        Open (DEFAULT)
 *                      Closed
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_pwrsw_config(ADS124S08 *ads124s08, ADS_PWR_SW_CONFIG config);

/**
 * @brief Selects the value of the excitation current sources. Sets IDAC1 and IDAC2 to the same value.
 * 
 * @note Register 0x06 Excitation Current Register 1 (IDACMAG) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param magnitude     Magnitude (DEFAULT OFF)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_idac_magnitude(ADS124S08 *ads124s08, ADS_IDAC_MAGNITUDE magnitude);

/**
 * @brief Select the output channel for IDAC2
 * 
 * @note Register 0x07 Excitation Current Register 2 (IDACMUX) bits [7:4]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param channel       output channel (DEFAULT DISCONNECTED)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_idac2_ch(ADS124S08 *ads124s08, ADS_CHANNEL channel);

/**
 * @brief Select the output channel for IDAC1
 * 
 * @note Register 0x07 Excitation Current Register 2 (IDACMUX) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param channel       output channel (DEFAULT DISCONNECTED)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_idac1_ch(ADS124S08 *ads124s08, ADS_CHANNEL channel);

/**
 * @brief Sets the VBIAS output voltage level. VBIAS is disabled when not connected to any input.
 * 
 * @note Register 0x08 Sensor Biasing (VBIAS) bits [7]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param lvl           VBIAS output voltage level (DEFAULT (AVDD + AVSS) / 2)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_vbias_lvl(ADS124S08 *ads124s08, ADS_VBIAS_LEVEL lvl);

/**
 * @brief Enables VBIAS on selected pins. Can OR elements in the ADS_VBIAS_PINS enum to select multiple pins. 
 * 
 * @note Register 0x08 Sensor Biasing (VBIAS) bits [6:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pins          ORing of pins to connect VBIAS to. Any pins not included default to disconnected.
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_vbias_pins(ADS124S08 *ads124s08, ADS_VBIAS_PINS pins);

/**
 * @brief Enables a set of system monitor measurements using the ADC
 * 
 * @note Register 0x09 System Control (SYS) bits [7:5]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param config        System monitor configuration (DEFAULT disabled)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_sysmon_config(ADS124S08 *ads124s08, ADS_SYS_MONITOR_CONFIG config);

/**
 * @brief Configures the number of samples averaged for self and system offset and system gain calibration
 * 
 * @note Register 0x09 System Control (SYS) bits [4:3]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param samples       Number of samples (DEFAULT 8)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_cal_samples(ADS124S08 *ads124s08, ADS_CAL_SAMPLES samples);

/**
 * @brief Enables the SPI timeout function
 * 
 * @note Register 0x09 System Control (SYS) bits [2]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false to disable (DEFAULT)
 *                      true to enable
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_spi_timeout_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Enables the CRC byte appended to the conversion result. When enabled, CRC is calculated across the 24-bit conversion 
 * result (plus the STATUS byte if enabled).
 * 
 * @note the ads124s08_get_data() function does not currently support the CRC feature.
 * 
 * @note Register 0x09 System Control (SYS) bits [1]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false to disable (DEFAULT)
 *                      true to enable
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_crc_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Enables the STATUS byte prepended to the conversion result.
 * 
 * @note the ads124s08_get_data() function does not currently support the STATUS byte feature. Activating this
 * feature will corrupt the data returned.
 * 
 * @note Register 0x09 System Control (SYS) bits [0] 
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param enabled       false to disable (DEFAULT)
 *                      true to enable
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_status_byte_enable(ADS124S08 *ads124s08, bool enabled);

/**
 * @brief Sets the offset calibration
 * 
 * @note Registers 0x0A - 0x0C (OFCAL)
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param cal           offset calibration (DEFAULT 0)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_offset_cal(ADS124S08 *ads124s08, uint32_t cal);

/**
 * @brief Sets the gain calibration
 * 
 * @note Registers 0x0D - 0x0F (FSCAL)
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param cal           gain calibration (DEFAULT 0x40)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_set_gain_cal(ADS124S08 *ads124s08, uint32_t cal);

/**
 * @brief Sets the gpio direction of GPIO pins 0-3
 * 
 * @note Pins must be configured as GPIO via the ads124s08_pin_mode() function 
 * 
 * @note Register 0x10 (GPIODAT) bits [7:4]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pins          ORing of pins to be configured as inputs. Any not included will be set to outputs (default)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_gpio_direction(ADS124S08 *ads124s08, ADS_GPIO pins);

/**
 * @brief Sets the gpio level of GPIO pins 0-3 that are configured as outputs
 * 
 * @note Pins must be configured as GPIO via the ads124s08_pin_mode() function 
 * 
 * @note Register 0x10 (GPIODAT) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pins          ORing of pins to set high. All pins not included will be set low (default)
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_gpio_write_pins(ADS124S08 *ads124s08, ADS_GPIO pins);

/**
 * @brief Sets the gpio level of a single GPIO pins that is configured as an output
 * 
 * @note Pins must be configured as GPIO via the ads124s08_pin_mode() function 
 * 
 * @note Register 0x10 (GPIODAT) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pin           Pin to set
 * @param level         false for low (DEFAULT)
 *                      true for high
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_gpio_write_pin(ADS124S08 *ads124s08, ADS_GPIO pin, bool level);

/**
 * @brief Gets the gpio level of GPIO pins 0-3
 * 
 * @note Pins must be configured as GPIO via the ads124s08_pin_mode() function 
 * 
 * @note Register 0x10 (GPIODAT) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 * @return Byte where bit 0 is the level of GPIO0, bit 1 is the level of GPIO1, and so on
 */
uint8_t ads124s08_gpio_read_pins(ADS124S08 *ads124s08, nrfx_err_t *ret_code);

/**
 * @brief Gets the gpio level of a single GPIO pins
 * 
 * @note Pins must be configured as GPIO via the ads124s08_pin_mode() function 
 * 
 * @note Register 0x10 (GPIODAT) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pin           Pin to get level off
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 * @return level        false for low
 *                      true for high
 */
bool ads124s08_gpio_read_pin(ADS124S08 *ads124s08,  ADS_GPIO pin, nrfx_err_t *ret_code);

/**
 * @brief Sets pins to GPIO or ADC input mode
 * 
 * @note Register 0x11 (GPIOCON) bits [3:0]
 * 
 * @param ads124s08     pointer to an ADS124S08 struct
 * @param pins          ORing of pins that shall be set to GPIO mode. All pins not included will default to ADC input mode
 * @return nrfx_err_t   0 if successful. For all others, refer to the Global Error Codes document at the top of this file
 */
nrfx_err_t ads124s08_pin_mode(ADS124S08 *ads124s08, ADS_GPIO pins);


/**
 * @brief Get the required wait time in milliseconds from starting a sample to read availbility
 * 
 * @note The timing is taken from Texas Instruments SBAS660C Aug 2016 Tables 13 to 19.
 * 
 * @param data_rate register value to specific data rate setting (2.5SPS to 4000SPS) 
 * @param chop_on   1 if the chopper is enabled, o for disabled.
 * @param low_lat   1 if the low latency filter is enabled, 0 for the sinc3 filter.
 * 
 * @return uint16_t  Required wait time between sample start and data availability
 *                  
 */
uint16_t ads124s08_get_read_delay (ADS_DATARATE data_rate, bool chop_on, bool low_lat);

#endif