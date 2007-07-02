/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsLib.h

Abstract:

  BDS library definition, include the file and data structure

--*/

#ifndef _BDS_LIB_H_
#define _BDS_LIB_H_

#include <Protocol/SimpleTextOut.h>
#include <Protocol/HiiFramework.h>

extern EFI_HANDLE mBdsImageHandle;

//
// Constants which are variable names used to access variables
//
#define VarLegacyDevOrder L"LegacyDevOrder"

//
// Data structures and defines
//
#define FRONT_PAGE_QUESTION_ID  0x0000
#define FRONT_PAGE_DATA_WIDTH   0x01

//
// ConnectType
//
#define CONSOLE_OUT 0x00000001
#define STD_ERROR   0x00000002
#define CONSOLE_IN  0x00000004
#define CONSOLE_ALL (CONSOLE_OUT | CONSOLE_IN | STD_ERROR)

//
// Load Option Attributes defined in EFI Specification
//
#define LOAD_OPTION_ACTIVE              0x00000001
#define LOAD_OPTION_FORCE_RECONNECT     0x00000002
#define IS_LOAD_OPTION_TYPE(_c, _Mask)  (BOOLEAN) (((_c) & (_Mask)) != 0)

//
// Define Maxmim characters that will be accepted
//
#define MAX_CHAR            480
#define MAX_CHAR_SIZE       (MAX_CHAR * 2)

#define MIN_ALIGNMENT_SIZE  4
#define ALIGN_SIZE(a)       ((a % MIN_ALIGNMENT_SIZE) ? MIN_ALIGNMENT_SIZE - (a % MIN_ALIGNMENT_SIZE) : 0)

//
// Define maximum characters for boot option variable "BootXXXX"
//
#define BOOT_OPTION_MAX_CHAR 10

//
// This data structure is the part of BDS_CONNECT_ENTRY that we can hard code.
//
#define BDS_LOAD_OPTION_SIGNATURE EFI_SIGNATURE_32 ('B', 'd', 'C', 'O')

typedef struct {

  UINTN                     Signature;
  LIST_ENTRY                Link;

  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  CHAR16                    *OptionName;
  UINTN                     OptionNumber;
  UINT16                    BootCurrent;
  UINT32                    Attribute;
  CHAR16                    *Description;
  VOID                      *LoadOptions;
  UINT32                    LoadOptionsSize;

} BDS_COMMON_OPTION;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     ConnectType;
} BDS_CONSOLE_CONNECT_ENTRY;

//
// Lib Functions
//

//
// Bds boot relate lib functions
//
EFI_STATUS
BdsLibUpdateBootOrderList (
  IN  LIST_ENTRY                 *BdsOptionList,
  IN  CHAR16                     *VariableName
  );

VOID
BdsLibBootNext (
  VOID
  );

EFI_STATUS
BdsLibBootViaBootOption (
  IN  BDS_COMMON_OPTION             * Option,
  IN  EFI_DEVICE_PATH_PROTOCOL      * DevicePath,
  OUT UINTN                         *ExitDataSize,
  OUT CHAR16                        **ExitData OPTIONAL
  );

EFI_STATUS
BdsLibEnumerateAllBootOption (
  IN OUT LIST_ENTRY    *BdsBootOptionList
  );

VOID
BdsLibBuildOptionFromHandle (
  IN  EFI_HANDLE      Handle,
  IN  LIST_ENTRY      *BdsBootOptionList
  );

VOID
BdsLibBuildOptionFromShell (
  IN  EFI_HANDLE                 Handle,
  IN  LIST_ENTRY                 *BdsBootOptionList
  );

//
// Bds misc lib functions
//
UINT16
BdsLibGetTimeout (
  VOID
  );

VOID
BdsLibLoadDrivers (
  IN  LIST_ENTRY          *BdsDriverLists
  );

EFI_STATUS
BdsLibBuildOptionFromVar (
  IN  LIST_ENTRY          *BdsCommonOptionList,
  IN  CHAR16              *VariableName
  );

VOID                      *
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

EFI_STATUS
BdsLibOutputStrings (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID   *ConOut,
  ...
  );

BDS_COMMON_OPTION         *
BdsLibVariableToOption (
  IN OUT LIST_ENTRY               *BdsCommonOptionList,
  IN CHAR16                       *VariableName
  );

EFI_STATUS
BdsLibRegisterNewOption (
  IN  LIST_ENTRY                     *BdsOptionList,
  IN  EFI_DEVICE_PATH_PROTOCOL       *DevicePath,
  IN  CHAR16                         *String,
  IN  CHAR16                         *VariableName
  );

//
// Bds connect or disconnect driver lib funcion
//
VOID
BdsLibConnectAllDriversToAllControllers (
  VOID
  );

VOID
BdsLibConnectAll (
  VOID
  );

EFI_STATUS
BdsLibConnectDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePathToConnect
  );

EFI_STATUS
BdsLibConnectAllEfi (
  VOID
  );

EFI_STATUS
BdsLibDisconnectAllEfi (
  VOID
  );

//
// Bds console relate lib functions
//
VOID
BdsLibConnectAllConsoles (
  VOID
  );

EFI_STATUS
BdsLibConnectAllDefaultConsoles (
  VOID
  );

EFI_STATUS
BdsLibUpdateConsoleVariable (
  IN  CHAR16                    *ConVarName,
  IN  EFI_DEVICE_PATH_PROTOCOL  *CustomizedConDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL  *ExclusiveDevicePath
  );

EFI_STATUS
BdsLibConnectConsoleVariable (
  IN  CHAR16                 *ConVarName
  );

//
// Bds device path relate lib functions
//
EFI_DEVICE_PATH_PROTOCOL  *
BdsLibUnpackDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  );

VOID
BdsLibSafeFreePool (
  IN  VOID             *Buffer
  );

EFI_DEVICE_PATH_PROTOCOL *
BdsLibDelPartMatchInstance (
  IN     EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN     EFI_DEVICE_PATH_PROTOCOL  *Single
  );

BOOLEAN
BdsLibMatchDevicePaths (
  IN  EFI_DEVICE_PATH_PROTOCOL  *Multi,
  IN  EFI_DEVICE_PATH_PROTOCOL  *Single
  );

CHAR16                    *
DevicePathToStr (
  EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );

VOID                      *
EfiLibGetVariable (
  IN CHAR16               *Name,
  IN EFI_GUID             *VendorGuid
  );

//
// Internal definitions
//
typedef struct {
  CHAR16  *str;
  UINTN   len;
  UINTN   maxlen;
} POOL_PRINT;

typedef struct {
  UINT8 Type;
  UINT8 SubType;
  VOID (*Function) (POOL_PRINT *, VOID *);
} DEVICE_PATH_STRING_TABLE;

//
// Internal functions
//
EFI_STATUS
BdsBootByDiskSignatureAndPartition (
  IN  BDS_COMMON_OPTION          * Option,
  IN  HARDDRIVE_DEVICE_PATH      * HardDriveDevicePath,
  IN  UINT32                     LoadOptionsSize,
  IN  VOID                       *LoadOptions,
  OUT UINTN                      *ExitDataSize,
  OUT CHAR16                     **ExitData OPTIONAL
  );

//
// Notes: EFI 64 shadow all option rom
//
#if   defined (MDE_CPU_IPF)
#define EFI64_SHADOW_ALL_LEGACY_ROM() ShadowAllOptionRom ();
VOID
ShadowAllOptionRom();
#else
#define EFI64_SHADOW_ALL_LEGACY_ROM()
#endif

//
// BBS support macros and functions
//
#if   defined (MDE_CPU_IA32)
#define REFRESH_LEGACY_BOOT_OPTIONS \
        BdsDeleteAllInvalidLegacyBootOptions ();\
        BdsAddNonExistingLegacyBootOptions (); \
        BdsUpdateLegacyDevOrder ()
#else
#define REFRESH_LEGACY_BOOT_OPTIONS
#endif

EFI_STATUS
BdsDeleteAllInvalidLegacyBootOptions (
  VOID
  );

EFI_STATUS
BdsAddNonExistingLegacyBootOptions (
  VOID
  );

EFI_STATUS
BdsUpdateLegacyDevOrder (
  VOID
  );

EFI_STATUS
BdsRefreshBbsTableForBoot (
  IN BDS_COMMON_OPTION        *Entry
  );

EFI_STATUS
BdsDeleteBootOption (
  IN UINTN                       OptionNumber,
  IN OUT UINT16                  *BootOrder,
  IN OUT UINTN                   *BootOrderSize
  );

//
//The interface functions relate with Setup Browser Reset Reminder feature
//
VOID
EnableResetReminderFeature (
  VOID
  );

VOID
DisableResetReminderFeature (
  VOID
  );

VOID
EnableResetRequired (
  VOID
  );

VOID
DisableResetRequired (
  VOID
  );

BOOLEAN
IsResetReminderFeatureEnable (
  VOID
  );

BOOLEAN
IsResetRequired (
  VOID
  );

VOID
SetupResetReminder (
  VOID
  );
  
EFI_STATUS
BdsLibGetHiiHandles (
  IN     EFI_HII_PROTOCOL *Hii,
  IN OUT UINT16           *HandleBufferLength,
  OUT    EFI_HII_HANDLE   **HiiHandles
  );

#endif // _BDS_LIB_H_
