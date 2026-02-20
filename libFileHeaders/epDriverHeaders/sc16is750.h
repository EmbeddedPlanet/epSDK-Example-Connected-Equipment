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
 * @file    sc16is750.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @author  Andrew Halverson
 * @date    28 June 2022
 * 
 * @brief Contains functions for the configuration and control of the I2C/TWI SC16IS750 UART interface.
 * 
 * Built for use with the nRF SDK 17.1
 * 
 * @see Data sheet:     https://www.nxp.com/docs/en/data-sheet/SC16IS740_750_760.pdf
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 */

#ifndef SC16IS750_H
#define SC16IS750_H

#include "nrfx_twi.h"

/** @brief Used to hold twi configuration and register data of a single sc16. Required prior to call to sc16_init.
*/
typedef struct{
    nrfx_twi_t twi;                     /** < TWI interface that hosts the SC16 */
    uint8_t address;                    /** < Address of the SC16. Has to be 0x90 - 0xAE per the data sheet section 10.3. */
    uint8_t sc16_general_registers[20]; /** < 20 byte array to hold the data of the 20 general registers of the SC16. Most functions
    within this library sync register changes with this array, backing up the SC16 data locally. In this way, if the SC16 is ever powered down, 
    settings are saved and can be sent back to the device with a call to sc16_config_send. */
    uint8_t sc16_special_registers[2];  /** < 2 byte array to hold the divisor MSB and LSB. */
    uint8_t sc16_enhanced_registers[5]; /** < 5 byte array to hold the enhanced register values */

} SC16IS750;

/** @brief SC16 register types. These are used internally only to differentiate to the sc16_twi_tx function which locally saved register to
 * pull/store data to/from. This is needed because Special and Enhanced registers share a register address with some of the general registers,
 * so data can easily be overwritten if we're not careful.
 */
typedef enum{
    SC16_GENERAL_REG,   /** < General register set. */
    SC16_SPECIAL_REG,   /** < Special register set. Holds the MSB and LSB of the divisor */
    SC16_ENHANCED_REG   /** < Ehanced register set. Mostly concerned with software flow control */
} sc16_reg_type;

/** @brief SC16 registers. These are used during transmission as well as for accessing locally storied copies
 * from the sc16_x_registers array. Detailed register descriptions starting on page 20 of the data sheet.
 */
typedef enum{
    //General register set
    SC16_RHR_REG        = 0x00, /** < Receive Holding Register (R) */
    SC16_THR_REG        = 0x00, /** < Transmit Holding register (W) */
    SC16_IER_REG        = 0x01, /** < Interrupt Enable Register (R/W) */
    SC16_IIR_REG        = 0x02, /** < Interupt Identification Register (R) */
    SC16_FCR_REG        = 0x02, /** < FIFO Control Register (W) */
    SC16_LCR_REG        = 0x03, /** < Line Control Register (R/W) */
    SC16_MCR_REG        = 0x04, /** < Modem Control Register (R/W) */
    SC16_LSR_REG        = 0x05, /** < Line Status Register (R) */
    SC16_MSR_REG        = 0x06, /** < Modem Status Register (R) */
    SC16_TCR_REG        = 0x06, /** < Transmission Control Register (R/W) */
    SC16_SPR_REG        = 0x07, /** < Scrathpad Register (R/W) */
    SC16_TLR_REG        = 0x07, /** < Trigger Level Register (R/W) */
    SC16_TXLVL_REG      = 0x08, /** < Transmit FIFO Level Register (R) */
    SC16_RXLVL_REG      = 0x09, /** < Receive FIFO Level Register (R) */
    SC16_IODIR_REG      = 0x0A, /** < I/O Pin Direction Register (R/W) */
    SC16_IOSTATE_REG    = 0x0B, /** < I/O Pin States Register (R/W) */
    SC16_IOINTENA_REG   = 0x0C, /** < I/O Interrupt Enable Register (R/W)*/
    SC16_RSVD_REG       = 0x0D, /** < Reserved Register (N/A) */
    SC16_IOCTRL_REG     = 0x0E, /** < I/O Pins Control Register (R/W) */
    SC16_EFCR_REG       = 0x0F, /** < Extra Features Register (R/W) */

    //Special register set
    SC16_DLL_REG    = 0x00, /** < Divisor Latch LSB (R/W) */
    SC16_DLH_REG    = 0x01, /** < Divisor Latch MSB (R/W) */

    //Enhanced register set
    SC16_EFR_REG    = 0x02, /** < Enhanced Feature Register (R/W) */
    SC16_XON1_REG   = 0x04, /** < Xon1 Word (R/W) */
    SC16_XON2_REG   = 0x05, /** < Xon2 Word (R/W) */
    SC16_XOFF1_REG  = 0x06, /** < Xoff1 Word (R/W) */
    SC16_XOFF2_REG  = 0x07  /** < Xoff2 Word (R/W) */

} sc16_register;

/** @brief Holds the options for trigger levels for the RX FIFO. Used with sc16_rx_trigger_lvl.
 */
typedef enum{
    SC16_RX_TRIG_8_chars,
    SC16_RX_TRIG_16_chars,
    SC16_RX_TRIG_56_chars,
    SC16_RX_TRIG_60_chars
} sc16_rx_trigger_lvl_enum;

/** @brief Holds the options for trigger levels for the tx FIFO. Used with sc16_tx_trigger_lvl.
 */
typedef enum{
    SC16_TX_TRIG_8_spaces,
    SC16_TX_TRIG_16_spaces,
    SC16_TX_TRIG_32_spaces,
    SC16_TX_TRIG_56_spaces
} sc16_tx_trigger_lvl_enum;

/** @brief Holds the options for tx/rx word length. Used with sc16_set_comm_format.
 */
typedef enum{
    SC16_WORD_LEN_5_BITS,
    SC16_WORD_LEN_6_BITS,
    SC16_WORD_LEN_7_BITS,
    SC16_WORD_LEN_8_BITS,
} sc16_word_length_enum;

/** @brief Holds the options for parity. Used with sc16_set_comm_format.
 */
typedef enum{
    SC16_NO_PARITY      = 0x00,
    SC16_ODD_PARITY     = 0x01,
    SC16_EVEN_PARITY    = 0x03
} sc16_parity_enum;

/** @brief Used with sc16_interrupts_en to enabled interrupt triggers. An ORing of the members
 * of these enum can be passed to sc16_interrupts_en.
 */
typedef enum{
    SC16_RHR_INT_EN     = 0x01, /** < Receive Holding Register Interrupt */
    SC16_THR_INT_EN     = 0x02, /** < Transmit Holding Register Interrupt */
    SC16_REC_INT_EN     = 0x04, /** < Receive Line Status Interrupt */
    SC16_MODEM_INT_EN   = 0x08, /** < Modem Status Interrupt */
    SC16_XOFF_INT_EN    = 0x20, /** < Xoff Interrupt */
    SC16_RTS_INT_EN     = 0x40, /** < RTS interrupt */
    SC16_CTS_INT_EN     = 0x80  /** < CTS Interrupt */
} sc16_interrupt_enable_enum;

/** @brief Returned by sc16_get_interrupt to indicate the source of the buffered interrupt
 */
typedef enum{
    SC16_NO_INT                 = 0x00, /** < No interrupt is pending */
    SC16_INT_PENDING            = 0x01, /** < An interrupt is pending */
    SC16_REC_LINE_STATUS_INT    = 0x06, /** < Receiver Line Status Error */
    SC16_REC_TIMEOUT_INT        = 0x0C, /** < Receiver Time-Out */
    SC16_RHR_INT                = 0x04, /** < RHR Interrupt */
    SC16_THR_INT                = 0x02, /** < THR Interrupt */
    SC16_XOFF_INT               = 0x10, /** < Received Xoff Signal / Special Character */
    SC16_CTS_RTS_INT            = 0x20  /** < CTS, RTS change of state from active (LOW) to inactive (HIGH) */
} sc16_interrupt_id_enum;

/** @brief All possible software flow control settings for Xon/Xoff characters. Used as a parameter to
 * sc16_set_soft_flow_ctrl. See data sheet page 11 section 7.3 for details.
 * 
 * Pass an ORing of the TX/RX enums into sc16_set_soft_flow_ctrl. For example, to use Xon1 and Xoff1 only
 * for TX and RX, pass in as sc16_set_soft_flow_ctrl(&sc16, SC16_TX_XON1_XOFF1 | SC16_RX_XON1_XOFF1)
 */
typedef enum{
    //TX
    SC16_TX_XON1_XOFF1  = 0x08,
    SC16_TX_XON2_XOFF2  = 0x04,
    SC16_TX_XON1_XOFF1_XON2_XOFF2 = 0x0C,
    //RX
    SC16_RX_XON1_XOFF1  = 0x02,
    SC16_RX_XON2_XOFF2  = 0x01,
    SC16_RX_XON1_XOFF1_XON2_XOFF2 = 0x03,
} sc16_soft_flow_ctrl_enum;

/** @brief SC16 pin names. Used with the sc16_io functions for ease of configuring
 * and toggling I/O pins.
 */
typedef enum{
    SC16_GPIO0 = 0x01,              /** < GPIO pin 0 */
    SC16_GPIO1 = 0x02,              /** < GPIO pin 1 */
    SC16_GPIO2 = 0x04,              /** < GPIO pin 2 */
    SC16_GPIO3 = 0x08,              /** < GPIO pin 3 */
    SC16_GPIO4 = 0x10,              /** < GPIO pin 4 */
    SC16_GPIO5 = 0x20,              /** < GPIO pin 5 */
    SC16_GPIO6 = 0x40,              /** < GPIO pin 6 */
    SC16_GPIO7 = 0x80,              /** < GPIO pin 7 */
    SC16_GPIO_NONE = 0x00,          /** < All GPIO pins = 0 */
    SC16_GPIO_ALL = 0xFF,           /** < All GPIO pins = 1 */
    SC16_GPIO_ALL_OUTPUT = 0x00,    /** < All GPIO pins = 0 */
    SC16_GPIO_ALL_INPUT = 0xFF      /** < All GPIO pins = 1 */
}sc16_gpio_pins_enum;



/***************************************
 * Public Functions
 ***************************************/

/**
 * @brief   Inits an SC16 struct with a device address & the twi used. Turns on enhanced features, 
 * retrieves current register data from device TODO: Initial register retrieval in next version
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param address   Address of the sc16. Check the data sheet section 10.3 table 32 for possible addresses
 * @param twi       The twi port of the nRF used
 */
void sc16_init(SC16IS750 *sc16, uint8_t address, nrfx_twi_t twi);


//8.1 RHR
/**
 * @brief   Retrieves character on top of the sc16 RX FIFO and returns it to caller.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  Character received from sc16
 */
char sc16_rx_char(SC16IS750 *sc16, nrfx_err_t *ret_code);


//8.2 THR
/**
 * @brief   Transmits a specified amount of characters to the sc16 TX FIFO.
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param data  Pointer to a byte array that holds the characters to be transmitted
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t sc16_tx(SC16IS750 *sc16, char* data); //TODO: Next version

/**
 * @brief   Transmits a character to the sc16 TX FIFO.
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param ch    Character to send to TX FIFO
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t sc16_tx_char(SC16IS750 *sc16, char ch);

//8.3 FCR:
/**
 * @brief   Enables/disabled the sc16 FIFO.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 to disabled the FIFO
 *                  1 to enable the FIFO
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t sc16_enable_fifo(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Resets the sc16 RX FIFO.
 * 
 * @param sc16  Pointer to an sc16 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t sc16_rx_reset_fifo(SC16IS750 *sc16);

/**
 * @brief   Resets the sc16 TX FIFO.
 * 
 * @param sc16  Pointer to an sc16 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_tx_reset_fifo(SC16IS750 *sc16);

/**
 * @brief   Set ability to change enhanced functionality
 * 
 * @param sc16  Pointer to an sc16 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_enhanced_reg_enable(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Set enhanced functionality
 * 
 * @param sc16  Pointer to an sc16 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_set_enhanced(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Sets the sc16 TX FIFO trigger level
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param lvl   Number of spaces
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_tx_trigger_lvl(SC16IS750 *sc16, sc16_tx_trigger_lvl_enum lvl);

/**
 * @brief   Sets the sc16 RX FIFO trigger level
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param lvl   Number of characters
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_rx_trigger_lvl(SC16IS750 *sc16, sc16_rx_trigger_lvl_enum lvl);

//8.4 LCR:
/**
 * @brief   Sets the sc16 data communication format.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param wrd_len   SC16_WORD_LEN_5_BITS
                    SC16_WORD_LEN_6_BITS
                    SC16_WORD_LEN_7_BITS
                    SC16_WORD_LEN_8_BITS
 * @param stop_bit  0 for 0 to 1 stop bit if word length is 5, 6, 7, or 8
 *                  1 for 1 to 1.5 stop bits if word length is 5
 *                  1 for 2 stop bits if word length is 6, 7, 8
 *                  Note: Data sheet is not clear on this parameter. Refer to section 8.4 table 12.
 * @param parity    SC16_NO_PARITY
                    SC16_ODD_PARITY
                    SC16_EVEN_PARITY
 * @param force     0 if parity shall not be forced
                    1 to force parity based on the parity parameter of this function
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_set_comm_format(SC16IS750 *sc16, sc16_word_length_enum wrd_len, bool stop_bit, sc16_parity_enum parity, bool force);

/**
 * @brief   If enabled, causes a break condition to be transmitted (TX output isforced to a logic 0 state).
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param tx_break  0 = no TX break condition
 *                  1 = generates a TX break condition
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_tx_break(SC16IS750 *sc16, bool tx_break);

//8.5 LSR:
//TODO: The LSR functions below may be better utilized as internal error checking functions. Look into for future release
/**
 * @brief   Checks if there is data ready in the sc16 RX FIFO
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No data in RX FIFO
 *          1 = At least one character in RX FIFO
 * */
bool sc16_check_rx_fifo(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if an overrun error has occurred
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No overrun error has occurred
 *          1 = An overrun error has occurred
 * */
bool sc16_check_overrun(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if a parity error has occurred
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No parity error has occurred
 *          1 = A parity error has occurred
 * */
bool sc16_check_parity(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if a framing error has occurred
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No framing error has occurred
 *          1 = A framing error has occurred
 * */
bool sc16_check_framing(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if a break condition has occurred
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No break condition has occurred
 *          1 = A break condition has occurred
 * */
bool sc16_check_break(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if the TX hold register is empty
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = The TX hold register is not empty
 *          1 = The TX hold register is empty
 * */
bool sc16_check_transmit_holding(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if the TX hold register AND the TX shift register are empty
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = The TX hold register and shift registers are not empty
 *          1 = The TX hold register and shift registers are empty
 * */
bool sc16_check_transmit_empty(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Checks to see if a FIFO data error has occurred
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 = No error has occurred
 *          1 = At least one parity error, framing error, or break indication is in the receiver FIFO. 
 *              This bit is cleared when no more errors are present in the FIFO.
 * */
bool sc16_fifo_err_status(SC16IS750 *sc16, nrfx_err_t *ret_code);

//8.6 MCR:
//TODO: MODEM CONTROL NOT IMPLEMENTED

//8.7 MSR:
//TODO: MODEM STATUS NOT IMPLEMENTED

//8.8 SPR:
/**
 * @brief   Write to scratch pad register onboard the sc16
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param data  8-bit value to write to scratch
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_write_scratch(SC16IS750 *sc16, uint8_t data);

/**
 * @brief   Read from scratch pad register onboard the sc16
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  8-bit value read from scratch pad register
 * */
uint8_t sc16_read_scratch(SC16IS750 *sc16, nrfx_err_t *ret_code);

//8.9 IER
/**
 * @brief   Enables specific interrupts on the sc16
 * 
 * @param sc16          Pointer to an sc16 struct
 * @param interrupts    An ORing of the interrupts to be activated on the sc16
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_interrupts_en(SC16IS750 *sc16, sc16_interrupt_enable_enum interrupts); //Does not include modem or IO ints

//8.10 IIR
/**
 * @brief   Reads the source of an interrupt in  a prioritized manner   
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  Interrupt identifier. Refer to section 8.10 table 21 of the data sheet
 * */
sc16_interrupt_id_enum sc16_get_interrupt(SC16IS750 *sc16, nrfx_err_t *ret_code);

//8.11 EFR //TODO: Not yet implemented properly
/**
 * @brief   Sets up software flow control
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param flow_ctrl An ORing of the TX/RX software flow control schemes
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_set_soft_flow_ctrl(SC16IS750 *sc16, sc16_soft_flow_ctrl_enum flow_ctrl);

/**
 * @brief   Enables special character detection
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param ch    Character to be used                                                                       
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_set_special_char(SC16IS750 *sc16, char ch); //This also has to adjust Xoff2 reg

/**
 * @brief   Sets hardware flow control configuration
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param cts_en    1 = enable CTS flow control
 *                  0 = disable CTS flow control
 * @param rts_en    1 = enable RTS flow control
 *                  0 = disabled RTS flow control                                                       
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_set_hard_flow_ctrl(SC16IS750 *sc16, bool cts_en, bool rts_en);

//8.12 DLL DLH
/**
 * @brief   Sets the divisor used to generate the baud rate. The divisor depends on the clock speed and baud
 *          rate needed. Refer to section 7.8 of the data sheet for details.
 * 
 * @param sc16              Pointer to an sc16 struct
 * @param baud_rate         Baud rate required
 * @param crystal_frequency Frequency of attached crystal, in MHz                                                 
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_configure_baud(SC16IS750 *sc16,  uint32_t baud_rate, float crystal_frequency_MHz);

//8.13 TCR //TODO: NOT IMPLEMENTED - DEPENDS ON MCR
//nrfx_err_t sc16_set_rx_fifo_threshold(SC16IS750 *sc16, uint8_t halt, uint8_t resume); //0-60, increments of four. Figure out best way to parse. enum? conversion?

//8.14 TLR
//Do not implement. This driver uses the FCR register instead for this setting

//8.15 TXLVL
/**
 * @brief   Returns the number of spaces available in the TX FIFO
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  8-bit integer representing the number of spaces available in the TX FIFO (0-64)
 * */
uint8_t sc16_check_tx_level(SC16IS750 *sc16, nrfx_err_t *ret_code);

//8.16 RXLVL
/**
 * @brief   Returns the number of characters (fill level) in the RX FIFO
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  8-bit integer representing the number of characters in the RX FIFO (0-64)
 * */
uint8_t sc16_check_rx_level(SC16IS750 *sc16, nrfx_err_t *ret_code);

//8.17 IODir
/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Used to program I/O pin direction.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param inputs    An ORing of the GPIO pins to be inputs. All others will default to outputs.                                                    
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_direction(SC16IS750 *sc16, sc16_gpio_pins_enum inputs);

//8.18 IOState
/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Writes high/low to ALL pins.
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param pin   An ORing of the pins that shall be set to logic high (default none).
 * 
 * For ease of use, an ORing of elements in the sc16_gpio_pins_enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to be low.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_write_pins(SC16IS750 *sc16, sc16_gpio_pins_enum pins);

/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Writes high/low to a single pin.
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param pin   A single pin to be written to
 * @param level 0 = low
 *              1 = high
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_write_pin(SC16IS750 *sc16, sc16_gpio_pins_enum pin, bool level);

/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Returns the current read state of all I/O pins
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  An 8-bit unsigned integer with bits [7:0] representing the current state of the corresponding GPIO pins.
 * */
uint8_t sc16_io_read_pins(SC16IS750 *sc16, nrfx_err_t *ret_code);

/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Returns the current read state of a single I/O pin.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param pin       A single pin to be read
 * @param ret_code  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 if pin is state is low
 *          1 if pin if pin state is high
 * */
bool sc16_io_read_pin(SC16IS750 *sc16, sc16_gpio_pins_enum pin, nrfx_err_t *ret_code);

//8.19 IOIntEna
/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Used to enable interrupts due to a change
 *          in the I/O configured as inputs. Note that if GPIO[7:4] are programmed as modem pins, 
 *          their interrupt generation must be enabled via IER register bit 3. In this case bit 7 
 *          to bit 4 of IOIntEna will have no effect on GPIO[7:4].
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param pins  An ORing of the pins that shall be configured to cause an interrupt on change.
 * 
 * For ease of use, an ORing of elements in the sc16_gpio_pins_enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically to not cause an interrupt. 
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_enable_interrupts(SC16IS750 *sc16, sc16_gpio_pins_enum pins);

//8.20 IOControl
/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Used to enable/disable input latching during interrupt.
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = input values are not latched. A change in any input generates aninterrupt. A read of 
 *                      the input register clears the interrupt. If the input goes back to its initial logic 
 *                      state before the input register is read, then the interrupt is cleared.
 *                  1 = input values are latched. A change in the input generates an interrupt and the input 
 *                      logic value is loaded in the bit of the corresponding input state register (IOState). 
 *                      A read of the IOState register clears the interrupt. If the input pin goes back to its 
 *                      initial logic state before the interrupt register is read, then the interrupt is not 
 *                      cleared and the corresponding bit of the IOState register keeps the logic value that 
 *                      initiates the interrupt.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_latch_en(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Used to set/unset GPIO[7:4] as I/O pins or modem
 *          pins (RI, CD, DTR, DSR).
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = GPIO[7:4] behave as I/O pins
 *                  1 = GPIO[7:4] behave as RI, CD, DTR, DSR
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_io_modem_en(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Only available on the SC16IS750 and SC16IS760. Used to perform a software reset on the sc16.
 * 
 * @param sc16      Pointer to an sc16 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_reset(SC16IS750 *sc16);

//8.21 EFCR
/**
 * @brief   Used to enable 9-bit or Multidrop mode (RS-485)
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = normal RS-232 mode
 *                  1 = enables RS-485 mode 
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_rs485_en(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Used to enable/disable the receiver. 
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = Receiver is enabled
 *                  1 = Receiver is disabled 
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_rx_en(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Used to enable/disable the transmitter. 
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = Transmitter is enabled
 *                  1 = Transmitter is disabled 
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_tx_en(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Used to enable/disable the transmitter to control the RTS pin. 
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param enabled   0 = Transmitter does not control RTS pin
 *                  1 = Transmitter control RTS pin
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_rts_ctrl(SC16IS750 *sc16, bool enabled);

/**
 * @brief   Used to set polarity of RTS signal in RS-485 mode
 * 
 * @param sc16      Pointer to an sc16 struct
 * @param inverted  0 = RTS = 0 during transmission and RTS = 1 during reception
 *                  1 = RTS = 1 during transmission and RTS = 0 during reception
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_rts_invert(SC16IS750 *sc16, bool inverted);

/**
 * @brief   Used to set the IrDA mode
 * 
 * @param sc16  Pointer to an sc16 struct
 * @param mode  0 = IrDA SIR, 3v16 pulse ratio, data rate up to 115.2 kbit/s
 *              1 = IrDA SIR, 1v4 pulse ratio, data rate up to 1.152 Mbit/s
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * */
nrfx_err_t sc16_irda_mode(SC16IS750 *sc16, bool mode);

#endif