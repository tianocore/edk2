/** @file
*  Plugin library for setting up dynamic PCDs for TerminalDxe, from fw_cfg
*
*  Copyright (C) 2015-2020, Red Hat, Inc.
*  Copyright (c) 2014, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>

#define UPDATE_BOOLEAN_PCD_FROM_FW_CFG(TokenName)                             \
          do {                                                                \
            BOOLEAN       Setting;                                            \
            RETURN_STATUS PcdStatus;                                          \
                                                                              \
            if (!RETURN_ERROR (QemuFwCfgParseBool (                           \
                    "opt/org.tianocore.edk2.aavmf/" #TokenName, &Setting))) { \
              PcdStatus = PcdSetBoolS (TokenName, Setting);                   \
              ASSERT_RETURN_ERROR (PcdStatus);                                \
            }                                                                 \
          } while (0)

RETURN_STATUS
EFIAPI
TerminalPcdProducerLibConstructor (
  VOID
  )
{
  UPDATE_BOOLEAN_PCD_FROM_FW_CFG (PcdResizeXterm);
  return RETURN_SUCCESS;
}
