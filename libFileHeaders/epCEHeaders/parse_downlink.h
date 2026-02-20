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
#ifndef __PARSE_DOWNLINK_H__
#define __PARSE_DOWNLINK_H__

typedef enum{
    CH_TYPE_4_20,
    CH_TYPE_0_5
} enum_channel_type;

/* Cellular Transmission Interval */
extern uint32_t cellTransInt;

/**
 * @brief Task for parsing received downlink HTTP GET attributes
 * 
 */
void parseDownlink(void);

/**
 * @brief Function for setting status of each channel
 * 
 * @param channel   Channel 0-4
 * @param ch_type   4-20 or 0-5 channel type enum
 * @param status    0 - Disable, 1 - Enable
 * @param lowCal    Pointer to string containing low calibration value
 * @param highCal   Pointer to string containing high calibration value
 */
void setChannelStatus(uint8_t channel, enum_channel_type ch_type, bool status, char *lowCal, char *highCal);

#endif