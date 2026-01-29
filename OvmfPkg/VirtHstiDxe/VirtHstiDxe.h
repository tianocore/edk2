/** @file

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#define VIRT_HSTI_SECURITY_FEATURE_SIZE  2

#define VIRT_HSTI_BYTE0_SMM_SMRAM_LOCK         BIT0
#define VIRT_HSTI_BYTE0_SMM_SECURE_VARS_FLASH  BIT1
#define VIRT_HSTI_BYTE0_READONLY_CODE_FLASH    BIT2

typedef struct {
  // ADAPTER_INFO_PLATFORM_SECURITY
  UINT32    Version;
  UINT32    Role;
  CHAR16    ImplementationID[256];
  UINT32    SecurityFeaturesSize;
  // bitfields
  UINT8     SecurityFeaturesRequired[VIRT_HSTI_SECURITY_FEATURE_SIZE];
  UINT8     SecurityFeaturesImplemented[VIRT_HSTI_SECURITY_FEATURE_SIZE];
  UINT8     SecurityFeaturesVerified[VIRT_HSTI_SECURITY_FEATURE_SIZE];
  CHAR16    ErrorString[1];
} VIRT_ADAPTER_INFO_PLATFORM_SECURITY;

VOID
VirtHstiSetSupported (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti,
  IN UINT32                            ByteIndex,
  IN UINT8                             BitMask
  );

BOOLEAN
VirtHstiIsSupported (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti,
  IN UINT32                            ByteIndex,
  IN UINT8                             BitMask
  );

VOID
VirtHstiTestResult (
  CHAR16     *ErrorMsg,
  IN UINT32  ByteIndex,
  IN UINT8   BitMask
  );

/* QemuQ35.c */

VIRT_ADAPTER_INFO_PLATFORM_SECURITY *
VirtHstiQemuQ35Init (
  VOID
  );

VOID
VirtHstiQemuQ35Verify (
  VOID
  );

/* QemuPC.c */

VIRT_ADAPTER_INFO_PLATFORM_SECURITY *
VirtHstiQemuPCInit (
  VOID
  );

VOID
VirtHstiQemuPCVerify (
  VOID
  );

/* QemuCommon.c */

VOID
VirtHstiQemuCommonInit (
  VIRT_ADAPTER_INFO_PLATFORM_SECURITY  *VirtHsti
  );

VOID
VirtHstiQemuCommonVerify (
  VOID
  );

/* Flash.c */

#define QEMU_FIRMWARE_FLASH_UNKNOWN    0
#define QEMU_FIRMWARE_FLASH_IS_ROM     1
#define QEMU_FIRMWARE_FLASH_IS_RAM     2
#define QEMU_FIRMWARE_FLASH_READ_ONLY  3
#define QEMU_FIRMWARE_FLASH_WRITABLE   4

UINT32
VirtHstiQemuFirmwareFlashCheck (
  UINT32  Address
  );
