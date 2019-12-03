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

	.long	memcpy - .
	.long	memset - .
	.long	__stack_chk_fail - .
	.long	__stack_chk_guard - .
	.long __ashrdi3 - .
	.long __ashldi3 - .
	.long __aeabi_idiv - .
	.long __aeabi_idivmod - .
	.long __aeabi_uidiv - .
	.long __aeabi_uidivmod - .
	.long __divdi3 - .
	.long __divsi3 - .
	.long __lshrdi3 - .
	.long __aeabi_memcpy - .
	.long __aeabi_memset - .
	.long memmove - .
	.long __modsi3 - .
	.long __moddi3 - .
	.long __muldi3 - .
	.long __aeabi_lmul - .
	.long __ARM_ll_mullu - .
	.long __udivsi3 - .
	.long __umodsi3 - .
	.long __udivdi3 - .
	.long __umoddi3 - .
	.long __udivmoddi4 - .
	.long __clzsi2 - .
	.long __ctzsi2 - .
	.long __ucmpdi2 - .
	.long __switch8 - .
	.long __switchu8 - .
	.long __switch16 - .
	.long __switch32 - .
	.long __aeabi_ulcmp - .
	.long __aeabi_uldivmod - .
	.long __aeabi_ldivmod - .
	.long __aeabi_llsr - .
	.long __aeabi_llsl - .
