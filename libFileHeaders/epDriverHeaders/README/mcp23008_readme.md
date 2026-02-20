# MCP23008 Driver

This is a driver for the Microchip MCP23008 8-Bit I2C I/O Expander. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

## Contents
**mcp23008.h** - Driver header.  

## Set up
1. Include **mcp23008.h** in the necessary source files
2. Add the include path for **mcp23008.h** to your Makefile
3. Add the path for the associated static library to your Makefile

## Use
Detailed use can be found in the header file. However, the most simplified steps are as follows:

```C++
/* Create empty MCP struct */
MCP23008 mcp;

/* Initialize mcp struct with the proper settings. In this case, the device address 39 and a twi interface previously created: */
mcp23008_init(&mcp, 39, twi);

/* Set GPIO pin 2 and 7 to input, all others to output */
mcp23008_direction(&mcp, MCP23008_GPIO2 | MCP23008_GPIO7);

/* Set GPIO pin 3 to high: */
mcp23008_write_pin(&mcp, MCP23008_GPIO3, 1);

/* Set entire GPIO port to high except for GPIO pins 2 and 7: */
mcp23008_write_pins(&mcp, 0x7b);

/* Read GPIO 2: */
result = mcp23008_read_pin(&mcp, MCP23008_GPIO2, &err);
DBGI("GPIO 2: %d\r\n", result);

/* Read entire GPIO register: */
result = mcp23008_read_pins(&mcp, &err);
DBGI("GPIO register: %x\r\n", result);
```

## Versions
V1.0.0 Initial Release
