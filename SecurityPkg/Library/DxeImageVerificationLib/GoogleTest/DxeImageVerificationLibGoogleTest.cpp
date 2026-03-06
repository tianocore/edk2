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
  // LeakSanitizer workaround for AddImageExeInfo
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

template <size_t N, size_t M>
void
ExpectForbiddenByDBxHash (
  MockUefiRuntimeServicesTableLib  &RtServicesMock,
  UINT8 (&CertListBuffer)[N],
  UINTN                            BufferSize,
  UINT8 (&HashListBuffer)[M],
  UINTN                            HashBufferSize
  )
{
  // Signature DBx check - EFI_NOT_FOUND
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

  // IsAllowedByDb - DB size check
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

  // IsAllowedByDb - DB data (X509 certs entries)
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

  // IsAllowedByDb DBx check root cert revoked
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

  // IsSignatureFoundInDatabase prepare buffer
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
                  testing::SetArgPointee<3> (HashBufferSize),
                  testing::Return (EFI_BUFFER_TOO_SMALL)
                  )
       );

  // IsSignatureFoundInDatabase prohibit hash
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE1),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)HashBufferSize)),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (HashBufferSize),
                  SetArgBuffer<4> (&HashListBuffer[0], HashBufferSize),
                  testing::Return (EFI_SUCCESS)
                  )
       );

  EXPECT_CALL (RtServicesMock, gRT_GetVariable)
    .Times (testing::AnyNumber ())
    .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
}

template <size_t N>
void
ExpectNoSignatureHashDBValue (
  MockUefiRuntimeServicesTableLib  &RtServicesMock,
  UINT8 (&HashCertListBuffer)[N],
  UINTN                            HashBufferSize
  )
{
  // Signature DBx check - EFI_NOT_FOUND
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

  // IsAllowedByDb - DB size check
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
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (HashBufferSize),
                  testing::Return (EFI_BUFFER_TOO_SMALL)
                  )
       );

  // IsAllowedByDb - DB data (hash entries, no X509 certs)
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE),
      BufferEq (
        &gEfiImageSecurityDatabaseGuid,
        sizeof (EFI_GUID)
        ),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)HashBufferSize)),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (HashBufferSize),
                  SetArgBuffer<4> (&HashCertListBuffer[0], HashBufferSize),
                  testing::Return (EFI_SUCCESS)
                  )
       );

  // IsAllowedByDb (DBX) - NOT FOUND
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

  // IsSignatureFoundInDatabase (DBX) - NOT FOUND
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

  // IsSignatureFoundInDatabase (DB) - size check
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
                  testing::SetArgPointee<3> (HashBufferSize),
                  testing::Return (EFI_BUFFER_TOO_SMALL)
                  )
       );

  // IsSignatureFoundInDatabase (DB) - data (hash found)
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_IMAGE_SECURITY_DATABASE),
      BufferEq (&gEfiImageSecurityDatabaseGuid, sizeof (EFI_GUID)),
      testing::IsNull (),
      testing::Pointee (testing::Eq ((UINTN)HashBufferSize)),
      testing::NotNull ()
      )
    )
    .WillOnce (
       testing::DoAll (
                  testing::SetArgPointee<3> (HashBufferSize),
                  SetArgBuffer<4> (&HashCertListBuffer[0], HashBufferSize),
                  testing::Return (EFI_SUCCESS)
                  )
       );

  EXPECT_CALL (RtServicesMock, gRT_GetVariable)
    .Times (testing::AnyNumber ())
    .WillRepeatedly (testing::Return (EFI_NOT_FOUND));
}

template <size_t N>
void
ExpectCertForbiddenByDbx (
  MockUefiRuntimeServicesTableLib  &RtServicesMock,
  UINT8 (&CertListBuffer)[N],
  UINTN                            BufferSize
  )
{
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
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
    // match all calls.
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
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
    // match all calls.
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
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
    // match all calls.
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
    // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
    // match all calls.
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
  // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
  // match all calls.
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

  // Do not delete this. Otherwise, GetVariable from ExpectHashDBValue will
  // match all calls.
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

//////////////////////////////////////////////////////////////////////////////
class CheckSignedImage : public ::testing::Test
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

TEST_F (CheckSignedImage, NormalFlowPkcs7) {
  constexpr UINTN  CertSize   = sizeof (certs::TestCertDer);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    &certs::TestCertDer[0],
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  // Do not delete this. Otherwise, GetVariable from ExpectSignatureFoundInDb
  // will match all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectHashDBValue<BufferSize> (RtServicesMock, CertListBuffer, BufferSize);

  FileBuffer = (VOID *)&images::PkcsCOFF;
  FileSize   = sizeof (images::PkcsCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_SUCCESS);
}

TEST_F (CheckSignedImage, NormalFlowUefiGuid) {
  constexpr UINTN  CertSize   = sizeof (certs::TestCertDer);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    &certs::TestCertDer[0],
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  // Do not delete this. Otherwise, GetVariable from ExpectSignatureFoundInDb
  // will match all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectHashDBValue<BufferSize> (RtServicesMock, CertListBuffer, BufferSize);

  FileBuffer = (VOID *)&images::CertGuidCOFF;
  FileSize   = sizeof (images::CertGuidCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_SUCCESS);
}

TEST_F (CheckSignedImage, NormalFlowTwoCerts) {
  constexpr UINTN  CertSize          = sizeof (certs::TestCertDer);
  constexpr UINTN  SignatureDataSize = sizeof (EFI_GUID) + CertSize;
  constexpr UINTN  BufferSize        =
    sizeof (EFI_SIGNATURE_LIST) + (SignatureDataSize * 2);

  UINT8  CertListBuffer[BufferSize] = { 0 };

  UINT8  InvalidCert[CertSize] = { 0 };

  CopyMem (InvalidCert, certs::TestCertDer, CertSize);
  InvalidCert[0] = 0xAA;

  EFI_SIGNATURE_LIST  *CertList = (EFI_SIGNATURE_LIST *)&CertListBuffer;

  CertList->SignatureListSize   = (UINT32)BufferSize;
  CertList->SignatureHeaderSize = 0;
  CertList->SignatureSize       = (UINT32)SignatureDataSize;
  CopyGuid (&CertList->SignatureType, &gEfiCertX509Guid);

  EFI_SIGNATURE_DATA  *Cert1 = (EFI_SIGNATURE_DATA *)(CertList + 1);

  CopyGuid (&Cert1->SignatureOwner, &gEfiGlobalVariableGuid);
  CopyMem (Cert1->SignatureData, InvalidCert, CertSize);

  EFI_SIGNATURE_DATA  *Cert2 = (EFI_SIGNATURE_DATA *)((UINT8 *)Cert1 + SignatureDataSize);

  CopyGuid (&Cert2->SignatureOwner, &gEfiGlobalVariableGuid);
  CopyMem (Cert2->SignatureData, certs::TestCertDer, CertSize);

  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectHashDBValue<BufferSize> (RtServicesMock, CertListBuffer, BufferSize);

  FileBuffer = (VOID *)&images::PkcsCOFF;
  FileSize   = sizeof (images::PkcsCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );

  EXPECT_EQ (Status, EFI_SUCCESS);
}

void
UpdatePeAuthCodeHash (
  UINT8        *PkcsCopy,
  UINT32       HashType,
  CONST UINT8  *HashValue,
  UINTN        HashSize
  )
{
  constexpr UINTN  HashOidOffset   = 0x408; // AuthData(0x3e8) + 32
  constexpr UINTN  HashTagOffset   = 0x470; // AuthData + 132
  constexpr UINTN  HashValueOffset = 0x472; // AuthData + 134

  static const UINT8  OidSha1[]   = { 0x2B, 0x0E, 0x03, 0x02, 0x1A };
  static const UINT8  OidSha256[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01 };
  static const UINT8  OidSha384[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02 };
  static const UINT8  OidSha512[] = { 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03 };

  const UINT8  *Oid;
  UINTN        OidLength;

  switch (HashType) {
    case HASHALG_SHA1:
      Oid       = OidSha1;
      OidLength = sizeof (OidSha1);
      break;
    case HASHALG_SHA256:
      Oid       = OidSha256;
      OidLength = sizeof (OidSha256);
      break;
    case HASHALG_SHA384:
      Oid       = OidSha384;
      OidLength = sizeof (OidSha384);
      break;
    case HASHALG_SHA512:
      Oid       = OidSha512;
      OidLength = sizeof (OidSha512);
      break;
    default:
      return;
  }

  CopyMem (PkcsCopy + HashOidOffset, Oid, OidLength);
  PkcsCopy[HashTagOffset]     = 0x04; // ASN.1 Octet(value 0x04) string
  PkcsCopy[HashTagOffset + 1] = (UINT8)HashSize;
  CopyMem (PkcsCopy + HashValueOffset, HashValue, HashSize);
}

TEST_F (CheckSignedImage, NormalFlowPkcsNoCertHashAllowed) {
  constexpr UINTN  Hash512Size = sizeof (images::PkcsCOFFSha512);
  constexpr UINTN  Hash384Size = sizeof (images::PkcsCOFFSha384);
  constexpr UINTN  Hash256Size = sizeof (images::PkcsCOFFSha256);
  constexpr UINTN  Hash1Size   = sizeof (images::PkcsCOFFSha1);

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

  UINT8  PkcsSha1Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha256Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha384Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha512Copy[sizeof (images::PkcsCOFF)];

  CopyMem (PkcsSha1Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha256Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha384Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha512Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));

  UpdatePeAuthCodeHash (PkcsSha1Copy, HASHALG_SHA1, images::PkcsCOFFSha1, Hash1Size);
  UpdatePeAuthCodeHash (PkcsSha256Copy, HASHALG_SHA256, images::PkcsCOFFSha256, Hash256Size);
  UpdatePeAuthCodeHash (PkcsSha384Copy, HASHALG_SHA384, images::PkcsCOFFSha384, Hash384Size);
  UpdatePeAuthCodeHash (PkcsSha512Copy, HASHALG_SHA512, images::PkcsCOFFSha512, Hash512Size);

  PrepareCertList (
    Hash512Size,
    Hash512BufferSize,
    &gEfiCertSha512Guid,
    &images::PkcsCOFFSha512[0],
    (EFI_SIGNATURE_LIST *)&Hash512CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectNoSignatureHashDBValue<Hash512BufferSize> (
      RtServicesMock,
      Hash512CertListBuffer,
      Hash512BufferSize
      );

    FileBuffer = (VOID *)&PkcsSha512Copy;
    FileSize   = sizeof (images::PkcsCOFF);
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
    &images::PkcsCOFFSha384[0],
    (EFI_SIGNATURE_LIST *)&Hash384CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectNoSignatureHashDBValue<Hash384BufferSize> (
      RtServicesMock,
      Hash384CertListBuffer,
      Hash384BufferSize
      );

    FileBuffer = (VOID *)&PkcsSha384Copy;
    FileSize   = sizeof (images::PkcsCOFF);
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
    &images::PkcsCOFFSha256[0],
    (EFI_SIGNATURE_LIST *)&Hash256CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectNoSignatureHashDBValue<Hash256BufferSize> (
      RtServicesMock,
      Hash256CertListBuffer,
      Hash256BufferSize
      );

    FileBuffer = (VOID *)&PkcsSha256Copy;
    FileSize   = sizeof (images::PkcsCOFF);
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
    &images::PkcsCOFFSha1[0],
    (EFI_SIGNATURE_LIST *)&Hash1CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectNoSignatureHashDBValue<Hash1BufferSize> (
      RtServicesMock,
      Hash1CertListBuffer,
      Hash1BufferSize
      );

    FileBuffer = (VOID *)&PkcsSha1Copy;
    FileSize   = sizeof (images::PkcsCOFF);
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

TEST_F (CheckSignedImage, ForbiddenByDbxPkcs7Sign) {
  constexpr UINTN  CertSize   = sizeof (certs::TestCertDer);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    &certs::TestCertDer[0],
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  // Do not delete this. Otherwise, GetVariable from ExpectCertForbiddenByDbx
  // will match all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectCertForbiddenByDbx<BufferSize> (
    RtServicesMock,
    CertListBuffer,
    BufferSize
    );

  ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

  FileBuffer = (VOID *)&images::PkcsCOFF;
  FileSize   = sizeof (images::PkcsCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_ACCESS_DENIED);
}

TEST_F (CheckSignedImage, ForbiddenByDbxUefiGuidSign) {
  constexpr UINTN  CertSize   = sizeof (certs::TestCertDer);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    &certs::TestCertDer[0],
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  // Do not delete this. Otherwise, GetVariable from ExpectCertForbiddenByDbx
  // will match all calls.
  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectCertForbiddenByDbx<BufferSize> (
    RtServicesMock,
    CertListBuffer,
    BufferSize
    );

  ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

  FileBuffer = (VOID *)&images::CertGuidCOFF;
  FileSize   = sizeof (images::CertGuidCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_ACCESS_DENIED);
}

TEST_F (CheckSignedImage, NoCertPkcs7SignNoHash) {
  // This test also covers the code path for when a perfectly valid signature is
  // missing from the 'db' variable, resulting in the error: "Image is signed
  // but signature is not allowed by DB and %s hash of image is not found in
  // DB/DBX."
  constexpr UINTN  CertSize   = sizeof (certs::TestCertDer);
  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;

  UINT8  InvalidCert[CertSize] = { 0 };

  CopyMem (InvalidCert, images::PkcsCOFF, CertSize);

  InvalidCert[0] = 0xAA;

  UINT8  CertListBuffer[BufferSize] = { 0 };

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    InvalidCert,
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  testing::InSequence  s;

  ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);

  ExpectHashDBValue<BufferSize> (RtServicesMock, CertListBuffer, BufferSize);

  ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

  FileBuffer = (VOID *)&images::PkcsCOFF;
  FileSize   = sizeof (images::PkcsCOFF);
  Status     = DxeImageVerificationHandler (
                 AuthenticationStatus,
                 &File,
                 FileBuffer,
                 FileSize,
                 BootPolicy
                 );
  EXPECT_EQ (Status, EFI_ACCESS_DENIED);
}

TEST_F (CheckSignedImage, ForbiddenByDBxHash) {
  constexpr UINTN  CertSize    = sizeof (certs::TestCertDer);
  constexpr UINTN  Hash512Size = sizeof (images::PkcsCOFFSha512);
  constexpr UINTN  Hash384Size = sizeof (images::PkcsCOFFSha384);
  constexpr UINTN  Hash256Size = sizeof (images::PkcsCOFFSha256);
  constexpr UINTN  Hash1Size   = sizeof (images::PkcsCOFFSha1);

  constexpr UINTN  BufferSize =
    sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + CertSize;
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

  UINT8  CertListBuffer[BufferSize]               = { 0 };
  UINT8  Hash512CertListBuffer[Hash512BufferSize] = { 0 };
  UINT8  Hash384CertListBuffer[Hash384BufferSize] = { 0 };
  UINT8  Hash256CertListBuffer[Hash256BufferSize] = { 0 };
  UINT8  Hash1CertListBuffer[Hash1BufferSize]     = { 0 };

  UINT8  PkcsSha1Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha256Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha384Copy[sizeof (images::PkcsCOFF)];
  UINT8  PkcsSha512Copy[sizeof (images::PkcsCOFF)];

  CopyMem (PkcsSha1Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha256Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha384Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));
  CopyMem (PkcsSha512Copy, images::PkcsCOFF, sizeof (images::PkcsCOFF));

  UpdatePeAuthCodeHash (PkcsSha1Copy, HASHALG_SHA1, images::PkcsCOFFSha1, Hash1Size);
  UpdatePeAuthCodeHash (PkcsSha256Copy, HASHALG_SHA256, images::PkcsCOFFSha256, Hash256Size);
  UpdatePeAuthCodeHash (PkcsSha384Copy, HASHALG_SHA384, images::PkcsCOFFSha384, Hash384Size);
  UpdatePeAuthCodeHash (PkcsSha512Copy, HASHALG_SHA512, images::PkcsCOFFSha512, Hash512Size);

  PrepareCertList (
    CertSize,
    BufferSize,
    &gEfiCertX509Guid,
    certs::TestCertDer,
    (EFI_SIGNATURE_LIST *)&CertListBuffer
    );

  PrepareCertList (
    Hash512Size,
    Hash512BufferSize,
    &gEfiCertSha512Guid,
    &images::PkcsCOFFSha512[0],
    (EFI_SIGNATURE_LIST *)&Hash512CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectForbiddenByDBxHash<BufferSize, Hash512BufferSize> (
      RtServicesMock,
      CertListBuffer,
      BufferSize,
      Hash512CertListBuffer,
      Hash512BufferSize
      );
    ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

    FileBuffer = (VOID *)&PkcsSha512Copy;
    FileSize   = sizeof (images::PkcsCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_ACCESS_DENIED);
  }

  PrepareCertList (
    Hash384Size,
    Hash384BufferSize,
    &gEfiCertSha384Guid,
    &images::PkcsCOFFSha384[0],
    (EFI_SIGNATURE_LIST *)&Hash384CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectForbiddenByDBxHash<BufferSize, Hash384BufferSize> (
      RtServicesMock,
      CertListBuffer,
      BufferSize,
      Hash384CertListBuffer,
      Hash384BufferSize
      );
    ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

    FileBuffer = (VOID *)&PkcsSha384Copy;
    FileSize   = sizeof (images::PkcsCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_ACCESS_DENIED);
  }

  PrepareCertList (
    Hash256Size,
    Hash256BufferSize,
    &gEfiCertSha256Guid,
    &images::PkcsCOFFSha256[0],
    (EFI_SIGNATURE_LIST *)&Hash256CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectForbiddenByDBxHash<BufferSize, Hash256BufferSize> (
      RtServicesMock,
      CertListBuffer,
      BufferSize,
      Hash256CertListBuffer,
      Hash256BufferSize
      );
    ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

    FileBuffer = (VOID *)&PkcsSha256Copy;
    FileSize   = sizeof (images::PkcsCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_ACCESS_DENIED);
  }

  PrepareCertList (
    Hash1Size,
    Hash1BufferSize,
    &gEfiCertSha1Guid,
    &images::PkcsCOFFSha1[0],
    (EFI_SIGNATURE_LIST *)&Hash1CertListBuffer
    );

  {
    testing::InSequence  s;
    ExpectBeforeHashCheck (BsMock, DevicePathMock, RtServicesMock);
    ExpectForbiddenByDBxHash<BufferSize, Hash1BufferSize> (
      RtServicesMock,
      CertListBuffer,
      BufferSize,
      Hash1CertListBuffer,
      Hash1BufferSize
      );
    ExpectFailureJump (DevicePathMock, UefiMock, BsMock);

    FileBuffer = (VOID *)&PkcsSha1Copy;
    FileSize   = sizeof (images::PkcsCOFF);
    Status     = DxeImageVerificationHandler (
                   AuthenticationStatus,
                   &File,
                   FileBuffer,
                   FileSize,
                   BootPolicy
                   );
    EXPECT_EQ (Status, EFI_ACCESS_DENIED);
  }
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
