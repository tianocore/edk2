//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Configuration Header file for SmbiosMemoryRecords Driver.

  Copyright (c) 2015 - 2016, Intel Corporation.  All rights reserved.
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.
**/

#ifndef _CONFIG_H_
#define _CONFIG_H_

//
// All configuration items for this driver
// are be placed here.
//

#ifndef D_FUNC_NAME
#define D_FUNC_NAME         1     // Debug output with function name decoration.
#endif

#ifndef DEBUG_ERROR_OUTPUT
#define DEBUG_ERROR_OUTPUT  1     // Debug output on error with function name decoration.
#endif

#ifndef D_TRACE_ENTER
#define D_TRACE_ENTER       0     // Debug output when each function is entered. 
#endif

#ifndef D_TRACE_EXIT
#define D_TRACE_EXIT        0     // Debug output when each function is exited.
#endif

//
// Control of Debug on and off could be managed from a core include or other 
// master config file.
// 
// Turn on all bits of the ErrorLevel parameter of DebugPrint() when debug is turned on
// for a particular DEBUG output control definition.  Turn off all bits when debug is off. 
//
#ifndef DEBUG_ON
#define DEBUG_ON     0xFFFFFFFF
#endif

#ifndef DEBUG_OFF
#define DEBUG_OFF    0
#endif

//
// Defines to use locally in this driver for the ErrorLevel parameter of the DEBUG macro.
//
#ifndef D_FUNC_ARGS
#define D_FUNC_ARGS  DEBUG_OFF    // Debug output of function arguments
#endif

#ifndef D_STRUCT
#define D_STRUCT     DEBUG_ON     // Debug output of data structures
#endif

#ifndef D_PATHS
#define D_PATHS      DEBUG_OFF    // Dump detailed info on the code paths followed
#endif

#ifndef D_INFO
#define D_INFO       DEBUG_ON     // Dump Debug Info
#endif

#ifndef D_ERROR
#define D_ERROR      DEBUG_ON     // Debug output of error information
#endif

#ifndef D_VERBOSE
#define D_VERBOSE    DEBUG_OFF    // Verbose debug output
#endif

#endif // _CONFIG_H_

