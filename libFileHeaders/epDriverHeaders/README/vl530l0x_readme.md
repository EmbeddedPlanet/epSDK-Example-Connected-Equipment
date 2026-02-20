# VL53l0X Driver

This is a driver for the ST VL53L0X Time of Flight Sensor. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

## Contents
**vl53l0x.h** - Driver header.

## Set up
1. Include **vl53l0x.h** in the necessary source files
2. Add the include path for **vl53l0x.h** to your Makefile
3. Add the path of the associated static library to your Makefile

## Use
Detailed use can be found in the header file. However, the most simplified steps are as follows:

```C++
/* Create empty VL53L0X struct */
VL53L0X tof;

/* Initialize vl53l0x struct with the proper settings.  */
nrfx_err_t err = vl53l0x_init(&tof, twi);

/* Get data from vl53l0x to float.  */
static float vl5_data;
vl5_data = vl53l0x_get_data(&tof, &err);

}
```

## Versions
- V1.0.0 Initial Release  

