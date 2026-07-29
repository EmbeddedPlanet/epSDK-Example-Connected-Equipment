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
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
// FreeRTOS Includes
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "timers.h"
#include "queue.h"
// Nordic Includes
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_freertos.h"
#include "nrf_delay.h"
#include "nrf_crypto.h"
// Project Includes
#include "main.h"
#include "ep_bsp.h"
#include "uart_helper.h"
#include "cell_helper.h"
//#include "ep_ble_central.h"
#include "htu21d.h"
#include "vl53l0x.h"
#include "bme680.h"
#include "icm20602.h"
#include "mcp23008.h"
#include "sc16is750.h"
#include "ce_adc.h"
#include "parse_downlink.h"
#include "rs485.h"
#include "qspi_helper.h"
#include "led_helper.h"
#include "time_helper.h"
#include "compact_payload_config.h"
#include "epcp_builder_ce.h"
#include "LoRaWAN.h"
#if defined(THINGSBOARD_HTTPS_INTEGRATION) || defined(AZURE_MQTTS_X509) || defined(AWS_MQTTS_X509) || defined(AWS_HTTPS_X509) || defined(AZURE_MQTTS_SAS) || defined(AZURE_HTTPS_SAS)
#include "nrf_crypto.h"
#endif

//Activate / deactivate onboard sensors
#define SI7021_ACTIVE   1
#define VL53L0X_ACTIVE  0
#define BME680_ACTIVE   0
#define ICM20602_ACTIVE 1

#define mainLED_TASK_STACK_SIZE             128
#define mainSAMPLE_TASK_STACK_SIZE          1024
#define mainCell_TASK_STACK_SIZE            8196
#define LORAWAN_CLASSA_TASK_STACK_SIZE      2048
#define LORAWAN_CLASSA_TASK_PRIORITY        ( tskIDLE_PRIORITY + 1 )
#define CELL_QUEUE_TIMEOUT                  pdMS_TO_TICKS(3000)                     /** Time to wait on queue before moving on to the next sample */

// Declare FreeRTOS tasks, queues, and semaphore
SemaphoreHandle_t cellularSemaphore;
SemaphoreHandle_t dataReadySemaphore;
SemaphoreHandle_t loraSemaphore;
SemaphoreHandle_t xSPISemaphore;
SemaphoreHandle_t xSensPwrEnSemaphore;
QueueHandle_t xCellQueue;
QueueHandle_t xLoraQueue;
TaskHandle_t ledTaskHandle;
TaskHandle_t cellularTaskHandle;
TaskHandle_t sensorSampleTaskHandle;
TaskHandle_t loraTaskHandle;
TaskHandle_t mqttTaskHandle;

system_info_struct system_info;

//Sets the comm path configuration, Cell enabled by default, can also enable LoRa
uint8_t comm_conf = COMM_CELL_ONLY;

uint32_t cellTransInt = MIN_TRANS_INTERVAL;

// Frame counter for payload
static uint8_t frameCounter = 0;

#if ICM20602_ACTIVE
/* Empty ICM struct */
ICM20602 icm;
#endif

/* Empty MCP struct */
MCP23008 mcp;

/* TWI instance ID. */
#define TWI_INSTANCE_ID 0

/* TWI instance. */
static const nrfx_twi_t twi = NRFX_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Initial PDP config */
CellularBLEConfig_t pdnConfig = { '\0' };

/* Task for testing dequeueing of data from the BLE json queue and adding to cell data queue */
void bleToCellTask(void *pvParameters);

/* Task for reading local sensors and adding to cell data queue */
void sensorSampleTask(void *pvParameters);

/* Miscellaneous initialization including preparing the logging and cell. */
static void prvMiscInitialization( void );

/* TWI initialization */
void twi_init (void)
{
    nrfx_err_t err_code;

    const nrfx_twi_config_t twi_lm75b_config = {
       .scl                = PIN_NAME_SCL,
       .sda                = PIN_NAME_SDA,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .hold_bus_uninit     = false
    };

    err_code = nrfx_twi_init(&twi, &twi_lm75b_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrfx_twi_enable(&twi);
}

/*-----------------------------------------------------------*/
static void LEDTask( void * pvParameters )
{
    led_init();

    for(;;)
    {
        if( cellStatus == CellSuccess )
        {
            led_mode(LED_CELL_OK, true, 1);
        }
        else if( cellStatus == CellFailToSend || cellStatus == CellFailToRec )
        {
            led_mode(LED_FAIL_TO_SEND, true, 1);
        }
        else
        {
            led_mode(LED_FAIL_TO_REG, true, 1);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }

    vTaskDelete( NULL );
}

/*-----------------------------------------------------------*/
void sensorPwrEnConfig( bool status )
{
    static uint8_t counter = 0;
    static bool init = true;

    //If first pass then configure to output and create semaphore
    if(init)
    {
        //Create semaphore
        xSensPwrEnSemaphore = xSemaphoreCreateMutex();
        if(xSensPwrEnSemaphore == NULL){
            DBGE("PwrEn semaphore creation failure!");
        }

        //Give semaphore
        xSemaphoreGive(xSensPwrEnSemaphore);

        //Configure output
        nrf_gpio_cfg_output(PIN_NAME_SENSOR_POWER_ENABLE);

        //Clear init flag
        init = false;
    }

    //Try to take semaphore
    if(xSemaphoreTake(xSensPwrEnSemaphore, pdMS_TO_TICKS(500)) == pdTRUE){
        //If status is false, and all tasks want this pin disabled, then clear pin
        if(status == false)
        {
            //Decrement counter if above zero
            if(counter > 0)
            {
                counter--;
            }
            //If counter is now 0, then disable
            if(counter == 0)
            {
                nrf_gpio_pin_clear(PIN_NAME_SENSOR_POWER_ENABLE);
            }
        }

        //If status is true then set pin and increase counter
        if(status == true)
        {
            nrf_gpio_pin_set(PIN_NAME_SENSOR_POWER_ENABLE);
            if(counter < 0xFF)
            {
                counter++;
            }
        }

        //Give semaphore
        xSemaphoreGive(xSensPwrEnSemaphore);
    }
}

/*-----------------------------------------------------------*/

/**@brief Function for application main entry.
 */
int main(void)
{
    /* Miscellaneous initialization including preparing the logging and cell */
    prvMiscInitialization();

    // Start FreeRTOS scheduler.
    vTaskStartScheduler();

    for (;;)
    {
        APP_ERROR_HANDLER(NRF_ERROR_FORBIDDEN);
    }
}

/*-----------------------------------------------------------*/
// Taken from FreeRTOS demo: https://github.com/FreeRTOS/FreeRTOS/blob/main/FreeRTOS-Plus/Demo/FreeRTOS_Cellular_Interface_Windows_Simulator/Common/main.c
static void prvMiscInitialization( void )
{
    nrfx_err_t err_code = NRFX_SUCCESS;

    // Initialize bsp
    ep_bsp_init( WATCHDOG_RELOAD, WATCHDOG_RELOAD_RATE );

    // Init UART
    init_swo();
    init_uart(MAIN_LOOP);
    uart_helper.dbgi = true;

    set_time(0);
    get_time_s();

    // Initialize LEDs, default to alive blink normal operation
    led_init();
    led_mode(LED_ALIVE_BLINK, 1, true);    

    // Check for version number in qpsi, otherwise use default
    ImageDescriptor_t xDescriptor;
    ProductionTable_t xProdTable;

    // Initialize qspi driver
    err_code = qspi_init();

    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Unable to initialize QSPI driver" );
    }

    nrf_delay_ms(2500);

    err_code = qspi_read( (uint8_t * ) &xDescriptor, sizeof(xDescriptor), 0 );
    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Read failed with with error code %d", err_code );
    }

    if( xDescriptor.usImageFlags == otapalIMAGE_FLAG_VALID )
    {
        DBGI("Image Flag: 0x%X",xDescriptor.usImageFlags);
        DBGI("Image Checksum: 0x%X", xDescriptor.checksum);
        DBGI("Image Version: 0x%X", xDescriptor.updateVersion);

        system_info.updateVerMaj = ( ( xDescriptor.updateVersion >> 24 ) & 0xFF );
        system_info.updateVerMin = ( ( xDescriptor.updateVersion >> 16 ) & 0xFF );
        system_info.updateVerBui = ( xDescriptor.updateVersion & 0xFFFF );
        sprintf( system_info.updateVerStr,"%02x.%02x.%02x", system_info.updateVerMaj, system_info.updateVerMin, system_info.updateVerBui );       
    }
    else
    {
        // Set integer version
        system_info.updateVerMaj = VERSION_MAJOR;
        system_info.updateVerMin = VERSION_MINOR;
        system_info.updateVerBui = VERSION_BUILD;
        // Set string version
        sprintf( system_info.updateVerStr,"%02x.%02x.%02x", system_info.updateVerMaj, system_info.updateVerMin, system_info.updateVerBui );
    }

    // Read production table in external flash
    err_code = qspi_read( (uint8_t * ) &xProdTable, sizeof(xProdTable), otapal_PROD_TBL_START );
    if( err_code != NRFX_SUCCESS )
    {
        DBGE( "Prod Table read failed with with error code %d", err_code );
    }
    else
    {
        // Use production table values in external flash
        if( strncmp( xProdTable.serialNumber, "\xFF\xFF\xFF\xFF\xFF\xFF", sizeof( xProdTable.serialNumber ) ) != 0 )
        {
            memcpy( system_info.ep_serial, xProdTable.serialNumber, sizeof( xProdTable.serialNumber ) );
            system_info.ep_serial[SERIAL_LENGTH] = '\0';
            DBGI("Device SN: %s", system_info.ep_serial);
        }
        // Use default production table values, if needed
        else
        {
            DBGE("Invalid Production Table, Programming Defaults");
        }
    }

    qspi_uninit();

    DBGI("******************************************************************");
    DBGI("* Embedded Planet CONNECTED EQUIPMENT FreeRTOS Example v%s *", system_info.updateVerStr);
    DBGI("******************************************************************");    

    // Task to read local sensor data and forward it to the cell queue
    xTaskCreate(sensorSampleTask, 
                "SensorTask", 
                mainSAMPLE_TASK_STACK_SIZE, 
                NULL, 
                tskIDLE_PRIORITY+2,
                &sensorSampleTaskHandle);
#if BLE_ACTIVE
    // Task to receive data from BLE queue and forward it to the cell queue
    xTaskCreate(bleToCellTask, 
                "BLEDataTask",
                256, 
                NULL, 
                tskIDLE_PRIORITY+2,
                NULL);
#endif

#if ENABLE_LED_OPERATION
    // LED control task
    xTaskCreate( LEDTask,
                "LEDTask",
                mainLED_TASK_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY+1,
                &ledTaskHandle );
#endif
    
    // Config ADC parameters prior to lora and cell
    adc_config();

// Ensure only one cloud interface is active
    if(comm_conf != COMM_LORA_ONLY && comm_conf != COMM_CELL_ONLY){
        DBGE("Set either LORA_ACTIVE or CELLULAR_ACTIVE to 1!!");
    }
    else if(comm_conf == COMM_LORA_ONLY){
        //Setup LoRa data queue
        xLoraQueue = xQueueCreate(LORA_QUEUE_SIZE, sizeof(lora_queue_msg));
        if(xLoraQueue == NULL){
            DBGE("xLoraQueue creation failed!");
        }
        
        uint8_t minCalString[MAX_ATTR_VALUE_LENGTH], maxCalString[MAX_ATTR_VALUE_LENGTH];
        #ifdef LORA_4_20_CH_1_ENABLE
        memcpy(minCalString,LORA_4_20_CH_1_CAL_MIN,sizeof(LORA_4_20_CH_1_CAL_MIN));
        memcpy(maxCalString,LORA_4_20_CH_1_CAL_MAX,sizeof(LORA_4_20_CH_1_CAL_MAX));
        setChannelStatus(1, CH_TYPE_4_20, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_4_20_CH_2_ENABLE
        memcpy(minCalString,LORA_4_20_CH_2_CAL_MIN,sizeof(LORA_4_20_CH_2_CAL_MIN));
        memcpy(maxCalString,LORA_4_20_CH_2_CAL_MAX,sizeof(LORA_4_20_CH_2_CAL_MAX));
        setChannelStatus(2, CH_TYPE_4_20, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_4_20_CH_3_ENABLE
        DBGE("xLoraQueue creation failed!");
        memcpy(minCalString,LORA_4_20_CH_3_CAL_MIN,sizeof(LORA_4_20_CH_3_CAL_MIN));
        memcpy(maxCalString,LORA_4_20_CH_3_CAL_MAX,sizeof(LORA_4_20_CH_3_CAL_MAX));
        setChannelStatus(3, CH_TYPE_4_20, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_4_20_CH_4_ENABLE
        memcpy(minCalString,LORA_4_20_CH_4_CAL_MIN,sizeof(LORA_4_20_CH_4_CAL_MIN));
        memcpy(maxCalString,LORA_4_20_CH_4_CAL_MAX,sizeof(LORA_4_20_CH_4_CAL_MAX));
        setChannelStatus(4, CH_TYPE_4_20, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_0_5_CH_1_ENABLE
        memcpy(minCalString,LORA_0_5_CH_1_CAL_MIN,sizeof(LORA_0_5_CH_1_CAL_MIN));
        memcpy(maxCalString,LORA_0_5_CH_1_CAL_MAX,sizeof(LORA_0_5_CH_1_CAL_MAX));
        setChannelStatus(1, CH_TYPE_0_5, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_0_5_CH_2_ENABLE
        memcpy(minCalString,LORA_0_5_CH_2_CAL_MIN,sizeof(LORA_0_5_CH_2_CAL_MIN));
        memcpy(maxCalString,LORA_0_5_CH_2_CAL_MAX,sizeof(LORA_0_5_CH_2_CAL_MAX));
        setChannelStatus(2, CH_TYPE_0_5, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_0_5_CH_3_ENABLE
        memcpy(minCalString,LORA_0_5_CH_3_CAL_MIN,sizeof(LORA_0_5_CH_3_CAL_MIN));
        memcpy(maxCalString,LORA_0_5_CH_3_CAL_MAX,sizeof(LORA_0_5_CH_3_CAL_MAX));
        setChannelStatus(3, CH_TYPE_0_5, true, minCalString, maxCalString);
        #endif

        #ifdef LORA_0_5_CH_4_ENABLE
        memcpy(minCalString,LORA_0_5_CH_4_CAL_MIN,sizeof(LORA_0_5_CH_4_CAL_MIN));
        memcpy(maxCalString,LORA_0_5_CH_4_CAL_MAX,sizeof(LORA_0_5_CH_4_CAL_MAX));
        setChannelStatus(4, CH_TYPE_0_5, true, minCalString, maxCalString);
        #endif

        /* Initialize LoRa parameters */
        loraParam loraParams = {0};
        // Set lora parameters, must be called prior to scheduler starting if using SSL/TLS due to NRF SDK incompatibilities
        loraParams.region = LORAWAN_REGION;
        loraParams.appPort = LORAWAN_APP_PORT;
        loraParams.confSend = LORAWAN_CONFIRMED_SEND;
        loraParams.jitterMS = LORAWAN_APPLICATION_JITTER_MS;
        loraParams.rxWindowMS = CLASSA_RECEIVE_WINDOW_DURATION_MS;
        loraParams.txIntervalSec = cellTransInt;
        loraParams.mosi = SER_CON_SPIS_MOSI_PIN;
        loraParams.miso = SER_CON_SPIS_MISO_PIN;
        loraParams.sck = SER_CON_SPIS_SCK_PIN;
        loraParams.subBand = subbandDefault;
        loraParams.maxJoinAttempts = lorawanConfigMAX_JOIN_ATTEMPTS;
        loraParams.commInterface = comm_conf;
        memcpy(loraParams.devEUI, devEUIDefault, sizeof(devEUI));
        memcpy(loraParams.joinEUI, joinEUIDefault, sizeof(joinEUI));
        memcpy(loraParams.appKey, appKeyDefault, sizeof(appKey));

        // Initialize LoRa parameters
        if( loraConfig( loraParams ) == false )
        {
            DBGI( "LoRa parameters failed to initialize" );
        }
        else
        {
            DBGI( "LoRa parameters successfully initialized" );  
        }

        /* Add user tasks */
        xTaskCreate( vLorawanClassATask, "LoRaWanClassA", LORAWAN_CLASSA_TASK_STACK_SIZE, NULL, LORAWAN_CLASSA_TASK_PRIORITY, &loraTaskHandle );

    }
    else if(comm_conf == COMM_CELL_ONLY){
        // Cell needs a couple of seconds before starting
        nrf_delay_ms(2000);

        DBGI( ( "Cell Task Initializing...\r\n" ) );

        cellularSemaphore = xSemaphoreCreateBinary();
        if(!cellularSemaphore){
            DBGE("cellularSemaphore creation failed!");
        }

        //Setup cellular data queue
        xCellQueue = xQueueCreate(CELL_QUEUE_SIZE, sizeof(cell_queue_msg));
        if(xCellQueue == NULL){
            DBGE("xCellQueue creation failed!");
        }

        #if defined(THINGSBOARD_HTTPS_INTEGRATION) || defined(AZURE_MQTTS_X509) || defined(AWS_MQTTS_X509) || defined(AWS_HTTPS_X509) || defined(AZURE_MQTTS_SAS) || defined(AZURE_HTTPS_SAS)
        //If protocol is MQTT, then initialize crypto here
        if( nrf_crypto_init() != NRF_SUCCESS)
        {
            DBGE(("Failed to initialize nrf crypto"));
        }
        #endif

        #if defined(AZURE_MQTTS_X509) || defined(AWS_MQTTS_X509)|| defined(AZURE_MQTTS_SAS)
        //If protocol is MQTT and persistent then create task to manage connection
        if( PERSISTENT_CONNECT_FLAG == false )
        {
            xTaskCreate( mqttTask,
                        "mqttTask",
                        MQTT_TASK_STACK_SIZE,
                        NULL,
                        tskIDLE_PRIORITY+2,
                        &mqttTaskHandle );
        }
        #endif

        // Create the task to run tests
        xTaskCreate( CellularTask,
                    "CellularTask",
                    mainCell_TASK_STACK_SIZE,
                    NULL,
                    tskIDLE_PRIORITY+2,
                    &cellularTaskHandle );
    }

    //Create semaphore for SPI
    xSPISemaphore = xSemaphoreCreateBinary();
    if(xSPISemaphore == NULL){
        DBGE("xSPISemaphore semaphore creation failure!");
    }

    //Give semaphore
    xSemaphoreGive(xSPISemaphore);

#if BLE_ACTIVE
    DBGI( ( "BLE Task Initializing .\r\n" ) );
    // Initialize BLE stack and create the SoftDevice BLE task
    ep_ble_central_init();
#endif    

    //uninit_uart(MAIN_LOOP); //comment out to prevent sleep
}

#if BLE_ACTIVE
void bleToCellTask(void *pvParameters)
{
    /* Delay for messages to start being received */
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    /* Flag for data being received */
    bool dataRec = false;

    while(1){
        /* Delay to limit CPU resources */
        vTaskDelay(pdMS_TO_TICKS(3000));

        //Get data from BLE queue
        ble_json rec_data;

        dataRec = xQueueReceive(xBleJsonQueue, &rec_data, CELL_QUEUE_TIMEOUT);

        if( dataRec )
        {
            DBGI("%s\r\n", rec_data.data);

            //Forward data to cell queue
            if(xQueueSend(xCellQueue, &rec_data, CELL_QUEUE_TIMEOUT) != pdTRUE){
                DBGE("Queue Timeout!");
            }
        }
    }
}
#endif    


/* Waits for the semaphore that is given by sensor_sample_callback. Retrieves latest local sensor data and adds to cell queue */
void sensorSampleTask(void *pvParameters)
{
    float adc_batt = 0.0;
    float adc_4_20_ch1 = 0.0;
    float adc_4_20_ch2 = 0.0;
    float adc_4_20_ch3 = 0.0;
    float adc_4_20_ch4 = 0.0;
    float adc_0_5_ch1 = 0.0;
    float adc_0_5_ch2 = 0.0;
    float adc_0_5_ch3 = 0.0;
    float adc_0_5_ch4 = 0.0;

    while(1){
        nrfx_err_t err;    
                 
        init_uart(TASK_1);                

        //Init nRF TWI interface
        twi_init();

        //Small delay needed after twi/spi init before communication can begin
        vTaskDelay(pdMS_TO_TICKS(20));
        
        //Enable sensor power        
        DBGI("Sensor power on");
        sensorPwrEnConfig(true);
        vTaskDelay(pdMS_TO_TICKS(20));

        //Get latest cell diagnostic values
        cellDiag parameters = {'\0'};
        if( comm_conf == COMM_CELL_ONLY){
            //Get latest cell diagnostic values
            if( queryCellularDiag(&parameters) != CellSuccess )
            {
                /* Do not send if unable to attain values */
                continue;
            }

            //Loop and give time for IMEI during power up
            uint8_t loopCnt = 0;
            DBGI("Waiting for IMEI to be received");
            while( parameters.imei[0] == 0 && parameters.imei[15] == 0 )
            {
                vTaskDelay(pdMS_TO_TICKS(500));
                queryCellularDiag(&parameters);
                loopCnt++;
                if(loopCnt > 240)
                {
                    break;
                }
            }
            //Cannot send data on this loop if IMEI is unavailable
            if(loopCnt > 240)
            {
                DBGE("No IMEI to receive");
                continue;
            }
            DBGI("IMEI received");
        }

        //Init MCP23008 I/O expander
        mcp23008_init(&mcp, 35, twi);
        mcp23008_direction(&mcp, MCP23008_GPIO_ALL_OUTPUT);
        mcp23008_write_pin(&mcp, MCP23008_GPIO0, true); //Turn off blue LED
        rs485_init(&mcp, twi);

        // Sensor Initialize
        #if BME680_ACTIVE
        bme680_init(twi, BSEC_SAMPLE_RATE_LP, 0.0f);
        #endif
        #if ICM20602_ACTIVE
        icm20602_init(&icm, ICM20602_ADDR_LOW, twi);
        #endif 

        parseDownlink();

        uint64_t ts = get_time_s();

        //Increment frame counter or roll over if needed
        frameCounter = (uint8_t) (frameCounter + 1);

        #if SI7021_ACTIVE
        htu21_init(twi);
        err = htu21_is_connected();
        if(err != NRFX_SUCCESS){
            DBGW("SI7021 error!");
        }
        float si_temp, si_hum;
        htu21_read_temperature_and_relative_humidity(&si_temp, &si_hum);
        #endif
        
        #if BME680_ACTIVE
        bme680_sensor_data bme_data;

        //Get new BME680 data if available
        if(xSemaphoreTake(xBme680DataReadySemaphore, pdMS_TO_TICKS(5000))){
            bme_data = bme680_get_latest_data();
        }else{
            DBGI("BME680 data not available!");
        }
        #endif

        #if ICM20602_ACTIVE
        icm20602_sensor_data icm_data;

        //Get new ICM20602 data if available
        bool icm_data_ready = icm20602_data_ready(&icm, &err);
        if(icm_data_ready){
            icm_data = icm20602_get_data(&icm, &err); 
        }else{
            DBGE("icm20602 err: %x\r\n", err);
        }
        #endif

        #if VL53L0X_ACTIVE
        VL53L0X tof;
        err = vl53l0x_init(&tof, twi);
        if(err != NRFX_SUCCESS){
            DBGE("VL53L0X init error!");
        }
        //Set the tof timing budget to 200ms
        setMeasurementTimingBudget(&tof, 200000);
        #endif            

        //Set up compact payload
        epcp_builder_ce ce_compact_payload;
        epcp_builder_ce_init(&ce_compact_payload);

        //Set up data converter
        union epcp_convert_type ct;

        // Initialize SPI and takes ADC readings
        if(xSemaphoreTake(xSPISemaphore, pdMS_TO_TICKS(5000)) == pdTRUE){
            //Power up and Init ADC
            err= adc_init(&mcp);
            if(err != NRFX_SUCCESS){
                DBGE("ADC init failure! Err: %d", err);
            }
            else{
                //Add System data to subpacket
                adc_batt = adc_sample(CE_ADC_BATT);

                //Add Connected Equipment ADC data to subpacket
                if(adc_channels[CE_ADC_4_20_CH1].enabled){
                    adc_4_20_ch1 = adc_sample(CE_ADC_4_20_CH1);
                }
                if(adc_channels[CE_ADC_4_20_CH2].enabled){
                    adc_4_20_ch2 = adc_sample(CE_ADC_4_20_CH2);
                }
                if(adc_channels[CE_ADC_4_20_CH3].enabled){
                    adc_4_20_ch3 = adc_sample(CE_ADC_4_20_CH3);
                }
                if(adc_channels[CE_ADC_4_20_CH4].enabled){
                    adc_4_20_ch4 = adc_sample(CE_ADC_4_20_CH4);
                }
                if(adc_channels[CE_ADC_0_5_CH1].enabled){
                    adc_0_5_ch1 = adc_sample(CE_ADC_0_5_CH1);
                }
                if(adc_channels[CE_ADC_0_5_CH2].enabled){
                    adc_0_5_ch2 = adc_sample(CE_ADC_0_5_CH2);
                }
                if(adc_channels[CE_ADC_0_5_CH3].enabled){
                    adc_0_5_ch3 = adc_sample(CE_ADC_0_5_CH3);
                }
                if(adc_channels[CE_ADC_0_5_CH4].enabled){
                    adc_0_5_ch4 = adc_sample(CE_ADC_0_5_CH4);
                }

                //Add Connected Equipment ADC data to subpacket
                uint8_t active_channels = 0;
                if(adc_channels[CE_ADC_4_20_CH1].enabled){
                    active_channels |= 0x01;
                    DBGI("4-20mA AIN%d:     %2.2fmA", 1, adc_4_20_ch1);
                    ct.ui32 = (uint32_t) (adc_4_20_ch1 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_4_20_CH1, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_4_20_CH2].enabled){
                    active_channels |= 0x02;
                    DBGI("4-20mA AIN%d:     %2.2fmA", 2, adc_4_20_ch2);
                    ct.ui32 = (uint32_t) (adc_4_20_ch2 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_4_20_CH2, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_4_20_CH3].enabled){
                    active_channels |= 0x04;
                    DBGI("4-20mA AIN%d:     %2.2fmA", 3, adc_4_20_ch3);
                    ct.ui32 = (uint32_t) (adc_4_20_ch3 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_4_20_CH3, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_4_20_CH4].enabled){
                    active_channels |= 0x08;
                    DBGI("4-20mA AIN%d:     %2.2fmA", 4, adc_4_20_ch4);
                    ct.ui32 = (uint32_t) (adc_4_20_ch4 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_4_20_CH4, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_0_5_CH1].enabled){
                    active_channels |= 0x10;
                    DBGI("0-5V AIN%d:       %2.2fV", 1, adc_0_5_ch1);
                    ct.ui32 = (uint32_t) (adc_0_5_ch1 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_0_5_CH1, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_0_5_CH2].enabled){
                    active_channels |= 0x20;
                    DBGI("0-5V AIN%d:       %2.2fV", 2, adc_0_5_ch2);
                    ct.ui32 = (uint32_t) (adc_0_5_ch2 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_0_5_CH2, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_0_5_CH3].enabled){
                    active_channels |= 0x40;
                    DBGI("0-5V AIN%d:       %2.2fV", 3, adc_0_5_ch3);
                    ct.ui32 = (uint32_t) (adc_0_5_ch3 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_0_5_CH3, &(ct.ui32));
                }
                if(adc_channels[CE_ADC_0_5_CH4].enabled){
                    active_channels |= 0x80;
                    DBGI("0-5V AIN%d:       %2.2fV", 4, adc_0_5_ch4);
                    ct.ui32 = (uint32_t) (adc_0_5_ch4 * 1000);
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_0_5_CH4, &(ct.ui32));
                }
                if(active_channels != 0x00){
                    //At least one channel was added, so we add the active_chanels bitfield to the packet
                    ce_add_data(&ce_compact_payload, EPCP_CE, EPCP_ACTIVE_CHANNELS, &active_channels);
                }       

            }

            DBGI("ADC DEINIT");
            adc_deinit();

            //Give semaphore
            xSemaphoreGive(xSPISemaphore);
        }

        //Add System data to subpacket
        DBGI("Battery voltage:  %2.2fV\r\n", adc_batt);
        if(frameCounter == 1){
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_VER, &epcp_ver);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_FW_MAJOR, &system_info.updateVerMaj);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_FW_MINOR, &system_info.updateVerMin);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_FW_PATCH, &system_info.updateVerBui);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_MSG_CNT, &frameCounter);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_SN, system_info.ep_serial);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_TIME, &ts);
        
            //Convert battery from float to int per spec
            ct.ui32 = (uint32_t) (adc_batt * 100);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_FULL, EPCP_BATT, &(ct.ui32));
        }
        else{
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_VER, &epcp_ver);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_MSG_CNT, &frameCounter);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_TIME, &ts);

            //Convert battery from float to int per spec
            ct.ui32 = (uint32_t) (adc_batt * 100);
            ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_BATT, &(ct.ui32));
        }

        //Add Cell data to subpacket, if selected as comm interface
        if(comm_conf == COMM_CELL_ONLY){
            char* endptr;
            uint64_t hexIMEI = strtoull(parameters.imei,&endptr,10); //Convert IMEI from string to number
            DBGI("IMEI: %llu", hexIMEI);
            ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_IMEI, &(hexIMEI));
            ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_RSSI, &(parameters.rssi));
            ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_RSRQ, &(parameters.rsrq));
        }      

        #if SI7021_ACTIVE
        //Add HTU21D / SI7021 data to subpacket
        //Convert TEMP to int per spec
        ct.i32 = (int32_t) (si_temp * 100);
        ce_add_data(&ce_compact_payload, EPCP_SI7021, EPCP_TEMP, &(ct.ui32));
        //Convert HUM to int per spec
        ct.i32 = (int32_t) (si_hum * 100);
        ce_add_data(&ce_compact_payload, EPCP_SI7021, EPCP_HUM, &(ct.ui32));
        #endif

        #if BME680_ACTIVE
        //Convert data from to int per spec and add to BME subpacket
        ct.i32 = bme_data.temp * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_TEMP, &(ct.ui32));
        ct.i32 = bme_data.raw_pressure * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_PRES, &(ct.ui32));
        ct.ui32 = bme_data.humidity * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_HUM, &(ct.ui32));
        ct.ui32 = bme_data.raw_gas * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_GAS, &(ct.ui32));
        ct.ui32 = bme_data.co2_equivalent * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_CO2, &(ct.ui32));
        ct.ui32 = bme_data.breath_voc_equivalent * 100;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_BREATH, &(ct.ui32));
        ct.ui32 = bme_data.iaq;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_IAQ_SCORE, &(ct.ui32));
        ct.ui32 = bme_data.iaq_accuracy;
        ce_add_data(&ce_compact_payload, EPCP_BME680, EPCP_IAQ_ACCURACY, &(ct.ui32));
        #endif

        #if ICM20602_ACTIVE
        //Convert data from float to int per spec and add to ICM subpacket
        ct.i32 = (int32_t) (icm_data.accel_x * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_ACCEL_X, &(ct.ui32));
        ct.i32 = (int32_t) (icm_data.accel_y * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_ACCEL_Y, &(ct.ui32));
        ct.i32 = (int32_t) (icm_data.accel_z * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_ACCEL_Z, &(ct.ui32));
        ct.i32 = (int32_t) (icm_data.gyro_x * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_GYRO_X, &(ct.ui32));
        ct.i32 = (int32_t) (icm_data.gyro_y * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_GYRO_Y, &(ct.ui32));
        ct.i32 = (int32_t) (icm_data.gyro_z * 100);
        ce_add_data(&ce_compact_payload, EPCP_ICM20602, EPCP_GYRO_Z, &(ct.ui32));
        #endif

        #if VL53L0X_ACTIVE
        //Convert measurement to int per spec and add to TOF subpacket
        ct.ui32 = (uint32_t) vl53l0x_get_data(&tof, &err);
        if(err != NRFX_SUCCESS){
            DBGE("VL53L0X init error!");
        }
        ce_add_data(&ce_compact_payload, EPCP_VL53L0X, EPCP_DIST, &(ct.ui32));
        #endif

        //Generate completed packet
        cell_queue_msg local_sensor_data_msg;
        local_sensor_data_msg.size  = ce_get_packet(&ce_compact_payload, local_sensor_data_msg.data);

        if(comm_conf == COMM_CELL_ONLY){
            //Forward data to cell queue if there is space
            if(xQueueSend(xCellQueue, &local_sensor_data_msg, CELL_QUEUE_TIMEOUT) != pdTRUE){
                DBGE("Queue Timeout!");
            }

            // Set flag
            newDataAdded = true;

            // Resume mqtt task
            if(strstr( IOT_BROKER_ADDRESS_POST, "mqtt" ) != NULL && PERSISTENT_CONNECT_FLAG != true){
                vTaskResume( mqttTaskHandle );
            }
        }
        else if(comm_conf == COMM_LORA_ONLY){
            //Forward data to cell queue if there is space
            if(xQueueSend(xLoraQueue, &local_sensor_data_msg, CELL_QUEUE_TIMEOUT) != pdTRUE){
                DBGE("Queue Timeout!");
            }
        }

        epcp_builder_ce_deinit(&ce_compact_payload);

        // Monitor FreeRTOS stack usage
        DBGI("FreeRTOS Heap Remaining: %d Bytes", xPortGetFreeHeapSize());

        DBGI("Sleep for %d seconds", cellTransInt);
        DBGI("Sensor power off");
        nrfx_twi_uninit(&twi);        
        uninit_uart(TASK_1);
        sensorPwrEnConfig(false);        
        vTaskDelay(pdMS_TO_TICKS(cellTransInt * 1000));
    }
    vTaskDelete( NULL );
}

void addGnssQueueMsg( bool status )
{
    //Create message to send to queue
    cell_queue_msg cell_msg;
    static uint32_t prevFixTime = 0;

    cellDiag parameters = {'\0'};
    //Get latest cell values
    if( queryCellularDiag(&parameters) != CellSuccess )
    {
        /* retry once */
        if( queryCellularDiag(&parameters) != CellSuccess )
        {
            DBGE("Failed to query cell diagnostic data");
            return;
        }
    }

    /* Ensure new fix by change in timestamp */
    if(parameters.fixTime == prevFixTime && parameters.gtpLatFloat == 0 || status == false){
        DBGE("No new fix detected");
        return;
    }

    //Set up compact payload
    epcp_builder_ce ce_compact_payload;
    epcp_builder_ce_init(&ce_compact_payload);

    //Set up data converter
    union epcp_convert_type ct;

    // Increment frame counter
    frameCounter = (uint8_t) (frameCounter + 1);

    // Get time for data packet
    uint64_t ts = get_time_s();

    ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_VER, &epcp_ver);
    ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_MSG_CNT, &frameCounter);
    ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_TIME, &ts);
    ct.ui32 = ep_bsp_read_battery_voltage() * 100;
    ce_add_data(&ce_compact_payload, EPCP_SYSTEM_CON, EPCP_BATT, &(ct.ui32));

    //Add Cell data to subpacket
    char* endptr;
    uint64_t hexIMEI = strtoull(parameters.imei,&endptr,10); //Convert IMEI from string to number
    ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_IMEI, &(hexIMEI));
    ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_RSSI, &(parameters.rssi));
    ce_add_data(&ce_compact_payload, EPCP_CELL, EPCP_RSRQ, &(parameters.rsrq));

    if(parameters.fixTime != prevFixTime || status == false){
        //Add satellites
        ce_add_data(&ce_compact_payload, EPCP_GNSS, EPCP_SAT, &(parameters.satellites));
        //If failed fix then set values to 0
        if(parameters.fix < 2){
            parameters.lat_float = 0;
            parameters.long_float = 0;
            ce_set_error(&ce_compact_payload,EPCP_GNSS,true);
        }
        //Add GNSS data to subpacket    
        prevFixTime = parameters.fixTime;
        //Convert LAT from float to int per spec
        ct.i32 = parameters.lat_float * 100000;
        ce_add_data(&ce_compact_payload, EPCP_GNSS, EPCP_LAT, &(ct.ui32));
        //Convert LON from float to int per spec
        ct.i32 = parameters.long_float * 100000;
        ce_add_data(&ce_compact_payload, EPCP_GNSS, EPCP_LON, &(ct.ui32));
    }

    //Add GTP data to subpacket
    if(parameters.gtpLatFloat != 0){
        //Convert LAT from float to int per spec
        ct.i32 = parameters.gtpLatFloat * 100000;
        ce_add_data(&ce_compact_payload, EPCP_GTP, EPCP_GTP_LAT, &(ct.ui32));
        //Convert LON from float to int per spec
        ct.i32 = parameters.gtpLongFloat * 100000;
        ce_add_data(&ce_compact_payload, EPCP_GTP, EPCP_GTP_LON, &(ct.ui32));
        //Add accuracy
        ct.i32 = parameters.gtpAccuracy * 100;
        ce_add_data(&ce_compact_payload, EPCP_GTP, EPCP_GTP_ACC, &(ct.ui32));
        //Clear data
        parameters.gtpLatFloat = 0;
    }

    //Generate completed packet
    cell_queue_msg gnss_data_msg;
    gnss_data_msg.size = ce_get_packet(&ce_compact_payload, gnss_data_msg.data);   

    // Add new queue entry
    if(xQueueSend(xCellQueue, &gnss_data_msg, CELL_QUEUE_TIMEOUT) != pdTRUE){
        DBGE("Queue Timeout!");
    }

    //Free message
    epcp_builder_ce_deinit(&ce_compact_payload);           
}
