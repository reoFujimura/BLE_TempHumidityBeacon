#include "SHT31.h"
#include "nrf_soc.h"
#include "TWI.h"
#include <string.h>
#include "TimerManager.h"

/*============================================================================*/
// define
/*============================================================================*/
#define SHT31_TWI_ADDRESS 0x45
#define SHT31_HEATER 0 // 0:off,1:on

#define TIMER_WAITING_TIME_AFTER_SOFT_RESET_MS APP_TIMER_TICKS(2)
#define TIMER_WAITING_TIME_AFTER_MEASUREMENT_START_MS APP_TIMER_TICKS(16)

typedef enum {
    SHT31_CMD_NONE = 0,
    SHT31_CMD_SOFT_RESET = 0x30A2,
    SHT31_CMD_CLEAR_STATUS = 0x3041,
    SHT31_CMD_HEATER_ON = 0x306D,
    SHT31_CMD_HEATER_OFF = 0x3066,
    SHT31_CMD_MEASURE_START = 0x2400,
    SHT31_CMD_MEASURE_RESULT_GET = 0xFFFF,
} SHT31_COMMAND;

typedef struct {
    SHT31_COMMAND mCurrentCommand;
    bool mIsMeasuring;
    bool mWaitFlag;
    bool mInitCompete;
    uint8_t mRxData[6];
    app_timer_id_t *mpTimerId;
    SHT31_CALLBACK *mpCallback;
} SHT31;

/*============================================================================*/
// Local function
/*============================================================================*/
static bool SHT31_SendCmd(SHT31 *this, SHT31_COMMAND cmd);
static void SHT31_InitSequence(SHT31 *this);
static void SHT31_ResponseProcess(SHT31 *this);
static void SHT31_TwiEvtHandler(nrf_drv_twi_evt_t const *p_event, void *p_context);
static void SHT31_TimerCallback(void *pContext);

/*============================================================================*/
// Local variable
/*============================================================================*/
static SHT31 sht31;

void SHT31_Init(void) {
    memset(&sht31, 0, sizeof(sht31));
    sht31.mpTimerId = &TIMER_ID_SHT31;
    TimerManager_Register(sht31.mpTimerId, SHT31_TimerCallback, APP_TIMER_MODE_SINGLE_SHOT);
    TWI_Init(SHT31_TwiEvtHandler, (void*)&sht31);
    SHT31_InitSequence(&sht31);
}

void SHT31_GetValue(SHT31_CALLBACK *pCallback){
    if (!sht31.mInitCompete) {
        printf("%s(%d) initialization not yet completed.\n", __func__, __LINE__);
        return;
    }

    if (sht31.mIsMeasuring) {
        printf("%s(%d) Measurement already in progress.\n", __func__, __LINE__);
        return;
    }

    sht31.mIsMeasuring = true;
    sht31.mpCallback = pCallback;
    SHT31_SendCmd(&sht31, SHT31_CMD_MEASURE_START);
}

static bool SHT31_SendCmd(SHT31 *this, SHT31_COMMAND cmd) {

    if (this->mCurrentCommand != SHT31_CMD_NONE) {
        printf("%s(%d) Command already in progress.\n", __func__, __LINE__);
        return false;
    }

    this->mCurrentCommand = cmd;

    uint8_t command[2] = { (uint8_t)(cmd >> 8) , (uint8_t)(cmd & 0xFF) };
    if (cmd == SHT31_CMD_MEASURE_RESULT_GET) {
        TWI_Rx(SHT31_TWI_ADDRESS, this->mRxData, sizeof(this->mRxData));
        return;
    }

    TWI_Tx(SHT31_TWI_ADDRESS, command, sizeof(command), false);
}

static void SHT31_InitSequence(SHT31 *this) {

    printf("%s(%d) %04x\n", __func__, __LINE__, this->mCurrentCommand);

    switch (this->mCurrentCommand)
    {
    case SHT31_CMD_NONE:
        this->mCurrentCommand = SHT31_CMD_NONE;
        SHT31_SendCmd(this, SHT31_CMD_SOFT_RESET);
        break;
    
    case SHT31_CMD_SOFT_RESET:
        if (!this->mWaitFlag) {
            TimerManager_Start(this->mpTimerId, TIMER_WAITING_TIME_AFTER_SOFT_RESET_MS, this);
        } else {
            this->mWaitFlag = false;
            this->mCurrentCommand = SHT31_CMD_NONE;
            SHT31_SendCmd(this, SHT31_CMD_CLEAR_STATUS);
        }
        break;
    
    case SHT31_CMD_CLEAR_STATUS:
        this->mCurrentCommand = SHT31_CMD_NONE;
        SHT31_SendCmd(this, SHT31_HEATER ? SHT31_CMD_HEATER_OFF : SHT31_CMD_HEATER_ON);
        break;

    case SHT31_CMD_HEATER_OFF:
    case SHT31_CMD_HEATER_ON:
        this->mCurrentCommand = SHT31_CMD_NONE;
        this->mInitCompete = true;
        break;

    default:
        printf("%s(%d) Invalid command\n", __func__, __LINE__);
        break;
    }

}
static void SHT31_ResponseProcess(SHT31 *this) {

    printf("%s(%d) %04x\n", __func__, __LINE__, this->mCurrentCommand);

    if (!this->mInitCompete) {
        SHT31_InitSequence(this);
    }

    switch (this->mCurrentCommand)
    {

    case SHT31_CMD_MEASURE_START:
        if (!this->mWaitFlag) {
            TimerManager_Start(this->mpTimerId, TIMER_WAITING_TIME_AFTER_MEASUREMENT_START_MS, this);
        } else {
            this->mWaitFlag = false;
            this->mCurrentCommand = SHT31_CMD_NONE;
            SHT31_SendCmd(this, SHT31_CMD_MEASURE_RESULT_GET);
        }
        break;

    case SHT31_CMD_MEASURE_RESULT_GET: {
        uint16_t rawTemperature = (this->mRxData[0] << 8) | this->mRxData[1];
        uint16_t rawHumidity = (this->mRxData[3] << 8) | this->mRxData[4];

        int16_t temperature = (-45.0f + (175.0f * ((float)rawTemperature / 65534.0f))) * 100;
        int16_t humidity = (100.0f * ((float)rawHumidity / 65534.0f)) * 100;
        printf("%s(%d): Temperature: %d, Humidity: %d\n", __func__, __LINE__, temperature, humidity);
        if(sht31.mpCallback) sht31.mpCallback(temperature, humidity);
        this->mIsMeasuring = false;
        this->mCurrentCommand = SHT31_CMD_NONE;
    }
        break;

    default:
        printf("%s(%d) Invalid command %d" , __func__, __LINE__, this->mCurrentCommand);
        break;
    }
}

void SHT31_TwiEvtHandler(nrf_drv_twi_evt_t const *p_event, void *p_context) {

    SHT31 *this = (SHT31*)p_context;

    switch (p_event->type)
    {
    case NRF_DRV_TWI_EVT_DONE :
        SHT31_ResponseProcess(this);
        break;

    default:
        printf("%s(%d) Invalid type %d" , __func__, __LINE__, p_event->type);
        break;
    }

}

static void SHT31_TimerCallback(void *pContext) {
    SHT31 *this = (SHT31*)pContext;
    this->mWaitFlag = true;
    SHT31_ResponseProcess(this);
}
