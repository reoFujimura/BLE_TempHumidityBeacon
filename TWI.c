#include "TWI.h"
#include "nrf_soc.h"

/* Number of possible TWI addresses. */
#define TWI_ADDRESSES 127
#define ADAFRUIT_SCL 11
#define ADAFRUIT_SDA 12

/* TWI instance ID. */
#if TWI0_ENABLED
#define TWI_INSTANCE_ID 0
#elif TWI1_ENABLED
#define TWI_INSTANCE_ID 1
#endif

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

void TWI_Init(nrf_drv_twi_evt_handler_t eventHandler, void *pContext) {
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
        .scl = ADAFRUIT_SCL,
        .sda = ADAFRUIT_SDA,
        .frequency = NRF_DRV_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init = false};

    err_code = nrf_drv_twi_init(&m_twi, &twi_config, eventHandler, pContext);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

void TWI_Tx(uint8_t address, uint8_t const *pData, uint32_t length, bool xfer_pending) {
    ret_code_t ret = nrf_drv_twi_tx(&m_twi, address, pData, length, xfer_pending);
    APP_ERROR_CHECK(ret);
}

void TWI_Rx(uint8_t address, uint8_t const *pData, uint32_t length) {
    ret_code_t ret = nrf_drv_twi_rx(&m_twi, address, pData, length);
    APP_ERROR_CHECK(ret);
}
