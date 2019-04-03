/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

  @file
  PchPlatformLibrary.h

  @brief
  Header file for PCH Platform Lib implementation.

**/

#ifndef _PCH_PLATFORM_LIBRARY_IMPLEMENTATION_H_
#define _PCH_PLATFORM_LIBRARY_IMPLEMENTATION_H_

#include "PchAccess.h"
#ifdef ECP_FLAG
#include "EdkIIGlueBase.h"
#include "Library/EdkIIGlueMemoryAllocationLib.h"
#else
#include "Library/PciLib.h"
#include "Library/IoLib.h"
#include "Library/DebugLib.h"
#include "Library/PcdLib.h"

#include "PchCommonDefinitions.h"
#endif

#endif
