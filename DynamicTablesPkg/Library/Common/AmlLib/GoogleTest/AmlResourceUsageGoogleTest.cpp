/** @file
  Verify the Consumer/Producer bit emitted by AML address-space code generators.

  Copyright (c) 2026, Jin Kang. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <gtest/gtest.h>
#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/AmlLib/AmlLib.h>
  #include <Library/MemoryAllocationLib.h>
}

enum class ResourceKind {
  WordIo, DWordMemory, QWordMemory
};

static void
CheckResourceUsage (
  ResourceKind  Kind,
  BOOLEAN       Consumer
  )
{
  AML_ROOT_NODE_HANDLE         Root   = nullptr;
  AML_OBJECT_NODE_HANDLE       Device = nullptr;
  AML_OBJECT_NODE_HANDLE       Crs    = nullptr;
  EFI_ACPI_DESCRIPTION_HEADER  *Table = nullptr;
  EFI_STATUS                   Status;

  Status = AmlCodeGenDefinitionBlock ("SSDT", "TESTID", "AMLUSAGE", 1, &Root);
  ASSERT_EQ (Status, EFI_SUCCESS);
  Status = AmlCodeGenDevice ("DEV0", Root, &Device);
  ASSERT_EQ (Status, EFI_SUCCESS);
  Status = AmlCodeGenNameResourceTemplate ("_CRS", Device, &Crs);
  ASSERT_EQ (Status, EFI_SUCCESS);

  UINT8   Tag;
  UINT16  DescriptorLength;
  UINT8   ResourceType;

  switch (Kind) {
    case ResourceKind::WordIo:
      Tag              = 0x88;
      DescriptorLength = 0x0D;
      ResourceType     = 1;
      Status           = AmlCodeGenRdWordIo (
                           Consumer,
                           TRUE,
                           TRUE,
                           TRUE,
                           3,
                           0,
                           0x2000,
                           0x20FF,
                           0,
                           0x100,
                           0,
                           nullptr,
                           TRUE,
                           TRUE,
                           Crs,
                           nullptr
                           );
      break;
    case ResourceKind::DWordMemory:
      Tag              = 0x87;
      DescriptorLength = 0x17;
      ResourceType     = 0;
      Status           = AmlCodeGenRdDWordMemory (
                           Consumer,
                           TRUE,
                           TRUE,
                           TRUE,
                           AmlMemoryNonCacheable,
                           TRUE,
                           0,
                           0x10000000,
                           0x10000FFF,
                           0,
                           0x1000,
                           0,
                           nullptr,
                           AmlAddressRangeMemory,
                           TRUE,
                           Crs,
                           nullptr
                           );
      break;
    case ResourceKind::QWordMemory:
      Tag              = 0x8A;
      DescriptorLength = 0x2B;
      ResourceType     = 0;
      Status           = AmlCodeGenRdQWordMemory (
                           Consumer,
                           TRUE,
                           TRUE,
                           TRUE,
                           AmlMemoryNonCacheable,
                           TRUE,
                           0,
                           0x100000000ULL,
                           0x100000FFFULL,
                           0,
                           0x1000,
                           0,
                           nullptr,
                           AmlAddressRangeMemory,
                           TRUE,
                           Crs,
                           nullptr
                           );
      break;
  }

  EXPECT_EQ (Status, EFI_SUCCESS);
  if (!EFI_ERROR (Status)) {
    Status = AmlSerializeDefinitionBlock (Root, &Table);
    EXPECT_EQ (Status, EFI_SUCCESS);
  }

  if (Table != nullptr) {
    CONST UINT8  *Bytes = reinterpret_cast<CONST UINT8 *> (Table);
    BOOLEAN      Found  = FALSE;
    for (UINTN Offset = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
         Offset + 5 <= Table->Length; Offset++)
    {
      if ((Bytes[Offset] == Tag) &&
          (Bytes[Offset + 1] == (DescriptorLength & MAX_UINT8)) &&
          (Bytes[Offset + 2] == (DescriptorLength >> 8)) &&
          (Bytes[Offset + 3] == ResourceType))
      {
        Found = TRUE;
        EXPECT_EQ ((Bytes[Offset + 4] & BIT0) != 0, Consumer != FALSE);
        break;
      }
    }

    EXPECT_TRUE (Found);
    FreePool (Table);
  }

  EXPECT_EQ (AmlDeleteTree (Root), EFI_SUCCESS);
}

TEST (AmlResourceUsage, WordIoConsumer) {
  CheckResourceUsage (ResourceKind::WordIo, TRUE);
}

TEST (AmlResourceUsage, WordIoProducer) {
  CheckResourceUsage (ResourceKind::WordIo, FALSE);
}

TEST (AmlResourceUsage, DWordMemoryConsumer) {
  CheckResourceUsage (ResourceKind::DWordMemory, TRUE);
}

TEST (AmlResourceUsage, DWordMemoryProducer) {
  CheckResourceUsage (ResourceKind::DWordMemory, FALSE);
}

TEST (AmlResourceUsage, QWordMemoryConsumer) {
  CheckResourceUsage (ResourceKind::QWordMemory, TRUE);
}

TEST (AmlResourceUsage, QWordMemoryProducer) {
  CheckResourceUsage (ResourceKind::QWordMemory, FALSE);
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
