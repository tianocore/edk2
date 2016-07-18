//
// Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
//
// This program and the accompanying materials are licensed and made available under
// the terms and conditions of the BSD License that accompanies this distribution.
// The full text of the license may be found at
// http://opensource.org/licenses/bsd-license.php.
//
// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

	.long	memcpy - .
	.long	memset - .
	.long	__stack_chk_fail - .
	.long	__stack_chk_guard - .
