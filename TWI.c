#include "TWI.h"
#include "nrf_soc.h"

/*============================================================================*/
// define
/*============================================================================*/
#define TWI_ADDRESSES 127
#define ADAFRUIT_SCL 11
#define ADAFRUIT_SDA 12

#if TWI0_ENABLED
#define TWI_INSTANCE_ID 0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID 1
#endif

/*============================================================================*/
// Local variable
/*============================================================================*/
/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

void TWI_Init(nrf_drv_twi_evt_handler_t eventHandler, void *pContext) {

    const nrf_drv_twi_config_t twi_config = {
        .scl = ADAFRUIT_SCL,
        .sda = ADAFRUIT_SDA,
        .frequency = NRF_DRV_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init = false
    };

    ret_code_t ret = nrf_drv_twi_init(&m_twi, &twi_config, eventHandler, pContext);
    if (ret != NRF_SUCCESS) {
        printf("%s(%d) TWI initialization failed with error code: %d\n", __func__, __LINE__, ret);
        APP_ERROR_CHECK(ret);
    }
    APP_ERROR_CHECK(ret);

    nrf_drv_twi_enable(&m_twi);
}

void TWI_Tx(uint8_t address, uint8_t const *pData, uint32_t length, bool xfer_pending) {
    ret_code_t ret = nrf_drv_twi_tx(&m_twi, address, pData, length, xfer_pending);
    if (ret != NRF_SUCCESS) {
        printf("%s(%d) TWI Tx failed (Address: 0x%X, Error: %d)\n", __func__, __LINE__, address, ret);
    }
    APP_ERROR_CHECK(ret);
}

void TWI_Rx(uint8_t address, uint8_t const *pData, uint32_t length) {
    ret_code_t ret = nrf_drv_twi_rx(&m_twi, address, pData, length);
    if (ret != NRF_SUCCESS) {
        printf("%s(%d) TWI Rx failed (Address: 0x%X, Error: %d)\n", __func__, __LINE__, address, ret);
    }
    APP_ERROR_CHECK(ret);
}
