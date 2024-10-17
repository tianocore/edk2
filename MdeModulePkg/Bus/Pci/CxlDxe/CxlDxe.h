/** @file
  Header file for CxlDxe driver
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef _EFI_CXLDXE_H_
#define _EFI_CXLDXE_H_

#include <string.h>
#include <Protocol/PciIo.h>
#include <Protocol/FirmwareManagement.h>
#include <IndustryStandard/Pci.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#define CXL_MEMORY_CLASS                                0x05
#define CXL_MEMORY_SUB_CLASS                            0x02
#define CXL_MEMORY_PROGIF                               0x10
#define CXL_PCIE_EXTENDED_CAP_OFFSET                    0x100
#define CXL_PCIE_EXTENDED_NEXT_CAP_OFFSET_SHIFT         20
#define CXL_PCIE_EXTENDED_CAP_NEXT(n)                   ((n) >> (CXL_PCIE_EXTENDED_NEXT_CAP_OFFSET_SHIFT))
#define CXL_IS_DVSEC(n)                                 (((n) & (0xFFFF)) == 0x1E98)
#define CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE           SIGNATURE_32 ('C','X','L','X')
#define CXL_PCI_CFG_SPACE_SIZE                          256
#define CXL_PCI_CFG_SPACE_EXP_SIZE                      4096
#define CXL_PCI_DVSEC_HEADER1                           0x4    /* Designated Vendor-Specific Header1 */
#define CXL_PCI_DVSEC_HEADER2                           0x8    /* Designated Vendor-Specific Header2 */
#define CXL_PCI_EXT_CAP_ID_DVSEC                        0x23   /* Designated Vendor-Specific */
#define CXL_DVSEC_REG_LOCATOR                           8
#define CXL_DVSEC_REG_LOCATOR_BLOCK1_OFFSET             0xC
#define CXL_PCI_DVSEC_VENDOR_ID                         0x1E98
#define CXL_PCI_DVSEC_VENDOR_ID_DECIMAL                 7832
#define CXL_PCI_EXT_CAP_ID(header)                      (header & 0x0000ffff)
#define CXL_PCI_EXT_CAP_NEXT(header)                    ((header >> 20) & 0xffc)
#define CXL_DEV_CAP_ARRAY_OFFSET                        0x0
#define CXL_DEV_CAP_ARRAY_CAP_ID                        0
#define CXL_BIT(nr)                                     ((UINT32)1 << nr)
#define CXL_DEV_MBOX_CAP_BG_CMD_IRQ                     CXL_BIT(6)
#define CXL_DEV_MBOX_CTRL_DOORBELL                      CXL_BIT(0)
#define CXL_DEV_MBOX_CTRL_BG_CMD_IRQ                    CXL_BIT(2)
#define CXL_MBOX_CMD_RC_SUCCESS                         0
#define CXL_MBOX_CMD_RC_BACKGROUND                      1
#define CXL_MBOX_CMD_INVALID_INPUT                      2
#define CXL_MBOX_CMD_UNSUPPORTED                        3
#define CXL_MBOX_CMD_INTERNAL_ERROR                     4
#define CXL_MBOX_CMD_RETRY_REQUIRED                     5
#define CXL_MBOX_CMD_BUSY                               6
#define CXL_MBOX_CMD_MEDIA_DISABLED                     7
#define CXL_MBOX_CMD_FW_TRANSFER_IN_PROGRESS            8
#define CXL_MBOX_CMD_FW_TRANSFER_OUT_OF_ORDER           9
#define CXL_MBOX_CMD_FW_VERIFICATION_FAILED             10
#define CXL_MBOX_CMD_INVALID_SLOT                       11
#define CXL_MBOX_CMD_ACTIVATION_FAILED_FW_ROLLED_BACK   12
#define CXL_MBOX_CMD_COLD_RESET_REQUIRED                13
#define CXL_MBOX_CMD_INVALID_HANDLE                     14
#define CXL_MBOX_CMD_INVALID_PHYSICAL_ADDRESS           15
#define CXL_MBOX_CMD_INJECT_POISON_LIMIT_REACHED        16
#define CXL_MBOX_CMD_PERMANENT_MEDIA_FAILURE            17
#define CXL_MBOX_CMD_ABORTED                            18
#define CXL_MBOX_CMD_INVALID_SECURITY_STATE             19
#define CXL_MBOX_CMD_INCORRECT_PASSPHRASE               20
#define CXL_DEV_MBOX_CAPS_OFFSET                        0x00
#define CXL_DEV_MBOX_CTRL_OFFSET                        0x04
#define CXL_MAILBOX_TIMEOUT_MS                          2000
#define CXL_DEV_MBOX_PAYLOAD_OFFSET                     0x20
#define CXL_DEV_MBOX_BG_CMD_STATUS_OFFSET               0x18
#define CXL_DEV_MBOX_STATUS_OFFSET                      0x10
#define CXL_DEV_MBOX_CMD_OFFSET                         0x08
#define CXL_DEV_CAP_CAP_ID_PRIMARY_MAILBOX              0x2
#define CXL_DEV_CAP_CAP_ID_SECONDARY_MAILBOX            0x3
#define CXL_SZ_1M                                       0x00100000
#define CXL_FW_TRANSFER_ALIGNMENT                       128
#define CXL_FW_TRANSFER_ACTION_FULL                     0x0
#define CXL_FW_TRANSFER_ACTION_INITIATE                 0x1
#define CXL_FW_TRANSFER_ACTION_CONTINUE                 0x2
#define CXL_FW_TRANSFER_ACTION_END                      0x3
#define CXL_FW_TRANSFER_ACTION_ABORT                    0x4
#define CXL_FW_ACTIVATE_ONLINE                          0x0
#define CXL_FW_ACTIVATE_OFFLINE                         0x1
#define CXL_FW_MAX_SLOTS                                5
#define CXL_FW_IMAGE_DESCRIPTOR_COUNT                   5
#define CXL_STRING_BUFFER_WIDTH                         256
#define CXL_FW_IMAGE_SIZE_DEFAULT                       2097152    /* Size of firmware image */
#define CXL_FW_IMAGE_ID                                 1
#define CXL_FW_VERSION                                  1
#define CXL_PACKAGE_VERSION_FFFFFFFE                    0xFFFFFFFE
#define CXL_FIRMWARE_IMAGE_ID_NAME                      L"CXL Firmware Version 1.0"
#define CXL_PACKAGE_VERSION_NAME                        L"CXL Firmware Package Version Name UEFI Driver"
#define CXL_FW_SIZE                                     32768    /* 32 mb */
#define CXL_BITS_PER_LONG                               32
#define CXL_UL                                          (UINTN)
#define CXL_GENMASK(h, l)                               (((~CXL_UL(0)) - (CXL_UL(1) << (l)) + 1) & (~CXL_UL(0) >> (CXL_BITS_PER_LONG - 1 - (h))))
#define CXL_CONTROLLER_PRIVATE_FROM_FIRMWARE_MGMT(a)    CR (a, CXL_CONTROLLER_PRIVATE_DATA, FirmwareMgmt, CXL_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define CXL_QEMU                                        1

typedef struct {
  UINT16    VendorID;
  UINT16    DeviceID;
} CXL_VENDORID;

typedef enum {
  PCIE_EXT_CAP_HEADER,
  PCIE_DVSEC_HEADER_1,
  PCIE_DVSEC_HEADER_2,
  PCIE_DVSEC_HEADER_MAX
} CXL_PCIE_DVSEC_HEADER_ENUM;

/* Register Block Identifier (RBI) */
enum cxl_regloc_type {
  CXL_REGLOC_RBI_EMPTY = 0,
  CXL_REGLOC_RBI_COMPONENT,
  CXL_REGLOC_RBI_VIRT,
  CXL_REGLOC_RBI_MEMDEV,
  CXL_REGLOC_RBI_PMU,
  CXL_REGLOC_RBI_TYPES
};

struct cxl_reg_map {
  BOOLEAN          valid;
  UINT32           id;
  unsigned long    offset;
  unsigned long    size;
};

struct cxl_device_reg_map {
  struct    cxl_reg_map status;
  struct    cxl_reg_map mbox;
};

struct cxl_register_map {
  UINT32                reg_type;
  UINT32                bar;
  unsigned long long    regoffset;
  unsigned long         mBoxoffset;
};

enum cxl_opcode {
  CXL_MBOX_OP_INVALID     = 0x0000,
  CXL_MBOX_OP_GET_FW_INFO = 0x0200,
  CXL_MBOX_OP_TRANSFER_FW = 0x0201,
  CXL_MBOX_OP_ACTIVATE_FW = 0x0202,
  CXL_MBOX_OP_MAX         = 0x10000
};

#pragma pack(1)
struct cxl_mbox_get_fw_info {
  UINT8    num_slots;
  UINT8    slot_info;
  UINT8    activation_cap;
  UINT8    reserved[13];
  char     slot_1_revision[16];
  char     slot_2_revision[16];
  char     slot_3_revision[16];
  char     slot_4_revision[16];
};
#pragma pack()

#pragma pack(1)
struct cxl_mbox_transfer_fw {
  UINT8     action;
  UINT8     slot;
  UINT8     reserved[2];
  UINT32    offset;
  UINT8     reserved2[0x78];
  UINT8     data[];
};
#pragma pack()

#pragma pack(1)
struct cxl_mbox_activate_fw {
  UINT8    action;
  UINT8    slot;
};
#pragma pack()

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
  UINT8      num_slots;
  UINTN      imageFileSize[CXL_FW_MAX_SLOTS];
  CHAR16     *imageFileBuffer[CXL_FW_MAX_SLOTS];
  BOOLEAN    isSetImageDone[CXL_FW_MAX_SLOTS];
  char       firmware_version[CXL_FW_MAX_SLOTS][0x10];
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
  UINT32    payload_size;
  struct    cxl_fw_state fw;
  char      firmware_version[0x10];
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

extern EFI_FIRMWARE_MANAGEMENT_PROTOCOL    gCxlFirmwareManagement;

EFI_STATUS
EFIAPI
CxlDxeComponentNameGetDriverName(
  IN EFI_COMPONENT_NAME_PROTOCOL    *This,
  IN CHAR8                          *Language,
  OUT CHAR16                        **DriverName
  );

EFI_STATUS
EFIAPI
CxlDxeComponentNameGetControllerName(
  IN EFI_COMPONENT_NAME_PROTOCOL    *This,
  IN EFI_HANDLE                     ControllerHandle,
  IN EFI_HANDLE                     ChildHandle        OPTIONAL,
  IN CHAR8                          *Language,
  OUT CHAR16                        **ControllerName
  );

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.

  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.

  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.

  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.

  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.

  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/

EFI_STATUS
EFIAPI
CxlDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.

  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.

  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.

  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.

  @retval Others                   The driver failded to start the device.

**/

EFI_STATUS
EFIAPI
CxlDriverBindingStart(
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This               A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.

  @param[in]  ControllerHandle   A handle to the device being stopped. The handle must
                                 support a bus specific I/O protocol for the driver
                                 to use to stop the device.
  @param[in]  NumberOfChildren   The number of child device handles in ChildHandleBuffer.

  @param[in]  ChildHandleBuffer  An array of child handles to be freed. May be NULL
                                 if NumberOfChildren is 0.

  @retval EFI_SUCCESS            The device was stopped.

  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.

**/

EFI_STATUS
EFIAPI
CxlDriverBindingStop(
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN  UINTN                       NumberOfChildren,
  IN  EFI_HANDLE                  *ChildHandleBuffer
  );

UINT64 min2(UINT64 a, UINT64 b);

size_t min2size(size_t a, size_t b);

UINT64 min3(UINT64 a, UINT64 b, UINT64 c);

void getChunkCnt(int filesize, int maxPayloadSize, int *chunkCnt, int *chunkSize);

UINT64 field_get(UINT64 reg, UINT32 p1, UINT32 p2);

void strCpy(CHAR16 *st1, char *st2);

void strCpy_c16(CHAR16 *st1, CHAR16 *st2);

void strCpy_const16(CHAR16 *st1, CONST CHAR16 *st2);

int strCmp(CHAR16 *str1, CHAR16 *str2);

void InitializeFwImageDescriptor(CXL_CONTROLLER_PRIVATE_DATA *Private);

EFI_STATUS pci_uefi_read_config_word(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val);

EFI_STATUS pci_uefi_mem_read_32(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val);

EFI_STATUS pci_uefi_mem_read_64(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT64 *val);

EFI_STATUS pci_uefi_mem_read_n(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, CHAR8 Buffer[], UINT32 Size);

EFI_STATUS pci_uefi_mem_write_32(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT32 *val);

EFI_STATUS pci_uefi_mem_write_64(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, UINT64 *val);

EFI_STATUS pci_uefi_mem_write_n(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 start, CHAR8 Buffer[], UINT32 Size);

EFI_STATUS cxl_mem_get_fw_info(CXL_CONTROLLER_PRIVATE_DATA *Private);

EFI_STATUS cxl_mem_transfer_fw(CXL_CONTROLLER_PRIVATE_DATA *Private, UINT32 nextslot, const UINT8 *data, UINT32 offset, UINT32 size, UINT32 *written);

EFI_STATUS cxl_mem_activate_fw(CXL_CONTROLLER_PRIVATE_DATA *Private);

EFI_STATUS cxl_pci_mbox_send(CXL_CONTROLLER_PRIVATE_DATA *Private);
#endif // _EFI_CXLDXE_H_
