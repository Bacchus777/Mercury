#ifndef ZCL_APP_H
#define ZCL_APP_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * INCLUDES
 */
#include "version.h"
#include "zcl.h"

/*********************************************************************
 * CONSTANTS
 */

// Application Events
#define APP_REPORT_EVT        0x0001
#define APP_SAVE_ATTRS_EVT    0x0002
#define APP_READ_SENSORS_EVT  0x0004
   
   
#define FIRST_ENDPOINT        1
#define SECOND_ENDPOINT       2

/*********************************************************************
 * MACROS
 */
#define NW_APP_CONFIG 0x0402

#define R     ACCESS_CONTROL_READ
#define RW    (R | ACCESS_CONTROL_WRITE | ACCESS_CONTROL_AUTH_WRITE)
#define RR    (R | ACCESS_REPORTABLE)
#define RWR   (RW | RR)

#define BASIC         ZCL_CLUSTER_ID_GEN_BASIC
#define GEN_ON_OFF    ZCL_CLUSTER_ID_GEN_ON_OFF
#define POWER_CFG     ZCL_CLUSTER_ID_GEN_ON
#define TEMP          ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
#define HUMIDITY      ZCL_CLUSTER_ID_MS_RELATIVE_HUMIDITY
#define PRESSURE      ZCL_CLUSTER_ID_MS_PRESSURE_MEASUREMENT
#define ELECTRICAL    ZCL_CLUSTER_ID_HA_ELECTRICAL_MEASUREMENT
#define SE_METERING   ZCL_CLUSTER_ID_SE_METERING

// Custom Attributes
#define ZCL_ATTRID_CUSTOM_DEVICE_ADDRESS          0xF001
#define ZCL_ATTRID_CUSTOM_MEASUREMENT_PERIOD      0xF002
  
#define ATTRID_SE_METERING_CURR_SUMM_DLVD         0x0000
#define ATTRID_SE_METERING_CURR_TIER1_SUMM_DLVD   0x0100
#define ATTRID_SE_METERING_CURR_TIER2_SUMM_DLVD   0x0102
#define ATTRID_SE_METERING_CURR_TIER3_SUMM_DLVD   0x0104
#define ATTRID_SE_METERING_CURR_TIER4_SUMM_DLVD   0x0106


#define ZCL_UINT8     ZCL_DATATYPE_UINT8
#define ZCL_UINT16    ZCL_DATATYPE_UINT16
#define ZCL_UINT32    ZCL_DATATYPE_UINT32
#define ZCL_UINT48    ZCL_DATATYPE_UINT48
#define ZCL_INT8      ZCL_DATATYPE_INT8
#define ZCL_INT16     ZCL_DATATYPE_INT16
#define ZCL_INT32     ZCL_DATATYPE_INT32
#define ZCL_SINGLE    ZCL_DATATYPE_SINGLE_PREC
/*********************************************************************
 * TYPEDEFS
 */

typedef struct {
    uint32  DeviceAddress;
    uint16  MeasurementPeriod;
    uint16  VoltageDivisor;
    uint16  CurrentDivisor;
    uint16  PowerDivisor;
    uint16  VoltageMultiplier;
    uint16  CurrentMultiplier;
    uint16  PowerMultiplier;
} application_config_t;


typedef struct {
    uint16 Current;
    int16 Power;
    uint16 Voltage;
} current_values_t;

typedef struct {
    uint32 Energy_T0;
    uint32 Energy_T1;
    uint32 Energy_T2;
    uint32 Energy_T3;
    uint32 Energy_T4;
} energy_t;

/*********************************************************************
 * VARIABLES
 */

extern SimpleDescriptionFormat_t zclApp_FirstEP;
extern CONST zclAttrRec_t zclApp_Attrs_FirstEP[];
extern CONST uint8 zclApp_AttrsCount_FirstEP;

extern SimpleDescriptionFormat_t zclApp_SecondEP;
extern CONST zclAttrRec_t zclApp_Attrs_SecondEP[];
extern CONST uint8 zclApp_AttrsCount_SecondEP;


extern const uint8 zclApp_ManufacturerName[];
extern const uint8 zclApp_ModelId[];
extern const uint8 zclApp_PowerSource;

extern application_config_t zclApp_Config;
extern energy_t zclApp_Energies;
extern current_values_t zclApp_CurrentValues;
extern int16 zclApp_Temperature;

// APP_TODO: Declare application specific attributes here

/*********************************************************************
 * FUNCTIONS
 */

/*
 * Initialization for the task
 */
extern void zclApp_Init(byte task_id);

/*
 *  Event Process for the task
 */
extern UINT16 zclApp_event_loop(byte task_id, UINT16 events);

extern void zclApp_ResetAttributesToDefaultValues(void);

/*********************************************************************
 *********************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* ZCL_APP_H */
