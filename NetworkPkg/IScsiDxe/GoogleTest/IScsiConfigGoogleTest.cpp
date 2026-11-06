/** @file
  Host based unit tests for IScsiConfig.c.

  Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>
#include <GoogleTest/Library/MockHiiLib.h>
#include <GoogleTest/Library/MockUefiRuntimeServicesTableLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseLib.h>
  #include <Library/BaseMemoryLib.h>
  #include <Library/PrintLib.h>
  #include "../IScsiImpl.h"

  EFI_STATUS
  EFIAPI
  IScsiFormRouteConfig (
    IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
    IN  CONST EFI_STRING                      Configuration,
    OUT EFI_STRING                            *Progress
    );
}

using ::testing::_;
using ::testing::DoAll;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::Pointee;
using ::testing::Return;
using ::testing::SetArgPointee;

//
// Link dependencies of IScsiConfig.c that the tested paths do not use.
//
extern "C" {
  ISCSI_PRIVATE_DATA  *mPrivate           = NULL;
  UINT8               IScsiConfigVfrBin[] = { 0 };
  UINT8               IScsiDxeStrings[]   = { 0 };

  EFI_HII_HANDLE
  EFIAPI
  HiiAddPackages (
    IN CONST EFI_GUID    *PackageListGuid,
    IN       EFI_HANDLE  DeviceHandle  OPTIONAL,
    ...
    )
  {
    return NULL;
  }

  UINT8
  IScsiGetSubnetMaskPrefixLength (
    IN EFI_IPv4_ADDRESS  *SubnetMask
    )
  {
    return 0;
  }

  VOID
  IScsiLunToUnicodeStr (
    IN UINT8    *Lun,
    OUT CHAR16  *String
    )
  {
    *String = L'\0';
  }

  VOID
  IScsiMacAddrToStr (
    IN  EFI_MAC_ADDRESS  *Mac,
    IN  UINT32           Len,
    IN  UINT16           VlanId,
    OUT CHAR16           *Str
    )
  {
    *Str = L'\0';
  }

  ISCSI_NIC_INFO *
  IScsiGetNicInfoByIndex (
    IN UINT8  NicIndex
    )
  {
    return NULL;
  }

  VOID *
  IScsiGetVariableAndSize (
    IN  CHAR16    *Name,
    IN  EFI_GUID  *VendorGuid,
    OUT UINTN     *VariableSize
    )
  {
    *VariableSize = 0;
    return NULL;
  }
}

namespace {
UINTN  mConfigToBlockSize;

EFI_STATUS
EFIAPI
MockConfigToBlock (
  IN     CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN     CONST EFI_STRING                       ConfigResp,
  IN OUT UINT8                                  *Block,
  IN OUT UINTN                                  *BlockSize,
  OUT    EFI_STRING                             *Progress
  )
{
  mConfigToBlockSize = *BlockSize;
  *Progress          = ConfigResp + StrLen (ConfigResp);
  return EFI_SUCCESS;
}
} // namespace

class IScsiFormRouteConfigTest : public ::testing::Test {
protected:
  MockHiiLib HiiLibMock;
  MockUefiRuntimeServicesTableLib RtServicesMock;
  EFI_HII_CONFIG_ROUTING_PROTOCOL ConfigRouting;
  EFI_HII_CONFIG_ACCESS_PROTOCOL ConfigAccess;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *SavedConfigRouting;

  void
  SetUp (
    ) override
  {
    ZeroMem (&ConfigRouting, sizeof (ConfigRouting));
    ZeroMem (&ConfigAccess, sizeof (ConfigAccess));
    ConfigRouting.ConfigToBlock = MockConfigToBlock;
    mConfigToBlockSize          = 0;
    SavedConfigRouting          = gHiiConfigRouting;
    gHiiConfigRouting           = &ConfigRouting;
  }

  void
  TearDown (
    ) override
  {
    gHiiConfigRouting = SavedConfigRouting;
  }
};

TEST_F (IScsiFormRouteConfigTest, ReadsInitiatorNameWithNameBufferSize) {
  CHAR16      Configuration[32];
  EFI_STRING  Progress;
  CHAR8       InitiatorName[] = "iqn.2026-10.com.example:initiator";

  //
  // MockConfigToBlock() leaves the IFR InitiatorName empty, so an OFFSET in
  // [ATTEMPT_MAC_ADDR_VAR_OFFSET, ATTEMPT_BOOTENABLE_VAR_OFFSET) makes
  // IScsiFormRouteConfig() read the stored initiator name without modifying
  // any attempt.
  //
  UnicodeSPrintAsciiFormat (
    Configuration,
    sizeof (Configuration),
    "&OFFSET=%04x",
    (UINT32)ATTEMPT_MAC_ADDR_VAR_OFFSET
    );

  EXPECT_CALL (HiiLibMock, HiiIsConfigHdrMatch (_, _, _))
    .WillOnce (Return (TRUE));
  EXPECT_CALL (
    RtServicesMock,
    gRT_GetVariable (
      Char16StrEq (ISCSI_INITIATOR_NAME_VAR_NAME),
      BufferEq (&gEfiIScsiInitiatorNameProtocolGuid, sizeof (EFI_GUID)),
      _,
      Pointee (Eq ((UINTN)ISCSI_NAME_MAX_SIZE)),
      NotNull ()
      )
    )
    .WillOnce (
       DoAll (
         SetArgBuffer<4>(InitiatorName, sizeof (InitiatorName)),
         SetArgPointee<3>(sizeof (InitiatorName)),
         Return (EFI_SUCCESS)
         )
       );

  //
  // Stop IScsiConfigUpdateAttempt() before it updates the HII form.
  //
  EXPECT_CALL (HiiLibMock, HiiAllocateOpCodeHandle ())
    .WillOnce (Return (NULL));

  EXPECT_EQ (IScsiFormRouteConfig (&ConfigAccess, Configuration, &Progress), EFI_SUCCESS);
  EXPECT_EQ (mConfigToBlockSize, sizeof (ISCSI_CONFIG_IFR_NVDATA));
}
