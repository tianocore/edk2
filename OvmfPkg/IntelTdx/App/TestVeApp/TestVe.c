/** @file
  This is a test application that demonstrates how to use the C-style entry point
  for a shell application.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellCEntryLib.h>
#include <Library/PciLib.h>
#include <Protocol/PciIo.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "InternalMmio.h"

STATIC SHELL_PARAM_ITEM  mParamList[] = {
  { L"-r", TypeValue },
  { L"-c", TypeValue },
  { L"-i", TypeFlag  },
  { L"-u", TypeValue },
  { L"-a", TypeFlag  },
  { L"-?", TypeFlag  },
  { L"-h", TypeFlag  },
  { NULL,  TypeMax   },
};

STATIC BOOLEAN  mIsRdMsr       = FALSE;
STATIC BOOLEAN  mIsCpuid       = FALSE;
STATIC BOOLEAN  mIsMmio        = FALSE;
STATIC BOOLEAN  mIsUnsupported = FALSE;
STATIC BOOLEAN  mIsAll         = FALSE;

/**
   Display Application Header
 **/
STATIC
VOID
DisplayHeader (
  )
{
  CHAR16  build_date[20];
  CHAR16  build_time[20];

  AsciiStrToUnicodeStrS (__DATE__, build_date, 100);
  AsciiStrToUnicodeStrS (__TIME__, build_time, 100);

  Print (L"*********************************************\n");
  Print (L"Built: %s %s\n", build_date, build_time);
  Print (L"*********************************************\n");
}

STATIC
VOID
PrintUsage (
  VOID
  )
{
  Print (
    L"Test Application to inject VE and trigger #VE handler in TDVF\n"
    L"usage: TestVe [-r] [-c] [-i] [-u]\n"
    L"  -r    Inject Read Msr VE.\n"
    L"  -c    Inject CPUID VE.\n"
    L"  -i    Inject IO_Instruction VE.\n"
    L"  -u    Inject unsupported VE, INVD.\n"
    L"  -a    Inject All VE above one by one.\n"
    L"\n"
    );
  return;
}

UINT64
TestMmioRead (
  UINT64  Address,
  UINT32  MmioSize,
  UINT64  *Val
  )
{
  UINT64  Value;

  Value = 0;

  if (MmioSize == 1) {
    Value = *(volatile UINT8 *)Address;
  } else if (MmioSize == 2) {
    Value = *(volatile UINT16 *)Address;
  } else if (MmioSize == 4) {
    Value = *(volatile UINT32 *)Address;
  } else if (MmioSize == 8) {
    Value = *(volatile UINT64 *)Address;
  } else {
    Print (L"Read of MmioSize (%d) is not supported!\n", MmioSize);
    ASSERT (FALSE);
  }

  *Val = Value;

  return 0;
}

UINT64
TestMmioWrite (
  UINT64  Address,
  UINT32  MmioSize,
  UINT64  *Val
  )
{
  if (MmioSize == 1) {
    *(volatile UINT8 *)Address = *(UINT8 *)Val;
  } else if (MmioSize == 2) {
    *(volatile UINT16 *)Address = *(UINT16 *)Val;
  } else if (MmioSize == 4) {
    *(volatile UINT32 *)Address = *(UINT32 *)Val;
  } else if (MmioSize == 8) {
    *(volatile UINT64 *)Address = *(UINT64 *)Val;
  } else {
    Print (L"Write of MmioSize (%d) is not supported!\n", MmioSize);
    ASSERT (FALSE);
  }

  return 0;
}

// EFI_STATUS
// FindWritableMemory (
//   UINT64 Address,
//   UINT32 Length
//   )
// {
//   UINT32 Val1, Val2;
//   UINT32 Index;

//   for (Index = 0; Index < Length / 4; Index++) {
//     Val1 = MmioRead32 (Address + Index * 4);
//     MmioWrite32 (Address + Index * 4, Val1+1);
//     Val2 = MmioRead32 (Address + Index * 4);
//     if (Val1 != Val2) {
//       break;
//     }
//   }

//   if (Index != Length >> 2) {
//     Print (L"Find writable Mem at %llx\n", Address + Index*4);
//   }

//   return 0;
// }

STATIC
VOID
InternalDumpData (
  UINT8   *Data,
  UINT32  Size
  )
{
  UINT32  Index;

  for (Index = 0; Index < Size; Index++) {
    Print (L"%02x ", Data[Index]);
  }

  Print (L"\n");
}

STATIC
EFI_STATUS
TestMmioReadWrite (
  UINT64  Address,
  UINT32  MmioSize,
  UINT64  TestValue
  )
{
  UINT64  Value;

  Value = 0;

  Print (L"%a: Test MMIO Read/Write %d bytes at Address of 0x%llx\n", __FUNCTION__, MmioSize, Address);

  TestMmioRead (Address, MmioSize, &Value);
  Print (L"%a: Read  %d bytes from Address = 0x%llx, Value: ", __FUNCTION__, MmioSize, Address);
  InternalDumpData ((UINT8 *)&Value, MmioSize);

  Print (L"%a: Write %d bytes to   Address = 0x%llx, Value: ", __FUNCTION__, MmioSize, Address);
  InternalDumpData ((UINT8 *)&TestValue, MmioSize);
  TestMmioWrite (Address, MmioSize, &TestValue);

  Value = 0;
  TestMmioRead (Address, MmioSize, &Value);
  Print (L"%a: Read  %d bytes from Address = 0x%llx, Value: ", __FUNCTION__, MmioSize, Address);
  InternalDumpData ((UINT8 *)&Value, MmioSize);

  Print (L"\n");

  return 0;
}

STATIC
EFI_STATUS
TestMmio2 (
  UINT64  Address
  )
{
  TestMmioReadWrite (Address, 1, 0x11);
  TestMmioReadWrite (Address, 2, 0x2233);
  TestMmioReadWrite (Address, 4, 0x44556677);
  TestMmioReadWrite (Address, 8, 0x8899aabbccddeeff);
  return 0;
}

STATIC
EFI_STATUS
TestMmio1 (
  VOID
  )
{
  Print (L"MMIO instruction will cause #VE and TDVF should handle,expected to see MMIO #VE handler log in Debug TDVF\n");

  Print (L"MOV reg/memX, regX \n");
  TestMmioWrite_8889_1 ();
  TestMmioWrite_8889_2 ();
  TestMmioWrite_8889_4 ();
  TestMmioWrite_8889_8 ();

  Print (L"MOV reg/memX, immX \n");
  TestMmioWrite_C6C7_1 ();
  TestMmioWrite_C6C7_2 ();
  TestMmioWrite_C6C7_4 ();
  TestMmioWrite_C6C7_8 ();

  Print (L"MOV regX, reg/memX \n");
  TestMmioRead_8A8B_1 ();
  TestMmioRead_8A8B_2 ();
  TestMmioRead_8A8B_4 ();
  TestMmioRead_8A8B_8 ();

  Print (L"MOVZX regX, reg/memX \n");
  TestMmioRead_B6B7_1 ();
  TestMmioRead_B6B7_2 ();
  TestMmioRead_B6B7_3 ();
  TestMmioRead_B6B7_4 ();
  TestMmioRead_B6B7_5 ();

  Print (L"MOVSX regX, reg/memX \n");
  TestMmioRead_BEBF_1 ();
  TestMmioRead_BEBF_2 ();
  TestMmioRead_BEBF_3 ();
  TestMmioRead_BEBF_4 ();
  TestMmioRead_BEBF_5 ();

  return EFI_SUCCESS;
}

/**
  UEFI application entry point which has an interface similar to a
  standard C main function.

  The ShellCEntryLib library instance wrappers the actual UEFI application
  entry point and calls this ShellAppMain function.

  @param[in] Argc     The number of items in Argv.
  @param[in] Argv     Array of pointers to strings.

  @retval  0               The application exited normally.
  @retval  Other           An error occurred.

**/
INTN
EFIAPI
ShellAppMain (
  IN UINTN   Argc,
  IN CHAR16  **Argv
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  *ParamPackage;

  //
  // Display header
  //
  DisplayHeader ();

  Status = ShellCommandLineParse (mParamList, &ParamPackage, NULL, TRUE);

  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Incorrect command line.\n");
    return Status;
  }

  if (ParamPackage == NULL) {
    mIsAll = TRUE;
  } else {
    if (ShellCommandLineGetFlag (ParamPackage, L"-?") ||
        ShellCommandLineGetFlag (ParamPackage, L"-h"))
    {
      PrintUsage ();
      return EFI_SUCCESS;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-r")) {
      mIsRdMsr = TRUE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-c")) {
      mIsCpuid = TRUE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-i")) {
      mIsMmio = TRUE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-u")) {
      mIsUnsupported = TRUE;
    }

    if (ShellCommandLineGetFlag (ParamPackage, L"-a")) {
      mIsAll = TRUE;
    }
  }

  if (mIsAll) {
    // Do all
    TestMmio1 ();
    TestMmio2 (0xc0000100);
  } else {
    if (mIsMmio) {
      TestMmio2 (0xc0000100);
    }
  }

  return 0;
}
