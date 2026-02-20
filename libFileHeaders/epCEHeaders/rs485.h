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
#ifndef __RS485_H__
#define __RS485_H__

#include "mcp23008.h"
#include "nrfx_twi.h"

//Basic interface to the MCP23008 and SC16 for receiving and sending data via the RS485 interface on Connected Equipment

/**
 * @brief Initializes the SC16
 * 
 * @param mcp MCP struct that we use to control the reset line to the SC16
 * @param twi TWI interface for the SC16
 */
void rs485_init(MCP23008 *mcp, nrfx_twi_t twi);

/**
 * @brief Sends a data string on the RS485 bus via the SC16
 * 
 * @param data          Data to transmit
 * @return nrfx_err_t   0 = success. Otherwise indicates a TWI error. See NRFX err codes.
 */
nrfx_err_t rs485_xmit(char *data);

/**
 * @brief Checks to see if there is data in the RX buffer
 * 
 * @param ret_code  0 = success. Otherwise indicates a TWI error. See NRFX err codes.
 * @return true     If there is data
 * @return false    If there is no data
 */
bool rs485_check(nrfx_err_t *ret_code);

/**
 * @brief Called after rs485_check returns true. Receives all data from the RX buffer on the SC16
 * 
 * @param data          Received data
 * @param data_len      Received data length
 * @return nrfx_err_t   0 = success. Otherwise indicates a TWI error. See NRFX err codes.
 */
nrfx_err_t rs485_rec(char *data, uint16_t *data_len);

#endif