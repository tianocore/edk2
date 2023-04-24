/** @file
  This library defines some routines that are generic for IA32 family CPU.

  The library routines are UEFI specification compliant.

  Copyright (c) 2020, AMD Inc. All rights reserved.<BR>
  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Intel/Cpuid.h>
#include <Register/Amd/Cpuid.h>

#include <Library/BaseLib.h>
#include <Library/CpuLib.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Uefi/UefiBaseType.h>

/**
  Determine if the standard CPU signature is "AuthenticAMD".

  @retval TRUE  The CPU signature matches.
  @retval FALSE The CPU signature does not match.

**/
BOOLEAN
EFIAPI
StandardSignatureIsAuthenticAMD (
  VOID
  )
{
  UINT32  RegEbx;
  UINT32  RegEcx;
  UINT32  RegEdx;

  AsmCpuid (CPUID_SIGNATURE, NULL, &RegEbx, &RegEcx, &RegEdx);
  return (RegEbx == CPUID_SIGNATURE_AUTHENTIC_AMD_EBX &&
          RegEcx == CPUID_SIGNATURE_AUTHENTIC_AMD_ECX &&
          RegEdx == CPUID_SIGNATURE_AUTHENTIC_AMD_EDX);
}

/**
  Return the 32bit CPU family and model value.

  @return CPUID[01h].EAX with Processor Type and Stepping ID cleared.
**/
UINT32
EFIAPI
GetCpuFamilyModel (
  VOID
  )
{
  CPUID_VERSION_INFO_EAX  Eax;

  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, NULL, NULL);

  //
  // Mask other fields than Family and Model.
  //
  Eax.Bits.SteppingId    = 0;
  Eax.Bits.ProcessorType = 0;
  Eax.Bits.Reserved1     = 0;
  Eax.Bits.Reserved2     = 0;
  return Eax.Uint32;
}

/**
  Return the CPU stepping ID.
  @return CPU stepping ID value in CPUID[01h].EAX.
**/
UINT8
EFIAPI
GetCpuSteppingId (
  VOID
  )
{
  CPUID_VERSION_INFO_EAX  Eax;

  AsmCpuid (CPUID_VERSION_INFO, &Eax.Uint32, NULL, NULL, NULL);

  return (UINT8)Eax.Bits.SteppingId;
}

/**
  Get the max platform addressable bits.
  Max physical address bits can be get from CPUID. When TME-MK feature
  is enabled, the upper bits of the max physical address bits are
  repurposed for usage as a KeyID.
  Therefore, the max platform addressable bits is the max physical
  address bits minus the upper bits used for KeyID if TME-MK is enable.

  @param[out] ValidAddressMask          Bitmask with valid address bits set to
                                        one; other bits are clear. Optional
                                        parameter.

  @param[out] ValidPageBaseAddressMask  Bitmask with valid page base address
                                        bits set to one; other bits are clear.
                                        Optional parameter.

  @return  The max platform addressable bits.
**/
UINT8
EFIAPI
GetMaxPlatformAddressBits (
  OUT UINT64  *ValidAddressMask         OPTIONAL,
  OUT UINT64  *ValidPageBaseAddressMask OPTIONAL
  )
{
  UINT32                                       MaxExtendedFunction;
  CPUID_VIR_PHY_ADDRESS_SIZE_EAX               VirPhyAddressSize;
  UINT64                                       AddressMask;
  UINT64                                       PageBaseAddressMask;
  UINT32                                       MaxFunction;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX  ExtendedFeatureFlagsEcx;
  MSR_IA32_TME_ACTIVATE_REGISTER               TmeActivate;
  MSR_IA32_TME_CAPABILITY_REGISTER             TmeCapability;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunction, NULL, NULL, NULL);
  if (MaxExtendedFunction >= CPUID_VIR_PHY_ADDRESS_SIZE) {
    AsmCpuid (
      CPUID_VIR_PHY_ADDRESS_SIZE,
      &VirPhyAddressSize.Uint32,
      NULL,
      NULL,
      NULL
      );
  } else {
    VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
  }

  //
  // CPUID enumeration of MAX_PA is unaffected by TME-MK activation and will continue
  // to report the maximum physical address bits available for software to use,
  // irrespective of the number of KeyID bits.
  // So, we need to check if TME is enabled and adjust the PA size accordingly.
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxFunction, NULL, NULL, NULL);
  if (MaxFunction >= CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS) {
    AsmCpuidEx (CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS, 0, NULL, NULL, &ExtendedFeatureFlagsEcx.Uint32, NULL);
    if (ExtendedFeatureFlagsEcx.Bits.TME_EN == 1) {
      TmeActivate.Uint64   = AsmReadMsr64 (MSR_IA32_TME_ACTIVATE);
      TmeCapability.Uint64 = AsmReadMsr64 (MSR_IA32_TME_CAPABILITY);
      if ((TmeActivate.Bits.TmeEnable == 1) && (TmeCapability.Bits.MkTmeMaxKeyidBits != 0)) {
        VirPhyAddressSize.Bits.PhysicalAddressBits -= TmeActivate.Bits.MkTmeKeyidBits;
      }
    }
  }

  AddressMask         = LShiftU64 (1, VirPhyAddressSize.Bits.PhysicalAddressBits) - 1;
  PageBaseAddressMask = AddressMask & ~(UINT64)EFI_PAGE_MASK;

  if (ValidAddressMask != NULL) {
    *ValidAddressMask = AddressMask;
  }

  if (ValidPageBaseAddressMask != NULL) {
    *ValidPageBaseAddressMask = PageBaseAddressMask;
  }

  return (UINT8)VirPhyAddressSize.Bits.PhysicalAddressBits;
}
