/** @file
  Implement TPM2 Test related command.

Copyright (c) 2013, Intel Corporation. All rights reserved. <BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/UefiTcgPlatform.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#pragma pack(1)

typedef struct {
  TPM2_COMMAND_HEADER    Header;
  TPMI_YES_NO            FullTest;
} TPM2_SELF_TEST_COMMAND;

typedef struct {
  TPM2_RESPONSE_HEADER    Header;
} TPM2_SELF_TEST_RESPONSE;

#pragma pack()

/**
  This command causes the TPM to perform a test of its capabilities.
  If the fullTest is YES, the TPM will test all functions.
  If fullTest = NO, the TPM will only test those functions that have not previously been tested.

  @param[in] FullTest    YES if full test to be performed
                         NO if only test of untested functions required

  @retval EFI_SUCCESS      Operation completed successfully.
  @retval EFI_DEVICE_ERROR Unexpected device behavior.
**/
EFI_STATUS
EFIAPI
Tpm2SelfTest (
  IN TPMI_YES_NO  FullTest
  )
{
  EFI_STATUS               Status;
  TPM2_SELF_TEST_COMMAND   Cmd;
  TPM2_SELF_TEST_RESPONSE  Res;
  UINT32                   ResultBufSize;

  Cmd.Header.tag         = SwapBytes16 (TPM_ST_NO_SESSIONS);
  Cmd.Header.paramSize   = SwapBytes32 (sizeof (Cmd));
  Cmd.Header.commandCode = SwapBytes32 (TPM_CC_SelfTest);
  Cmd.FullTest           = FullTest;

  ResultBufSize = sizeof (Res);
  Status        = Tpm2SubmitCommand (sizeof (Cmd), (UINT8 *)&Cmd, &ResultBufSize, (UINT8 *)&Res);

  return Status;
}
