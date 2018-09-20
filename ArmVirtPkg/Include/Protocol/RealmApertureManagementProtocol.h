/** @file
  Realm Aperture Management Protocol (RAMP)
  On Arm CCA Systems the Realm protects access and visibility of Guest memory
  and code execution from software outside the realm.

  However, software executing in a Realm needs to interact with the external
  world. This may be done using virtualised disk, network interfaces, etc.
  The drivers for these virtualised devices need to share buffers with the host
  OS to exchange information/data.

  Since the Guest memory is protected by the Realm, the host cannot access these
  buffers unless the IPA state of the buffers is changed to Protected EMPTY by
  the software executing in the Realm.

  By enabling the sharing of the buffers, we are essentially opening an
  aperture so that the host OS can access the range of pages that are shared.

  The virtual firmware (Guest firmware) needs a mechanism to manage the sharing
  of buffers. The Realm Aperture Management Protocol provides an interface that
  UEFI drivers/modules can use to enable/disable the sharing of buffers with the
  Host. The protocol also tracks open apertures and ensures they are shut on
  ExitBootServices.

  Copyright (c) 2022 - 2023, ARM Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - RAMP  - Realm Aperture Management Protocol
**/

#ifndef REALM_APERTURE_MANAGEMENT_PROTOCOL_H_
#define REALM_APERTURE_MANAGEMENT_PROTOCOL_H_

/** This macro defines the Realm Aperture Management Protocol GUID.

  GUID: {585C00BE-CF7C-4DB8-8AA2-490D67F5F6E6}
*/
#define EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_GUID     \
  { 0x585c00be, 0xcf7c, 0x4db8,                         \
    { 0x8a, 0xa2, 0x49, 0xd, 0x67, 0xf5, 0xf6, 0xe6 }   \
  };

/** This macro defines the Realm Aperture Management Protocol Revision.
*/
#define EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_REVISION  0x00010000

#pragma pack(1)

/** Enables sharing of the memory buffers with the host.

  @param [in]  Memory             Pointer to the page start address.
  @param [in]  Pages              Number of pages to share.
  @param [out] ApertureReference  Reference to the opened aperture.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @retval EFI_ACCESS_DENIED       Aperture already open over memory region.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_OPEN_APERTURE)(
  IN  CONST EFI_PHYSICAL_ADDRESS                    Memory,
  IN  CONST UINTN                                   Pages,
  OUT       EFI_HANDLE                      *CONST ApertureReference
  );

/** Disables the sharing of the buffers.

  @param [in] ApertureReference   Reference to the aperture for closing.
  @param [in] MemoryMapLocked     The function is executing on the stack of
                                  gBS->ExitBootServices(); changes to the UEFI
                                  memory map are forbidden.

  @retval EFI_SUCCESS             The operation completed successfully.
  @retval EFI_INVALID_PARAMETER   A parameter is invalid.
  @retval EFI_NOT_FOUND           The required buffer information is not found.
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_CLOSE_APERTURE)(
  IN  CONST EFI_HANDLE                              ApertureReference,
  IN        BOOLEAN                                 MemoryMapLocked
  );

/** A structure describing the interface provided by the Realm Aperture
    Management Protocol.
*/
typedef struct RealmApertureManagementProtocol {
  /// The Realm Aperture Management Protocol revision.
  UINT64                                                     Revision;

  /// Shares Realm Pages(s) with the Host.
  EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_OPEN_APERTURE     OpenAperture;

  /// Makes the Realm Pages(s) private to the Realm.
  EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL_CLOSE_APERTURE    CloseAperture;
} EDKII_REALM_APERTURE_MANAGEMENT_PROTOCOL;

/** The Realm Aperture Management Protocol GUID.
*/
extern EFI_GUID  gEfiRealmApertureManagementProtocolGuid;

#pragma pack()

#endif // REALM_APERTURE_MANAGEMENT_PROTOCOL_H_
