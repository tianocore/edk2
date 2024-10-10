/** @file
  Header file for CxlDxe Application
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _CXL_FIRMWARE_MGMT_H_
#define _CXL_FIRMWARE_MGMT_H_

#include <string.h>
#include <stdbool.h>
#include <Protocol/PciIo.h>
#include <Protocol/FirmwareManagement.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/Shell.h>
#include <Protocol/EfiShellInterface.h>
#include <Protocol/EfiShellEnvironment2.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Pci22.h>
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
#include <Library/ShellCEntryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>
#include <Library/ShellLib.h>

#define CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE           SIGNATURE_32 ('C','X','L','X')
#define CXL_MAX_FILE_NAME_LENGTH                        256
#define CXL_FW_IMAGE_DESCRIPTOR_COUNT                   5
#define CXL_FW_MAX_SLOTS                                5
#define CXL_STRING_BUFFER_WIDTH                         256
#define CXL_PACKAGE_VERSION_NAME_APP                    L"CXL Firmware Package Name Application"
#define CXL_FW_SIZE                                     32768 /* 32 mb */
#define CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(a)    CR (a, CXL_CONTROLLER_PRIVATE_DATA, FirmwareMgmt, CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE)

typedef enum {
  OpTypeDisplayHelp,
  OpTypeListDevice,
  OpTypeSetActiveFwSlot,
  OpTypeFmpGetImgInfo,
  OpTypeGetImage,
  OpTypeSetPkgInfo,
  OpTypeFmpSetImg,
  OpTypeFmpCheckImg,
  OpTypeFmpGetPkgInfo,
} CXL_FMP_OPERATION_TYPE;

struct cxl_reg_map {
  BOOLEAN          valid;
  UINT32           id;
  unsigned long    offset;
  unsigned long    size;
};

struct cxl_device_reg_map {
  struct cxl_reg_map     status;
  struct cxl_reg_map     mbox;
};

struct cxl_register_map {
  UINT32                reg_type;
  UINT32                bar;
  unsigned long long    offset;
  unsigned long         mBoxoffset;
};

struct cxl_mbox_cmd {
  UINT16    opcode;
  void      *payload_in;
  void      *payload_out;
  UINT64    size_in;
  UINT64    size_out;
  UINT64    min_out;
  UINT32    poll_count;
  UINT32    poll_interval_ms;
  UINT16    return_code;
};

struct cxl_slot_info {
  UINT8                            num_slots;
  UINTN                            imageFileSize[CXL_FW_MAX_SLOTS];
  CHAR16                           *imageFileBuffer[CXL_FW_MAX_SLOTS];
  BOOLEAN                          isSetImageDone[CXL_FW_MAX_SLOTS];
  char                             firmware_version[CXL_FW_MAX_SLOTS][0x10];
  EFI_FIRMWARE_IMAGE_DESCRIPTOR    FwImageDescriptor[CXL_FW_IMAGE_DESCRIPTOR_COUNT];
};

struct cxl_fw_state {
  UINT32     state;
  BOOLEAN    oneshot;
  UINT32     num_slots;
  UINT32     cur_slot;
  UINT32     next_slot;
  UINT8      fwActivationCap;
  char       fwRevisionslot1[16];
  char       fwRevisionslot2[16];
  char       fwRevisionslot3[16];
  char       fwRevisionslot4[16];
};

struct cxl_memdev_state {
  UINT32     payload_size;
  struct     cxl_fw_state fw;
  char       firmware_version[0x10];
};

typedef struct cxl_ctrl_private_data {
  UINT32                      Signature;
  EFI_HANDLE                  ControllerHandle;
  EFI_HANDLE                  ImageHandle;
  EFI_HANDLE                  DriverBindingHandle;
  EFI_PCI_IO_PROTOCOL         *PciIo;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;

  //MailBox Register
  struct cxl_register_map     map;
  struct cxl_memdev_state     mds;
  struct cxl_mbox_cmd         mbox_cmd;

  //Image Info
  struct cxl_slot_info        slotInfo;

  //BDF Value
  UINTN                       Seg;
  UINTN                       Bus;
  UINTN                       Dev;
  UINTN                       Func;

  UINT32                      PackageVersion;
  CHAR16                      PackageVersionName[CXL_STRING_BUFFER_WIDTH];

  // Produced protocols
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL    FirmwareMgmt;
} CXL_CONTROLLER_PRIVATE_DATA;

#endif //_CXL_FIRMWARE_MGMT_H_
