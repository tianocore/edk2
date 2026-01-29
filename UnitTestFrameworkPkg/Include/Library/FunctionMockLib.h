/** @file
  This header allows the mocking of free (C style) functions using gmock.

  Copyright (c) 2023, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef FUNCTION_MOCK_LIB_H_
#define FUNCTION_MOCK_LIB_H_

#include <Library/GoogleTestLib.h>
#include <Library/SubhookLib.h>
#include <type_traits>

//////////////////////////////////////////////////////////////////////////////
// The below macros are the public function mock interface that are intended
// to be used outside this file.

#define MOCK_INTERFACE_DECLARATION(MOCK) \
  static MOCK * Instance;                \
  MOCK ();                               \
  ~MOCK ();

#define MOCK_INTERFACE_DEFINITION(MOCK)   \
  MOCK_STATIC_INSTANCE_DEFINITION (MOCK)  \
  MOCK_INTERFACE_CONSTRUCTOR (MOCK)       \
  MOCK_INTERFACE_DECONSTRUCTOR (MOCK)

// Mock function declaration for external functions (i.e. functions to
// mock that do not exist in the compilation unit).
#define MOCK_FUNCTION_DECLARATION(RET_TYPE, FUNC, ARGS)  \
  MOCK_FUNCTION_TYPE_DEFINITIONS(RET_TYPE, FUNC, ARGS)   \
  MOCK_METHOD (RET_TYPE, FUNC, ARGS);

// Mock function definition for external functions (i.e. functions to
// mock that do not exist in the compilation unit).
#define MOCK_FUNCTION_DEFINITION(MOCK, FUNC, NUM_ARGS, CALL_TYPE)         \
  FUNCTION_DEFINITION_TO_CALL_MOCK(MOCK, FUNC, FUNC, NUM_ARGS, CALL_TYPE)

// Mock function declaration for internal functions (i.e. functions to
// mock that already exist in the compilation unit).
#define MOCK_FUNCTION_INTERNAL_DECLARATION(RET_TYPE, FUNC, ARGS) \
  MOCK_FUNCTION_DECLARATION(RET_TYPE, FUNC, ARGS)                \
  MOCK_FUNCTION_HOOK_DECLARATIONS(FUNC)

// Mock function definition for internal functions (i.e. functions to
// mock that already exist in the compilation unit). This definition also
// implements MOCK_FUNC() which is later hooked as FUNC().
#define MOCK_FUNCTION_INTERNAL_DEFINITION(MOCK, FUNC, NUM_ARGS, CALL_TYPE)         \
  FUNCTION_DEFINITION_TO_CALL_MOCK(MOCK, FUNC, MOCK##_##FUNC, NUM_ARGS, CALL_TYPE) \
  MOCK_FUNCTION_HOOK_DEFINITIONS(MOCK, FUNC)

//////////////////////////////////////////////////////////////////////////////
// The below macros are private and should not be used outside this file.

#define MOCK_FUNCTION_HOOK_DECLARATIONS(FUNC)     \
  static subhook::Hook Hook##FUNC;                \
  struct MockContainer_##FUNC {                   \
    MockContainer_##FUNC ();                      \
    ~MockContainer_##FUNC ();                     \
  };                                              \
  MockContainer_##FUNC MockContainerInst_##FUNC;

// This definition implements a constructor and destructor inside a nested
// class to enable automatic installation of the hooks to the associated
// MOCK_FUNC() when the mock object is instantiated in scope and automatic
// removal when the instantiated mock object goes out of scope.
#define MOCK_FUNCTION_HOOK_DEFINITIONS(MOCK, FUNC)                        \
  subhook :: Hook MOCK :: Hook##FUNC;                                     \
  MOCK :: MockContainer_##FUNC :: MockContainer_##FUNC () {               \
    if (MOCK :: Instance == NULL)                                         \
      MOCK :: Hook##FUNC .Install(                                        \
        (FUNC##_ret_type *) ::FUNC,                                       \
        (FUNC##_ret_type *) MOCK##_##FUNC);                               \
  }                                                                       \
  MOCK :: MockContainer_##FUNC :: ~MockContainer_##FUNC () {              \
      MOCK :: Hook##FUNC .Remove();                                       \
  }                                                                       \
  static_assert(                                                          \
    std::is_same<decltype(&::FUNC), decltype(&MOCK##_##FUNC)>::value,     \
    "Function signature from 'MOCK_FUNCTION_INTERNAL_DEFINITION' macro "  \
    "invocation for '" #FUNC "' does not match the function signature "   \
    "of '" #FUNC "' function it is mocking. Mismatch could be due to "    \
    "different return type, arguments, or calling convention. See "       \
    "associated 'MOCK_FUNCTION_INTERNAL_DECLARATION' macro invocation "   \
    "for more details.");

#define MOCK_FUNCTION_TYPE_DEFINITIONS(RET_TYPE, FUNC, ARGS)  \
  using FUNC##_ret_type = RET_TYPE;                           \
  using FUNC##_type = FUNC##_ret_type ARGS;

// This function definition simply calls MOCK::Instance->FUNC() which is the
// mocked counterpart of the original function. This allows using gmock with
// C free functions (since by default gmock only works with object methods).
#define FUNCTION_DEFINITION_TO_CALL_MOCK(MOCK, FUNC, FUNC_DEF_NAME, NUM_ARGS, CALL_TYPE) \
  extern "C" {                                                                           \
    typename MOCK :: FUNC##_ret_type CALL_TYPE FUNC_DEF_NAME(                            \
      GMOCK_PP_REPEAT(GMOCK_INTERNAL_PARAMETER,                                          \
                      (MOCK :: FUNC##_type),                                             \
                      NUM_ARGS))                                                         \
    {                                                                                    \
      EXPECT_TRUE(MOCK :: Instance != NULL)                                              \
        << "Called '" #FUNC "' in '" #MOCK "' function mock object before "              \
        << "an instance of '" #MOCK "' was created in test '"                            \
        << ::testing::UnitTest::GetInstance()->current_test_info()->name()               \
        << "'.";                                                                         \
      return MOCK :: Instance->FUNC(                                                     \
        GMOCK_PP_REPEAT(GMOCK_INTERNAL_FORWARD_ARG,                                      \
                        (MOCK :: FUNC##_type),                                           \
                        NUM_ARGS));                                                      \
    }                                                                                    \
  }

#define MOCK_STATIC_INSTANCE_DEFINITION(MOCK)  MOCK * MOCK :: Instance = NULL;

#define MOCK_INTERFACE_CONSTRUCTOR(MOCK)                                     \
  MOCK :: MOCK () {                                                          \
    EXPECT_TRUE(MOCK :: Instance == NULL)                                    \
      << "Multiple instances of '" #MOCK "' function mock object were "      \
      << "created and only one instance is allowed in test '"                \
      << ::testing::UnitTest::GetInstance()->current_test_info()->name()     \
      << "'.";                                                               \
    MOCK :: Instance = this;                                                 \
  }

#define MOCK_INTERFACE_DECONSTRUCTOR(MOCK) \
  MOCK :: ~ MOCK () {                      \
    MOCK :: Instance = NULL;               \
  }

#endif // FUNCTION_MOCK_LIB_H_
