/** @file
  Boot functions declaration for UefiPxeBc Driver.

  Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_PXEBC_BOOT_H__
#define __EFI_PXEBC_BOOT_H__

#define PXEBC_DISPLAY_MAX_LINE             70
#define PXEBC_DEFAULT_UDP_OVERHEAD_SIZE    8
#define PXEBC_DEFAULT_TFTP_OVERHEAD_SIZE   4

#define PXEBC_IS_SIZE_OVERFLOWED(x)   ((sizeof (UINTN) < sizeof (UINT64)) && ((x) > 0xFFFFFFFF))


/**
  Extract the discover information and boot server entry from the
  cached packets if unspecified.

  @param[in]      Private      Pointer to PxeBc private data.
  @param[in]      Type         The type of bootstrap to perform.
  @param[in, out] DiscoverInfo Pointer to EFI_PXE_BASE_CODE_DISCOVER_INFO.
  @param[out]     BootEntry    Pointer to PXEBC_BOOT_SVR_ENTRY.
  @param[out]     SrvList      Pointer to EFI_PXE_BASE_CODE_SRVLIST.

  @retval EFI_SUCCESS       Successfully extracted the information.
  @retval EFI_DEVICE_ERROR  Failed to extract the information.

**/
EFI_STATUS
PxeBcExtractDiscoverInfo (
  IN     PXEBC_PRIVATE_DATA               *Private,
  IN     UINT16                           Type,
  IN OUT EFI_PXE_BASE_CODE_DISCOVER_INFO  **DiscoverInfo,
     OUT PXEBC_BOOT_SVR_ENTRY             **BootEntry,
     OUT EFI_PXE_BASE_CODE_SRVLIST        **SrvList
  );


/**
  Build the discover packet and send out for boot.

  @param[in]  Private               Pointer to PxeBc private data.
  @param[in]  Type                  PxeBc option boot item type.
  @param[in]  Layer                 Pointer to option boot item layer.
  @param[in]  UseBis                Use BIS or not.
  @param[in]  DestIp                Pointer to the server address.
  @param[in]  IpCount               The total count of the server address.
  @param[in]  SrvList               Pointer to the server address list.

  @retval     EFI_SUCCESS           Successfully discovered boot file.
  @retval     EFI_OUT_OF_RESOURCES  Failed to allocate resources.
  @retval     EFI_NOT_FOUND         Can't get the PXE reply packet.
  @retval     Others                Failed to discover boot file.

**/
EFI_STATUS
PxeBcDiscoverBootServer (
  IN  PXEBC_PRIVATE_DATA                *Private,
  IN  UINT16                            Type,
  IN  UINT16                            *Layer,
  IN  BOOLEAN                           UseBis,
  IN  EFI_IP_ADDRESS                    *DestIp,
  IN  UINT16                            IpCount,
  IN  EFI_PXE_BASE_CODE_SRVLIST         *SrvList
  );


/**
  Load boot file into user buffer.

  @param[in]      Private           Pointer to PxeBc private data.
  @param[in, out] BufferSize        Size of user buffer for input;
                                    required buffer size for output.
  @param[in]      Buffer            Pointer to user buffer.

  @retval EFI_SUCCESS          Successfully obtained all the boot information.
  @retval EFI_BUFFER_TOO_SMALL The buffer size is not enough for boot file.
  @retval EFI_ABORTED          User cancelled the current operation.
  @retval Others               Failed to parse out the boot information.

**/
EFI_STATUS
PxeBcLoadBootFile (
  IN     PXEBC_PRIVATE_DATA           *Private,
  IN OUT UINTN                        *BufferSize,
  IN     VOID                         *Buffer         OPTIONAL
  );

#endif
