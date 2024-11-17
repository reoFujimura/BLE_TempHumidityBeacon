#pragma once

#include "app_timer.h"
#include <stdint.h>

#define TIMER_CREATE_COUNT 2  /**< APP_TIMERをCreateする最大数 */

APP_TIMER_DEF(TIMER_ID_SHT31);
APP_TIMER_DEF(TIMER_ID_MAIN);

typedef void(TIMER_CALLBACK)(void *pContext);

void TimerManager_Init(void);
void TimerManager_Register(app_timer_id_t *pTimerId, TIMER_CALLBACK *pCallback, app_timer_mode_t mode);
void TimerManager_Start(app_timer_id_t *pTimerId, uint32_t timeoutTicks, void *pContext);
void TimerManager_Stop(app_timer_id_t *pTimerId);
