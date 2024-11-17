#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "nrf_drv_twi.h"

void TWI_Init(nrf_drv_twi_evt_handler_t eventHandler, void *pContext);
void TWI_Tx(uint8_t address, uint8_t const *pData, uint32_t length, bool xfer_pending);
void TWI_Rx(uint8_t address, uint8_t const *pData, uint32_t length);
