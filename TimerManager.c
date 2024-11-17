#include "TimerManager.h"
#include "nrf_soc.h"

/*============================================================================*/
// define
/*============================================================================*/
typedef struct
{
    app_timer_id_t *mpTimerId;
    TIMER_CALLBACK *mCallback;
} Timer;

typedef struct
{
    Timer mTimerInstance[TIMER_CREATE_COUNT];
    uint8_t mRegisteredCount;
} TimerManager;

#define APP_TIMER_OP_QUEUE_SIZE (TIMER_CREATE_COUNT)         /**< APP_TIMERをCreateする最大数 */

/*============================================================================*/
// Local function
/*============================================================================*/
static Timer* TimerManager_SearchTimer(TimerManager *this, app_timer_id_t *mpTimerId);

/*============================================================================*/
// Local variable
/*============================================================================*/
TimerManager timerManager;

void TimerManager_Init(void) {
    for (size_t i = 0; i < TIMER_CREATE_COUNT; i++)
    {
        Timer *pTimer = &timerManager.mTimerInstance[i];
        pTimer->mpTimerId = 0;
        pTimer->mCallback = NULL;
    }
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
    timerManager.mRegisteredCount = 0;
}

void TimerManager_Register(app_timer_id_t *pTimerId, TIMER_CALLBACK *pCallback, app_timer_mode_t mode) {
    printf("%s(%d) TimerId %d\n", __func__, __LINE__, *pTimerId);
    printf("%s(%d) RegisteredCount %d\n", __func__, __LINE__, timerManager.mRegisteredCount);

    if (timerManager.mRegisteredCount >= TIMER_CREATE_COUNT) {
        printf("%s(%d) Unable to register because the maximum number has been reached\n", __func__, __LINE__);
    }

    Timer *pTimer = &timerManager.mTimerInstance[timerManager.mRegisteredCount];
    pTimer->mCallback = pCallback;
    pTimer->mpTimerId = pTimerId;
    ret_code_t err_code = app_timer_create(pTimer->mpTimerId, mode, pTimer->mCallback);
    APP_ERROR_CHECK(err_code);
    timerManager.mRegisteredCount++;
}

void TimerManager_Start(app_timer_id_t *pTimerId, uint32_t timeoutTicks, void *pContext) {
    Timer *pTimer = TimerManager_SearchTimer(&timerManager, pTimerId);
    if (pTimer == NULL) {
        printf("%s(%d) Timer not registered\n", __func__, __LINE__);        
    }

    ret_code_t err_code = app_timer_start(*pTimer->mpTimerId, timeoutTicks, pContext);
    APP_ERROR_CHECK(err_code);
}

void TimerManager_Stop(app_timer_id_t *pTimerId) {
    Timer *pTimer = TimerManager_SearchTimer(&timerManager, pTimerId);
    if (pTimer == NULL) {
        printf("%s(%d) Timer not registered\n", __func__, __LINE__);        
    }

    ret_code_t err_code = app_timer_stop(*pTimer->mpTimerId);
    APP_ERROR_CHECK(err_code);
}

static Timer* TimerManager_SearchTimer(TimerManager *this, app_timer_id_t *mpTimerId) {

    for (size_t i = 0; i < this->mRegisteredCount; i++)
    {
        Timer *pTimer = &this->mTimerInstance[i];
        if (pTimer->mpTimerId == mpTimerId) {
            return pTimer;
        }
    }
    printf("%s(%d) Timer not registered\n", __func__, __LINE__);
    return NULL;
}
