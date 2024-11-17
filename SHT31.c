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
    bool mIsMeasuringData;
    bool mCommandWaitFlag;
    bool mInitCompete;
    uint8_t mRxData[6];
    app_timer_id_t *mpTimerId;
    SHT31_CALLBACK *mpCallback;
} SHT31;

/*============================================================================*/
// Local function
/*============================================================================*/
static bool SHT31_SendCmd(SHT31 *this, SHT31_COMMAND cmd);
static void SHT31_InitSeq(SHT31 *this);
static void SHT31_SendCmdSoftReset(const SHT31 *this);
static void SHT31_SendCmdClearStatusRegister(const SHT31 *this);
static void SHT31_SendCmdSetHearter(const SHT31 *this, bool enable);
static void SHT31_SendCmdMeasurementStart(SHT31 *this);
static void SHT31_SendCmdMeasurementResGet(const SHT31 *this);
static void SHT31_ResponseProcess(SHT31 *this);
static void SHT31_TwiEvtHandler(nrf_drv_twi_evt_t const *p_event, void *p_context);
static void SHT31_WaitingAfterResponce(void *pContext);

/*============================================================================*/
// Local variable
/*============================================================================*/
static SHT31 sht31;

void SHT31_TwiEvtHandler(nrf_drv_twi_evt_t const *p_event, void *p_context) {

    SHT31 *this = (SHT31*)p_context;

    switch (p_event->type)
    {
    case NRF_DRV_TWI_EVT_DONE :
        SHT31_ResponseProcess(this);
        break;

    default:
        break;
    }

}

static void SHT31_WaitingAfterResponce(void *pContext) {
    SHT31 *this = (SHT31*)pContext;
    this->mCommandWaitFlag = true;
    SHT31_ResponseProcess(this);
}

void SHT31_Init(void) {
    memset(&sht31, 0, sizeof(sht31));
    sht31.mCurrentCommand = SHT31_CMD_NONE;
    sht31.mIsMeasuringData = false;
    sht31.mCommandWaitFlag = false;
    sht31.mInitCompete = false;
    sht31.mpTimerId = &TIMER_ID_SHT31;
    sht31.mpCallback = NULL;
    TimerManager_Register(sht31.mpTimerId, SHT31_WaitingAfterResponce, APP_TIMER_MODE_SINGLE_SHOT);
    TWI_Init(SHT31_TwiEvtHandler, (void*)&sht31);
    printf("%s(%d)\n", __func__, __LINE__);
    SHT31_InitSeq(&sht31);
    printf("%s(%d)\n", __func__, __LINE__);
}

static void SHT31_ResponseProcess(SHT31 *this) {

    printf("%s(%d) %04x\n", __func__, __LINE__, this->mCurrentCommand);

    if (!this->mInitCompete) {
        SHT31_InitSeq(this);
    }

    switch (this->mCurrentCommand)
    {

    case SHT31_CMD_MEASURE_START:
        if (!this->mCommandWaitFlag) {
            TimerManager_Start(this->mpTimerId, TIMER_WAITING_TIME_AFTER_MEASUREMENT_START_MS, this);
        } else {
            this->mCommandWaitFlag = false;
            this->mCurrentCommand = SHT31_CMD_NONE;
            SHT31_SendCmdMeasurementResGet(this);
        }
        break;

    case SHT31_CMD_MEASURE_RESULT_GET: {
        uint16_t tmpTemperature = (this->mRxData[0] << 8) | this->mRxData[1];
        uint16_t tmpHumidity = (this->mRxData[3] << 8) | this->mRxData[4];

        int16_t temperature = (-45.0f + (175.0f * ((float)tmpTemperature / 65534.0f))) * 100;
        int16_t humidity = (100.0f * ((float)tmpHumidity / 65534.0f)) * 100;
        printf("%s(%d): Temperature: %d, Humidity: %d\n", __func__, __LINE__, temperature, humidity);
        if(sht31.mpCallback) sht31.mpCallback(temperature, humidity);
        this->mIsMeasuringData = false;
        this->mCurrentCommand = SHT31_CMD_NONE;
    }
        break;

    default:
        break;
    }
}

static bool SHT31_SendCmd(SHT31 *this, SHT31_COMMAND cmd) {
    uint8_t command[2] = { (uint8_t)(cmd >> 8) , (uint8_t)(cmd & 0xFF) };

    if (this->mCurrentCommand != SHT31_CMD_NONE) {
        printf("%s(%d) command being executed\n", __func__, __LINE__);
        return false;
    }

    this->mCurrentCommand = cmd;

    if (cmd == SHT31_CMD_MEASURE_RESULT_GET) {
        TWI_Rx(SHT31_TWI_ADDRESS, this->mRxData, sizeof(this->mRxData));
        return;
    }

    TWI_Tx(SHT31_TWI_ADDRESS, command, sizeof(command), false);
}

static void SHT31_InitSeq(SHT31 *this) {

    printf("%s(%d) %04x\n", __func__, __LINE__, this->mCurrentCommand);

    switch (this->mCurrentCommand)
    {
    case SHT31_CMD_NONE:
        this->mCurrentCommand = SHT31_CMD_NONE;
        SHT31_SendCmdSoftReset(this);
        break;
    
    case SHT31_CMD_SOFT_RESET:
        if (!this->mCommandWaitFlag) {
            TimerManager_Start(this->mpTimerId, TIMER_WAITING_TIME_AFTER_SOFT_RESET_MS, this);
        } else {
            this->mCommandWaitFlag = false;
            this->mCurrentCommand = SHT31_CMD_NONE;
            SHT31_SendCmdClearStatusRegister(this);
        }
        break;
    
    case SHT31_CMD_CLEAR_STATUS:
        this->mCurrentCommand = SHT31_CMD_NONE;
        SHT31_SendCmdSetHearter(this, SHT31_HEATER);
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

static void SHT31_SendCmdSoftReset(const SHT31 *this) {
    SHT31_SendCmd(this, SHT31_CMD_SOFT_RESET);
}

static void SHT31_SendCmdClearStatusRegister(const SHT31 *this) {
    SHT31_SendCmd(this, SHT31_CMD_CLEAR_STATUS);
}

static void SHT31_SendCmdSetHearter(const SHT31 *this, bool enable) {
    uint16_t cmd;
    if (enable) {
        cmd = SHT31_CMD_HEATER_ON;
    } else {
        cmd = SHT31_CMD_HEATER_OFF;
    }
    SHT31_SendCmd(this, cmd);
}

void SHT31_GetValue(SHT31_CALLBACK *pCallback){

    if (!sht31.mInitCompete) {
        printf("%s(%d) SHT initialization not yet completed.\n", __func__, __LINE__);
        return;
    }

    if (sht31.mIsMeasuringData) {
        printf("%s(%d) busy.\n", __func__, __LINE__);
        return;
    }

    sht31.mIsMeasuringData = true;
    sht31.mpCallback = pCallback;
    SHT31_SendCmdMeasurementStart(&sht31);
}

static void SHT31_SendCmdMeasurementStart(SHT31 *this) {
    SHT31_SendCmd(this, SHT31_CMD_MEASURE_START);
}

static void SHT31_SendCmdMeasurementResGet(const SHT31 *this) {
    SHT31_SendCmd(this, SHT31_CMD_MEASURE_RESULT_GET);
}
