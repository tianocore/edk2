/** @file
  vm launch update protocol

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#define VM_LAUNCH_UPDATE_PROTOCOL_GUID                                           \
  { 0x136d1973, 0x2de0, 0x4736, { 0xb0, 0x08, 0xd6, 0x02, 0x94, 0x72, 0x89, 0x33 }}

#define VM_LAUNCH_UPDATE_PROTOCOL_VERSION  1

typedef struct VM_LAUNCH_UPDATE_PROTOCOL VM_LAUNCH_UPDATE_PROTOCOL;

// Capabilities and Control
#define VM_LAUNCH_UPDATE_CAP_IGVM  (1llu << 32)

// Control only
#define VM_LAUNCH_UPDATE_CTL_DISABLE  (1llu << 0)
#define VM_LAUNCH_UPDATE_CTL_ONCE     (1llu << 1)

/**
  Get VM Launch Update Capabilities

  @param[in] This         Instance of VM_LAUNCH_UPDATE_PROTOCOL

  @retval Capabilities    VM_LAUNCH_UPDATE_CAP_* bits

**/
typedef UINT64
(EFIAPI *VM_LAUNCH_UPDATE_GET_CAPABILITIES)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This
  );

/**
  Set VM Launch Update Control Flags

  The VM_LAUNCH_UPDATE_CAP_IGVM format flag must be set as this is the only
  format currently supported.

  @param[in] This         Instance of VM_LAUNCH_UPDATE_PROTOCOL
  @param[in] Control      VM_LAUNCH_UPDATE_CAP_* and VM_LAUNCH_UPDATE_CTL_*

  @retval EFI_SUCCESS             Setting the flags was succcessful.
  @retval EFI_INVALID_PARAMETER   One or more flags are invalid.

**/
typedef EFI_STATUS
(EFIAPI *VM_LAUNCH_UPDATE_SET_CONTROL)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This,
  IN UINT64 Control
  );

/**
  Set VM Launch Update Firmware Image

  Pass the firmware in the format specified via SetControl.  Currently IGVM is
  the only format supported.

  @param[in] This         Instance of VM_LAUNCH_UPDATE_PROTOCOL
  @param[in] Image        Pointer to the Firmware Image
  @param[in] Size         Size of the Firmware Image

  @retval EFI_SUCCESS             Setting the Firmware Image was successful.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
**/
typedef EFI_STATUS
(EFIAPI *VM_LAUNCH_UPDATE_SET_IMAGE)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This,
  IN VOID *Image,
  IN UINT64 Size
  );

/**
  Set VM Launch Update Opaque Blob

  Setup a opaque blob which will be passed through to the updated firmware.

  @param[in] This         Instance of VM_LAUNCH_UPDATE_PROTOCOL
  @param[in] Opaque       Pointer to the Opaque Blob
  @param[in] Size         Size of the Opaque Blob

  @retval EFI_SUCCESS             Setting the Opaque Blob was successful.
  @retval EFI_INVALID_PARAMETER   One or more parameters are invalid.
**/
typedef EFI_STATUS
(EFIAPI *VM_LAUNCH_UPDATE_SET_OPAQUE)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This,
  IN VOID *Opaque,
  IN UINT64 Size
  );

/**
  Load VM Launch Update

  Load the updated firmware image to the host and prepare everything for the
  launch of the updated firmware.

  @param[in] This         Instance of VM_LAUNCH_UPDATE_PROTOCOL

  @retval EFI_SUCCESS            Loading the Firmware Image was successful.
  @retval EFI_LOAD_ERROR         Loading the Firmware Image failed.
  @retval EFI_INVALID_PARAMETER  Processing the Firmware Image failed.
  @retval EFI_DEVICE_ERROR       Other error.
**/
typedef EFI_STATUS
(EFIAPI *VM_LAUNCH_UPDATE_LOAD)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This
  );

/**
  Launch VM with updated firmware image

  This should never fail when called after a successful Load() call.

  @param[in] This                Instance of VM_LAUNCH_UPDATE_PROTOCOL

  @retval EFI_INVALID_PARAMETER  No firmware image loaded.
  @retval EFI_DEVICE_ERROR       Launching the Firmware Image failed.
**/
typedef EFI_STATUS
(EFIAPI *VM_LAUNCH_UPDATE_LAUNCH)(
  IN VM_LAUNCH_UPDATE_PROTOCOL *This
  );

struct VM_LAUNCH_UPDATE_PROTOCOL {
  UINT64                               Version;
  VM_LAUNCH_UPDATE_GET_CAPABILITIES    GetCapabilities;
  VM_LAUNCH_UPDATE_SET_CONTROL         SetControl;
  VM_LAUNCH_UPDATE_SET_IMAGE           SetImage;
  VM_LAUNCH_UPDATE_SET_OPAQUE          SetOpaque;
  VM_LAUNCH_UPDATE_LOAD                Load;
  VM_LAUNCH_UPDATE_LAUNCH              Launch;
};

extern EFI_GUID  gVMLaunchUpdateProtocolGuid;
