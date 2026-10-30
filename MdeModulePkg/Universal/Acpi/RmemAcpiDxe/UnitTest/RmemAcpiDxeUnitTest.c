/** @file  // [CODE_FIRST] 13253
  Host-based unit tests for the RMEM ACPI publisher.  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Copyright (c) Microsoft Corporation.  // [CODE_FIRST] 13253
  SPDX-License-Identifier: BSD-2-Clause-Patent  // [CODE_FIRST] 13253
**/  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Uefi.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include <Library/BaseMemoryLib.h>  // [CODE_FIRST] 13253
#include <Library/DebugLib.h>  // [CODE_FIRST] 13253
#include <Library/UnitTestLib.h>  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#include "../RmemAcpiDxe.c"  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
#define UNIT_TEST_APP_NAME     "RMEM ACPI DXE Unit Tests"  // [CODE_FIRST] 13253
#define UNIT_TEST_APP_VERSION  "1.0"  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
EFI_BOOT_SERVICES               MockBoot;  // [CODE_FIRST] 13253
EFI_BOOT_SERVICES               *gBS = &MockBoot;  // [CODE_FIRST] 13253
STATIC EFI_ACPI_TABLE_PROTOCOL  mMockAcpiTableProtocol;  // [CODE_FIRST] 13253
STATIC EFI_STATUS               mLocateProtocolStatus;  // [CODE_FIRST] 13253
STATIC EFI_STATUS               mInstallAcpiTableStatus;  // [CODE_FIRST] 13253
STATIC EFI_STATUS               mInstallProtocolStatus;  // [CODE_FIRST] 13253
STATIC EFI_STATUS               mCreateEventStatus;  // [CODE_FIRST] 13253
STATIC UINTN                    mCloseEventCalls;  // [CODE_FIRST] 13253
STATIC UINTN                    mUninstallProtocolCalls;  // [CODE_FIRST] 13253
STATIC UINTN                    mInstalledTableSize;  // [CODE_FIRST] 13253
STATIC UINT8                    mInstalledTable[sizeof (RMEM_TABLE_HEADER) + sizeof (RMEM_ENTRY)];  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
VOID *  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
GetFirstHob (  // [CODE_FIRST] 13253
  IN UINT16  Type  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  return NULL;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
VOID *  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
GetFirstGuidHob (  // [CODE_FIRST] 13253
  IN CONST EFI_GUID  *Guid  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  return NULL;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
VOID *  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
GetNextGuidHob (  // [CODE_FIRST] 13253
  IN CONST EFI_GUID  *Guid,  // [CODE_FIRST] 13253
  IN CONST VOID      *HobStart  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  return NULL;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockInstallAcpiTable (  // [CODE_FIRST] 13253
  IN  EFI_ACPI_TABLE_PROTOCOL  *This,  // [CODE_FIRST] 13253
  IN  VOID                     *AcpiTableBuffer,  // [CODE_FIRST] 13253
  IN  UINTN                    AcpiTableBufferSize,  // [CODE_FIRST] 13253
  OUT UINTN                    *TableKey  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  if (EFI_ERROR (mInstallAcpiTableStatus)) {  // [CODE_FIRST] 13253
    return mInstallAcpiTableStatus;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  if (AcpiTableBufferSize > sizeof (mInstalledTable)) {  // [CODE_FIRST] 13253
    return EFI_BAD_BUFFER_SIZE;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  CopyMem (mInstalledTable, AcpiTableBuffer, AcpiTableBufferSize);  // [CODE_FIRST] 13253
  mInstalledTableSize = AcpiTableBufferSize;  // [CODE_FIRST] 13253
  *TableKey           = 1;  // [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockLocateProtocol (  // [CODE_FIRST] 13253
  IN  EFI_GUID  *Protocol,  // [CODE_FIRST] 13253
  IN  VOID      *Registration OPTIONAL,  // [CODE_FIRST] 13253
  OUT VOID      **Interface  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  if (!EFI_ERROR (mLocateProtocolStatus)) {  // [CODE_FIRST] 13253
    *Interface = &mMockAcpiTableProtocol;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return mLocateProtocolStatus;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockInstallProtocolInterface (  // [CODE_FIRST] 13253
  IN OUT EFI_HANDLE          *Handle,  // [CODE_FIRST] 13253
  IN     EFI_GUID            *Protocol,  // [CODE_FIRST] 13253
  IN     EFI_INTERFACE_TYPE  InterfaceType,  // [CODE_FIRST] 13253
  IN     VOID                *Interface  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  if (!EFI_ERROR (mInstallProtocolStatus)) {  // [CODE_FIRST] 13253
    *Handle = (EFI_HANDLE)(UINTN)1;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return mInstallProtocolStatus;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockUninstallProtocolInterface (  // [CODE_FIRST] 13253
  IN EFI_HANDLE  Handle,  // [CODE_FIRST] 13253
  IN EFI_GUID    *Protocol,  // [CODE_FIRST] 13253
  IN VOID        *Interface  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  mUninstallProtocolCalls++;  // [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockCreateEventEx (  // [CODE_FIRST] 13253
  IN       UINT32            Type,  // [CODE_FIRST] 13253
  IN       EFI_TPL           NotifyTpl,  // [CODE_FIRST] 13253
  IN       EFI_EVENT_NOTIFY  NotifyFunction OPTIONAL,  // [CODE_FIRST] 13253
  IN CONST VOID              *NotifyContext OPTIONAL,  // [CODE_FIRST] 13253
  IN CONST EFI_GUID          *EventGroup OPTIONAL,  // [CODE_FIRST] 13253
  OUT      EFI_EVENT         *Event  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  if (!EFI_ERROR (mCreateEventStatus)) {  // [CODE_FIRST] 13253
    *Event = (EFI_EVENT)(UINTN)2;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return mCreateEventStatus;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MockCloseEvent (  // [CODE_FIRST] 13253
  IN EFI_EVENT  Event  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  mCloseEventCalls++;  // [CODE_FIRST] 13253
  return EFI_SUCCESS;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
ResetRmemState (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  ZeroMem (mEntries, sizeof (mEntries));  // [CODE_FIRST] 13253
  mEntryCount             = 0;  // [CODE_FIRST] 13253
  mFinalized              = FALSE;  // [CODE_FIRST] 13253
  mPublicationEvent       = NULL;  // [CODE_FIRST] 13253
  mMaximumPhysicalAddress = MAX_UINT64;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  ZeroMem (&MockBoot, sizeof (MockBoot));  // [CODE_FIRST] 13253
  MockBoot.LocateProtocol             = MockLocateProtocol;  // [CODE_FIRST] 13253
  MockBoot.InstallProtocolInterface   = MockInstallProtocolInterface;  // [CODE_FIRST] 13253
  MockBoot.UninstallProtocolInterface = MockUninstallProtocolInterface;  // [CODE_FIRST] 13253
  MockBoot.CreateEventEx              = MockCreateEventEx;  // [CODE_FIRST] 13253
  MockBoot.CloseEvent                 = MockCloseEvent;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mMockAcpiTableProtocol.InstallAcpiTable = MockInstallAcpiTable;  // [CODE_FIRST] 13253
  mLocateProtocolStatus                   = EFI_SUCCESS;  // [CODE_FIRST] 13253
  mInstallAcpiTableStatus                 = EFI_SUCCESS;  // [CODE_FIRST] 13253
  mInstallProtocolStatus                  = EFI_SUCCESS;  // [CODE_FIRST] 13253
  mCreateEventStatus                      = EFI_SUCCESS;  // [CODE_FIRST] 13253
  mCloseEventCalls                        = 0;  // [CODE_FIRST] 13253
  mUninstallProtocolCalls                 = 0;  // [CODE_FIRST] 13253
  mInstalledTableSize                     = 0;  // [CODE_FIRST] 13253
  ZeroMem (mInstalledTable, sizeof (mInstalledTable));  // [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
ValidRangesAreRegistered (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 1);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntries[0].Base, 0x1000);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntries[0].Size, 0x1000);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntries[0].Category, RmemCategorySecurity);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (AsciiStrCmp (mEntries[0].Label, "Secure"), 0);  // [CODE_FIRST] 13253
  UT_ASSERT_TRUE (  // [CODE_FIRST] 13253
    IsZeroBuffer (  // [CODE_FIRST] 13253
      &mEntries[0].Label[sizeof ("Secure")],  // [CODE_FIRST] 13253
      sizeof (mEntries[0].Label) - sizeof ("Secure")  // [CODE_FIRST] 13253
      )  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryFirmwareRuntime,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 2);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntries[1].Label[0], '\0');  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
DuplicateRangesAreRejected (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 1);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
OverlappingRangesAreRejected (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Overlap"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 1);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
InvalidParametersAreRejected (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EDKII_RMEM_REGISTRATION_PROTOCOL  InvalidProtocol;  // [CODE_FIRST] 13253
  EFI_STATUS                        Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             NULL,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  InvalidProtocol = mRmemProtocol;  // [CODE_FIRST] 13253
  InvalidProtocol.Revision++;  // [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &InvalidProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
InvalidRangesAndCategoriesAreRejected (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0xFFFFFFFFFFFFF000,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1001,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1001,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mMaximumPhysicalAddress = 0x3FFF;  // [CODE_FIRST] 13253
  Status                  = RmemAddReservedRange (  // [CODE_FIRST] 13253
                              &mRmemProtocol,  // [CODE_FIRST] 13253
                              0x3000,  // [CODE_FIRST] 13253
                              0x2000,  // [CODE_FIRST] 13253
                              RmemCategorySecurity,  // [CODE_FIRST] 13253
                              0,  // [CODE_FIRST] 13253
                              NULL  // [CODE_FIRST] 13253
                              );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryUnknown,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryMax,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             BIT15,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
MaximumLengthLabelsAreAccepted (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  CHAR8       Label[RMEM_LABEL_MAX_LEN];  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  SetMem (Label, RMEM_LABEL_MAX_LEN - 1, 'A');  // [CODE_FIRST] 13253
  Label[RMEM_LABEL_MAX_LEN - 1] = '\0';  // [CODE_FIRST] 13253
  Status                        = RmemAddReservedRange (  // [CODE_FIRST] 13253
                                    &mRmemProtocol,  // [CODE_FIRST] 13253
                                    0x1000,  // [CODE_FIRST] 13253
                                    0x1000,  // [CODE_FIRST] 13253
                                    RmemCategoryOther,  // [CODE_FIRST] 13253
                                    0,  // [CODE_FIRST] 13253
                                    Label  // [CODE_FIRST] 13253
                                    );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (AsciiStrCmp (mEntries[0].Label, Label), 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
OversizedLabelsAreRejected (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  CHAR8       Label[RMEM_LABEL_MAX_LEN + 1];  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  SetMem (Label, RMEM_LABEL_MAX_LEN, 'A');  // [CODE_FIRST] 13253
  Label[RMEM_LABEL_MAX_LEN] = '\0';  // [CODE_FIRST] 13253
  Status                    = RmemAddReservedRange (  // [CODE_FIRST] 13253
                                &mRmemProtocol,  // [CODE_FIRST] 13253
                                0x1000,  // [CODE_FIRST] 13253
                                0x1000,  // [CODE_FIRST] 13253
                                RmemCategoryOther,  // [CODE_FIRST] 13253
                                0,  // [CODE_FIRST] 13253
                                Label  // [CODE_FIRST] 13253
                                );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_BAD_BUFFER_SIZE);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
CapacityIsEnforced (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
  UINT32      Index;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  for (Index = 0; Index < RMEM_MAX_ENTRIES; Index++) {  // [CODE_FIRST] 13253
    Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
               &mRmemProtocol,  // [CODE_FIRST] 13253
               (UINT64)Index * 0x1000,  // [CODE_FIRST] 13253
               0x1000,  // [CODE_FIRST] 13253
               RmemCategoryOther,  // [CODE_FIRST] 13253
               0,  // [CODE_FIRST] 13253
               NULL  // [CODE_FIRST] 13253
               );  // [CODE_FIRST] 13253
    UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             (UINT64)RMEM_MAX_ENTRIES * 0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, RMEM_MAX_ENTRIES);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
FinalizationPreventsRegistration (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mFinalized = TRUE;  // [CODE_FIRST] 13253
  Status     = RmemAddReservedRange (  // [CODE_FIRST] 13253
                 &mRmemProtocol,  // [CODE_FIRST] 13253
                 0x1000,  // [CODE_FIRST] 13253
                 0x1000,  // [CODE_FIRST] 13253
                 RmemCategoryOther,  // [CODE_FIRST] 13253
                 0,  // [CODE_FIRST] 13253
                 NULL  // [CODE_FIRST] 13253
                 );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntryCount, 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
TableIsSerializedAndInstalled (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  RMEM_ENTRY         *Entry;  // [CODE_FIRST] 13253
  RMEM_TABLE_HEADER  *Table;  // [CODE_FIRST] 13253
  EFI_STATUS         Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mInstalledTableSize, sizeof (mInstalledTable));  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Table = (RMEM_TABLE_HEADER *)mInstalledTable;  // [CODE_FIRST] 13253
  Entry = (RMEM_ENTRY *)(mInstalledTable + Table->EntryOffset);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Table->Header.Signature, RMEM_TABLE_SIGNATURE);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Table->Header.Length, sizeof (mInstalledTable));  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Table->Header.Revision, RMEM_TABLE_REVISION);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Table->EntryCount, 1);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Table->EntryOffset, sizeof (RMEM_TABLE_HEADER));  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (CalculateSum8 (mInstalledTable, mInstalledTableSize), 0);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Entry->Base, 0x1000);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Entry->Size, 0x2000);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Entry->Category, RmemCategorySecurity);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (AsciiStrCmp (Entry->Label, "Secure"), 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
HiddenAddressesAreRedacted (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  RMEM_ENTRY  *Entry;  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             RMEM_ENTRY_FLAG_ADDRESS_HIDDEN,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mEntries[0].Base, 0x1000);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  Entry = (RMEM_ENTRY *)(  // [CODE_FIRST] 13253
                         mInstalledTable +  // [CODE_FIRST] 13253
                         ((RMEM_TABLE_HEADER *)mInstalledTable)->EntryOffset  // [CODE_FIRST] 13253
                         );  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Entry->Base, 0);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (Entry->Flags, RMEM_ENTRY_FLAG_ADDRESS_HIDDEN);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
HiddenRangesStillRejectOverlap (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             RmemCategorySecurity,  // [CODE_FIRST] 13253
             RMEM_ENTRY_FLAG_ADDRESS_HIDDEN,  // [CODE_FIRST] 13253
             "Secure"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             "Overlap"  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
PublicationFailuresAreReturned (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mLocateProtocolStatus = EFI_NOT_FOUND;  // [CODE_FIRST] 13253
  Status                = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mLocateProtocolStatus   = EFI_SUCCESS;  // [CODE_FIRST] 13253
  mInstallAcpiTableStatus = EFI_ACCESS_DENIED;  // [CODE_FIRST] 13253
  Status                  = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_ACCESS_DENIED);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
RejectedRegistrationDoesNotBlockPublication (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x1001,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_INVALID_PARAMETER);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAddReservedRange (  // [CODE_FIRST] 13253
             &mRmemProtocol,  // [CODE_FIRST] 13253
             0x2000,  // [CODE_FIRST] 13253
             0x1000,  // [CODE_FIRST] 13253
             RmemCategoryOther,  // [CODE_FIRST] 13253
             0,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemPublishTable ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
PublicationEventFinalizesRegistration (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_EVENT  Event;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Event = (EFI_EVENT)(UINTN)3;  // [CODE_FIRST] 13253
  RmemOnPublicationEvent (Event, NULL);  // [CODE_FIRST] 13253
  UT_ASSERT_TRUE (mFinalized);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mCloseEventCalls, 1);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mPublicationEvent, NULL);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
ProtocolAndEventAreInstalled (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemInstallProtocolAndEvent ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mPublicationEvent, (EFI_EVENT)(UINTN)2);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
ProtocolIsRemovedAfterEventFailure (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mCreateEventStatus = EFI_OUT_OF_RESOURCES;  // [CODE_FIRST] 13253
  Status             = RmemInstallProtocolAndEvent ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 1);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
ProtocolInstallFailureIsReturned (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  mInstallProtocolStatus = EFI_OUT_OF_RESOURCES;  // [CODE_FIRST] 13253
  Status                 = RmemInstallProtocolAndEvent ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_OUT_OF_RESOURCES);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mUninstallProtocolCalls, 0);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
EntryPointReturnsAddressInitializationFailure (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemAcpiDxeEntryPoint (NULL, NULL);  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_NOT_FOUND);  // [CODE_FIRST] 13253
  UT_ASSERT_EQUAL (mPublicationEvent, NULL);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
UNIT_TEST_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
HobImportWithoutRecordsSucceeds (  // [CODE_FIRST] 13253
  IN UNIT_TEST_CONTEXT  Context  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS  Status;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RmemImportHobs ();  // [CODE_FIRST] 13253
  UT_ASSERT_STATUS_EQUAL (Status, EFI_SUCCESS);  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  return UNIT_TEST_PASSED;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
STATIC  // [CODE_FIRST] 13253
EFI_STATUS  // [CODE_FIRST] 13253
EFIAPI  // [CODE_FIRST] 13253
UnitTestingEntry (  // [CODE_FIRST] 13253
  VOID  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  EFI_STATUS                  Status;  // [CODE_FIRST] 13253
  UNIT_TEST_FRAMEWORK_HANDLE  Framework;  // [CODE_FIRST] 13253
  UNIT_TEST_SUITE_HANDLE      RegistrationTests;  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Framework = NULL;  // [CODE_FIRST] 13253
  Status    = InitUnitTestFramework (  // [CODE_FIRST] 13253
                &Framework,  // [CODE_FIRST] 13253
                UNIT_TEST_APP_NAME,  // [CODE_FIRST] 13253
                gEfiCallerBaseName,  // [CODE_FIRST] 13253
                UNIT_TEST_APP_VERSION  // [CODE_FIRST] 13253
                );  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    return Status;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = CreateUnitTestSuite (  // [CODE_FIRST] 13253
             &RegistrationTests,  // [CODE_FIRST] 13253
             Framework,  // [CODE_FIRST] 13253
             "RMEM registration tests",  // [CODE_FIRST] 13253
             "RmemAcpiDxe.Registration",  // [CODE_FIRST] 13253
             NULL,  // [CODE_FIRST] 13253
             NULL  // [CODE_FIRST] 13253
             );  // [CODE_FIRST] 13253
  if (EFI_ERROR (Status)) {  // [CODE_FIRST] 13253
    FreeUnitTestFramework (Framework);  // [CODE_FIRST] 13253
    return EFI_OUT_OF_RESOURCES;  // [CODE_FIRST] 13253
  }  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Valid ranges are registered",  // [CODE_FIRST] 13253
    "Valid",  // [CODE_FIRST] 13253
    ValidRangesAreRegistered,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Duplicate ranges are rejected",  // [CODE_FIRST] 13253
    "Duplicate",  // [CODE_FIRST] 13253
    DuplicateRangesAreRejected,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Overlapping ranges are rejected",  // [CODE_FIRST] 13253
    "Overlap",  // [CODE_FIRST] 13253
    OverlappingRangesAreRejected,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Invalid parameters are rejected",  // [CODE_FIRST] 13253
    "Parameters",  // [CODE_FIRST] 13253
    InvalidParametersAreRejected,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Invalid ranges and categories are rejected",  // [CODE_FIRST] 13253
    "Validation",  // [CODE_FIRST] 13253
    InvalidRangesAndCategoriesAreRejected,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Maximum-length labels are accepted",  // [CODE_FIRST] 13253
    "LabelBoundary",  // [CODE_FIRST] 13253
    MaximumLengthLabelsAreAccepted,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Oversized labels are rejected",  // [CODE_FIRST] 13253
    "Label",  // [CODE_FIRST] 13253
    OversizedLabelsAreRejected,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Registration capacity is enforced",  // [CODE_FIRST] 13253
    "Capacity",  // [CODE_FIRST] 13253
    CapacityIsEnforced,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Finalization prevents registration",  // [CODE_FIRST] 13253
    "Finalized",  // [CODE_FIRST] 13253
    FinalizationPreventsRegistration,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Table is serialized and installed",  // [CODE_FIRST] 13253
    "Publish",  // [CODE_FIRST] 13253
    TableIsSerializedAndInstalled,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Hidden addresses are redacted during serialization",  // [CODE_FIRST] 13253
    "HiddenAddress",  // [CODE_FIRST] 13253
    HiddenAddressesAreRedacted,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Hidden ranges still reject overlap",  // [CODE_FIRST] 13253
    "HiddenOverlap",  // [CODE_FIRST] 13253
    HiddenRangesStillRejectOverlap,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Publication failures are returned",  // [CODE_FIRST] 13253
    "PublishFailure",  // [CODE_FIRST] 13253
    PublicationFailuresAreReturned,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Rejected registration does not block publication",  // [CODE_FIRST] 13253
    "RejectedRegistration",  // [CODE_FIRST] 13253
    RejectedRegistrationDoesNotBlockPublication,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Publication event finalizes registration",  // [CODE_FIRST] 13253
    "Finalize",  // [CODE_FIRST] 13253
    PublicationEventFinalizesRegistration,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Protocol and event are installed",  // [CODE_FIRST] 13253
    "ProtocolAndEvent",  // [CODE_FIRST] 13253
    ProtocolAndEventAreInstalled,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Protocol is removed after event failure",  // [CODE_FIRST] 13253
    "ProtocolCleanup",  // [CODE_FIRST] 13253
    ProtocolIsRemovedAfterEventFailure,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Protocol install failure is returned",  // [CODE_FIRST] 13253
    "ProtocolFailure",  // [CODE_FIRST] 13253
    ProtocolInstallFailureIsReturned,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "Entry point returns address initialization failure",  // [CODE_FIRST] 13253
    "EntryPointInitialization",  // [CODE_FIRST] 13253
    EntryPointReturnsAddressInitializationFailure,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
  AddTestCase (  // [CODE_FIRST] 13253
    RegistrationTests,  // [CODE_FIRST] 13253
    "HOB import without records succeeds",  // [CODE_FIRST] 13253
    "HobImport",  // [CODE_FIRST] 13253
    HobImportWithoutRecordsSucceeds,  // [CODE_FIRST] 13253
    ResetRmemState,  // [CODE_FIRST] 13253
    NULL,  // [CODE_FIRST] 13253
    NULL  // [CODE_FIRST] 13253
    );  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
  Status = RunAllTestSuites (Framework);  // [CODE_FIRST] 13253
  FreeUnitTestFramework (Framework);  // [CODE_FIRST] 13253
  return Status;  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
// [CODE_FIRST] 13253
int  // [CODE_FIRST] 13253
main (  // [CODE_FIRST] 13253
  int   argc,  // [CODE_FIRST] 13253
  char  *argv[]  // [CODE_FIRST] 13253
  )  // [CODE_FIRST] 13253
{  // [CODE_FIRST] 13253
  return UnitTestingEntry ();  // [CODE_FIRST] 13253
}  // [CODE_FIRST] 13253
