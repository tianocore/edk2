/** @file
  Unit tests for PCI BAR parsing.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Library/BaseMemoryLib.h>
  #include "../PciBus.h"
}

typedef struct {
  UINTN     Offset;
  UINT32    Value;
  UINT32    OriginalValue;
} BAR_PROBE_RESULT;

STATIC BAR_PROBE_RESULT   mProbeResults[PCI_MAX_BAR];
STATIC UINTN              mProbeResultCount;
STATIC UINTN              mProbeResultIndex;
STATIC BOOLEAN            mProbeActive;
STATIC EFI_BOOT_SERVICES  mBootServices;

extern "C" {
  UINT64             gAllOne = MAX_UINT64;
  EFI_BOOT_SERVICES  *gBS    = &mBootServices;

  EFI_TPL
  EFIAPI
  TestRaiseTpl (
    IN EFI_TPL  NewTpl
    )
  {
    EXPECT_EQ (NewTpl, (EFI_TPL)TPL_HIGH_LEVEL);
    return TPL_APPLICATION;
  }

  VOID
  EFIAPI
  TestRestoreTpl (
    IN EFI_TPL  OldTpl
    )
  {
    EXPECT_EQ (OldTpl, (EFI_TPL)TPL_APPLICATION);
  }

  EFI_STATUS
  EFIAPI
  TestPciRead (
    IN EFI_PCI_IO_PROTOCOL        *This,
    IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
    IN UINT32                     Offset,
    IN UINTN                      Count,
    IN OUT VOID                   *Buffer
    )
  {
    BAR_PROBE_RESULT  *Result;

    EXPECT_NE (This, nullptr);
    EXPECT_EQ (Width, EfiPciIoWidthUint32);
    EXPECT_EQ (Count, (UINTN)1);
    if (mProbeResultIndex >= mProbeResultCount) {
      ADD_FAILURE () << "Unexpected PCI configuration read";
      return EFI_DEVICE_ERROR;
    }

    Result = &mProbeResults[mProbeResultIndex];
    EXPECT_EQ (Offset, Result->Offset);
    *(UINT32 *)Buffer = mProbeActive ? Result->Value : Result->OriginalValue;
    return EFI_SUCCESS;
  }

  EFI_STATUS
  EFIAPI
  TestPciWrite (
    IN EFI_PCI_IO_PROTOCOL        *This,
    IN EFI_PCI_IO_PROTOCOL_WIDTH  Width,
    IN UINT32                     Offset,
    IN UINTN                      Count,
    IN OUT VOID                   *Buffer
    )
  {
    BAR_PROBE_RESULT  *Result;
    UINT32            Value;

    EXPECT_NE (This, nullptr);
    EXPECT_EQ (Width, EfiPciIoWidthUint32);
    EXPECT_EQ (Count, (UINTN)1);
    if (mProbeResultIndex >= mProbeResultCount) {
      ADD_FAILURE () << "Unexpected PCI configuration write";
      return EFI_DEVICE_ERROR;
    }

    Result = &mProbeResults[mProbeResultIndex];
    Value  = *(UINT32 *)Buffer;
    EXPECT_EQ (Offset, Result->Offset);
    if (!mProbeActive) {
      EXPECT_EQ (Value, MAX_UINT32);
      mProbeActive = TRUE;
    } else {
      EXPECT_EQ (Value, Result->OriginalValue);
      mProbeActive = FALSE;
      mProbeResultIndex++;
    }

    return EFI_SUCCESS;
  }
}

class PciParseBarTest : public ::testing::Test {
protected:
  PCI_IO_DEVICE PciIoDevice;

  void
  SetUp (
    ) override
  {
    ZeroMem (&PciIoDevice, sizeof (PciIoDevice));
    ZeroMem (mProbeResults, sizeof (mProbeResults));
    ZeroMem (&mBootServices, sizeof (mBootServices));
    mProbeResultCount = 0;
    mProbeResultIndex = 0;
    mProbeActive      = FALSE;

    mBootServices.RaiseTPL      = TestRaiseTpl;
    mBootServices.RestoreTPL    = TestRestoreTpl;
    PciIoDevice.PciIo.Pci.Read  = TestPciRead;
    PciIoDevice.PciIo.Pci.Write = TestPciWrite;
  }

  void
  AddProbeResult (
    UINTN   Offset,
    UINT32  Value,
    UINT32  OriginalValue
    )
  {
    ASSERT_LT (mProbeResultCount, ARRAY_SIZE (mProbeResults));
    mProbeResults[mProbeResultCount++] = { Offset, Value, OriginalValue };
  }

  void
  TearDown (
    ) override
  {
    EXPECT_FALSE (mProbeActive);
    EXPECT_EQ (mProbeResultIndex, mProbeResultCount);
  }
};

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ----------------
//   BAR0      0x10    0x00000000   0x00000000      Not implemented
//
// Verify that BAR0 is cleared and parsing advances to BAR1 at offset 0x14.
//
TEST_F (PciParseBarTest, RecordsAbsentBarAndContinuesScanning) {
  AddProbeResult (0x10, 0, 0);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x14);
  EXPECT_EQ (PciIoDevice.PciBar[0].Offset, 0x10);
  EXPECT_EQ (PciIoDevice.PciBar[0].BaseAddress, (UINT64)0);
  EXPECT_EQ (PciIoDevice.PciBar[0].Length, (UINT64)0);
  EXPECT_EQ (PciIoDevice.PciBar[0].Alignment, (UINT64)0);
}

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ------------------
//   BAR0      0x10    0x0000FFE1   0x0000C001      16-bit I/O BAR
//
// Bit 0 selects I/O space. The probe mask describes a 0x20-byte resource
// based at 0xC000 with 0x1F alignment.
//
TEST_F (PciParseBarTest, ParsesIo16Bar) {
  AddProbeResult (0x10, 0x0000FFE1, 0x0000C001);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x14);
  EXPECT_EQ (PciIoDevice.PciBar[0].BarType, PciBarTypeIo16);
  EXPECT_EQ (PciIoDevice.PciBar[0].BaseAddress, (UINT64)0xC000);
  EXPECT_EQ (PciIoDevice.PciBar[0].Length, (UINT64)0x20);
  EXPECT_EQ (PciIoDevice.PciBar[0].Alignment, (UINT64)0x1F);
}

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ------------------
//   BAR0      0x10    0xFFFFFF01   0x12345001      32-bit I/O BAR
//
// Bit 0 selects I/O space. Implemented address bits above bit 15 select
// I/O32, and the probe mask describes a 0x100-byte resource at 0x12345000.
//
TEST_F (PciParseBarTest, ParsesIo32Bar) {
  AddProbeResult (0x10, 0xFFFFFF01, 0x12345001);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x14);
  EXPECT_EQ (PciIoDevice.PciBar[0].BarType, PciBarTypeIo32);
  EXPECT_EQ (PciIoDevice.PciBar[0].BaseAddress, (UINT64)0x12345000);
  EXPECT_EQ (PciIoDevice.PciBar[0].Length, (UINT64)0x100);
  EXPECT_EQ (PciIoDevice.PciBar[0].Alignment, (UINT64)0xFF);
}

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ------------------
//   BAR0      0x10    0xFFFFE008   0x80001008      Prefetchable MEM32
//
// Bit 3 marks the BAR prefetchable and bits 2:1 select a 32-bit memory BAR.
// The probe mask describes a 0x2000-byte resource based at 0x80001000.
//
TEST_F (PciParseBarTest, ParsesPrefetchableMemory32Bar) {
  AddProbeResult (0x10, 0xFFFFE008, 0x80001008);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x14);
  EXPECT_EQ (PciIoDevice.PciBar[0].BarType, PciBarTypePMem32);
  EXPECT_EQ (PciIoDevice.PciBar[0].BaseAddress, (UINT64)0x80001000);
  EXPECT_EQ (PciIoDevice.PciBar[0].Length, (UINT64)0x2000);
  EXPECT_EQ (PciIoDevice.PciBar[0].Alignment, (UINT64)0x1FFF);
}

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ------------------
//   BAR0      0x10    0xFFFFF004   0x34567004      MEM64 low DWORD
//   BAR1      0x14    0xFFFFFFFF   0x00000012      MEM64 high DWORD
//
// Bits 2:1 in BAR0 select a 64-bit memory BAR. Verify that BAR0 and BAR1
// combine into base 0x0000001234567000 and a 0x1000-byte resource, and that
// parsing advances to BAR2 at offset 0x18.
//
TEST_F (PciParseBarTest, ParsesMemory64Bar) {
  AddProbeResult (0x10, 0xFFFFF004, 0x34567004);
  AddProbeResult (0x14, 0xFFFFFFFF, 0x00000012);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x18);
  EXPECT_EQ (PciIoDevice.PciBar[0].BarType, PciBarTypeMem64);
  EXPECT_EQ (PciIoDevice.PciBar[0].BaseAddress, 0x0000001234567000ULL);
  EXPECT_EQ (PciIoDevice.PciBar[0].Length, (UINT64)0x1000);
  EXPECT_EQ (PciIoDevice.PciBar[0].Alignment, (UINT64)0xFFF);
}

//
// PCI configuration:
//
//   Register  Offset  Probe value  Original value  Interpretation
//   --------  ------  -----------  --------------  ------------------
//   BAR0      0x10    0xFFFFF004   0x34567004      MEM64 low DWORD
//   BAR1      0x14    0xFFFFFFFF   0x00000012      MEM64 high DWORD
//   BAR2      0x18    0xFFFFF000   0x80000000      MEM32
//
// BAR0 and BAR1 form one 64-bit resource. Verify that the independent
// resource at offset 0x18 is recorded as PciBar[2], leaving PciBar[1] empty
// because BAR1 is the upper DWORD rather than a separate resource.
//
TEST_F (PciParseBarTest, PreservesBarNumberAfterMemory64Bar) {
  AddProbeResult (0x10, 0xFFFFF004, 0x34567004);
  AddProbeResult (0x14, 0xFFFFFFFF, 0x00000012);
  AddProbeResult (0x18, 0xFFFFF000, 0x80000000);

  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x10), (UINTN)0x18);
  EXPECT_EQ (PciParseBar (&PciIoDevice, 0x18), (UINTN)0x1C);

  EXPECT_EQ (PciIoDevice.PciBar[1].BarType, PciBarTypeUnknown);
  EXPECT_EQ (PciIoDevice.PciBar[1].Length, (UINT64)0);
  EXPECT_EQ (PciIoDevice.PciBar[2].Offset, 0x18);
  EXPECT_EQ (PciIoDevice.PciBar[2].BarType, PciBarTypeMem32);
  EXPECT_EQ (PciIoDevice.PciBar[2].BaseAddress, (UINT64)0x80000000);
  EXPECT_EQ (PciIoDevice.PciBar[2].Length, (UINT64)0x1000);
}

class PciIovParseVfBarTest : public PciParseBarTest {
protected:
  void
  SetUp (
    ) override
  {
    PciParseBarTest::SetUp ();
    PciIoDevice.SrIovCapabilityOffset = 0x100;
    PciIoDevice.InitialVFs            = 4;
    PciIoDevice.SystemPageSize        = SIZE_4KB;
  }
};

//
// SR-IOV capability starts at PCI configuration offset 0x100:
//
//   VF BAR   Relative  Absolute  Probe value  Original value
//   -------  --------  --------  -----------  --------------
//   VF BAR0  0x24      0x124     0x00000000   0x00000000
//
// A zero probe value means VF BAR0 is not implemented. Verify that its entry
// is cleared and parsing advances to VF BAR1 at absolute offset 0x128.
//
TEST_F (PciIovParseVfBarTest, RecordsAbsentVfBarAndContinuesScanning) {
  AddProbeResult (0x124, 0, 0);

  EXPECT_EQ (PciIovParseVfBar (&PciIoDevice, 0x124), (UINTN)0x128);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Offset, 0x124);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].BaseAddress, (UINT64)0);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Length, (UINT64)0);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Alignment, (UINT64)0);
}

//
// SR-IOV capability starts at PCI configuration offset 0x100:
//
//   VF BAR   Relative  Absolute  Probe value  Original value
//   -------  --------  --------  -----------  --------------
//   VF BAR0  0x24      0x124     0xFFFFF000   0x80000000
//
// VF BAR0 is a 0x1000-byte MEM32 BAR. With InitialVFs set to 4, verify that
// the aggregate resource length is 0x4000 and that its alignment is 0xFFF
// for the selected 4-KB SR-IOV system page size.
//
TEST_F (PciIovParseVfBarTest, ParsesMemory32VfBar) {
  AddProbeResult (0x124, 0xFFFFF000, 0x80000000);

  EXPECT_EQ (PciIovParseVfBar (&PciIoDevice, 0x124), (UINTN)0x128);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Offset, 0x124);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].BarType, PciBarTypeMem32);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].BaseAddress, (UINT64)0x80000000);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Length, (UINT64)0x4000);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Alignment, (UINT64)0xFFF);
}

//
// SR-IOV capability starts at PCI configuration offset 0x100:
//
//   VF BAR   Relative  Absolute  Probe value  Original value
//   -------  --------  --------  -----------  --------------
//   VF BAR0  0x24      0x124     0xFFFFF004   0x34567004
//   VF BAR1  0x28      0x128     0xFFFFFFFF   0x00000012
//
// Bits 2:1 in VF BAR0 select a 64-bit memory BAR. Verify that both DWORDs
// combine into base 0x0000001234567000, that the per-VF 0x1000-byte size is
// scaled to 0x4000 for four VFs, and that parsing advances to VF BAR2.
//
TEST_F (PciIovParseVfBarTest, ParsesMemory64VfBar) {
  AddProbeResult (0x124, 0xFFFFF004, 0x34567004);
  AddProbeResult (0x128, 0xFFFFFFFF, 0x00000012);

  EXPECT_EQ (PciIovParseVfBar (&PciIoDevice, 0x124), (UINTN)0x12C);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Offset, 0x124);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].BarType, PciBarTypeMem64);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].BaseAddress, 0x0000001234567000ULL);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Length, (UINT64)0x4000);
  EXPECT_EQ (PciIoDevice.VfPciBar[0].Alignment, (UINT64)0xFFF);
}

//
// SR-IOV capability starts at PCI configuration offset 0x100:
//
//   VF BAR   Relative  Absolute  Probe value  Original value
//   -------  --------  --------  -----------  --------------
//   VF BAR0  0x24      0x124     0xFFFFF004   0x34567004
//   VF BAR1  0x28      0x128     0xFFFFFFFF   0x00000012
//   VF BAR2  0x2C      0x12C     0xFFFFF000   0x80000000
//
// VF BAR0 and VF BAR1 form one 64-bit resource. Verify that the independent
// MEM32 resource at relative offset 0x2C is recorded as VfPciBar[2], leaving
// VfPciBar[1] empty because VF BAR1 is the upper DWORD.
//
TEST_F (PciIovParseVfBarTest, PreservesVfBarNumberAfterMemory64Bar) {
  AddProbeResult (0x124, 0xFFFFF004, 0x34567004);
  AddProbeResult (0x128, 0xFFFFFFFF, 0x00000012);
  AddProbeResult (0x12C, 0xFFFFF000, 0x80000000);

  EXPECT_EQ (PciIovParseVfBar (&PciIoDevice, 0x124), (UINTN)0x12C);
  EXPECT_EQ (PciIovParseVfBar (&PciIoDevice, 0x12C), (UINTN)0x130);

  EXPECT_EQ (PciIoDevice.VfPciBar[1].BarType, PciBarTypeUnknown);
  EXPECT_EQ (PciIoDevice.VfPciBar[1].Length, (UINT64)0);
  EXPECT_EQ (PciIoDevice.VfPciBar[2].Offset, 0x12C);
  EXPECT_EQ (PciIoDevice.VfPciBar[2].BarType, PciBarTypeMem32);
  EXPECT_EQ (PciIoDevice.VfPciBar[2].BaseAddress, (UINT64)0x80000000);
  EXPECT_EQ (PciIoDevice.VfPciBar[2].Length, (UINT64)0x4000);
}

int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
