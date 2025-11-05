/** @file
  Unit tests for the implementation of SecureBootVariableLib.

  Copyright (c) 2022, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockUefiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  #include <UefiSecureBoot.h>
  #include <Guid/AuthenticatedVariableFormat.h>
  #include <Guid/ImageAuthentication.h>
  #include <Library/SecureBootVariableLib.h>
  #include <Library/MemoryAllocationLib.h>
}

using namespace testing;

//////////////////////////////////////////////////////////////////////////////
class SetSecureBootModeTest : public Test {
protected:
  MockUefiRuntimeServicesTableLib RtServicesMock;
  UINT8 SecureBootMode;
  EFI_STATUS Status;

  void
  SetUp (
    ) override
  {
    // Any random magic number can be used for these tests
    SecureBootMode = 0xAB;
  }
};

// Test SetSecureBootMode() API from SecureBootVariableLib to verify the
// expected error is returned when the call to gRT->SetVariable() fails.
TEST_F (SetSecureBootModeTest, SetVarError) {
  EXPECT_CALL (RtServicesMock, gRT_SetVariable)
    .WillOnce (Return (EFI_INVALID_PARAMETER));

  Status = SetSecureBootMode (SecureBootMode);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
}

// Test SetSecureBootMode() API from SecureBootVariableLib to verify the
// expected secure boot mode is written to the correct variable in the call
// to gRT->SetVariable().
TEST_F (SetSecureBootModeTest, PropogateModeToSetVar) {
  EXPECT_CALL (
    RtServicesMock,
    gRT_SetVariable (
      Char16StrEq (EFI_CUSTOM_MODE_NAME),
      BufferEq (&gEfiCustomModeEnableGuid, sizeof (EFI_GUID)),
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
      sizeof (SecureBootMode),
      BufferEq (&SecureBootMode, sizeof (SecureBootMode))
      )
    )
    .WillOnce (Return (EFI_SUCCESS));

  Status = SetSecureBootMode (SecureBootMode);
  EXPECT_EQ (Status, EFI_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////////
class GetSetupModeTest : public Test {
protected:
  MockUefiRuntimeServicesTableLib RtServicesMock;
  UINT8 SetupMode;
  EFI_STATUS Status;
  UINT8 ExpSetupMode;

  void
  SetUp (
    ) override
  {
    // Any random magic number can be used for these tests
    ExpSetupMode = 0xAB;
  }
};

// Test GetSetupMode() API from SecureBootVariableLib to verify the expected
// error is returned when the call to gRT->GetVariable() fails.
TEST_F (GetSetupModeTest, GetVarError) {
  EXPECT_CALL (RtServicesMock, gRT_GetVariable)
    .WillOnce (Return (EFI_INVALID_PARAMETER));

  Status = GetSetupMode (&SetupMode);
  EXPECT_EQ (Status, EFI_INVALID_PARAMETER);
}

// Test GetSetupMode() API from SecureBootVariableLib to verify the expected
// setup mode is returned (and with a success return code) when the mode is
// successfully read from the call to gRT->GetVariable().
TEST_F (GetSetupModeTest, FetchModeFromGetVar) {
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (EFI_SETUP_MODE_NAME),
      BufferEq (&gEfiGlobalVariableGuid, sizeof (EFI_GUID)),
      _,
      Pointee (Eq (sizeof (SetupMode))),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgPointee<3>(sizeof (ExpSetupMode)),
         SetArgBuffer<4>(&ExpSetupMode, sizeof (ExpSetupMode)),
         Return (EFI_SUCCESS)
         )
       );

  Status = GetSetupMode (&SetupMode);
  ASSERT_EQ (Status, EFI_SUCCESS);
  EXPECT_EQ (SetupMode, ExpSetupMode);
}

//////////////////////////////////////////////////////////////////////////////
class IsSecureBootEnabledTest : public Test {
protected:
  MockUefiLib UefiLibMock;
  BOOLEAN Enabled;
};

// Test IsSecureBootEnabled() API from SecureBootVariableLib to verify FALSE
// is returned when the call to GetEfiGlobalVariable2() fails.
TEST_F (IsSecureBootEnabledTest, GetVarError) {
  EXPECT_CALL (UefiLibMock, GetEfiGlobalVariable2)
    .WillOnce (Return (EFI_ABORTED));

  Enabled = IsSecureBootEnabled ();
  EXPECT_EQ (Enabled, FALSE);
}

//////////////////////////////////////////////////////////////////////////////
class IsSecureBootEnabledAllocTest : public IsSecureBootEnabledTest {
protected:
  UINT8 *BootEnabledBuffer;

  void
  SetUp (
    ) override
  {
    BootEnabledBuffer = (UINT8 *)AllocatePool (1);
    ASSERT_NE (BootEnabledBuffer, nullptr);
  }
};

// Test IsSecureBootEnabled() API from SecureBootVariableLib to verify TRUE
// is returned when the call to GetEfiGlobalVariable2() is successful and
// returns SECURE_BOOT_MODE_ENABLE.
TEST_F (IsSecureBootEnabledAllocTest, IsEnabled) {
  *BootEnabledBuffer = SECURE_BOOT_MODE_ENABLE;
  EXPECT_CALL (
    UefiLibMock,
    GetEfiGlobalVariable2 (
      Char16StrEq (EFI_SECURE_BOOT_MODE_NAME),
      NotNull (),
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<1>(&BootEnabledBuffer, sizeof (VOID *)),
         Return (EFI_SUCCESS)
         )
       );

  Enabled = IsSecureBootEnabled ();
  EXPECT_EQ (Enabled, TRUE);
}

// Test IsSecureBootEnabled() API from SecureBootVariableLib to verify FALSE
// is returned when the call to GetEfiGlobalVariable2() is successful and
// returns SECURE_BOOT_MODE_DISABLE.
TEST_F (IsSecureBootEnabledAllocTest, IsDisabled) {
  *BootEnabledBuffer = SECURE_BOOT_MODE_DISABLE;
  EXPECT_CALL (
    UefiLibMock,
    GetEfiGlobalVariable2 (
      Char16StrEq (EFI_SECURE_BOOT_MODE_NAME),
      NotNull (),
      _
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<1>(&BootEnabledBuffer, sizeof (VOID *)),
         Return (EFI_SUCCESS)
         )
       );

  Enabled = IsSecureBootEnabled ();
  EXPECT_EQ (Enabled, FALSE);
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
