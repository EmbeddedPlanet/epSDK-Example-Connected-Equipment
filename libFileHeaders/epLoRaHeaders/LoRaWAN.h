/****************************************************************************                                                                     *
 * Copyright (c) 2023 Embedded Planet, Inc.                                 *
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
 * @file    LoRaWAN.h
 * @version 1.0.0
 * @author  Embedded Planet, Inc.
 * @author  golobmichael
 * @date    20 FEB 2024
 * 
 * @brief Header for LoRa library APIs and configurations.
 * versions:
 *  1.0.0 - 20FEB2024  Initial - golobmichael
 * 
 * Built for use with the nRF SDK 17.1 and FreeRTOS.
 */

/**
 * @file lora_helper.h
 */

#ifndef LORAWAN_H
#define LORAWAN_H

#include "FreeRTOS.h"

/**
 * @brief Max join attempts
 * 
 */
#define lorawanConfigMAX_JOIN_ATTEMPTS  20

/**
 * @brief LoRa Credential Variables
 * 
 */
extern uint8_t devEUI[8];
extern uint8_t joinEUI[8];
extern uint8_t appKey[16];
extern uint8_t subBand;
extern uint8_t appSessionKey[16];
extern uint8_t nwkSessionKey[16];

/* LoRa task handle */
extern void vLorawanClassATask( void * params );

/* LoRa Queue */
extern QueueHandle_t xLoraQueue;

/**
 * @brief Structure for storing lora parameters
 * 
 */
typedef struct loraParam {
    uint8_t region;                          /* LoRa region */
    uint16_t appPort;                        /* Applicatin port for transmissions */
    bool confSend;                           /* Status of lora messages, 0-unconfirmed, 1-confirmed */
    int32_t jitterMS;                        /* Jitter for spacing unit transmissions, rand between +/- jitter in ms */
    uint32_t rxWindowMS;                     /* RX receive window in ms */
    uint32_t txIntervalSec;                  /* Interval at which lora transmissions are attempted in s */
    uint32_t mosi;                           /* Lora modem MOSI pin */
    uint32_t miso;                           /* Lora modem MISO pin */
    uint32_t sck;                            /* Lora modem clock pin */
    uint8_t subBand;                         /* Lora subBand */
    uint8_t devEUI[8];                       /* Device EUI */
    uint8_t joinEUI[8];                      /* Join EUI */
    uint8_t appKey[16];                      /* App key */
    uint8_t appSessionKey[16];               /* App session key */
    uint8_t nwkSessionKey[16];               /* Network session key */
    uint16_t maxJoinAttempts;                /* Max number of join attempts before waiting for interval time */
    uint8_t commInterface;                   /* Communcation interface for comms to server */
} loraParam;

/**
 * @brief Maximum length of the data packet allowed in a cell queue message
 *
 */
#define LORA_QUEUE_MSG_MAX_LENGTH   500U

/**
 * @brief Used to send data from the main application through the cellular queue
 * 
 */
typedef struct{
    char        data[LORA_QUEUE_MSG_MAX_LENGTH];  /** < Pointer to the JSON string */
    uint16_t    size;   /** < Size of the JSON string */
} lora_queue_msg;

/**
 * @brief End device address required for ABP activation.
 */
#define END_DEVICE_ADDR    ( ( uint32_t ) ( 0x0 ) )

/**
 * @brief Device EUI is a globaly Unique identifier used to identify the devices across LoRaWAN networks.
 * Device EUI is a 64 bit value and returned as an array of 8 hex byte values in big endian form.
 * Example: { 0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE }
 *
 * Note: If the device EUI is pre-provisioned using a secure element, remove this config parameter to use the pre-provisioned value.
 */
extern void getDeviceEUI( uint8_t * deviceEUI );
#define lorawanConfigGET_DEV_EUI    getDeviceEUI

/**
 * @brief IN EUI or APP EUI is a globaly Unique identifier used to identify the application this device is associated with..
 * Join EUI is a 64 bit value and returned as an array of 8 hex values in big endian form.
 * Example: { 0x11, 0x22, 0x33, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE }
 *
 * Note: If the join EUI is pre-provisioned using a secure element, remove this config parameter to use the pre-provisioned value.
 */
extern void getJoinEUI( uint8_t * joinEUI );
#define lorawanConfigGET_JOIN_EUI    getJoinEUI

/**
 * @brief App key is used to derive session keys used for OTAA join session.
 * App key is a 128 bit value and returned as an array of 16 hex values in big endian form.
 *
 * Note: If the App key is pre-provisioned using a secure element, remove this config parameter to use the pre-provisioned value.
 */
extern void getAppKey( uint8_t * appKey );
#define lorawanConfigGET_APP_KEY    getAppKey

/**
 * @brief End-device address which is only used for ABP join .
 *
 */
extern uint32_t getDeviceAddress( void );
#define lorawanConfigGET_DEV_ADDR    getDeviceAddress

/**
 * @brief Application Session key to be configured beforehand, only required for ABP join.
 * Application session key is a 128 bit value and returned as an array of 16 hex values in big endian form.
 *
 *  Note: If the application session key is pre-provisioned using a secure element, remove this config parameter to use the pre-provisioned value.
 */
extern void getGetAppSessionKey( uint8_t * appSessionKey );
#define lorawanConfigGET_APP_SESSION_KEY    getGetAppSessionKey

/**
 * @brief Network session key to be configured beforehand, only required for ABP join.
 * Network session key is a 128 bit value and returned as an array of 16 hex values in big endian form.
 *
 *  Note: If the network session key is pre-provisioned using a secure element, remove this config parameter to use the pre-provisioned value.
 */
extern void getGetNwkSessionKey( uint8_t * nwkSessionKey );
#define lorawanConfigGET_NETWORK_SESSION_KEY    getGetNwkSessionKey

/*
 * @brief The version of LoRaWAN stack on Network Server, to be configured beforehand, only required for ABP activation.
 * Version is set by default to 1.0.3.0.
 */
#define lorawanConfigABP_LORAWAN_VERSION        0x01000300

/*
 * @brief LoRaWAN network ID, only required for ABP activation.
 */
#define lorawanConfigNETWORK_ID                 ( ( uint32_t ) ( 0 ) )

/**
 * @brief Flag to indicate if application is using a public network such
 * as The Things Network.
 */
#define lorawanConfigPUBLIC_NETWORK             ( 1 )


/**
 * @brief Interval between retry attempts for OTAA join.
 * It waits for a retry interval +- random jitter ( to avoid dos ) before attempting to
 * join again with LoRaWAN network.
 */
#define lorawanConfigJOIN_RETRY_INTERVAL_MS    ( 2000 )


/**
 * @brief Defines a random jitter bound in milliseconds for application data transmission duty cycle.
 *
 * This allows devices to space their transmissions slighltly between each other in cases like all devices reboots and tries to
 * join server at same time.
 */
#define lorawanConfigMAX_JITTER_MS    ( 500 )



/**
 * @brief Default config to enable or disable adaptive data rate.
 *
 * Enabling adaptive data rate allows the network to set optimized data rates for end devices
 * thereby optimizing on air time and power consumption. Its recommended to enable adaptive
 * data rate for static devices and devices with stable RF conditions.
 * Adaptive data rate can be toggled runtime using API.
 *
 */
#define lorawanConfigADR_ON    ( 1 )


/**
 * @brief Default config to set the number of retries of a failed send attempt.
 *
 */
#define lorawanConfigMAX_SEND_RETRIES    ( 8 )


/**
 * @brief Overall timing error threshold for the system.
 */
#define lorawanConfigRX_MAX_TIMING_ERROR    ( 50 )


/**
 * @brief Maximum payload length defined by LoRaWAN spec
 *
 * This can be used to cap the maximum packet size that can be transferred anytime by the application.
 * LoRaWAN payload can vary upto 222 bytes. However applications should take care of duty cycle restrictions and
 * fair access policies for each region while determining the size of a message to be transmitted.
 * Larger messages leads to longer air-time and increased power consumption for the
 * radio as well as using up all of the duty cycle for a channel.
 */
#define lorawanConfigMAX_MESSAGE_SIZE    ( 222 )


/**
 * @brief Size of response queue used to receive responses to requests.
 * Queue is used to separate out events from responses so application can do a synchronous call to
 * join to a network or send a confirmed message. Since there is atmost 1 LoRaWAN operation at a time, queue size
 * is set to 1.
 */
#define lorawanConfigRESPONSE_QUEUE_SIZE    ( 1 )

/**
 * @breif Queue size for downlink data.
 *
 * Class A application sends an uplink and then polls for downlink messages, the next two receive windows. Only one message is sent
 * by downlink server for each uplink. Hence setting the queue size to 1.
 */
#define lorawanConfigDOWNLINK_QUEUE_SIZE    ( 1 )

/**
 * @breif Queue size for downlink events.
 *
 * For class A application at most 4 events can be received downlink per uplink at any time (SRV_MAC_LINK_CHECK_ANS, SRV_MAC_DEVICE_TIME_ANS, FRAME LOSS, DOWNLINK DATA)
 * Queue size can be adjusted based on application needs.
 */
#define lorawanConfigEVENT_QUEUE_SIZE       ( 4 )

/**
 * @brief Stack size for LoRaMAC task.
 * Set to a reasonable size as required for LoRaMAC layer functions.
 */
#define lorawanConfigLORAMAC_TASK_STACK_SIZE    ( 2048 )

/**
 * @brief Priority for LoRaMAC task.
 * LoRaMAC task is set to wake up on interrupts from radio layer and needs to process
 * radio interrupts as soon as possible. Hence setting to the max possible priority.
 */
#define lorawanConfigLORAMAC_TASK_PRIORITY      ( configMAX_PRIORITIES - 1 )

/*!
 * LoRa region enumeration
 */
typedef enum eLoRaRegion_t
{
    /*!
     * AS band on 923MHz
     */
    LORA_REGION_AS923,
    /*!
     * Australian band on 915MHz
     */
    LORA_REGION_AU915,
    /*!
     * Chinese band on 470MHz
     */
    LORA_REGION_CN470,
    /*!
     * Chinese band on 779MHz
     */
    LORA_REGION_CN779,
    /*!
     * European band on 433MHz
     */
    LORA_REGION_EU433,
    /*!
     * European band on 868MHz
     */
    LORA_REGION_EU868,
    /*!
     * South korean band on 920MHz
     */
    LORA_REGION_KR920,
    /*!
     * India band on 865MHz
     */
    LORA_REGION_IN865,
    /*!
     * North american band on 915MHz
     */
    LORA_REGION_US915,
    /*!
     * Russia band on 864MHz
     */
    LORA_REGION_RU864,
}LoRaRegion_t;

/*!
 * LoRaMAC Status
 */
typedef enum eLoRaStatus
{
    /*!
     * Service started successfully
     */
    LORA_STATUS_OK,
    /*!
     * Service not started - LoRaMAC is busy
     */
    LORA_STATUS_BUSY,
    /*!
     * Service unknown
     */
    LORA_STATUS_SERVICE_UNKNOWN,
    /*!
     * Service not started - invalid parameter
     */
    LORA_STATUS_PARAMETER_INVALID,
    /*!
     * Service not started - invalid frequency
     */
    LORA_STATUS_FREQUENCY_INVALID,
    /*!
     * Service not started - invalid datarate
     */
    LORA_STATUS_DATARATE_INVALID,
    /*!
     * Service not started - invalid frequency and datarate
     */
    LORA_STATUS_FREQ_AND_DR_INVALID,
    /*!
     * Service not started - the device is not in a LoRaWAN
     */
    LORA_STATUS_NO_NETWORK_JOINED,
    /*!
     * Service not started - payload length error
     */
    LORA_STATUS_LENGTH_ERROR,
    /*!
     * Service not started - the specified region is not supported
     * or not activated with preprocessor definitions.
     */
    LORA_STATUS_REGION_NOT_SUPPORTED,
    /*!
     * The application data was not transmitted
     * because prioritized pending MAC commands had to be sent.
     */
    LORA_STATUS_SKIPPED_APP_DATA,
    /*!
     * An MCPS or MLME request can return this status. In this case,
     * the MAC cannot send the frame, as the duty cycle limits all
     * available bands. When a request returns this value, the
     * variable "DutyCycleWaitTime" in "ReqReturn" of the input
     * parameters contains the remaining time to wait. If the
     * value is constant and does not change, the expected time
     * on air for this frame is exceeding the maximum permitted
     * time according to the duty cycle time period, defined
     * in Region.h, DUTY_CYCLE_TIME_PERIOD. By default this time
     * is 1 hour, and a band with 1% duty cycle is then allowed
     * to use an air time of 36 seconds.
     */
    LORA_STATUS_DUTYCYCLE_RESTRICTED,
    /*!
     *
     */
    LORA_STATUS_NO_CHANNEL_FOUND,
    /*!
     *
     */
    LORA_STATUS_NO_FREE_CHANNEL_FOUND,
     /*!
      * ToDo
      */
    LORA_STATUS_BUSY_BEACON_RESERVED_TIME,
     /*!
      * ToDo
      */
    LORA_STATUS_BUSY_PING_SLOT_WINDOW_TIME,
     /*!
      * ToDo
      */
    LORA_STATUS_BUSY_UPLINK_COLLISION,
    /*!
     * An error in the cryptographic module is occurred
     */
    LORA_STATUS_CRYPTO_ERROR,
    /*!
     * An error in the frame counter handler module is occurred
     */
    LORA_STATUS_FCNT_HANDLER_ERROR,
    /*!
     * An error in the MAC command module is occurred
     */
    LORA_STATUS_MAC_COMMAD_ERROR,
    /*!
     * An error in the Class B module is occurred
     */
    LORA_STATUS_CLASS_B_ERROR,
    /*!
     * An error in the Confirm Queue module is occurred
     */
    LORA_STATUS_CONFIRM_QUEUE_ERROR,
    /*!
     * The multicast group doesn't exist
     */
    LORA_STATUS_MC_GROUP_UNDEFINED,
    /*!
     * Undefined error occurred
     */
    LORA_STATUS_ERROR
}LoRaStatus_t;

/**
 * @brief Structure which holds the LoRaWAN payload information.
 * The same structure is used for both payload send and received.
 */
typedef struct LoRaWANMessage
{
    uint16_t port;                                 /**< @brief Application port for the payload. */
    uint8_t data[ lorawanConfigMAX_MESSAGE_SIZE ]; /**< @brief The buffer of fixed maximum size used to hold the payload. */
    size_t length;                                 /**< @brief Length of the payload. */
    uint8_t dataRate;                              /**< @brief the data rate used to transfer the payload. */
} LoRaWANMessage_t;

/**
 * @brief Information sent as part of link check reply event.
 */
typedef struct LoRaWANLinkCheckInfo
{
    uint8_t DemodMargin; /**< @brief Demodulation margin. Contains the link margin [dB] of the last successfully received LinkCheckReq. */
    uint8_t NbGateways;  /**< @brief Number of gateways which received the last LinkCheckReq. */
} LoRaWANLinkCheckInfo_t;

/**
 * @brief Event types received from LoRaWAN network.
 */
typedef enum LoRaWANEventType
{
    LORAWAN_EVENT_UNKNOWN = 0,         /**< @brief Type to denote an unexpected event type. */
    LORAWAN_EVENT_DOWNLINK_PENDING,    /**< @brief Indicates that server has to send more downlink data or waiting for a mac command uplink. */
    LORAWAN_EVENT_TOO_MANY_FRAME_LOSS, /**< @brief Indicates too many frames are missed between end device and LoRa network server. */
    LORAWAN_EVENT_DEVICE_TIME_UPDATED, /**< @brief Indicates the device time has been synchronized with LoRa network server. */
    LORAWAN_EVENT_LINK_CHECK_REPLY     /**< @brief Reply for a link check request from end device. */
} LoRaWANEventType_t;

/*!
 * LoRaMAC region enumeration
 */
typedef enum eLoRaMacRegion_t
{
    /*!
     * AS band on 923MHz
     */
    LORAMAC_REGION_AS923,
    /*!
     * Australian band on 915MHz
     */
    LORAMAC_REGION_AU915,
    /*!
     * Chinese band on 470MHz
     */
    LORAMAC_REGION_CN470,
    /*!
     * Chinese band on 779MHz
     */
    LORAMAC_REGION_CN779,
    /*!
     * European band on 433MHz
     */
    LORAMAC_REGION_EU433,
    /*!
     * European band on 868MHz
     */
    LORAMAC_REGION_EU868,
    /*!
     * South korean band on 920MHz
     */
    LORAMAC_REGION_KR920,
    /*!
     * India band on 865MHz
     */
    LORAMAC_REGION_IN865,
    /*!
     * North american band on 915MHz
     */
    LORAMAC_REGION_US915,
    /*!
     * Russia band on 864MHz
     */
    LORAMAC_REGION_RU864,
}LoRaMacRegion_t;

/**
 * @brief Structure to hold event information.
 */
typedef struct LoRaWANEventInfo
{
    LoRaWANEventType_t type;         /**< @brief Type of event. */
    uint8_t status; /**< @breif Status associated with the event. */

    union
    {
        LoRaWANLinkCheckInfo_t linkCheck; /**< @brief Link check information associated with LORAWAN_EVENT_LINK_CHECK_REPLY. */
        bool ackReceived;                 /**< @brief Acknoweldgement flag for a confirmed uplink. */
    } info;
} LoRaWANEventInfo_t;

/**
 * @brief Initializes LoRaWAN stack for the specified region.
 * Configures and starts the underlying LoRaMAC stack. Creates a high priority task to process LoRaMAC events from Radio.
 *
 * @param[in] region The region for the LoRaWAN network.
 * @return LORA_STATUS_OK if the initialization was successful. Appropriate error code otherwise.
 */
LoRaStatus_t LoRaWAN_Init( LoRaRegion_t region );

/**
 * @brief Performs a join operation using OTAA handshake with the LoRa Network Server.
 * API is blocking untill the handshake is complete. It performs JOIN retries at
 * specified interval with a random jitter, for a configured number of tries.
 *
 * @return LORA_STATUS_OK if the join was successful. Appropriate error code otherwise.
 */
LoRaStatus_t LoRaWAN_Join( uint16_t maxJoinAttempts );

/**
 * @brief Activates the device by personalization without doing a JOIN handshake.
 * For ABP join, end-device does not exchange any message with LoRa Network Server.
 *
 * @return LORA_STATUS_OK if the join was successful. Appropriate error code otherwise.
 */
LoRaStatus_t LoRaWAN_ActivateByPersonalization( void );


/**
 * @brief Enables or disables adaptive data rate.
 * Adaptive data rate mechanism is used by LoRa Network Server to find the right data rate for the device by observing the
 * uplink traffic from end-device. Its recommended to be always turned on for devices with fixed location.
 *
 * @param[in] enable Enable or disable flag
 * @return LORA_STATUS_OK if the operation was successful. Appropriate error code otherwise.
 */
LoRaStatus_t LoRaWAN_SetAdaptiveDataRate( bool enable );

/**
 * @brief Request for device time synchronization with LoRa Network Server.
 * Piggy backs a MAC command along with the next uplink payload to request for time sync from LoRa network server. LoRaWAN stack gets the response from
 * LoRa network server to correct the clock drift for the device. An event is generated for a successful device time update.
 *
 * @return LORA_STATUS_OK if the request operation was successful. Appropirate error code otherwise.
 */
LoRaStatus_t LoRaWAN_RequestDeviceTimeSync( void );

/**
 * @brief Request for link check with LoRa Network Server.
 * Piggy backs a MAC command along with next uplink  payload to perform link connectivity check with LoRa Network Server. Gets back the response from LoRa Network
 * Server and sends an event witht the link check infromation to the user.
 *
 * @return LORA_STATUS_OK if the request operation was successful. Appropirate error code otherwise.
 */
LoRaStatus_t LoRaWAN_RequestLinkCheck( void );

/**
 * @brief Sends a payload to LoRa Network server.
 * This is blocking call untill the payload is send out of radio for an unconfirmed message, or an acknoweledgement is received or the retries
 * are exhausted for a confirmed payload. Number of retries for a confirmed payload is configurable. The retries uses different
 * frequencies uplink so as to find the right overlapping frequency with the gateway.
 *
 * @param[in] pMessage Pointer to the payload along with other information.
 * @param[in] confirmed Should send a confirmed payload or not.
 * @return LORA_STATUS_OK if the request operation was successful. Appropirate error code otherwise.
 */
LoRaStatus_t LoRaWAN_Send( LoRaWANMessage_t * pMessage,
                              bool confirmed );

/**
 * @brief Receives a downlink message from LoRa Network server.
 * Blocks for the specified timeout provided.
 *
 * @param[out] pEventInfo Pointer to structure containing event type and other information.
 * @param[in] timeoutMS Timeout in milliseconds to block for an event.  Set to 0 to not block for an event.
 * @return pdFALSE if there is no data.
 */
BaseType_t LoRaWAN_Receive( LoRaWANMessage_t * pMessage,
                            uint32_t timeoutMS );


/**
 * @brief Poll for a downlink event from LoRa Network server.
 * Blocks for the specified timeout provided.
 *
 * @param[out] pEventInfo Pointer to structure containing event type and other information.
 * @param[in] timeoutMS Timeout in milliseconds to block for an event. Set to 0 to not block for an event.
 * @return pdFALSE if there are no events to be processed.
 */
BaseType_t LoRaWAN_PollEvent( LoRaWANEventInfo_t * pEventInfo,
                              uint32_t timeoutMS );

/**
 * @brief Cleans up LoRaWAN stack.
 * Stops and deinits the LoRaMAC stack. Deletes the LoRaMAC task and associated resources.
 */
void LoRaWAN_Cleanup( void );

/**
 * @brief Sets configurable LoRa parameters from app.
 *
 * @param[in] loraParams Structure containing configurable parameters.
 * @return true if successful.
 */
bool loraConfig( loraParam loraParams );

/*!
 * \brief Initializes the radio I/Os pins interface
 */
void SX1276IoInit( void );

#endif /* LORAWAN_H */