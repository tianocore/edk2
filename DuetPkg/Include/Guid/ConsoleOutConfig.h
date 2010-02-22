/**@file
  Setup Variable data structure for Duet platform.

Copyright (c)  2010 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#ifndef __DUET_CONSOLEOUT_CONFIG_H__
#define __DUET_CONSOLEOUT_CONFIG_H__

#define DUET_CONSOLEOUT_CONFIG_GUID  \
  { 0xED150714, 0xDF30, 0x407D, { 0xB2, 0x4A, 0x4B, 0x74, 0x2F, 0xD5, 0xCE, 0xA2 } }

#pragma pack(1)
typedef struct {
  //
  // Console output mode
  //
  UINT32        ConOutColumn;
  UINT32        ConOutRow;
} DUET_CONSOLEOUT_CONFIG;
#pragma pack()

extern EFI_GUID   gDuetConsoleOutConfigGuid;

#endif
