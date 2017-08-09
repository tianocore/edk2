//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Top level include file for PeiWindowsFileIo Driver.

  Copyright (c) 2015 Intel Corporation.  All rights reserved.
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.
**/

#ifndef _META_H_
#define _META_H_

//
// The package level header files this module uses
//

#include <Uefi.h>
#include <WinNtPeim.h>

//
// The protocols, PPI and GUID defintions for this module
//

#include <Ppi/HostOsSocketIoPpi.h>
#include <Ppi/NtThunk.h>

//
// The Library classes this module consumes
//
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>

//
// Driver specifc include files
//

#include "WindowsFunctionImplementations.h"
#include "PeiWindowsSocketIo.h"
#include "Config.h"
#include "Debug.h"

#endif // _META_H_

