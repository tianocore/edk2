/*++

Copyright (c) 2004 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenericBdsLib.h

Abstract:

  Generic BDS library definition, include the file and data structure

--*/

#ifndef _GENERIC_BDS_LIB_H_
#define _GENERIC_BDS_LIB_H_

//
// WQBugBug: These Macro will be handled properly later.
//
//#undef EFI_SPECIFICATION_VERSION
//#define EFI_SPECIFICATION_VERSION 0x0002000A

#define PI_SPECIFICATION_VERSION 0x00010000

#include <PiDxe.h>
#include <Protocol/HiiDatabase.h>
#include <IndustryStandard/PeImage.h>


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

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#define LOAD_OPTION_HIDDEN              0x00000008
#define LOAD_OPTION_CATEGORY            0x00001F00

#define LOAD_OPTION_CATEGORY_BOOT       0x00000000
#define LOAD_OPTION_CATEGORY_APP        0x00000100

#define EFI_BOOT_OPTION_SUPPORT_KEY     0x00000001
#define EFI_BOOT_OPTION_SUPPORT_APP     0x00000002
#endif

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
  CHAR16                    *StatusString;

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
  IN  CHAR16                         *VariableName
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
  IN  EFI_HANDLE          Handle,
  IN  LIST_ENTRY      *BdsBootOptionList,
  IN  CHAR16              *String
  );

VOID
BdsLibBuildOptionFromShell (
  IN  EFI_HANDLE                     Handle,
  IN  LIST_ENTRY                 *BdsBootOptionList
  );

//
// Bds misc lib functions
//
UINT16
BdsLibGetTimeout (
  VOID
  );

EFI_STATUS
BdsLibGetBootMode (
  OUT EFI_BOOT_MODE       *BootMode
  );

VOID
BdsLibLoadDrivers (
  IN  LIST_ENTRY              *BdsDriverLists
  );

EFI_STATUS
BdsLibBuildOptionFromVar (
  IN  LIST_ENTRY              *BdsCommonOptionList,
  IN  CHAR16                      *VariableName
  );

VOID                      *
BdsLibGetVariableAndSize (
  IN  CHAR16              *Name,
  IN  EFI_GUID            *VendorGuid,
  OUT UINTN               *VariableSize
  );

EFI_STATUS
BdsLibOutputStrings (
  IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL   *ConOut,
  ...
  );

BDS_COMMON_OPTION         *
BdsLibVariableToOption (
  IN OUT LIST_ENTRY               *BdsCommonOptionList,
  IN CHAR16                           *VariableName
  );

EFI_STATUS
BdsLibRegisterNewOption (
  IN  LIST_ENTRY                 *BdsOptionList,
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

extern EFI_GUID mEfiDevicePathMessagingUartFlowControlGuid;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

extern EFI_GUID mEfiDevicePathMessagingSASGuid;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR16                    iSCSITargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;

#endif

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
#if defined (MDE_CPU_IPF)
#define EFI64_SHADOW_ALL_LEGACY_ROM() ShadowAllOptionRom ();
VOID
ShadowAllOptionRom();
#else
#define EFI64_SHADOW_ALL_LEGACY_ROM()
#endif

//
// BBS support macros and functions
//

#if defined(MDE_CPU_IA32) || defined(MDE_CPU_X64)
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
BdsLibGetImageHeader (
  IN  EFI_HANDLE                  Device,
  IN  CHAR16                      *FileName,
  OUT EFI_IMAGE_DOS_HEADER        *DosHeader,
  OUT EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr
  )
;

EFI_STATUS
BdsLibGetHiiHandles (
#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
  IN     EFI_HII_DATABASE_PROTOCOL *HiiDatabase,
#else
  IN     EFI_HII_PROTOCOL          *Hii,
#endif
  IN OUT UINT16                    *HandleBufferLength,
  OUT    EFI_HII_HANDLE            **HiiHandleBuffer
  );
  
//
// Define the boot type which to classify the boot option type
// Different boot option type could have different boot behavior
// Use their device path node (Type + SubType) as type value
// The boot type here can be added according to requirement
//
//
// ACPI boot type. For ACPI device, cannot use sub-type to distinguish device, so hardcode their value
//
#define  BDS_EFI_ACPI_FLOPPY_BOOT         0x0201
//
// Message boot type
// If a device path of boot option only point to a message node, the boot option is message boot type
//
#define  BDS_EFI_MESSAGE_ATAPI_BOOT       0x0301 // Type 03; Sub-Type 01
#define  BDS_EFI_MESSAGE_SCSI_BOOT        0x0302 // Type 03; Sub-Type 02
#define  BDS_EFI_MESSAGE_USB_DEVICE_BOOT  0x0305 // Type 03; Sub-Type 05
#define  BDS_EFI_MESSAGE_MISC_BOOT        0x03FF
//
// Media boot type
// If a device path of boot option contain a media node, the boot option is media boot type
//
#define  BDS_EFI_MEDIA_HD_BOOT            0x0401 // Type 04; Sub-Type 01
#define  BDS_EFI_MEDIA_CDROM_BOOT         0x0402 // Type 04; Sub-Type 02
//
// BBS boot type
// If a device path of boot option contain a BBS node, the boot option is BBS boot type
//
#define  BDS_LEGACY_BBS_BOOT              0x0501 //  Type 05; Sub-Type 01

#define  BDS_EFI_UNSUPPORT                0xFFFF

//
// USB host controller Programming Interface.
//
#define  PCI_CLASSC_PI_UHCI               0x00
#define  PCI_CLASSC_PI_EHCI               0x20

BOOLEAN
MatchPartitionDevicePathNode (
  IN  EFI_DEVICE_PATH_PROTOCOL   *BlockIoDevicePath,
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );
  
EFI_DEVICE_PATH_PROTOCOL *
BdsExpandPartitionPartialDevicePathToFull (
  IN  HARDDRIVE_DEVICE_PATH      *HardDriveDevicePath
  );
  
EFI_HANDLE
BdsLibGetBootableHandle (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath
  );
  
BOOLEAN
BdsLibIsValidEFIBootOptDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath,
  IN BOOLEAN                      CheckMedia
  );
  
UINT32
BdsGetBootTypeFromDevicePath (
  IN  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
  );
  
VOID
EFIAPI
BdsLibSaveMemoryTypeInformation (
  VOID 
  );
  
EFI_STATUS
EFIAPI
BdsLibUpdateFvFileDevicePath (
  IN  OUT EFI_DEVICE_PATH_PROTOCOL      ** DevicePath,
  IN  EFI_GUID                          *FileGuid
  );

EFI_STATUS
BdsLibConnectUsbDevByShortFormDP (
  IN UINT8                      HostControllerPI,
  IN EFI_DEVICE_PATH_PROTOCOL   *RemainingDevicePath
  );
  
EFI_TPL
BdsLibGetCurrentTpl (
  VOID
  );

//
// The implementation of this function is provided by Platform code.
//
VOID
DevPathVendor (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  )
;

CHAR16 *
CatPrint (
  IN OUT POOL_PRINT   *Str,
  IN CHAR16           *fmt,
  ...
  )
;
#endif // _BDS_LIB_H_
