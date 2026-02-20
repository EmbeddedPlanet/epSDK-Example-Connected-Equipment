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
 * @file    bme680.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @date    9 August 2022
 * 
 * @brief Contains function declarations and structs for interaction with the I2C/TWI BME680 environmental sensor and supporting 
 * BSEC library. This file is a replacement of the bsec_integration.h header file that Bosch included with their library, since 
 * significant restructuring was needed to fit the nRF chip and our application. For the original header and source, download the
 * BSEC library and view the bsec_integration.c/h files.
 * 
 * Built for use with the nRF SDK 17.1 and FreeRTOS
 * 
 * @see Data sheet:     https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme680-ds001.pdf
 * @see BSEC library:   https://www.bosch-sensortec.com/software-tools/software/bsec/
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 */

#ifndef BME680_H
#define BME680_H


/**********************************************************************************************************************/
/* header files */
/**********************************************************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

#include "bme68x.h"
/* BSEC header files are available in the inc/ folder of the release package */
#include "bsec_interface.h"
#include "bsec_datatypes.h"

#include "nrfx_twi.h"

/**********************************************************************************************************************/
/* global defines */
/**********************************************************************************************************************/

/**
 * @brief Determines the interval (in samples) between saving the state of the sensor and settings to non-volatile 
 * memory. This feature is NOT implemented in the current version, as the state save/load functions are not
 * implemented.
 */
#define BME680_SAVE_INTERVAL 10000

/**
 * @brief Determines the I2C address of the device. The options are BME68X_I2C_ADDR_LOW (0x76) and 
 * BME68X_I2C_ADDR_HIGH (0x77)
 * 
 */
#define BME680_DEV_ADDR BME68X_I2C_ADDR_LOW;


/**********************************************************************************************************************/
/* global variable declarations */
/**********************************************************************************************************************/

/**
 * @brief FreeRTOS Semaphore for signaling to application that BME680 data is ready
 */
extern SemaphoreHandle_t xBme680DataReadySemaphore;

/**
 * @brief FreeRTOS timer for proper timing of BME680 sampling
 */
extern TimerHandle_t xSystemTimer;

/**
 * @brief System time in ms, which is used by the BSEC library for verifying sample time is within expencted
 * bounds. Incremented in the xSystemTimer.
 */
extern uint64_t bme680_time_ms;

/**********************************************************************************************************************/
/* structure definitions */
/**********************************************************************************************************************/

/**
 * @brief Structure that holds BME680 sensor data after reads. Should be accessed only after receiving 
 * xBme680DataReadySemaphore.
 */
typedef struct{
    int64_t timestamp;                  /** < Data reading timestamp in ns < */
    float iaq;                          /** < Indoor air quality score (optimized for mobile applications). Range is 0-500  */
    uint8_t iaq_accuracy;               /** < Accuracy of the iaq score. Range is 0(unreliable) - 3(high accuracy)  */
    float static_iaq;                   /** < Indoor air quality score (optimized for static applications). Range is 0-500  */
    uint8_t static_iaq_accuracy;        /** < Accuracy of the static iaq score. Range is 0(unreliable) - 3(high accuracy) */
    float temp;                         /** < Temperature in degrees C, adjusted to compensate for the onboard heater element  */
    float raw_temp;                     /** < Unadjusted temperature in degrees C < */
    float raw_pressure;                 /** < Unadjusted pressure in kPa */
    float humidity;                     /** < Humidity in %, adjusted to compensate for the onboard heater element*/
    float raw_humidity;                 /** < Unudjusted humidity in % */   
    float raw_gas;                      /** < Gas resistance in Ohms */
    float gas_percentage;               /** < Percentage of min and max filtered gas */
    uint8_t gas_percentage_acccuracy;   /** < Accuracy of the gas percentage value. Range is 0(unreliable) - 3(high accuracy) */
    uint8_t stabStatus;                 /** < Gas sensor stabilization status. 0 = stabalizing, 1 = stable */
    uint8_t runInStatus;                /** < Gas sensor power-on stabilization status. 0 = ongoing, 1 = finished */
    float co2_equivalent;               /** < co2 equivalent estimate in ppm */   
    uint8_t co2_accuracy;               /** < Accuracy of the c02 equivalent estimage. Range is 0(unreliable) - 3(high accuracy) */
    float breath_voc_equivalent;        /** < Breath VOC concentration estimate in ppm */
    uint8_t breath_voc_accuracy;        /** < Accuracy of the VOC concentration estimage. Range is 0(unreliable) - 3(high accuracy) */
}bme680_sensor_data;

/**
 * @brief Structure that holds the return values from bsec_iot_init()
 */
typedef struct{
    int8_t timer_status;                /** < Result of timer creation */
    int8_t semaphore_status;            /** < Result of data ready semaphore creation */
    int8_t task_status;                 /** < Result of task creation */
	int8_t bme680_status;               /** < Result of API execution status */
	bsec_library_return_t bsec_status;  /** < Result of BSEC library */
}bme680_init_return_values;

/**********************************************************************************************************************/
/* function declarations */
/**********************************************************************************************************************/

/*!
 * @brief           Return struct that contains the latest sensor data
 *
 * @return          Struct that contains all sensor data
 * 
 */
bme680_sensor_data bme680_get_latest_data();

/*!
 * @brief       Initialize the bme68x sensor and the BSEC library
 *
 * @param       
 * @param       sample_rate         mode to be used (either BSEC_SAMPLE_RATE_ULP or BSEC_SAMPLE_RATE_LP)
 * @param       temperature_offset  device-specific temperature offset (due to self-heating)
 * 
 * @return      zero if successful, negative otherwise
 */
bme680_init_return_values bme680_init(nrfx_twi_t twi, float sample_rate, float temperature_offset);

/*!
 * @brief       Runs the main (endless) loop that queries sensor settings, applies them, and processes the measured data
 *
 * @param[in]   sleep               pointer to the system-specific sleep function
 * @param[in]   get_timestamp_us    pointer to the system-specific timestamp derivation function
 * @param[in]   state_save          pointer to the system-specific state save function
 * @param[in]   save_intvl          interval at which BSEC state should be saved (in samples)
 *
 * @return      bme68x_init_return_values	struct with the result of the API and the BSEC library
 */ 
void bme680_run(void *pvParameters);

#endif