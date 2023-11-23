#include "AF.h"
#include "OSAL.h"
#include "ZComDef.h"
#include "ZDConfig.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"
#include "zcl_ms.h"
#include "zcl_se.h"
#include "zcl_electrical_measurement.h"

#include "zcl_app.h"

#include "version.h"

#include "bdb_touchlink.h"
#include "bdb_touchlink_target.h"
#include "stub_aps.h"

/*********************************************************************
 * CONSTANTS
 */

#define APP_DEVICE_VERSION 2
#define APP_FLAGS 0

#define APP_HWVERSION 1
#define APP_ZCLVERSION 1

#define ZCL_ELECTRICAL_MEASUREMENT

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

// Global attributes
const uint16 zclApp_clusterRevision_all = 0x0002;

// Basic Cluster
const uint8 zclApp_HWRevision = APP_HWVERSION;
const uint8 zclApp_ZCLVersion = APP_ZCLVERSION;
const uint8 zclApp_ApplicationVersion = 3;
const uint8 zclApp_StackVersion = 4;

const uint8 zclApp_ManufacturerName[] = {7, 'B', 'a', 'c', 'c', 'h', 'u', 's'};
const uint8 zclApp_ModelId[] = {15, 'M', 'e', 'r', 'c', 'u', 'r', 'y', '_', 'C', 'o', 'u', 'n', 't', 'e', 'r'};
const uint8 zclApp_PowerSource = POWER_SOURCE_MAINS_1_PHASE;

#define DEFAULT_DeviceAddress 11111111
#define DEFAULT_MeasurementPeriod 30

application_config_t zclApp_Config = {
    .DeviceAddress = DEFAULT_DeviceAddress,
    .MeasurementPeriod = DEFAULT_MeasurementPeriod,
    .VoltageDivisor = 10,
    .CurrentDivisor = 100,
    .PowerDivisor = 1,
    .VoltageMultiplier = 1,
    .CurrentMultiplier = 1,
    .PowerMultiplier = 1,
};

current_values_t zclApp_CurrentValues = {
    .Voltage = 0,
    .Current = 0,
    .Power = 0,
};

energy_t zclApp_Energies = {
    .Energy_T1 = 0,
    .Energy_T2 = 0,
    .Energy_T3 = 0,
    .Energy_T4 = 0
};

/*********************************************************************
 * ATTRIBUTE DEFINITIONS - Uses REAL cluster IDs
 */

CONST zclAttrRec_t zclApp_AttrsFirstEP[] = {
    {BASIC, {ATTRID_BASIC_ZCL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ZCLVersion}},
    {BASIC, {ATTRID_BASIC_APPL_VERSION, ZCL_UINT8, R, (void *)&zclApp_ApplicationVersion}},
    {BASIC, {ATTRID_BASIC_STACK_VERSION, ZCL_UINT8, R, (void *)&zclApp_StackVersion}},
    {BASIC, {ATTRID_BASIC_HW_VERSION, ZCL_UINT8, R, (void *)&zclApp_HWRevision}},
    {BASIC, {ATTRID_BASIC_MANUFACTURER_NAME, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ManufacturerName}},
    {BASIC, {ATTRID_BASIC_MODEL_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_ModelId}},
    {BASIC, {ATTRID_BASIC_DATE_CODE, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_BASIC_POWER_SOURCE, ZCL_DATATYPE_ENUM8, R, (void *)&zclApp_PowerSource}},
    {BASIC, {ATTRID_BASIC_SW_BUILD_ID, ZCL_DATATYPE_CHAR_STR, R, (void *)zclApp_DateCode}},
    {BASIC, {ATTRID_CLUSTER_REVISION, ZCL_UINT16, R, (void *)&zclApp_clusterRevision_all}},

    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_RMS_VOLTAGE, ZCL_UINT16, RR, (void *)&zclApp_CurrentValues.Voltage}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_RMS_CURRENT, ZCL_UINT16, RR, (void *)&zclApp_CurrentValues.Current}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_ACTIVE_POWER, ZCL_INT16, RR, (void *)&zclApp_CurrentValues.Power}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_DIVISOR, ZCL_UINT16, R, (void *)&zclApp_Config.VoltageDivisor}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_DIVISOR, ZCL_UINT16, R, (void *)&zclApp_Config.CurrentDivisor}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_DIVISOR, ZCL_UINT16, R, (void *)&zclApp_Config.PowerDivisor}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_VOLTAGE_MULTIPLIER, ZCL_UINT16, R, (void *)&zclApp_Config.VoltageMultiplier}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_CURRENT_MULTIPLIER, ZCL_UINT16, R, (void *)&zclApp_Config.CurrentMultiplier}},
    {ELECTRICAL, {ATTRID_ELECTRICAL_MEASUREMENT_AC_POWER_MULTIPLIER, ZCL_UINT16, R, (void *)&zclApp_Config.PowerMultiplier}},
    
    {SE_METERING, {ATTRID_SE_METERING_CURR_TIER1_SUMM_DLVD, ZCL_UINT32, R, (void *)&zclApp_Energies.Energy_T1}},
    {SE_METERING, {ATTRID_SE_METERING_CURR_TIER2_SUMM_DLVD, ZCL_UINT32, R, (void *)&zclApp_Energies.Energy_T1}},
    {SE_METERING, {ATTRID_SE_METERING_CURR_TIER3_SUMM_DLVD, ZCL_UINT32, R, (void *)&zclApp_Energies.Energy_T1}},
    {SE_METERING, {ATTRID_SE_METERING_CURR_TIER4_SUMM_DLVD, ZCL_UINT32, R, (void *)&zclApp_Energies.Energy_T1}},
    {SE_METERING, {ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS, ZCL_UINT32, RW, (void *)&zclApp_Config.DeviceAddress}},
    {SE_METERING, {ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD, ZCL_UINT16, RW, (void *)&zclApp_Config.MeasurementPeriod}},
};


uint8 CONST zclApp_AttrsCount = (sizeof(zclApp_AttrsFirstEP) / sizeof(zclApp_AttrsFirstEP[0]));

const cId_t zclApp_InClusterList[] = {ZCL_CLUSTER_ID_GEN_BASIC, SE_METERING};

#define APP_MAX_INCLUSTERS (sizeof(zclApp_InClusterList) / sizeof(zclApp_InClusterList[0]))

const cId_t zclApp_OutClusterList[] = {ELECTRICAL, SE_METERING};


#define APP_MAX_OUT_CLUSTERS (sizeof(zclApp_OutClusterList) / sizeof(zclApp_OutClusterList[0]))


SimpleDescriptionFormat_t zclApp_FirstEP = {
    1,                             //  int Endpoint;
    ZCL_HA_PROFILE_ID,             //  uint16 AppProfId[2];
    ZCL_HA_DEVICEID_SIMPLE_SENSOR, //  uint16 AppDeviceId[2];
    APP_DEVICE_VERSION,            //  int   AppDevVer:4;
    APP_FLAGS,                     //  int   AppFlags:4;
    APP_MAX_INCLUSTERS,            //  byte  AppNumInClusters;
    (cId_t *)zclApp_InClusterList, //  byte *pAppInClusterList;
    APP_MAX_OUT_CLUSTERS,          //  byte  AppNumInClusters;
    (cId_t *)zclApp_OutClusterList //  byte *pAppInClusterList;
};


void zclApp_ResetAttributesToDefaultValues(void) {
    zclApp_Config.DeviceAddress = DEFAULT_DeviceAddress;
    zclApp_Config.MeasurementPeriod = DEFAULT_MeasurementPeriod;
    zclApp_Config.VoltageDivisor = 10;
    zclApp_Config.CurrentDivisor = 10;
    zclApp_Config.PowerDivisor = 10;
    zclApp_Config.VoltageMultiplier = 1;
    zclApp_Config.CurrentMultiplier = 1;
    zclApp_Config.PowerMultiplier = 1;
}