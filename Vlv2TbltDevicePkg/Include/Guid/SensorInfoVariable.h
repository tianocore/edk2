/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   



Module Name:

  SensorInfoVariable.h

Abstract:

  GUID used for Sensor Info variable.

--*/


//
//  Module:         SensorInfoVariable.h
//
//  Description:    Provides  structure  and  literal  definitions for the
//                  Sensor Information Variable.  The  BIOS  will  provide
//                  this  variable  to  runtime  applications  via the EFI
//                  GetVariable function.
//
//  Notes:      1.  When defining and initializing the variable within the
//                  BIOS, the module will define the structure  using  the
//                  typedef  macros  in  a block. For an ATX board using a
//                  single Heceta 6P, which has 4 temperature  sensors,  6
//                  voltage  sensors,  4 fan speed sensors and 3 fan speed
//                  controllers, this block would be declared as follows:
//
//                      TYPEDEF_TEMP_SENSOR_SECTION(4);
//                      TYPEDEF_VOLT_SENSOR_SECTION(6);
//                      TYPEDEF_FAN_SENSOR_SECTION(4);
//                      TYPEDEF_FAN_CONTROLLER_SENSOR(3);
//                      TYPEDEF_SENSOR_INFO_VAR;
//
//              2.  When crafting code to access the variable, the  module
//                  will  also  need  to  invoke  the  typedef macros in a
//                  block but, since it cannot declare a structure for the
//                overall variable (because array lengths will vary), it
//                cannot  use  TYPEDEF_SENSOR_INFO_VAR.  The  block will
//                typically be used as follows:
//
//                      TYPEDEF_TEMP_SENSOR_SECTION(1);
//                     TYPEDEF_VOLT_SENSOR_SECTION(1);
//                     TYPEDEF_FAN_SENSOR_SECTION(1);
//                     TYPEDEF_FAN_CONTROLLER_SENSOR(1);
//
//                 The structure buffer should instead be declared  as  a
//                 BYTE  array. Pointers to the various sections can then
//                  be built using the XXXX_SECTION_LEN macros...
//


#ifndef _SENSOR_INFO_VAR_GUID_H_
#define _SENSOR_INFO_VAR_GUID_H_

#define SENSOR_INFO_VAR_GUID \
  { \
    0xE59E7B4D, 0x06DC, 0x44AB, 0xB3, 0x6D, 0x5E, 0xD7, 0x78, 0x9C, 0x53, 0x0A \
  }

extern EFI_GUID gEfiSensorInfoVarGuid;
extern CHAR16   gEfiSensorInfoVarName[];
extern CHAR16   gEfiSensorInfoVarNameWithPassword[];

#define SENSOR_INFO_VAR_NAME L"SensorInfoVar"
#define SENSOR_INFO_VAR_NAME_WITH_PASSWORD SENSOR_INFO_VAR_NAME L"S4k?A^7!"

//
// Sensor/Controller usage definitions
//

#define UNKNOWN_OTHER                   0

//
// Temperature Sensors
//
#define CPU_CORE_TEMPERATURE            1
#define CPU_DIE_TEMPERATURE             2
#define ICH_TEMPERATURE                 3
#define MCH_TEMPERATURE                 4
#define VR_TEMPERATURE                  5
#define MEMORY_TEMPERATURE              6
#define MOTHERBOARD_AMBIENT_TEMPERATURE 7
#define SYSTEM_AMBIENT_AIR_TEMPERATURE  8
#define CPU_INLET_AIR_TEMPERATURE       9
#define SYSTEM_INLET_AIR_TEMPERATURE    10
#define SYSTEM_OUTLET_AIR_TEMPERATURE   11
#define PSU_HOTSPOT_TEMPERATURE         12
#define PSU_INLET_AIR_TEMPERATURE       13
#define PSU_OUTLET_AIR_TEMPERATURE      14
#define DRIVE_TEMPERATURE               15
#define GPU_TEMPERATURE                 16
#define IOH_TEMPERATURE                 17

#define LAST_TEMPERATURE                17

//
// Voltage Sensors
//
#define PLUS_12_VOLTS                   1
#define NEG_12_VOLTS                    2
#define PLUS_5_VOLTS                    3
#define PLUS_5_VOLT_BACKUP              4
#define NEG_5_VOLTS                     5
#define PLUS_3P3_VOLTS                  6
#define PLUS_2P5_VOLTS                  7
#define PLUS_1P5_VOLTS                  8
#define CPU_1_VCCP_VOLTAGE              9
#define CPU_2_VCCP_VOLTAGE              10
#define CPU_3_VCCP_VOLTAGE              11
#define CPU_4_VCCP_VOLTAGE              12
#define PSU_INPUT_VOLTAGE               13
#define MCH_VCC_VOLTAGE                 14
#define PLUS_3P3_VOLT_STANDBY           15
#define CPU_VTT_VOLTAGE                 16
#define PLUS_1P8_VOLTS                  17

#define LAST_VOLTAGE                    17

//
// Fan Speed Sensors and Controllers.
//
#define CPU_COOLING_FAN                 1
#define SYSTEM_COOLING_FAN              2
#define MCH_COOLING_FAN                 3
#define VR_COOLING_FAN                  4
#define CHASSIS_COOLING_FAN             5
#define CHASSIS_INLET_FAN               6
#define CHASSIS_OUTLET_FAN              7
#define PSU_COOLING_FAN                 8
#define PSU_INLET_FAN                   9
#define PSU_OUTLET_FAN                  10
#define DRIVE_COOLING_FAN               11
#define GPU_COOLING_FAN                 12
#define AUX_COOLING_FAN                 13
#define IOH_COOLING_FAN                 14

#define LAST_FAN                        14

//
// Fan Type Definitions
//
#define FAN_TYPE_UNKNOWN                0
#define FAN_3WIRE_PULSE                 1
#define FAN_3WIRE_VOLTAGE               2
#define FAN_4WIRE                       3

#pragma pack(1)

//
// TEMP_SENSOR_INFO - Structure providing info for a temperature sensor.
//
typedef struct _TEMP_SENSOR_INFO
{
    UINT8                               byDevice;       // Device index
    UINT8                               byIndex;        // Physical sensor index
    UINT8                               byUsage;        // Usage indicator
    UINT8                               bRelative;      // Relative vs. Absolute readings

} TEMP_SENSOR_INFO, *P_TEMP_SENSOR_INFO;

//
// TYPEDEF_TEMP_SENSOR_SECTION - Macro that can be used to typedef the
// TEMP_SENSOR_SECTION structure, which provides information about all
// temperature sensors.
//
#define TYPEDEF_TEMP_SENSOR_SECTION(count)                              \
typedef struct _TEMP_SENSOR_SECTION                                     \
{                                                                       \
    UINT8                               byCount;                        \
    TEMP_SENSOR_INFO                    stSensor[count];                \
                                                                        \
} TEMP_SENSOR_SECTION, *P_TEMP_SENSOR_SECTION

//
// VOLT_SENSOR_INFO - Structure providing info for a voltage sensor.
//
typedef struct _VOLT_SENSOR_INFO
{
    UINT8                               byDevice;       // Device index
    UINT8                               byIndex;        // Physical sensor index
    UINT8                               byUsage;        // Usage indicator

} VOLT_SENSOR_INFO, *P_VOLT_SENSOR_INFO;

//
// TYPEDEF_VOLT_SENSOR_SECTION - Macro that can be used to typedef the
// VOLT_SENSOR_SECTION structure, which provides information about all
// voltage sensors.
//
#define TYPEDEF_VOLT_SENSOR_SECTION(count)                              \
typedef struct _VOLT_SENSOR_SECTION                                     \
{                                                                       \
    UINT8                               byCount;                        \
    VOLT_SENSOR_INFO                    stSensor[count];                \
                                                                        \
} VOLT_SENSOR_SECTION, *P_VOLT_SENSOR_SECTION

//
// FAN_SENSOR_INFO - Structure providing info for a fan speed sensor.
//
typedef struct _FAN_SENSOR_INFO
{
    UINT8                               byDevice;       // Device index
    UINT8                               byIndex;        // Physical sensor index
    UINT8                               byUsage;        // Usage indicator
    UINT8                               byType;         // Fan type
    UINT8                               byController;   // Associated Fan Controller

} FAN_SENSOR_INFO, *P_FAN_SENSOR_INFO;

//
// TYPEDEF_FAN_SENSOR_SECTION - Macro that can be used to typedef the
// FAN_SENSOR_SECTION structure, which provides information about all fan
// speed sensors.
//
#define TYPEDEF_FAN_SENSOR_SECTION(count)                               \
typedef struct _FAN_SENSOR_SECTION                                      \
{                                                                       \
    UINT8                               byCount;                        \
    FAN_SENSOR_INFO                     stSensor[count];                \
                                                                        \
} FAN_SENSOR_SECTION, *P_FAN_SENSOR_SECTION

//
// FAN_CONTROLLER_INFO - Structure providing info for a fan speed controller.
//
#define MAX_ASSOC_FANS                  4
#define ASSOC_UNUSED                    0xFF

typedef struct _FAN_CONTROLLER_INFO
{
    UINT8                               byDevice;       // Device index
    UINT8                               byIndex;        // Physical Controller Index
    UINT8                               byUsage;        // Usage Indicator
    UINT8                               byFan[MAX_ASSOC_FANS]; // Associated Fan Sensors

} FAN_CONTROLLER_INFO, *P_FAN_CONTROLLER_INFO;

//
// TYPEDEF_FAN_CONTROLLER_SECTION - Macro that can be used to typedef the
// FAN_CONTROLLER_SECTION structure, which provides information about all
// fan speed controllers.
//
#define TYPEDEF_FAN_CONTROLLER_SECTION(count)                           \
typedef struct _FAN_CONTROLLER_SECTION                                  \
{                                                                       \
    UINT8                               byCount;                        \
    FAN_CONTROLLER_INFO                 stController[count];            \
                                                                        \
} FAN_CONTROLLER_SECTION, *P_FAN_CONTROLLER_SECTION

//
// TYPEDEF_SENSOR_INFO_VAR - Macro that can be used to typedef the
// SENSOR_INFO_VAR structure, which provides information about all sensors
// and fan speed controllers. The other TYPEDEF macros must be invoked
// before using this one...
//
#define TYPEDEF_SENSOR_INFO_VAR                                         \
typedef struct _SENSOR_INFO_VAR                                         \
{                                                                       \
    TEMP_SENSOR_SECTION                 stTemps;                        \
    VOLT_SENSOR_SECTION                 stVolts;                        \
    FAN_SENSOR_SECTION                  stFans;                         \
    FAN_CONTROLLER_SECTION              stCtrls;                        \
                                                                        \
} SENSOR_INFO_VAR, *P_SENSOR_INFO_VAR

#pragma pack()

#endif
