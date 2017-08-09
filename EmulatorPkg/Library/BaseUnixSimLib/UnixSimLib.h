//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Top level include file for Unix implementation of RcSim Library.

  Copyright (c) 2016 Intel Corporation.  All rights reserved.
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.
**/

#ifndef _UNIX_SIM_LIB_H_
#define _UNIX_SIM_LIB_H_

//
// The package level header files this module uses
//

#include <Uefi.h>

//
// The protocols, PPI and GUID defintions for this module
//

#include <Ppi/EmuDynamicLoad.h>

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
// Host OS Specific include files
//

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

//
// Driver specifc include files
//

#include "Config.h"
#include "Debug.h"

#endif // _UNIX_SIM_LIB_H_

