/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_beacon_main main.c
 * @{
 * @ingroup ble_sdk_app_beacon
 * @brief Beacon Transmitter Sample Application main file.
 *
 * This file contains the source code for an Beacon transmitter sample application.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "nordic_common.h"
#include "bsp.h"
#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_advdata.h"
#include "app_timer.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_twi.h"
#include "nrf_delay.h"

#define APP_BLE_CONN_CFG_TAG            1                                  /**< A tag identifying the SoftDevice BLE configuration. */

#define NON_CONNECTABLE_ADV_INTERVAL    MSEC_TO_UNITS(100, UNIT_0_625_MS)  /**< The advertising interval for non-connectable advertisement (100 ms). This value can vary between 100ms to 10.24s). */

#define DATA_SCHEMA_VERSION             0x01                               /**< Reserved area. */
#define DEVICE_IDENTIFIER               0x11, 0x22, 0x33, 0x44             /**< Temporary value. */
#define DATA_TYPE_ILLUMINANCE           0x13                               /**< Illuminance (unit:0.1lux). */
#define ILLUMINANCE_LUX                 0x00, 0x00                         /**< Illuminance value. */
#define OPEN_SENSOR_SERVICE_UUID        0xFCBE                             /**< Assigned number by Musen connect. */

#define DEAD_BEEF                       0xDEADBEEF                         /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

static ble_gap_adv_params_t m_adv_params;                                  /**< Parameters to be passed to the stack when starting advertising. */
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET; /**< Advertising handle used to identify an advertising set. */
static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];  /**< Buffer for storing an encoded advertising set. */

APP_TIMER_DEF(TIMER_ID_TEST);
#define TIMER_FUNCTION_MS APP_TIMER_TICKS(100)
#define APP_TIMER_OP_QUEUE_SIZE (1)         /**< APP_TIMERをCreateする最大数 */

/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0

    }
};

static uint8_t m_beacon_info[] =                    /**< Information advertised by the Beacon. */
{
    DATA_SCHEMA_VERSION, 
    DEVICE_IDENTIFIER, 
    DATA_TYPE_ILLUMINANCE,
    ILLUMINANCE_LUX,
};

/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in]   line_num   Line number of the failing ASSERT call.
 * @param[in]   file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for initializing the Advertising functionality.
 *
 * @details Encodes the required advertising data and passes it to the stack.
 *          Also builds a structure to be passed to the stack when starting advertising.
 */
static void advertising_init(void)
{
    uint32_t      err_code;

    // Initialize advertising parameters (used when starting advertising).
    memset(&m_adv_params, 0, sizeof(m_adv_params));

    m_adv_params.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
    m_adv_params.p_peer_addr     = NULL;    // Undirected advertisement.
    m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
    m_adv_params.interval        = NON_CONNECTABLE_ADV_INTERVAL;
    m_adv_params.duration        = 0;       // Never time out.

    err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
    APP_ERROR_CHECK(err_code);
}

static uint16_t dCount = 0;

static void advertising_update(void)
{
    uint32_t err_code;

    ble_advdata_t advdata;

    m_beacon_info[6] = (uint8_t)((dCount >> 8) & 0x00FF);
    m_beacon_info[7] = (uint8_t)((dCount >> 0) & 0x00FF);

    dCount++;

    ble_advdata_service_data_t service_data;
    service_data.service_uuid = OPEN_SENSOR_SERVICE_UUID;
    service_data.data.p_data = &m_beacon_info[0];
    service_data.data.size = sizeof(m_beacon_info);

    // Build and set advertising data.
    memset(&advdata, 0, sizeof(advdata));

    advdata.p_service_data_array = &service_data;
    advdata.service_data_count   = 1;

    err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("[adv]len=%d", m_adv_data.adv_data.len);
    NRF_LOG_HEXDUMP_INFO(m_adv_data.adv_data.p_data, m_adv_data.adv_data.len);
}

/**@brief Function for starting advertising.
 */
static void advertising_start(void)
{
    ret_code_t err_code;

    err_code = sd_ble_gap_adv_start(m_adv_handle, APP_BLE_CONN_CFG_TAG);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_indication_set(BSP_INDICATE_ADVERTISING);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupt.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // Configure the BLE stack using the default settings.
    // Fetch the start address of the application RAM.
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

/**@brief Function for initializing logging. */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

/**@brief Function for initializing LEDs. */
static void leds_init(void)
{
    ret_code_t err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing timers. */
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}

static void timers_start(void)
{
    ret_code_t ret = app_timer_create(&TIMER_ID_TEST, APP_TIMER_MODE_REPEATED, advertising_update);
    APP_ERROR_CHECK(ret);
    ret = app_timer_start(TIMER_ID_TEST, TIMER_FUNCTION_MS, NULL);
    APP_ERROR_CHECK(ret);
}


/**@brief Function for initializing power management.
 */
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

/* Number of possible TWI addresses. */
#define TWI_ADDRESSES      127

#define ADAFRUIT_SCL 11
#define ADAFRUIT_SDA 12

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID     0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID     1
#endif

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/**
 * @brief TWI initialization.
 */
void twi_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
       .scl                = ADAFRUIT_SCL,
       .sda                = ADAFRUIT_SDA,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

void twi_scanI2cDevices(void);

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    // Initialize.
    log_init();
    timers_init();
    leds_init();
    power_management_init();
    ble_stack_init();
    advertising_init();
    advertising_update();

    // Start execution.
    NRF_LOG_INFO("Beacon example started.");
    advertising_start();
    timers_start();

    twi_init();
//    twi_scanI2cDevices();

    uint8_t tx_data[] = {0x24, 0x00};
    ret_code_t ret = nrf_drv_twi_tx(&m_twi, 0x45, tx_data, sizeof(tx_data), false);
    printf("@@@%s(%d):%d\n", __func__, __LINE__, ret);
    nrf_delay_ms(300);
    
    uint8_t rx_data[6];
    ret = nrf_drv_twi_rx(&m_twi, 0x45, rx_data, 6);
    printf("@@@%s(%d):%d\n", __func__, __LINE__, ret);

    uint16_t temp = (rx_data[0] << 8) + rx_data[1];
    uint16_t humi = (rx_data[3] << 8) + rx_data[4];

    temp = -45 + 175 * temp / (65536 - 1);
    humi = 100 * humi / (65536 - 1);

    printf("@@@(%d)%d\n"  , __LINE__, temp);
    printf("@@@(%d)%d\n"  , __LINE__, humi);

    // Enter main loop.
    for (;; )
    {
        idle_state_handle();
    }
}

void twi_scanI2cDevices(void) {
    bool detected_device = false;
    uint8_t sample_data;

    for (uint8_t address = 1; address <= TWI_ADDRESSES; address++)
    {
        ret_code_t err_code = nrf_drv_twi_rx(&m_twi, address, &sample_data, sizeof(sample_data));
        printf("@@@:%d\n",err_code);
        if (err_code == NRF_SUCCESS)
        {
            detected_device = true;
            printf("TWI device detected at address 0x%x.", address);
            NRF_LOG_INFO("TWI device detected at address 0x%x.", address);
        }
        NRF_LOG_FLUSH();
    }

    if (!detected_device)
    {
        NRF_LOG_INFO("No device was found.");
        printf("No device was found.");
        NRF_LOG_FLUSH();
    }  
}

