/** @file
  Define PPI to shadow Firmware Volume from flash to Permanent Memory.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_FIRMWARE_VOLUME_SHADOW_PPI_H_
#define PEI_FIRMWARE_VOLUME_SHADOW_PPI_H_

//
// Firmware Volume Shadow PPI GUID value
//
#define EDKII_FIRMWARE_VOLUME_SHADOW_PPI_GUID \
  { \
    0x7dfe756c, 0xed8d, 0x4d77, { 0x9e, 0xc4, 0x39, 0x9a, 0x8a, 0x81, 0x51, 0x16 } \
  }

/**
  Copy FV to Destination.  Length of copy is FV length from FV Header.

  @param[in]  FirmwareVolumeBase  Base address of FV to shadow. Length of FV
                                  is in FV Header.
  @param[in]  Destination         Pointer to the Buffer in system memory to
                                  shadow FV.
  @param[in]  DestinationLength   Size of Destination buffer in bytes.

  @retval EFI_SUCCESS            Shadow complete
  @retval EFI_INVALID_PARAMETER  Destination is NULL
  @retval EFI_INVALID_PARAMETER  DestinationLength = 0.
  @retval EFI_INVALID_PARAMETER  FV does not have valid FV Header.
  @retval EFI_INVALID_PARAMETER  FV overlaps Destination.
  @retval EFI_INVALID_PARAMETER  Destination + DestinationLength rolls over 4GB
                                 for 32-bit or 64-bit rollover.
  @retval EFI_BUFFER_TOO_SMALL   DestinationLength less than FV length from FV
                                 Header.
  @retval EFI_UNSUPPORTED        FirmwareVolumeBase to FVBase + FVLength does
                                 not support shadow.  Caller should fallback to
                                 CopyMem().

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_FIRMWARE_VOLUME_SHADOW)(
  IN EFI_PHYSICAL_ADDRESS  FirmwareVolumeBase,
  IN VOID                  *Destination,
  IN UINTN                 DestinationLength
  );

///
/// This PPI provides a service to shadow a FV from one location to another
///
typedef struct {
  EDKII_PEI_FIRMWARE_VOLUME_SHADOW    FirmwareVolumeShadow;
} EDKII_PEI_FIRMWARE_VOLUME_SHADOW_PPI;

extern EFI_GUID  gEdkiiPeiFirmwareVolumeShadowPpiGuid;

#endif
