//
// Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
//
// SPDX-License-Identifier: BSD-2-Clause-Patent
//

//
// GCC in LTO mode interoperates poorly with non-standard libraries that
// provide implementations of compiler intrinsics such as memcpy/memset
// or the stack protector entry points.
//
// By referencing these functions from a non-LTO object that can be passed
// to the linker via the -plugin-opt=-pass-through=-lxxx options, the
// intrinsics are included in the link in a way that allows them to be
// pruned again if no other references to them exist.
//

  .long memcpy - .
  .long memset - .
  .long __stack_chk_fail - .
  .long __stack_chk_guard - .
