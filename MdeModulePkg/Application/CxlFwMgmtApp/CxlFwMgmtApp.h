/** @file
  Header file for CxlDxe Application
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _CXL_FIRMWARE_MGMT_H_
#define _CXL_FIRMWARE_MGMT_H_

#include <stdbool.h>
#include <Protocol/PciIo.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/Shell.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Pci22.h>
#include <IndustryStandard/Cxl20.h>
#include <Uefi.h>
#include <Uefi/UefiSpec.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/ShellParameters.h>

#define CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('C','X','L','X')
#define CXL_MAX_FILE_NAME_LENGTH               256
#define CXL_FW_IMAGE_DESCRIPTOR_COUNT          5
#define CXL_FW_MAX_SLOTS                       5
#define CXL_STRING_BUFFER_WIDTH                256
#define CXL_PACKAGE_VERSION_NAME_APP           L"CXL Firmware Package Name Application"
#define CXL_FW_SIZE                            32768   /* 32 mb */
#define CXL_FW_REVISION_LENGTH_IN_BYTES        16

#define CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(a) \
  CR (a, \
      CXL_CONTROLLER_PRIVATE_DATA, \
      FirmwareMgmt, \
      CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE \
      )

//
// CXL Memory Device Firmware management supported command set
//
typedef enum {
  OpTypeDisplayHelp,
  OpTypeListDevice,
  OpTypeFmpGetImgInfo,
  OpTypeGetImage,
  OpTypeSetPkgInfo,
  OpTypeFmpSetImg,
  OpTypeFmpCheckImg,
  OpTypeFmpGetPkgInfo,
  OpTypeFmpMax
} CXL_FMP_OPERATION_TYPE;

//
// CXL Memory Device Register information
//
typedef struct {
  UINT32    RegisterType;
  UINT32    BaseAddressRegister;
  UINT64    Offset;
  UINT32    MailboxRegistersOffset;
} CXL_REGISTER_MAP;

//
// CXL Memory Device Firmware slot information
//
typedef struct {
  UINT8                            NumberOfSlots;
  UINTN                            ImageFileSize[CXL_FW_MAX_SLOTS];
  CHAR16                           *ImageFileBuffer[CXL_FW_MAX_SLOTS];
  BOOLEAN                          IsSetImageDone[CXL_FW_MAX_SLOTS];
  CHAR16                           *FirmwareVersion[CXL_FW_MAX_SLOTS];
  EFI_FIRMWARE_IMAGE_DESCRIPTOR    FwImageDescriptor[CXL_FW_IMAGE_DESCRIPTOR_COUNT];
} CXL_SLOT_INFO;

//
// CXL Memory Device Firmware state
//
typedef struct {
  UINT32     State;
  BOOLEAN    OneShot;
  UINT8      NumberOfSlots;
  UINT8      CurrentSlot;
  UINT8      NextSlot;
  UINT8      FwActivationCap;
  CHAR16     *FwRevisionSlot1;
  CHAR16     *FwRevisionSlot2;
  CHAR16     *FwRevisionSlot3;
  CHAR16     *FwRevisionSlot4;
} CXL_FW_STATE;

//
// CXL Memory Device Registers state
//
typedef struct {
  UINT32          PayloadSize;
  CXL_FW_STATE    FwState;
} CXL_MEMDEV_STATE;

//
// CXL device private data structure
//
typedef struct {
  UINT32                              Signature;
  EFI_HANDLE                          ControllerHandle;
  EFI_HANDLE                          ImageHandle;
  EFI_HANDLE                          DriverBindingHandle;
  EFI_PCI_IO_PROTOCOL                 *PciIo;
  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;

  // MailBox Register
  CXL_REGISTER_MAP                    RegisterMap;
  CXL_MEMDEV_STATE                    MemdevState;
  CXL_MBOX_CMD                        MailboxCmd;

  // Image Info
  CXL_SLOT_INFO                       SlotInfo;

  // BDF Value
  UINTN                               Seg;
  UINTN                               Bus;
  UINTN                               Device;
  UINTN                               Function;
  UINT32                              PackageVersion;
  CHAR16                              *PackageVersionName;

  // Produced protocols
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    FirmwareMgmt;
} CXL_CONTROLLER_PRIVATE_DATA;

#endif // _CXL_FIRMWARE_MGMT_H_


