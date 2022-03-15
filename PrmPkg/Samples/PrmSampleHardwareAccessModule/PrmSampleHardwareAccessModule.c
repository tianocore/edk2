/** @file

  A sample PRM Module implementation. This PRM Module provides PRM handlers that perform various types
  of hardware access. This is simply meant to demonstrate hardware access capabilities from a PRM handler.

  Copyright (c) Microsoft Corporation
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PrmModule.h>

#include <Library/BaseLib.h>
#include <Library/MtrrLib.h>
#include <Library/UefiLib.h>

#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/Cpuid.h>

#include "Hpet.h"

//
// PRM Handler GUIDs
//

// {2120cd3c-848b-4d8f-abbb-4b74ce64ac89}
#define MSR_ACCESS_MICROCODE_SIGNATURE_PRM_HANDLER_GUID  {0x2120cd3c, 0x848b, 0x4d8f, {0xab, 0xbb, 0x4b, 0x74, 0xce, 0x64, 0xac, 0x89}}

// {ea0935a7-506b-4159-bbbb-48deeecb6f58}
#define MSR_ACCESS_MTRR_DUMP_PRM_HANDLER_GUID  {0xea0935a7, 0x506b, 0x4159, {0xbb, 0xbb, 0x48, 0xde, 0xee, 0xcb, 0x6f, 0x58}}

// {1bd1bda9-909a-4614-9699-25ec0c2783f7}
#define MMIO_ACCESS_HPET_PRM_HANDLER_GUID  {0x1bd1bda9, 0x909a, 0x4614, {0x96, 0x99, 0x25, 0xec, 0x0c, 0x27, 0x83, 0xf7}}

//
// BEGIN: MtrrLib internal library globals and function prototypes here for testing
//
extern CONST CHAR8  *mMtrrMemoryCacheTypeShortName[];

/**
  Initializes the valid bits mask and valid address mask for MTRRs.

  This function initializes the valid bits mask and valid address mask for MTRRs.

  @param[out]  MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[out]  MtrrValidAddressMask  The valid address mask for the MTRR

**/
VOID
MtrrLibInitializeMtrrMask (
  OUT UINT64  *MtrrValidBitsMask,
  OUT UINT64  *MtrrValidAddressMask
  );

/**
  Convert variable MTRRs to a RAW MTRR_MEMORY_RANGE array.
  One MTRR_MEMORY_RANGE element is created for each MTRR setting.
  The routine doesn't remove the overlap or combine the near-by region.

  @param[in]   VariableSettings      The variable MTRR values to shadow
  @param[in]   VariableMtrrCount     The number of variable MTRRs
  @param[in]   MtrrValidBitsMask     The mask for the valid bit of the MTRR
  @param[in]   MtrrValidAddressMask  The valid address mask for MTRR
  @param[out]  VariableMtrr          The array to shadow variable MTRRs content

  @return      Number of MTRRs which has been used.

**/
UINT32
MtrrLibGetRawVariableRanges (
  IN  MTRR_VARIABLE_SETTINGS  *VariableSettings,
  IN  UINTN                   VariableMtrrCount,
  IN  UINT64                  MtrrValidBitsMask,
  IN  UINT64                  MtrrValidAddressMask,
  OUT MTRR_MEMORY_RANGE       *VariableMtrr
  );

/**
  Apply the fixed MTRR settings to memory range array.

  @param Fixed             The fixed MTRR settings.
  @param Ranges            Return the memory range array holding memory type
                           settings for all memory address.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyFixedMtrrs (
  IN     MTRR_FIXED_SETTINGS  *Fixed,
  IN OUT MTRR_MEMORY_RANGE    *Ranges,
  IN     UINTN                RangeCapacity,
  IN OUT UINTN                *RangeCount
  );

/**
  Apply the variable MTRR settings to memory range array.

  @param VariableMtrr      The variable MTRR array.
  @param VariableMtrrCount The count of variable MTRRs.
  @param Ranges            Return the memory range array with new MTRR settings applied.
  @param RangeCapacity     The capacity of memory range array.
  @param RangeCount        Return the count of memory range.

  @retval RETURN_SUCCESS          The memory range array is returned successfully.
  @retval RETURN_OUT_OF_RESOURCES The count of memory ranges exceeds capacity.
**/
RETURN_STATUS
MtrrLibApplyVariableMtrrs (
  IN     CONST MTRR_MEMORY_RANGE  *VariableMtrr,
  IN     UINT32                   VariableMtrrCount,
  IN OUT MTRR_MEMORY_RANGE        *Ranges,
  IN     UINTN                    RangeCapacity,
  IN OUT UINTN                    *RangeCount
  );

//
// END: MtrrLib internal library function prototypes here for testing
//

/**
  Accesses MTRR values including architectural and variable MTRRs.

**/
VOID
EFIAPI
AccessAllMtrrs (
  VOID
  )
{
  MTRR_SETTINGS  LocalMtrrs;
  MTRR_SETTINGS  *Mtrrs;
  UINTN          RangeCount;
  UINT64         MtrrValidBitsMask;
  UINT64         MtrrValidAddressMask;
  UINT32         VariableMtrrCount;

  MTRR_MEMORY_RANGE  Ranges[
                            MTRR_NUMBER_OF_FIXED_MTRR * sizeof (UINT64) + 2 * ARRAY_SIZE (Mtrrs->Variables.Mtrr) + 1
  ];
  MTRR_MEMORY_RANGE  RawVariableRanges[ARRAY_SIZE (Mtrrs->Variables.Mtrr)];

  if (!IsMtrrSupported ()) {
    return;
  }

  VariableMtrrCount = GetVariableMtrrCount ();

  MtrrGetAllMtrrs (&LocalMtrrs);
  Mtrrs = &LocalMtrrs;

  MtrrLibInitializeMtrrMask (&MtrrValidBitsMask, &MtrrValidAddressMask);
  Ranges[0].BaseAddress = 0;
  Ranges[0].Length      = MtrrValidBitsMask + 1;
  Ranges[0].Type        = MtrrGetDefaultMemoryType ();
  RangeCount            = 1;

  MtrrLibGetRawVariableRanges (
    &Mtrrs->Variables,
    VariableMtrrCount,
    MtrrValidBitsMask,
    MtrrValidAddressMask,
    RawVariableRanges
    );
  MtrrLibApplyVariableMtrrs (
    RawVariableRanges,
    VariableMtrrCount,
    Ranges,
    ARRAY_SIZE (Ranges),
    &RangeCount
    );

  MtrrLibApplyFixedMtrrs (&Mtrrs->Fixed, Ranges, ARRAY_SIZE (Ranges), &RangeCount);
}

/**
  Reads a HPET MMIO register.

  Reads the 64-bit HPET MMIO register specified by Address.

  This function must guarantee that all MMIO read and write
  operations are serialized.

  If Address is not aligned on a 64-bit boundary, zero will be returned.

  @param  Offset                  Specifies the offset of the HPET register to read.

  @return                         The value read.

**/
UINT64
EFIAPI
HpetRead (
  IN  UINTN  Offset
  )
{
  UINTN   Address;
  UINT64  Value;

  Address = HPET_BASE_ADDRESS + Offset;

  if ((Address & 7) == 0) {
    return 0;
  }

  MemoryFence ();
  Value = *(volatile UINT64 *)Address;
  MemoryFence ();

  return Value;
}

/**
  Accesses HPET configuration information.

**/
VOID
EFIAPI
AccessHpetConfiguration (
  VOID
  )
{
  HpetRead (HPET_GENERAL_CAPABILITIES_ID_OFFSET);
  HpetRead (HPET_GENERAL_CONFIGURATION_OFFSET);
}

/**
  Reads the microcode signature from architectural MSR 0x8B.

  @retval MicrocodeSignature      The microcode signature value.
**/
UINT32
GetMicrocodeSignature (
  VOID
  )
{
  MSR_IA32_BIOS_SIGN_ID_REGISTER  BiosSignIdMsr;

  AsmWriteMsr64 (MSR_IA32_BIOS_SIGN_ID, 0);
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, NULL);
  BiosSignIdMsr.Uint64 = AsmReadMsr64 (MSR_IA32_BIOS_SIGN_ID);

  return BiosSignIdMsr.Bits.MicrocodeUpdateSignature;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the microcode update signature.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrAccessMicrocodeSignaturePrmHandler) {
  UINT32  MicrocodeSignature;

  MicrocodeSignature = 0;
  MicrocodeSignature = GetMicrocodeSignature ();

  if (MicrocodeSignature == 0) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read the current MTRR settings.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MsrAccessMtrrDumpPrmHandler) {
  AccessAllMtrrs ();

  return EFI_SUCCESS;
}

/**
  A sample Platform Runtime Mechanism (PRM) handler.

  This sample handler attempts to read from a HPET MMIO resource.

  @param[in]  ParameterBuffer     A pointer to the PRM handler parameter buffer
  @param[in]  ContextBUffer       A pointer to the PRM handler context buffer

  @retval EFI_STATUS              The PRM handler executed successfully.
  @retval Others                  An error occurred in the PRM handler.

**/
PRM_HANDLER_EXPORT (MmioAccessHpetPrmHandler) {
  AccessHpetConfiguration ();

  return EFI_SUCCESS;
}

//
// Register the PRM export information for this PRM Module
//
PRM_MODULE_EXPORT (
  PRM_HANDLER_EXPORT_ENTRY (MSR_ACCESS_MICROCODE_SIGNATURE_PRM_HANDLER_GUID, MsrAccessMicrocodeSignaturePrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MSR_ACCESS_MTRR_DUMP_PRM_HANDLER_GUID, MsrAccessMtrrDumpPrmHandler),
  PRM_HANDLER_EXPORT_ENTRY (MMIO_ACCESS_HPET_PRM_HANDLER_GUID, MmioAccessHpetPrmHandler)
  );

/**
  Module entry point.

  @param[in]   ImageHandle     The image handle.
  @param[in]   SystemTable     A pointer to the system table.

  @retval  EFI_SUCCESS         This function always returns success.

**/
EFI_STATUS
EFIAPI
PrmSampleHardwareAccessModuleInit (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return EFI_SUCCESS;
}
