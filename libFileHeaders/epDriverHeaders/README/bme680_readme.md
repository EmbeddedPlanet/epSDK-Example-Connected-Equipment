# BME680 Driver
This directory contains the low-level driver, Bosch BSEC library, and supporting integration files for the Bosch BME680 gas, pressure, temperature & humidity sensor. It utilizes the nRF SDK for use with the nRF52XXX series SoCs.

As described below, this Bosch-provided driver and supporting files use an independent sensor task that runs continuously, so FreeRTOS is needed for task, timer, semaphore, and mutex management.

## Contents
- **BME68x.h & bme68x_defs.h** - Low-level driver for the sensor. ([Bosch provided, unmodified](https://github.com/BoschSensortec/BME68x-Sensor-API))  
  
- **bsec_serialized_configurations_iaq.h** - Holds the configuration blob. See the **configuration** section for details. ([Bosch provided, unmodified](https://www.bosch-sensortec.com/software-tools/software/bsec/))    
  
- **bsec_interface.h & bsec_datatypes.h** - Provides access to the Bosch BSEC library for post-processing of sensor data. This is used to generate virtual sensor information, such as co2 levels and breath VOC. ([Bosch provided, unmodified](https://www.bosch-sensortec.com/software-tools/software/bsec/))  

- **libalgobsec.a** - Precompiled library that bsec_interface.h uses. Note that the Cortex_M4F version must be used. ([Bosch provided, unmodified](https://www.bosch-sensortec.com/software-tools/software/bsec/))  

- **bme680.h** - This is where the sensor task and the user functions reside. ([Bosch provided (original file name bsec_integration.c/h), modified by Embedded Planet](https://www.bosch-sensortec.com/software-tools/software/bsec/))  

### Note  
The BME68x low-level driver and bsec libraries are provided by Bosch and are unmodified. All functions needed to interface with the nRF52XXX and the application code are located in the bme680.h files, which have been modified slightly to work on Embedded Planet products.  

## Set Up
1. Include **bme680.h** in the necessary source files
2. Add the include path for **all included header files** to your Makefile
3. Add the path for the associated EP static library to your Makefile
4. Add the precompiled .a BSEC library to the linker input of your Makefile. For example:
```C++
# Add in BME680 BSEC libraries
LIB_FILES += -L$(PROJ_DIR)/source/ep-nrf/drivers/bme680 -l:libalgobsec.a 
```

### Note
This driver requires several FreeRTOS and nRF SDK libraries and utilities. Review the example.c file for specific header files and functions required for set up.  
&nbsp;

## Use
Detailed use can be found in the **bme680.h** header file. However, the most simplified steps are as follows:

```C++
/* Initialize the bme680 sensor and library with the proper settings. In this case, a twi interface previously created, the sample rate, and the temperature offset.

This also creates a bme680 FreeRTOS task. */
bme680_init(twi, BSEC_SAMPLE_RATE_LP, 0.0f);

/* Create your other tasks. Here we create a task that points to the function "application_loop that will be used for this example */
xTaskCreate(application_loop, "app_loop", 1000, NULL, 1, NULL);

/* Start the FreeRTOS scheduler */
vTaskStartScheduler();
```

Now that the bme680 has been initialized and the task has been started, let's take a closer look inside the example application_loop function to see how we can retrieve and use sensor data. Note that the semaphore xBme680DataReadySemaphore is a freeRTOS semaphore set up and provided by the BME680 driver, and is used to indicate that new data from the sensor is ready.
```C++
/* This example task continuously waits for the BME680 to have data ready, and then prints the values to the console */
void application_loop(void *pvParameters){
    while(1){
        /* Wait for BME680 semaphore, which is given by the bme680 interface */
        bool data_received = xSemaphoreTake(xBme680DataReadySemaphore, pdMS_TO_TICKS(5000));

        if(data_received == pdTRUE){
            /* Once data is received, we access it via the bme680_get_latest_data function, which returns a 
            bme680_sensor_data struct. This struct contains all physical and virtual sensor data from the last reading. */
            bme680_sensor_data read_data = bme680_get_latest_data();

            /* We can then use the data how we wish. In this case, we are printing it to the console. */
            int timestamp_ms = read_data.timestamp/1000000UL;
            printf("[%d]\r\n", timestamp_ms);

            printf("Raw Temp: %2.2f | ", read_data.raw_temp);
            printf("Compensated Temp: %2.2f | ", read_data.temp);
            printf("Raw rH: %2.2f | ", read_data.raw_humidity);
            printf("Compensated rH: %2.2f | ", read_data.humidity);
            printf("kPa: %2.2f\r\n", read_data.raw_pressure);

            printf("Gas Stab Status: %d | ", read_data.stabStatus);
            printf("Gas Run-in Status: %d | ", read_data.runInStatus);
            printf("Raw Gas: %2.2f | ", read_data.raw_gas);
            printf("Gas : %2.2f | ", read_data.gas_percentage);
            printf("co2: %2.2f | ",read_data. co2_equivalent);
            printf("co2 Accuracy: %d\r\n",read_data. co2_accuracy);

            printf("Breath VOC: %2.2f | ", read_data.breath_voc_equivalent);
            printf("Breath VOC Accuracy: %d | ", read_data.breath_voc_accuracy);
            printf("Static IAQ: %2.2f | ", read_data.static_iaq);
            printf("Static IAQ Accuracy: %d | ", read_data.static_iaq_accuracy);
            printf("IAQ: %2.2f | ", read_data.iaq);
            printf("IAQ Accuracy: %d\r\n\r\n", read_data.iaq_accuracy);

        }else{
            printf("Data not received in time alotted! Retrying...\r\n");
        }
    }
}
```

This will print out data in the following format at 3-second intervals:
```console
[73210566]
Raw Temp: 24.84 | Compensated Temp: 24.78 | Raw rH: 51.28 | Compensated rH: 51.49 | kPa: 98136.36
Gas Stab Status: 1 | Gas Run-in Status: 1 | Raw Gas: 466969.97 | Gas : 50.12 | co2: 655.60 | co2 Accuracy: 1
Breath VOC: 0.69 | Breath VOC Accuracy: 1 | Static IAQ: 63.90 | Static IAQ Accuracy: 1 | IAQ: 81.82 | IAQ Accuracy: 1

```
A fairly detailed description of each data point is located in section 4.1.3.3 of the Bosch BME680 BSEC Integration Guide.  
&nbsp;

## Configuration
There are several configuration options for the BME680:

### **bme680.h defines**
The **bme680.h** header file exposes two configuration options:
- BME680_SAVE_INTERVAL defines how many samples to take between saving the BSEC state. This define is not currently used in this version (save/load state not implemented).
- BME680_DEV_ADDR defines the I2C address that the sensor is using. There are two options for the BME680, BME68X_I2C_ADDR_LOW (0x76) and BME68X_I2C_ADDR_HIGH (0x77). The default is BME68X_I2C_ADDR_LOW 

### **bme680_init function**
The **bme680_init** function take three parameters that are used for configuring the sensor and library:
- **twi** is the nRF twi interface that the sensor is using.
- **sample_rate** is the sample rate that the driver uses to communicate with the bme680. The sample rate options are pre-defined and described in **bsec_datatypes.h**. 
- **temperature_offset** is the temperate offset, in degrees C, to subtract from bme680 readings. The purpose of this is to compensate for the heat generated by other components on the device, as the BME860 already compensates for it's own heat generation.  

### **bsec_serialized_configurations_iaq.c/h**
These files hold the configuration blob for the BSEC library. There are several different options available that come with the BSEC library, and can be changed out if needed. That said, the included version should be applicable to the majority of Embedded Planet projects. The configuration sets the following:
- Supply voltage of the BME680 (1.8v or 3.3v)
- Maximum allowed time between two bsec_sensor_control calls (3s or 300s)
- The history that the BSEC library considers for the automatic background calibration of the IAQ in days (4d or 28d)

See section 1.2.2 of the BSEC BME680 Integration Guide for details and options. The included configuration is 3v, 300s, 4d.
&nbsp;
```
&nbsp;

## References
- The BSEC BME680 Integration Guide is an extremely helpful tool in understanding how the bme680.h/c (formeraly bsec_integration.c/h) and the BSEC library operate. It is downloaded alongside the BSEC library itself from [Bosch's website](https://www.bosch-sensortec.com/software-tools/software/bsec/), but it also included in this repository.  
- [BME680 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme680-ds001.pdf)  
  
&nbsp;
## Versions
V1.0.0 Initial Release