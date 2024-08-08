/** @file
  NULL TCG platform event log library that provides the additional TCG event log.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  This function returns the existence of platform specific TCG event
  log.

  @return TRUE  Platform specific TCG event log is exist.
  @return FALSE Platform specific TCG event log is not exist.
**/
BOOLEAN
PlatformTcgEventLogProvided (
  VOID
  )
{
  return FALSE;
}

/**
  Platform implementation of setting up TCG event log before combining
  with PEI TCG event log from HOB.

  @retval  EFI_SUCCESS      TCG event log is setup successfully.
  @retval  EFI_UNSUPPORTED  No platform-specific implementation of
                            seting up TCG event log.
  @retval  Otherwise        Some other erros.
**/
EFI_STATUS
PlatformTcgSetupEventLog (
  VOID
  )
{
  return EFI_UNSUPPORTED;
}
