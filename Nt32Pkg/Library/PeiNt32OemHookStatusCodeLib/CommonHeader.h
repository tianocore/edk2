/**@file
  Common header file shared by all source files.

  This file includes package header files, library classes and protocol, PPI & GUID definitions.

  Copyright (c) 2007, Intel Corporation.
  All rights reserved.
   This software and associated documentation (if any) is furnished
   under a license and may only be used or copied in accordance
   with the terms of the license. Except as permitted by such
   license, no part of this software or documentation may be
   reproduced, stored in a retrieval system, or transmitted in any
   form or by any means without the express written consent of
   Intel Corporation.
**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_


//
// The package level header files this module uses
//
#include <Peim.h>
#include <EdkPeim.h>
#include <WinNtPeim.h>
//
// The protocols, PPI and GUID defintions for this module
//
#include <Guid/StatusCodeDataTypeId.h>
#include <Ppi/NtThunk.h>
//
// The Library classes this module consumes
//
#include <Library/OemHookStatusCodeLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PeiServicesLib.h>

#endif
