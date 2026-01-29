/** @file

  Internal definitions for the virtio-blk driver, which produces Block I/O
  Protocol instances for virtio-blk devices.

  Copyright (C) 2012, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_BLK_DXE_H_
#define _VIRTIO_BLK_DXE_H_

#include <Protocol/BlockIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>

#include <IndustryStandard/Virtio.h>

#define VBLK_SIG  SIGNATURE_32 ('V', 'B', 'L', 'K')

typedef struct {
  //
  // Parts of this structure are initialized / torn down in various functions
  // at various call depths. The table to the right should make it easier to
  // track them.
  //
  //                     field                    init function       init dpth
  //                     ---------------------    ------------------  ---------
  UINT32                    Signature;         // DriverBindingStart  0
  VIRTIO_DEVICE_PROTOCOL    *VirtIo;           // DriverBindingStart  0
  EFI_EVENT                 ExitBoot;          // DriverBindingStart  0
  VRING                     Ring;              // VirtioRingInit      2
  EFI_BLOCK_IO_PROTOCOL     BlockIo;           // VirtioBlkInit       1
  EFI_BLOCK_IO_MEDIA        BlockIoMedia;      // VirtioBlkInit       1
  VOID                      *RingMap;          // VirtioRingMap       2
} VBLK_DEV;

#define VIRTIO_BLK_FROM_BLOCK_IO(BlockIoPointer) \
        CR (BlockIoPointer, VBLK_DEV, BlockIo, VBLK_SIG)

/**

  Device probe function for this driver.

  The DXE core calls this function for any given device in order to see if the
  driver can drive the device.

  Specs relevant in the general sense:

  - UEFI Spec 2.3.1 + Errata C:
    - 6.3 Protocol Handler Services -- for accessing the underlying device
    - 10.1 EFI Driver Binding Protocol -- for exporting ourselves

  - Driver Writer's Guide for UEFI 2.3.1 v1.01:
    - 5.1.3.4 OpenProtocol() and CloseProtocol() -- for accessing the
      underlying device
    - 9 Driver Binding Protocol -- for exporting ourselves

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The device to probe.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS      The driver supports the device being probed.

  @retval EFI_UNSUPPORTED  Based on virtio-blk discovery, we do not support
                           the device.

  @return                  Error codes from the OpenProtocol() boot service.

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**

  After we've pronounced support for a specific device in
  DriverBindingSupported(), we start managing said device (passed in by the
  Driver Execution Environment) with the following service.

  See DriverBindingSupported() for specification references.

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The supported device to drive.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS           Driver instance has been created and
                                initialized  for the virtio-blk device, it
                                is now accessible via EFI_BLOCK_IO_PROTOCOL.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @return                       Error codes from the OpenProtocol() boot
                                service, VirtioBlkInit(), or the
                                InstallProtocolInterface() boot service.

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**

  Stop driving a virtio-blk device and remove its BlockIo interface.

  This function replays the success path of DriverBindingStart() in reverse.
  The host side virtio-blk device is reset, so that the OS boot loader or the
  OS may reinitialize it.

  @param[in] This               The EFI_DRIVER_BINDING_PROTOCOL object
                                incorporating this driver (independently of any
                                device).

  @param[in] DeviceHandle       Stop driving this device.

  @param[in] NumberOfChildren   Since this function belongs to a device driver
                                only (as opposed to a bus driver), the caller
                                environment sets NumberOfChildren to zero, and
                                we ignore it.

  @param[in] ChildHandleBuffer  Ignored (corresponding to NumberOfChildren).

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   DeviceHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer
  );

//
// UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol
// Driver Writer's Guide for UEFI 2.3.1 v1.01,
//   24.2 Block I/O Protocol Implementations
//
EFI_STATUS
EFIAPI
VirtioBlkReset (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN BOOLEAN                ExtendedVerification
  );

/**

  ReadBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.ReadBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.2. ReadBlocks() and
    ReadBlocksEx() Implementation.

  Parameter checks and conformant return values are implemented in
  VerifyReadWriteRequest() and SynchronousRequest().

  A zero BufferSize doesn't seem to be prohibited, so do nothing in that case,
  successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL  *This,
  IN  UINT32                 MediaId,
  IN  EFI_LBA                Lba,
  IN  UINTN                  BufferSize,
  OUT VOID                   *Buffer
  );

/**

  WriteBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.WriteBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.3 WriteBlocks() and
    WriteBlockEx() Implementation.

  Parameter checks and conformant return values are implemented in
  VerifyReadWriteRequest() and SynchronousRequest().

  A zero BufferSize doesn't seem to be prohibited, so do nothing in that case,
  successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This,
  IN UINT32                 MediaId,
  IN EFI_LBA                Lba,
  IN UINTN                  BufferSize,
  IN VOID                   *Buffer
  );

/**

  FlushBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.FlushBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.4 FlushBlocks() and
    FlushBlocksEx() Implementation.

  If the underlying virtio-blk device doesn't support flushing (ie.
  write-caching), then this function should not be called by higher layers,
  according to EFI_BLOCK_IO_MEDIA characteristics set in VirtioBlkInit().
  Should they do nonetheless, we do nothing, successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL  *This
  );

//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("Virtio Block Device") are not formatted because the
// driver supports only that device type. Therefore the driver name suffices
// for unambiguous identification.
//

EFI_STATUS
EFIAPI
VirtioBlkGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

EFI_STATUS
EFIAPI
VirtioBlkGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   DeviceHandle,
  IN  EFI_HANDLE                   ChildHandle,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

#endif // _VIRTIO_BLK_DXE_H_
