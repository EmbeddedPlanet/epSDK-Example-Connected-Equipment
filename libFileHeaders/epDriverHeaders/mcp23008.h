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
 * @file    mcp23008.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  Dan Maher
 * @date    7 June 2022
 * 
 * @brief Contains functions for the configuration and control of the I2C/TWI MCP23008 8-bit I/O expander.
 * 
 * Built for use with the nRF SDK 17.1
 * 
 * @see Data sheet:     https://ww1.microchip.com/downloads/en/DeviceDoc/21919e.pdf
 * @see Error codes:    https://infocenter.nordicsemi.com/index.jsp?topic=%2Fsdk_nrf5_v17.1.0%2Fgroup__nrfx__error__codes.html
 */

#ifndef MCP23008_H
#define MCP23008_H

#include "nrfx_twi.h"

/** @brief Used to hold twi configuration and register data of a single mcp23008. Required prior to call to mcp23008_init.
*/
typedef struct{
    nrfx_twi_t twi;             /** < TWI interface that hosts the mcp23008 */
    uint8_t address;            /** < Address of the mcp23008. Has to be 37 - 39 per the data sheet DS21919E page 8. */
    uint8_t mcp_registers[11];  /** < 11 byte array to hold the data of the 11 registers of the mcp23008. Most functions
    within this library sync register changes with this array, backing up the mcp23008 data locally. In this way, if the 
    mcp23008 is ever powered down, settings are saved and can be sent back to the device with a call to mcp23008_config_send. */    
} MCP23008;

/** @brief MCP23008 registers. These are used during transmission as well as for accessing locally storied copies
 * from the mcp_registers array. Detailed register descriptions starting on page 10 of the data sheet DS21919E.
 */
typedef enum{
    REG_IODIR,      /** < I/O DIRECTION */
    REG_IPOL,       /** < INPUT POLARITY */
    REG_GPINTEN,    /** < INTERRUPT-ON-CHANGE CONTROL */
    REG_DEFVAL,     /** < DEFAULT COMPARE VALUES FOR INTERRUPT-ON-CHANGE */
    REG_INTCON,     /** < INTERRUPT CONTROL */
    REG_IOCON,      /** < CONFIGURATION */
    REG_GPPU,       /** < PULL-UP RESISTOR CONFIGURATION */
    REG_INTF,       /** < INTERRUPT FLAG */
    REG_INTCAP,     /** < INTERRUPT CAPTURE */
    REG_GPIO,       /** < PORT */
    REG_OLAT        /** < OUTPUT LATCH REGISTER */
} mcp23008_register;

/** @brief MCP23008 pin names for use as parameter helper. Many functions in this library allow for the ORing
 * of pins as a parameter using these labels.
 */
typedef enum{
    MCP23008_GPIO0 = 0x01,              /** < GPIO pin 0 */
    MCP23008_GPIO1 = 0x02,              /** < GPIO pin 1 */
    MCP23008_GPIO2 = 0x04,              /** < GPIO pin 2 */
    MCP23008_GPIO3 = 0x08,              /** < GPIO pin 3 */
    MCP23008_GPIO4 = 0x10,              /** < GPIO pin 4 */
    MCP23008_GPIO5 = 0x20,              /** < GPIO pin 5 */
    MCP23008_GPIO6 = 0x40,              /** < GPIO pin 6 */
    MCP23008_GPIO7 = 0x80,              /** < GPIO pin 7 */
    MCP23008_GPIO_NONE = 0x00,          /** < All GPIO pins = 0 */
    MCP23008_GPIO_ALL = 0xFF,           /** < All GPIO pins = 1 */
    MCP23008_GPIO_ALL_OUTPUT = 0x00,    /** < All GPIO pins = 0 */
    MCP23008_GPIO_ALL_INPUT = 0xFF      /** < All GPIO pins = 1 */
}mcp23008_pins;

/***************************************
 * Public Functions
 ***************************************/

/**
 * @brief   Inits an mcp23008 struct with a device address & the twi used.
 * 
 * @param mcp23008  Pointer to an MCP23008 struct
 * @param address   Address of the mcp23008. Can only be 32 - 39 per the data sheet DS21919E page 8.
 * @param twi       The twi port of the nRF used
 * 
 * @return  false if address out of range
 *          true if init successful
 */
bool mcp23008_init(MCP23008 *mcp23008, uint8_t address, nrfx_twi_t twi);

/**
 * @brief   Sets the direction of the mcp23008 I/O pins.
 * 
 * Configures register IODIR - Data sheet DS21919E page 10
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param input_pins    An ORing of the pins that shall be input pins on the mcp23008. (default all outputs)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to be output pins.
 *                      
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_direction(MCP23008 *mcp23008, mcp23008_pins input_pins);

/**
 * @brief Sets the polarity on the corresponding mcp23008 I/O pins.
 * 
 * Configures register IPOL - Data sheet DS21919E page 11
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pins          An ORing of the input pins that shall show reverse polarity on the corresponding GPIO
 * port bits of the GPIO register. If a bit is set, the corresponding GPIO register bit will reflect the inverted 
 * value on the pin. (default all zeros)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to have normal polarity.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_polarity(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief Configures the interrupt trigger for each mcp23008 GPIO pin. Note that the actual behavior of the interrupt depends
 * on the settings in register DEFVAL, INTCON, and IOCON registers.
 * 
 * Configures register GPINTEN - Data sheet DS21919E page 12
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pins          An ORing of the GPIO pins that shall trigger an interrupt on pin 8 of the mcp23008. (default all zeros)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to not trigger an interrupt.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_interrupt_on_change(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief When activated per-pin via the INTCON register, sets the default comparison value for pins configured as interrupts.
 * An opposite value on the associated pin will cause an interrupt to occur. 
 * 
 * Configures register DEFVAL - Data sheet DS21919E page 13
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pins          An ORing of the GPIO pins that shall trigger an interrupt when the input falls low. (default all zeros)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will trigger an interrupt when the corresponding GPIO pin
 * goes high. All pins included (or set to 1) will trigger an interrupt when the corresponding GPIO pin goes low.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_default_compare(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief Controls how the MCP23008 GPIO pin values are compared when configured as interrupt-on-change pins. If a bit
 * is set, the corresponding GPIO pin is compared against the associated bit in the DEFVAL register. if a bit value is clear,
 * the corresponding GPIO pin is compared against the previous value
 * 
 * Configures register INTCON - Data sheet DS21919E page 14
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pins          An ORing of the GPIO pins that shall trigger an interrupt based on a comparison with a corresponding
 * bit value in the DEFVAL register. (default all zeros)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to compare against
 * the previous pin value in order to trigger an interrupt.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_interrupt_control(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief Configures whether or not the MCP23008's internal register address pointer increments after a read/write,
 * or if it does not increment and instead continues to point to the last accessed register.
 * 
 * Configures bit 5 SEQOP of register IOCON - Data sheet DS21919E page 15
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param enabled       0 if sequential operation shall be enabled (default)
 *                      1 if sequential operation shall be disabled
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_sequential_operation_mode(MCP23008 *mcp23008, bool enabled);

/**
 * @brief Configures whether or not slew rate is enabled for SDA output.
 * 
 * Configures bit 4 DISSLW of register IOCON - Data sheet DS21919E page 15
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param enabled       0 if slew rate shall be enabled (default)
 *                      1 if slew rate shall be disabled
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_slew_rate_control(MCP23008 *mcp23008, bool enabled);

/**
 * @brief Configures whether or not the interrupt pin 8 is an open-drain output.
 * 
 * Configures bit 2 ODR of register IOCON - Data sheet DS21919E page 15
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param enabled       0 if the interrupt pin 8 shall be an active driver output (INTPOL bit sets the polarity) (default)
 *                      1 if the interrupt pin 8 shall be an open-drain output (overrides the INTPOL bit)
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_interrupt_open_drain(MCP23008 *mcp23008, bool enabled);

/**
 * @brief Configures whether or not the interrupt pin 8 is active-high or active-low.
 * 
 * Configures bit 1 INTPOL of register IOCON - Data sheet DS21919E page 15
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param enabled       0 if the interrupt pin 8 shall be active-low (default)
 *                      1 if the interrupt pin 8 shall be active-high
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_interrupt_polarity(MCP23008 *mcp23008, bool polarity);

/**
 * @brief Controls the internal 100k pull-up resistors for each GPIO port pin
 * 
 *  Configures register GPPU - Data sheet DS21919E page 16
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pins          An ORing of the GPIO pins that shall use the internal 100k pull-up resistor (default all zeros)
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to not use the internal
 * 100k pull-up resistor.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_pullup(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief Retrieves the interrupt status of each GPIO pin from the MCP23008 that is enabled for interrupts via the
 * GPINTEN register.
 * 
 * Retrieves register INTF - Data sheet DS21919E page 17
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  An 8-bit unsigned integer with bits [7:0] representing the interrupt status of the corresponding GPIO pins.
 */
uint8_t mcp23008_read_interrupt_flag(MCP23008 *mcp23008, nrfx_err_t *ret_code);

/**
 * @brief Retrieves the GPIO port pin values that were present at the time that the last interrupt occurred. Updated
 * when an interrupt occurs, and remains unchanged until the interrupt is cleared via a read of the INTCAP or GPIO registers.
 * 
 * Retrieves register INCAP - Data sheet DS21919E page 18
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  An 8-bit unsigned integer with bits [7:0] representing the state of the corresponding GPIO pins during the last
 * interrupt occurrence.
 */
uint8_t mcp23008_read_interrupt_capture(MCP23008 *mcp23008, nrfx_err_t *ret_code);

/**
 * @brief Retrieves the current values of the GPIO pins from the GPIO register. Reading from the GPIO port also clears
 * any active interrupts.
 * 
 * Retrieves register GPIO - Data sheet DS21919E page 19
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  An 8-bit unsigned integer with bits [7:0] representing the current state of the corresponding GPIO pins.
 */
uint8_t mcp23008_read_pins(MCP23008 *mcp23008, nrfx_err_t *ret_code);

/**
 * @brief Retrieves the current value of a single GPIO pin from the GPIO register. Reading from the GPIO port also clears
 * any active interrupts.
 * 
 * Retrieves register GPIO - Data sheet DS21919E page 19
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pin           Pin to be read. Can use an element from the mcp23008_pins enum, or just a number 0-7.
 * @param ret_code      0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 * 
 * @return  0 if pin is state is low
 *          1 if pin if pin state is high
 */
bool mcp23008_read_pin(MCP23008 *mcp23008, mcp23008_pins pin, nrfx_err_t *ret_code);

/**
 * @brief Sets the output values of all GPIO pins.
 * 
 * Sets register OLAT - Data sheet DS21919E page 20
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pin           An ORing of the pins that shall be set to logic high (default none).
 * 
 * For ease of use, an ORing of elements in the mcp23008_pins enum can be used, or just a raw byte where bits 7:0 
 * correspond to I/O pins 7:0. All pins not included (or set to 0) will be automatically configured to be low.
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_write_pins(MCP23008 *mcp23008, mcp23008_pins pins);

/**
 * @brief Sets the output value of a single GPIO pin.
 * 
 * Sets register OLAT - Data sheet DS21919E page 20
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * @param pin           Pin to be written. Can use an element from the mcp23008_pins enum, or just a number 0-7.
 * @param level         0 for logic low
 *                      1 for logic high
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_write_pin(MCP23008 *mcp23008, mcp23008_pins pin, bool level);

/**
 * @brief   Sends locally stored configuration, the contents of the mcp_registers array, to the mcp23008.
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_config_send(MCP23008 *mcp23008);

/**
 * @brief   Received configuration from mcp23008 and saves it locally into the mcp_registers array.
 * 
 * @param mcp23008      Pointer to an MCP23008 struct
 * 
 * @return  0 if successful. For all others, refer to the Global Error Codes document at the top of this file.
 */
nrfx_err_t mcp23008_config_receive(MCP23008 *mcp23008);

#endif