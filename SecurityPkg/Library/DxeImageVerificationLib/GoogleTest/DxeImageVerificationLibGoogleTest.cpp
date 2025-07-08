/** @file
  Unit tests for the implementation of DxeImageVerificationLib.

  Copyright (c) 2025, Yandex. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <GoogleTest/Library/MockDevicePathLib.h>
#include <GoogleTest/Library/MockUefiBootServicesTableLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>
#include <Library/GoogleTestLib.h>

extern "C" {
  #include "Library/MemoryAllocationLib.h"
  #include <Library/BaseLib.h>
  #include <Library/DebugLib.h>
  #include <Uefi.h>

  #include "DxeImageVerificationLibGoogleTest.h"
}

//////////////////////////////////////////////////////////////////////////////
void
PrepareCertList (
  UINTN               Size,
  UINTN               BufferSize,
  CONST EFI_GUID      *SignatureType,
  CONST UINT8         *Src,
  EFI_SIGNATURE_LIST  *Dst
  )
{
  Dst->SignatureListSize   = (UINT32)BufferSize;
  Dst->SignatureSize       = (UINT32)(sizeof (EFI_SIGNATURE_DATA) - 1 + Size);
  Dst->SignatureHeaderSize = 0;
  CopyGuid (&Dst->SignatureType, SignatureType);

  EFI_SIGNATURE_DATA  *Data = (EFI_SIGNATURE_DATA *)(Dst + 1);

  CopyGuid (&Data->SignatureOwner, &gEfiGlobalVariableGuid);
  CopyMem (&Data->SignatureData[0], Src, Size);
}

void
ExpectBeforeHashCheck (
  MockUefiBootServicesTableLib     &BsMock,
  MockDevicePathLib                &DevicePathMock,
  MockUefiRuntimeServicesTableLib  &RtServicesMock
  )
{
  // Skip all checks that are called before hash verification. This also applies
  // to signed binary files, as no additional mocks are needed before the
  // signature verification starts.

  // Do not allocate on stack
  static UINT8  SetupMode = SECURE_BOOT_MODE_ENABLE;

  EXPECT_CALL (BsMock, gBS_LocateDevicePath)
    .Times (3)
    .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
  EXPECT_CALL (DevicePathMock, IsDevicePathEndType)
    .WillOnce (testing::Return ((BOOLEAN)TRUE));

  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_SECURE_BOOT_MODE_NAME),
      BufferEq (&gEfiGlobalVariableGuid, sizeof (EFI_GUID)),
      testing::NotNull (),
      testing::Pointee (testing::Eq (sizeof (SetupMode))),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (sizeof (SetupMode)),
                  SetArgBuffer<4> (&SetupMode, sizeof (SetupMode)),
                  testing::Return (EFI_SUCCESS)
                  )
       );
}

void
ExpectHashCheckSkip (
  MockUefiRuntimeServicesTableLib  &RtServicesMock,
  int                              n
  )
{
  // Here, the cycle and WillOnce are used because InSequence must be specified
  // in the main function. When using InSequence, Times will retire.
  for (int i = 0; i < n; i++) {
    // Return ‘not found’ for DBx to exit from the first
    // IsSignatureFoundInDatabase
    EXPECT_CALL (
      RtServicesMock,
      gRT_GetVariable (
        Char16StrEq (EFI_IMAGE_SECURITY_DATABASE1),
        BufferEq (
          &gEfiImageSecurityDatabaseGuid,
          sizeof (EFI_GUID)
          ),
        testing::IsNull (),
        testing::Pointee (testing::Eq ((UINTN)0)),
        testing::IsNull ()
        )
      )
      .WillOnce (testing::Return (EFI_NOT_FOUND));

    // IsFound is false, EFI_NOT_FOUND will proceed to the next algorithm
    EXPECT_CALL (
      RtServicesMock,
      gRT_GetVariable (
        Char16StrEq (EFI_IMAGE_SECURITY_DATABASE),
        BufferEq (
          &gEfiImageSecurityDatabaseGuid,
          sizeof (EFI_GUID)
          ),
        testing::IsNull (),
        testing::Pointee (testing::Eq ((UINTN)0)),
        testing::IsNull ()
        )
      )
      .WillOnce (testing::Return (EFI_NOT_FOUND));
  }
}

template <size_t N>
void
ExpectHashDBValue (
  MockUefiRuntimeServicesTableLib  &RtServicesMock,
  UINT8 (&CertListBuffer)[N],
  UINTN                            BufferSize
  )
{
  // Return ‘not found’ for DBx to exit from the first
  // IsSignatureFoundInDatabase
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE1),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)0)),
      testing::IsNull ()
      )
    )
    .WillOnce (testing::Return (EFI_NOT_FOUND));

  // Second call of IsSignatureFoundInDatabase for DB
  // Get Size
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)0)),
      testing::IsNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (BufferSize),
                  testing::Return (EFI_BUFFER_TOO_SMALL)
                  )
       );

  // Return hash
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)BufferSize)),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (BufferSize),
                  SetArgBuffer<4> (&CertListBuffer[0], BufferSize),
                  testing::Return (EFI_SUCCESS)
                  )
       );

  // Hash verification does't exit the loop upon the first match
  EXPECT_CALL (RtServicesMock, gRT_GetVariable)
    .Times (testing::AnyNumber ())
    .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
}

EFI_STATUS
MockInstallConfigurationTable (
  EFI_GUID  *Guid,
  VOID      *Table
  )
{
  if (Table != NULL) {
    FreePool (Table);
  }

  return EFI_SUCCESS;
}

void
ExpectFailureJump (
  MockDevicePathLib             &DevicePathMock,
  MockUefiLib                   &UefiMock,
  MockUefiBootServicesTableLib  &BsMock
  )
{
  EXPECT_CALL (DevicePathMock, ConvertDevicePathToText)
    .WillRepeatedly (testing::Return ((CHAR16 *)NULL));

  EXPECT_CALL (UefiMock, EfiGetSystemConfigurationTable)
    .WillRepeatedly (testing::Return (EFI_SUCCESS));

  EXPECT_CALL (DevicePathMock, GetDevicePathSize)
    .WillRepeatedly (testing::Return ((UINTN)0));

  EXPECT_CALL (BsMock, gBS_InstallConfigurationTable)
    .WillRepeatedly (testing::Invoke (MockInstallConfigurationTable));
}

//////////////////////////////////////////////////////////////////////////////
class CheckImageTypeResult : public ::testing::Test
{
public:
  EFI_DEVICE_PATH_PROTOCOL File;

protected:
  MockUefiRuntimeServicesTableLib RtServicesMock;
  MockUefiBootServicesTableLib BsMock;
  MockDevicePathLib DevicePathMock;

  EFI_STATUS Status;

  UINT32 AuthenticationStatus;
  VOID *FileBuffer;
  UINTN FileSize;
  BOOLEAN BootPolicy;

  virtual void
  SetUp (
    )
  {
    AuthenticationStatus = 0;
    FileBuffer           = NULL;
    FileSize             = 0;
    BootPolicy           = FALSE;
  }
};

TEST_F (CheckImageTypeResult, ImageTypeVerifySanity) {
  // Sanity check
  Status = DxeImageVerificationHandler (
             AuthenticationStatus,
             NULL,
             FileBuffer,
             FileSize,
             BootPolicy
             );
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromFv) {
  EXPECT_CALL (BsMock, gBS_LocateDevicePath)
    .WillRepeatedly (testing::Return (EFI_SUCCESS));
  EXPECT_CALL (BsMock, gBS_OpenProtocol)
    .WillRepeatedly (testing::Return (EFI_SUCCESS));

  Status = DxeImageVerificationHandler (
             AuthenticationStatus,
             &File,
             FileBuffer,
             FileSize,
             BootPolicy
             );
  EXPECT_EQ (Status, EFI_SUCCESS);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromOptionRom) {
  auto  TestFunc = [&] (EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .Times (3)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (BsMock, gBS_OpenProtocol)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (DevicePathMock, IsDevicePathEndType)
                       .WillOnce (testing::Return ((BOOLEAN)FALSE));
                     EXPECT_CALL (DevicePathMock, DevicePathType)
                       .WillOnce (testing::Return ((UINT8)MEDIA_DEVICE_PATH));
                     EXPECT_CALL (DevicePathMock, DevicePathSubType)
                       .WillOnce (testing::Return ((UINT8)MEDIA_RELATIVE_OFFSET_RANGE_DP));

                     Status = DxeImageVerificationHandler (
                                AuthenticationStatus,
                                &File,
                                FileBuffer,
                                FileSize,
                                BootPolicy
                                );
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdOptionRomImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdOptionRomImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromRemovableMedia) {
  auto  TestFunc = [&] (EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .Times (3)
                       .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
                     EXPECT_CALL (DevicePathMock, IsDevicePathEndType)
                       .WillOnce (testing::Return ((BOOLEAN)FALSE));
                     EXPECT_CALL (DevicePathMock, DevicePathType)
                       .WillOnce (testing::Return ((UINT8)MESSAGING_DEVICE_PATH));
                     EXPECT_CALL (DevicePathMock, DevicePathSubType)
                       .WillOnce (testing::Return ((UINT8)MSG_MAC_ADDR_DP));

                     Status = DxeImageVerificationHandler (
                                AuthenticationStatus,
                                &File,
                                FileBuffer,
                                FileSize,
                                BootPolicy
                                );
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdRemovableMediaImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdRemovableMediaImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

TEST_F (CheckImageTypeResult, ImageTypeVerifyImageFromFixedMedia) {
  auto  TestFunc = [&] (EFI_STATUS ExpectedStatus) {
                     EXPECT_CALL (BsMock, gBS_LocateDevicePath)
                       .WillOnce (testing::Return (EFI_NOT_FOUND))
                       .WillOnce (testing::Return (EFI_NOT_FOUND))
                       .WillOnce (testing::Return (EFI_SUCCESS));

                     Status = DxeImageVerificationHandler (
                                AuthenticationStatus,
                                &File,
                                FileBuffer,
                                FileSize,
                                BootPolicy
                                );
                     EXPECT_EQ (Status, ExpectedStatus);
                   };

  PatchPcdSet32 (PcdFixedMediaImageVerificationPolicy, ALWAYS_EXECUTE);
  TestFunc (EFI_SUCCESS);
  PatchPcdSet32 (PcdFixedMediaImageVerificationPolicy, NEVER_EXECUTE);
  TestFunc (EFI_ACCESS_DENIED);
}

//////////////////////////////////////////////////////////////////////////////
class CheckUnsignedImage : public ::testing::Test
{
public:
  EFI_DEVICE_PATH_PROTOCOL File;

protected:
  MockUefiRuntimeServicesTableLib RtServicesMock;
  MockUefiBootServicesTableLib BsMock;
  MockDevicePathLib DevicePathMock;
  MockUefiLib UefiMock;

  EFI_STATUS Status;

  UINT32 AuthenticationStatus;
  VOID *FileBuffer;
  UINTN FileSize;
  BOOLEAN BootPolicy;

  virtual void
  SetUp (
    )
  {
    AuthenticationStatus = 0;
    FileBuffer           = NULL;
    FileSize             = 0;
    BootPolicy           = FALSE;
  }
};

TEST_F (CheckUnsignedImage, HashNormalFlow) {
  constexpr UINTN  Hash512Size = sizeof (images::UnsignedCOFFSha512);
  constexpr UINTN  Hash384Size = sizeof (images::UnsignedCOFFSha384);
  constexpr UINTN  Hash256Size = sizeof (images::UnsignedCOFFSha256);
  constexpr UINTN  Hash1Size   = sizeof (images::UnsignedCOFFSha1);

  constexpr UINTN  Hash512BufferSize = sizeof (EFI_SIGNATURE_LIST) +
                                       sizeof (EFI_SIGNATURE_DATA) - 1 +
                                       Hash512Size;
  constexpr UINTN  Hash384BufferSize = sizeof (EFI_SIGNATURE_LIST) +
                                       sizeof (EFI_SIGNATURE_DATA) - 1 +
                                       Hash384Size;
  constexpr UINTN  Hash256BufferSize = sizeof (EFI_SIGNATURE_LIST) +
                                       sizeof (EFI_SIGNATURE_DATA) - 1 +
                                       Hash256Size;
  constexpr UINTN  Hash1BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + Hash1Size;

  UINT8  Hash512CertListBuffer[Hash512BufferSize] = { 0 };
  UINT8  Hash384CertListBuffer[Hash384BufferSize] = { 0 };
  UINT8  Hash256CertListBuffer[Hash256BufferSize] = { 0 };
  UINT8  Hash1CertListBuffer[Hash1BufferSize]     = { 0 };

  PrepareCertList (
    Hash512Size,
    Hash512BufferSize,
    &gEfiCertSha512Guid,
    &images::UnsignedCOFFSha512[0],
    (EFI_SIGNATURE_LIST *)&Hash512CertListBuffer
    );

  {
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
    // all calls.
    testing::InSequence  s;

    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

    ExpectHashDBValue<Hash512BufferSize> (
      RtServicesMock,
      Hash512CertListBuffer,
      Hash512BufferSize
      );

    FileBuffer = (VOID *)&images::UnsignedCOFF;
    FileSize   = sizeof (images::UnsignedCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_SUCCESS);
  }

  PrepareCertList (
    Hash384Size,
    Hash384BufferSize,
    &gEfiCertSha384Guid,
    &images::UnsignedCOFFSha384[0],
    (EFI_SIGNATURE_LIST *)&Hash384CertListBuffer
    );

  {
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
    // all calls.
    testing::InSequence  s;

    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

    // Skip SHA512 check
    ExpectHashCheckSkip (RtServicesMock, 1);

    // SHA384 check
    ExpectHashDBValue<Hash384BufferSize> (
      RtServicesMock,
      Hash384CertListBuffer,
      Hash384BufferSize
      );

    FileBuffer = (VOID *)&images::UnsignedCOFF;
    FileSize   = sizeof (images::UnsignedCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_SUCCESS);
  }

  PrepareCertList (
    Hash256Size,
    Hash256BufferSize,
    &gEfiCertSha256Guid,
    &images::UnsignedCOFFSha256[0],
    (EFI_SIGNATURE_LIST *)&Hash256CertListBuffer
    );

  {
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
    // all calls.
    testing::InSequence  s;

    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

    // Skip SHA512, SHA384 check
    ExpectHashCheckSkip (RtServicesMock, 2);

    // SHA256 check
    ExpectHashDBValue<Hash256BufferSize> (
      RtServicesMock,
      Hash256CertListBuffer,
      Hash256BufferSize
      );

    FileBuffer = (VOID *)&images::UnsignedCOFF;
    FileSize   = sizeof (images::UnsignedCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_SUCCESS);
  }

  PrepareCertList (
    Hash1Size,
    Hash1BufferSize,
    &gEfiCertSha1Guid,
    &images::UnsignedCOFFSha1[0],
    (EFI_SIGNATURE_LIST *)&Hash1CertListBuffer
    );

  {
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
    // all calls.
    testing::InSequence  s;

    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

    // Skip SHA512, SHA384, SHA256 check
    ExpectHashCheckSkip (RtServicesMock, 3);

    ExpectHashDBValue<Hash1BufferSize> (
      RtServicesMock,
      Hash1CertListBuffer,
      Hash1BufferSize
      );

    // Last check. No need to loop.
    FileBuffer = (VOID *)&images::UnsignedCOFF;
    FileSize   = sizeof (images::UnsignedCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_SUCCESS);
  }
}

TEST_F (CheckUnsignedImage, HashNoDBRecods) {
  // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
  // all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  // Skip SHA512, SHA384, SHA256, SHA1 check
  ExpectHashCheckSkip (RtServicesMock, 4);

  ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

  // Last check. No need to loop.
  FileBuffer = (VOID *)&images::UnsignedCOFF;
  FileSize   = sizeof (images::UnsignedCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_ACCESS_DENIED);
}

TEST_F (CheckUnsignedImage, HashFoundDBx) {
  constexpr UINTN  Size       = sizeof (images::UnsignedCOFFSha512);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + Size;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    Size,
    BufferSize,
    &gEfiCertSha512Guid,
    &images::UnsignedCOFFSha512[0],
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will match
  // all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  // DBx routines
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE1),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)0)),
      testing::IsNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (BufferSize),
                  testing::Return (EFI_BUFFER_TOO_SMALL)
                  )
       );

  // Return hash
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE1),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)BufferSize)),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (BufferSize),
                  SetArgBuffer<4> (&CertListBuffer[0], BufferSize),
                  testing::Return (EFI_SUCCESS)
                  )
       );

  ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

  // Last check. No need to loop.
  FileBuffer = (VOID *)&images::UnsignedCOFF;
  FileSize   = sizeof (images::UnsignedCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_ACCESS_DENIED);
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
