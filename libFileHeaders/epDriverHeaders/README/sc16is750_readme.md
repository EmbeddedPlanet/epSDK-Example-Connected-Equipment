# SC16IS750 Driver

This is a driver for the NXP SC16IS7X0 I2C to single-channel high UART. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

## Contents
**sc16is750.h** - Driver header.

## Set up
1. Include **sc16is750.h** in the necessary source files
2. Add the include path for **sc16is750.h** to your Makefile
3. Add the path of the associated static library to your Makefile

## Use
Simple use can be found in the header file. However, simplified steps for basic RS232 communication are as follows::

```C++
/* Create empty SC16 struct */
SC16IS750 sc16;

/* Initialize sc16 struct with the proper settings. In this case, the device address 9A and a twi interface previously created: */
sc16_init(&sc16, 0x9A, twi);

/* Make sure to set the sc16 reset line to high. Here, we're using an mcp23008 I/O expander to do this: */
mcp23008_write_pin(&mcp, MCP23008_GPIO4, 1);

/* The sc16 needs to be explicitly told the frequency crystal it is connected to in order to properly set the baud rate. In this case, 
we want a baud of 115200 and our crystal is 1.8432 MHz. */
sc16_configure_baud(&sc16, 115200, 1.8432);

/* Set the communication format. In this case word length is 8 bits, 1 stop bit, and no parity. */
sc16_set_comm_format(&sc16, SC16_WORD_LEN_8_BITS, 0, 0, 0);

/* The FIFO isn't required, but we add it here to utilize the internal tx/rx buffer on the SC16 */
sc16_enable_fifo(&sc16, true);

char ch = 'A';

/* Send a character on the TX line */
sc16_tx_char(&sc16, ch);

bool rx_ready = false;
/* Check the RX FIFO to see if there's any character ready to be received */
rx_ready = sc16_check_rx_level(&sc16, &err);

if (rx_ready){
    /* Read a character from the RX FIFO */
    ch = sc16_rx_char(&sc16, &err);
}
```

## Versions
V1.0.0 Initial Release
