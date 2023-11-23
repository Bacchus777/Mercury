
#include "AF.h"
#include "OSAL.h"
#include "OSAL_Clock.h"
#include "OSAL_PwrMgr.h"
#include "ZComDef.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "math.h"
#include <stdint.h>

#include "nwk_util.h"
#include "zcl.h"
#include "zcl_app.h"
#include "zcl_diagnostic.h"
#include "zcl_general.h"
#include "zcl_ms.h"
#include "zcl_se.h"
#include "zcl_electrical_measurement.h"

#include "bdb.h"
#include "bdb_interface.h"
#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"

#include "gp_interface.h"

#include "Debug.h"

#include "OnBoard.h"

#include "commissioning.h"
#include "factory_reset.h"
/* HAL */
#include "hal_drivers.h"
#include "hal_key.h"
#include "hal_led.h"
#include "mercury200.h"
#include "utils.h"
#include "version.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclApp_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */
void user_delay_ms(uint32_t period);
void user_delay_ms(uint32_t period) { MicroWait(period * 1000); }
/*********************************************************************
 * LOCAL VARIABLES
 */
static zclMercury_t const *mercury_dev = &mercury200_dev;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zclApp_Report(void);
static void zclApp_BasicResetCB(void);
static void zclApp_RestoreAttributesFromNV(void);
static void zclApp_SaveAttributesToNV(void);
static void zclApp_ReadSensors(void);
static void zclApp_HandleKeys(byte portAndAction, byte keyCode);
static void zclApp_InitMercuryUart(void);
static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper);

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclApp_CmdCallbacks = {
    zclApp_BasicResetCB, // Basic Cluster Reset command
    NULL,                // Identify Trigger Effect command
    NULL,                // On/Off cluster commands
    NULL,                // On/Off cluster enhanced command Off with Effect
    NULL,                // On/Off cluster enhanced command On with Recall Global Scene
    NULL,                // On/Off cluster enhanced command On with Timed Off
    NULL,                // RSSI Location command
    NULL                 // RSSI Location Response command
};

void zclApp_Init(byte task_id) {
    HalLedSet(HAL_LED_ALL, HAL_LED_MODE_BLINK);

    zclApp_RestoreAttributesFromNV();
    zclApp_InitMercuryUart();
    zclApp_TaskID = task_id;

    bdb_RegisterSimpleDescriptor(&zclApp_FirstEP);

    zclGeneral_RegisterCmdCallbacks(zclApp_FirstEP.EndPoint, &zclApp_CmdCallbacks);

    zcl_registerAttrList(zclApp_FirstEP.EndPoint, zclApp_AttrsCount, zclApp_AttrsFirstEP);

    zcl_registerReadWriteCB(zclApp_FirstEP.EndPoint, NULL, zclApp_ReadWriteAuthCB);
    zcl_registerForMsg(zclApp_TaskID);
    RegisterForKeys(zclApp_TaskID);

    LREP("Build %s \r\n", zclApp_DateCodeNT);

    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, zclApp_Config.MeasurementPeriod * 1000);
}

static void zclApp_HandleKeys(byte portAndAction, byte keyCode) {
    LREP("zclApp_HandleKeys portAndAction=0x%X keyCode=0x%X\r\n", portAndAction, keyCode);
    zclFactoryResetter_HandleKeys(portAndAction, keyCode);
    zclCommissioning_HandleKeys(portAndAction, keyCode);
    if (portAndAction & HAL_KEY_PRESS) {
        LREPMaster("Key press\r\n");
        zclApp_Report();
    }
}

static void zclApp_InitMercuryUart(void) {
    LREPMaster("Initializing Mercury UART \r\n");
    halUARTCfg_t halUARTConfig;
    halUARTConfig.configured = TRUE;
    halUARTConfig.baudRate = HAL_UART_BR_9600;
    halUARTConfig.flowControl = FALSE;
    halUARTConfig.flowControlThreshold = 48; // this parameter indicates number of bytes left before Rx Buffer
                                             // reaches maxRxBufSize
    halUARTConfig.idleTimeout = 10;          // this parameter indicates rx timeout period in millisecond
    halUARTConfig.rx.maxBufSize = 15;
    halUARTConfig.tx.maxBufSize = 15;
    halUARTConfig.intEnable = TRUE;
    halUARTConfig.callBackFunc = NULL;
    HalUARTInit();
    if (HalUARTOpen(MERCURY_PORT, &halUARTConfig) == HAL_UART_SUCCESS) {
        LREPMaster("Initialized Mercury UART \r\n");
    }
}

uint16 zclApp_event_loop(uint8 task_id, uint16 events) {
    LREP("events 0x%x \r\n", events);
    if (events & SYS_EVENT_MSG) {
        afIncomingMSGPacket_t *MSGpkt;
        while ((MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive(zclApp_TaskID))) {
            LREP("MSGpkt->hdr.event 0x%X clusterId=0x%X\r\n", MSGpkt->hdr.event, MSGpkt->clusterId);
            switch (MSGpkt->hdr.event) {
            case KEY_CHANGE:
                zclApp_HandleKeys(((keyChange_t *)MSGpkt)->state, ((keyChange_t *)MSGpkt)->keys);
                break;

            case ZCL_INCOMING_MSG:
                if (((zclIncomingMsg_t *)MSGpkt)->attrCmd) {
                    osal_mem_free(((zclIncomingMsg_t *)MSGpkt)->attrCmd);
                }
                break;

            default:
                break;
            }

            // Release the memory
            osal_msg_deallocate((uint8 *)MSGpkt);
        }
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if (events & APP_REPORT_EVT) {
        LREPMaster("APP_REPORT_EVT\r\n");
        zclApp_Report();
        return (events ^ APP_REPORT_EVT);
    }

    if (events & APP_SAVE_ATTRS_EVT) {
        LREPMaster("APP_SAVE_ATTRS_EVT\r\n");
        zclApp_SaveAttributesToNV();
        return (events ^ APP_SAVE_ATTRS_EVT);
    }
    if (events & APP_READ_SENSORS_EVT) {
        LREPMaster("APP_READ_SENSORS_EVT\r\n");
        zclApp_ReadSensors();
        return (events ^ APP_READ_SENSORS_EVT);
    }
    return 0;
}

static void zclApp_ReadSensors(void) 
{
  static uint8 currentSensorsReadingPhase = 0;
  current_values_t CurrentValues;
  energy_t Energies;
  uint8 NUM_ATTRIBUTES;

  LREP("currentSensorsReadingPhase %d\r\n", currentSensorsReadingPhase);
    // FYI: split reading sensors into phases, so single call wouldn't block processor
    // for extensive ammount of time
  switch (currentSensorsReadingPhase++) {
  case 0: // 
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);
    (*mercury_dev->RequestMeasure)(zclApp_Config.DeviceAddress, 0x63);
    break;
  case 1:
    
    CurrentValues = (*mercury_dev->ReadCurrentValues)();
    LREP("Voltage = %d\r\n", CurrentValues.Voltage);
    LREP("Current = %d\r\n", CurrentValues.Current);
    LREP("Power = %d\r\n", CurrentValues.Power);
    if (CurrentValues.Voltage == MERCURY_INVALID_RESPONSE) {
      LREPMaster("Invalid response from counter\r\n");
      osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
      break;
    }
    zclApp_CurrentValues = CurrentValues;

    LREP("Voltage = %d\r\n", zclApp_CurrentValues.Voltage);
    LREP("Current = %d\r\n", zclApp_CurrentValues.Current);
    LREP("Power = %d\r\n", zclApp_CurrentValues.Power);

//    bdb_RepChangedAttrValue(1, ELECTRICAL, ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE);
//    bdb_RepChangedAttrValue(1, ELECTRICAL, ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT);
//    bdb_RepChangedAttrValue(1, ELECTRICAL, ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER);

    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
    break;
  case 2:
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);
    (*mercury_dev->RequestMeasure)(zclApp_Config.DeviceAddress, 0x27);
    break;
  case 3:
    Energies = (*mercury_dev->ReadEnergy)();
    if (Energies.Energy_T1 == MERCURY_INVALID_RESPONSE) {
      LREPMaster("Invalid response from counter\r\n");
      osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
      break;
    }
    zclApp_Energies = Energies;
    LREP("Energy T1 = %ld\r\n", zclApp_Energies.Energy_T1);
    LREP("Energy T2 = %ld\r\n", zclApp_Energies.Energy_T2);
    LREP("Energy T3 = %ld\r\n", zclApp_Energies.Energy_T3);
    LREP("Energy T4 = %ld\r\n", zclApp_Energies.Energy_T4);
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
    break;
  default:
    osal_stop_timerEx(zclApp_TaskID, APP_READ_SENSORS_EVT);
    osal_clear_event(zclApp_TaskID, APP_READ_SENSORS_EVT);
    currentSensorsReadingPhase = 0;
    
    NUM_ATTRIBUTES = 3;
    zclReportCmd_t *pReportCmd;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
      pReportCmd->numAttr = NUM_ATTRIBUTES;

      pReportCmd->attrList[0].attrID = ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE;
      pReportCmd->attrList[0].dataType = ZCL_UINT16;
      pReportCmd->attrList[0].attrData = (void *)(&zclApp_CurrentValues.Voltage);

      pReportCmd->attrList[1].attrID = ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT;
      pReportCmd->attrList[1].dataType = ZCL_UINT16;
      pReportCmd->attrList[1].attrData = (void *)(&zclApp_CurrentValues.Current);

      pReportCmd->attrList[2].attrID = ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER;
      pReportCmd->attrList[2].dataType = ZCL_INT16;
      pReportCmd->attrList[2].attrData = (void *)(&zclApp_CurrentValues.Power);
      
      afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 1, .addr.shortAddr = 0};
      zcl_SendReportCmd(1, &inderect_DstAddr, ELECTRICAL, pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, bdb_getZCLFrameCounter());
    }
    osal_mem_free(pReportCmd);

    NUM_ATTRIBUTES = 4;
    pReportCmd = osal_mem_alloc(sizeof(zclReportCmd_t) + (NUM_ATTRIBUTES * sizeof(zclReport_t)));
    if (pReportCmd != NULL) {
      pReportCmd->numAttr = NUM_ATTRIBUTES;
      pReportCmd->attrList[0].attrID = ATTRID_SE_METERING_CURR_TIER1_SUMM_DLVD;
      pReportCmd->attrList[0].dataType = ZCL_UINT32;
      pReportCmd->attrList[0].attrData = (void *)(&zclApp_Energies.Energy_T1);
      
      pReportCmd->attrList[1].attrID = ATTRID_SE_METERING_CURR_TIER2_SUMM_DLVD;
      pReportCmd->attrList[1].dataType = ZCL_UINT32;
      pReportCmd->attrList[1].attrData = (void *)(&zclApp_Energies.Energy_T2);
      
      pReportCmd->attrList[2].attrID = ATTRID_SE_METERING_CURR_TIER3_SUMM_DLVD;
      pReportCmd->attrList[2].dataType = ZCL_UINT32;
      pReportCmd->attrList[2].attrData = (void *)(&zclApp_Energies.Energy_T3);
      
      pReportCmd->attrList[3].attrID = ATTRID_SE_METERING_CURR_TIER4_SUMM_DLVD;
      pReportCmd->attrList[3].dataType = ZCL_UINT32;
      pReportCmd->attrList[3].attrData = (void *)(&zclApp_Energies.Energy_T4);
      
      
      afAddrType_t inderect_DstAddr = {.addrMode = (afAddrMode_t)AddrNotPresent, .endPoint = 1, .addr.shortAddr = 0};
      zcl_SendReportCmd(1, &inderect_DstAddr, SE_METERING , pReportCmd, ZCL_FRAME_CLIENT_SERVER_DIR, TRUE, bdb_getZCLFrameCounter());
    }
    osal_mem_free(pReportCmd);

    break;

  }

}

static void zclApp_Report(void) { osal_start_reload_timer(zclApp_TaskID, APP_READ_SENSORS_EVT, 500); }

static void zclApp_BasicResetCB(void) {
    LREPMaster("BasicResetCB\r\n");
    zclApp_ResetAttributesToDefaultValues();
    zclApp_SaveAttributesToNV();
}

static ZStatus_t zclApp_ReadWriteAuthCB(afAddrType_t *srcAddr, zclAttrRec_t *pAttr, uint8 oper) {
    LREPMaster("AUTH CB called\r\n");
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_HOLD);
    osal_pwrmgr_task_state(zclApp_TaskID, PWRMGR_CONSERVE);
    osal_start_timerEx(zclApp_TaskID, APP_SAVE_ATTRS_EVT, 2000);
    return ZSuccess;
}

static void zclApp_SaveAttributesToNV(void) {
    uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    LREP("Saving attributes to NV write=%d\r\n", writeStatus);
    osal_start_reload_timer(zclApp_TaskID, APP_REPORT_EVT, zclApp_Config.MeasurementPeriod * 1000);
}

static void zclApp_RestoreAttributesFromNV(void) {
    uint8 status = osal_nv_item_init(NW_APP_CONFIG, sizeof(application_config_t), NULL);
    LREP("Restoring attributes from NV  status=%d \r\n", status);
    if (status == NV_ITEM_UNINIT) {
        uint8 writeStatus = osal_nv_write(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
        LREP("NV was empty, writing %d\r\n", writeStatus);
    }
    if (status == ZSUCCESS) {
        LREPMaster("Reading from NV\r\n");
        osal_nv_read(NW_APP_CONFIG, 0, sizeof(application_config_t), &zclApp_Config);
    }
}

/****************************************************************************
****************************************************************************/
