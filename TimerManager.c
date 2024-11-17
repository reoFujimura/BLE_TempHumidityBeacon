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
    Timer mTimers[TIMER_CREATE_COUNT];
    uint8_t mRegisteredCount;
} TimerManager;

#define APP_TIMER_OP_QUEUE_SIZE (TIMER_CREATE_COUNT)

/*============================================================================*/
// Local function
/*============================================================================*/
static Timer* TimerManager_FindTimerById(TimerManager *this, app_timer_id_t *mpTimerId);

/*============================================================================*/
// Local variable
/*============================================================================*/
TimerManager timerManager;

void TimerManager_Init(void) {
    memset(timerManager, 0, sizeof(timerManager));

    ret_code_t err_code = app_timer_init();
    if (err_code != NRF_SUCCESS) {
        printf("%s(%d) Error initializing app_timer: %d\n", __func__, __LINE__, err_code);
        APP_ERROR_CHECK(err_code);
    }
}

void TimerManager_Register(app_timer_id_t *pTimerId, TIMER_CALLBACK *pCallback, app_timer_mode_t mode) {
    if (timerManager.mRegisteredCount >= TIMER_CREATE_COUNT) {
        printf("%s(%d) Failed to register timer: Maximum count (%d) reached\n", __func__, __LINE__, timerManager.mRegisteredCount);
        return;
    }

    Timer *pNewTimer = &timerManager.mTimers[timerManager.mRegisteredCount++];
    pNewTimer->mCallback = pCallback;
    pNewTimer->mpTimerId = pTimerId;
    ret_code_t err_code = app_timer_create(pNewTimer->mpTimerId, mode, pNewTimer->mCallback);
    if (err_code != NRF_SUCCESS) {
        printf("%s(%d) Error creating timer: %d\n", __func__, __LINE__, err_code);
        APP_ERROR_CHECK(err_code);
    } else {
        printf("%s(%d) Timer registered successfully (ID: %p)\n", __func__, __LINE__, pTimerId);
    }
}

void TimerManager_Start(app_timer_id_t *pTimerId, uint32_t timeoutTicks, void *pContext) {
    Timer *pTimer = TimerManager_FindTimerById(&timerManager, pTimerId);
    if (pTimer == NULL) {
        printf("%s(%d) Timer not registered ID %p\n", __func__, __LINE__, pTimerId);  
        return;      
    }

    ret_code_t err_code = app_timer_start(*pTimer->mpTimerId, timeoutTicks, pContext);
    if (err_code != NRF_SUCCESS) {
        printf("%s(%d) Error starting timer (ID: %p): %d\n", __func__, __LINE__, pTimerId, err_code);
        APP_ERROR_CHECK(err_code);
    }
}

void TimerManager_Stop(app_timer_id_t *pTimerId) {
    Timer *pTimer = TimerManager_FindTimerById(&timerManager, pTimerId);
    if (pTimer == NULL) {
        printf("%s(%d) Timer not registered\n", __func__, __LINE__);
        return;
    }

    ret_code_t err_code = app_timer_stop(*pTimer->mpTimerId);
    if (err_code != NRF_SUCCESS) {
        printf("%s(%d) Error stopping timer (ID: %p): %d\n", __func__, __LINE__, pTimerId, err_code);
        APP_ERROR_CHECK(err_code);
    }
}

static Timer* TimerManager_FindTimerById(TimerManager *this, app_timer_id_t *mpTimerId) {

    for (size_t i = 0; i < this->mRegisteredCount; i++)
    {
        if (this->mTimers[i].mpTimerId == mpTimerId) {
            return &this->mTimers[i];
        }
    }
    printf("%s(%d) Timer not found (ID: %p)\n", __func__, __LINE__, mpTimerId);
    return NULL;
}
