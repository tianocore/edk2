/** @file
  Host-based unit tests for AmdMmSaveState.c

  MmSaveStateReadRegisterByIndex() in the
  SysCall path always writes 4 bytes via a UINT32* cast, causing an
  out-of-bounds write when Width < 4 (e.g. a 2-byte segment register).

  Copyright (C) 2026 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

//
// MmSaveState.h includes MmServicesTableLib.h which declares gMmst as an
// extern.  We define the global here and point it at our fake save-state.
//

#include <Uefi.h>
#include <Pi/PiMmCis.h>
#include <Protocol/MmCpu.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/AmdSysCallLib.h>

// ── Fake save-state buffer ─────────────────────────────────────────────────
//
// We only need gMmst->CpuSaveState[0] to point somewhere valid; the actual
// content is irrelevant because our mock SysCall() ignores the address.
//
STATIC UINT8  mFakeSaveStateBuffer[4096];

STATIC VOID  *mCpuSaveStatePointers[1];

//
// Zero-initialise the entire table; CpuSaveState is set in main().
//
STATIC EFI_MM_SYSTEM_TABLE  mFakeMmst;

//
// gMmst is declared as extern in MmServicesTableLib.h; we provide the definition.
//
EFI_MM_SYSTEM_TABLE  *gMmst = &mFakeMmst;

/**
  Force the SysCall code path in MmSaveStateReadRegisterByIndex().

  @retval  TRUE   Always returns TRUE to force SysCall code path.
**/
BOOLEAN
EFIAPI
NeedSysCall (
  VOID
  )
{
  return TRUE;
}

/**
  Return a known 64-bit value regardless of which register is requested.

  Low 32 bits: 0x11223344. Low 16 bits: 0x3344. The bug writes all 4 bytes
  of the low UINT32 even when Width == 2, so the sentinel byte at Dest[2]
  will be overwritten with 0x22 on buggy code (little-endian byte 2 of
  0x11223344).

  @param[in]  CallIndex  The syscall index.
  @param[in]  Arg1       First argument.
  @param[in]  Arg2       Second argument.
  @param[in]  Arg3       Third argument.

  @return  Always returns 0x8877665511223344.
**/
UINT64
EFIAPI
SysCall (
  UINTN  CallIndex,
  UINTN  Arg1,
  UINTN  Arg2,
  UINTN  Arg3
  )
{
  return 0x8877665511223344ULL;
}

// ── Forward declaration from AmdMmSaveState.c ─────────────────────────────

//
// Defined in AmdMmSaveState.c, compiled directly into this test module.
//
EFI_STATUS
MmSaveStateReadRegisterByIndex (
  IN UINTN  CpuIndex,
  IN UINTN  RegisterIndex,
  IN UINTN  Width,
  OUT VOID  *Buffer
  );

// ── Register index constants ───────────────────────────────────────────────
//
// Indices into mCpuWidthOffset[] as laid out in AmdMmSaveState.c:
//   [0]  Reserved
//   [1]  SMMRevId    (Width64=4)
//   [2]  GDTBase     (Width64=8)
//   [3]  IDTBase     (Width64=8)
//   [4]  LDTBase     (Width64=8)
//   [5]  GDTLIMIT    (Width64=2)  ← 2-byte register
//   [6]  IDTLIMIT    (Width64=2)
//   [7]  LDTRLimit   (Width64=4)
//   [8]  LDTInfo     (Width64=0)
//   [9]  ES          (Width64=2)
//   [10] CS          (Width64=2)
//   [11] SS          (Width64=2)
//   [12] DS          (Width64=2)
//   [13] FS          (Width64=2)
//   [14] GS          (Width64=2)
//   [15] LDTR_SEL    (Width64=2)
//   [16] TR_SEL      (Width64=2)
//   [17] DR7         (Width64=8)  ← 8-byte register
//
#define TEST_INDEX_GDTLIMIT  5    // Width64 == 2
#define TEST_INDEX_DR7       17   // Width64 == 8

/**
  Test that a Width=2 read does not write beyond 2 bytes (2-byte register, low path).

  Buggy code: *(UINT32 *)Buffer = (UINT32)SysCall(...) writes 4 bytes into a
  2-byte destination, overflowing Dest[2..3]. We place a sentinel at Dest[2]
  and assert it is unchanged after the read.

  @param[in]  Context  Optional UNIT_TEST_CONTEXT.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
PartialWrite_Width2 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dest[4];
  UINT16      *Target;

  ZeroMem (Dest, sizeof (Dest));
  Dest[2] = 0xAB;   // sentinel – must survive the read
  Dest[3] = 0xCD;   // sentinel – must survive the read

  Target = (UINT16 *)&Dest[0];

  Status = MmSaveStateReadRegisterByIndex (
             0,                   // CpuIndex
             TEST_INDEX_GDTLIMIT, // RegisterIndex (Width64 == 2)
             2,                   // Width
             Target
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // On buggy code the UINT32* cast writes bytes 0..3, overwriting Dest[2]
  // with 0x22 (little-endian byte 2 of 0x11223344) and Dest[3] with 0x11.
  //
  UT_ASSERT_EQUAL (Dest[2], 0xAB);
  UT_ASSERT_EQUAL (Dest[3], 0xCD);

  return UNIT_TEST_PASSED;
}

/**
  Test that a Width=5 read does not write beyond 5 bytes (8-byte register, high path).

  When Width == 5 the function reads the low 4 bytes into Dest[0..3], then
  reads Width-4 == 1 byte into Dest[4]. Buggy code: *((UINT32 *)Buffer + 1) =
  (UINT32)SysCall() writes 4 bytes starting at Dest[4], overflowing Dest[5..7].
  Sentinel region is Dest[5..8]; all must be unchanged after the call.

  @param[in]  Context  Optional UNIT_TEST_CONTEXT.

  @retval  UNIT_TEST_PASSED             The Unit test has completed and the test
                                        case was successful.
  @retval  UNIT_TEST_ERROR_TEST_FAILED  A test case assertion has failed.
**/
UNIT_TEST_STATUS
EFIAPI
PartialWrite_Width5 (
  IN UNIT_TEST_CONTEXT  Context
  )
{
  EFI_STATUS  Status;
  UINT8       Dest[9];

  ZeroMem (Dest, sizeof (Dest));
  Dest[5] = 0xBE;
  Dest[6] = 0xBE;
  Dest[7] = 0xBE;
  Dest[8] = 0xBE;

  Status = MmSaveStateReadRegisterByIndex (
             0,              // CpuIndex
             TEST_INDEX_DR7, // RegisterIndex (Width64 == 8, so Width=5 is valid)
             5,              // Width
             Dest
             );

  UT_ASSERT_NOT_EFI_ERROR (Status);

  //
  // On buggy code the UINT32* cast writes 4 bytes starting at Dest[4],
  // overwriting Dest[5..7] with bytes of 0x11223344 and corrupting sentinels.
  //
  UT_ASSERT_EQUAL (Dest[5], 0xBE);
  UT_ASSERT_EQUAL (Dest[6], 0xBE);
  UT_ASSERT_EQUAL (Dest[7], 0xBE);
  UT_ASSERT_EQUAL (Dest[8], 0xBE);

  return UNIT_TEST_PASSED;
}

/**
  Standard POSIX C entry point for host based unit test execution.

  @param  Argc  Number of arguments.
  @param  Argv  Array of arguments.

  @return Test application exit code.
**/
INT32
main (
  INT32  Argc,
  CHAR8  *Argv[]
  )
{
  EFI_STATUS                  Status;
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;
  UNIT_TEST_SUITE_HANDLE      Suite;

  //
  // Wire up the fake CpuSaveState pointer array at runtime to avoid a
  // complex struct initializer for EFI_MM_SYSTEM_TABLE.
  //
  mCpuSaveStatePointers[0] = mFakeSaveStateBuffer;
  mFakeMmst.NumberOfCpus   = 1;
  mFakeMmst.CpuSaveState   = mCpuSaveStatePointers;

  Framework = NULL;
  Suite     = NULL;

  Status = InitUnitTestFramework (
             &Framework,
             "AmdMmSaveStateLib OOB Write Tests.",
             gEfiCallerBaseName,
             "1.0"
             );
  if (EFI_ERROR (Status)) {
    return 1;
  }

  Status = CreateUnitTestSuite (
             &Suite,
             Framework,
             "MmSaveStateReadRegisterByIndex SysCall path",
             "MmSaveStateLib.AmdSysCall",
             NULL,
             NULL
             );
  if (EFI_ERROR (Status)) {
    FreeUnitTestFramework (Framework);
    return 1;
  }

  AddTestCase (
    Suite,
    "Width=2 read must not write beyond 2 bytes (2-byte register, low path)",
    "PartialWrite_Width2",
    PartialWrite_Width2,
    NULL,
    NULL,
    NULL
    );

  AddTestCase (
    Suite,
    "Width=5 read must not write beyond 5 bytes (8-byte register, high path)",
    "PartialWrite_Width5",
    PartialWrite_Width5,
    NULL,
    NULL,
    NULL
    );

  Status = RunAllTestSuites (Framework);

  FreeUnitTestFramework (Framework);

  return EFI_ERROR (Status) ? 1 : 0;
}
