//
// This file contains a 'Sample Driver' and is licensed as such
// under the terms of your license agreement with Intel or your
// vendor.  This file may be modified by the user, subject to
// the additional terms of the license agreement
//
/** @file
  Debug header file for Unix implementation of RcSim Library.

  Copyright (c) 2016 Intel Corporation.  All rights reserved.
  This software and associated documentation (if any) is furnished
  under a license and may only be used or copied in accordance
  with the terms of the license. Except as permitted by such
  license, no part of this software or documentation may be
  reproduced, stored in a retrieval system, or transmitted in any
  form or by any means without the express written consent of
  Intel Corporation.

**/

#ifndef _DEBUG_H_
#define _DEBUG_H_

//
// Instrumentation for error paths.
//

#undef DPRINTF_ERROR

#if defined DEBUG_ERROR_OUTPUT && DEBUG_ERROR_OUTPUT == 1
#define DPRINTF_ERROR(...) DEBUG((DEBUG_ON, __FUNCTION__ )); DEBUG((DEBUG_ON, " ERROR: ")); DEBUG((DEBUG_ON, __VA_ARGS__))
#else
#define DPRINTF_ERROR(...)
#endif

//
// Instrumentation for initialization path.
//

#undef DPRINTF_INIT
#undef DEBUG_INIT_CODE
#undef ASSERT_INIT

#if defined D_FUNC_NAME && D_FUNC_NAME == 1
#define DPRINTF_INIT(...)               DEBUG((DEBUG_ON, __FUNCTION__ )); DEBUG((DEBUG_ON, __VA_ARGS__))
#define DEBUG_INIT_CODE(Expression)     DEBUG_CODE(Expression)
#define ASSERT_INIT                     ASSERT
#else
#define DPRINTF_INIT(...)
#define DEBUG_INIT_CODE(Expression)
#define ASSERT_INIT
#endif

#if defined D_TRACE_ENTER && D_TRACE_ENTER == 1
#define TRACE_ENTER()   DEBUG((DEBUG_ON, __FUNCTION__ )); DEBUG((DEBUG_ON, ":Enter\n"))
#else
#define TRACE_ENTER()
#endif

#if defined D_TRACE_EXIT && D_TRACE_EXIT == 1
#define TRACE_EXIT()    DEBUG((DEBUG_ON, __FUNCTION__ )); DEBUG((DEBUG_ON, ":Exit\n"))
#else
#define TRACE_EXIT()
#endif

//
// If x does not equal EFI_SUCCESS, this macro
// will print a debug message and assert if this
// is a debug build. If x does not equal EFI_SUCCESS
// and this is not a debug build, this macro will
// return EFI_STATUS x.
//

#define EFI_ERROR_RETURN_OR_ASSERT(x,...)       \
  if (EFI_ERROR(x)) {                           \
    DPRINTF_ERROR(__VA_ARGS__);                 \
    ASSERT_EFI_ERROR(x);                        \
    return (x);                                 \
  }

//
// If x equals NULL, this macro will print a debug
// message and assert on EFI_STATUS y if this
// is a debug build. If x equals NULL and this is
// not a debug build, this macro will return EFI_STATUS y.
//

#define CHECK_NULL_RETURN_OR_ASSERT(x, y, ...)  \
  if (x == NULL) {                              \
    DPRINTF_ERROR(__VA_ARGS__);                 \
    ASSERT_EFI_ERROR(y);                        \
    return (y);                                 \
  }

#endif // _DEBUG_H_

