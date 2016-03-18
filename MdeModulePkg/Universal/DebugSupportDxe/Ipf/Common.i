/// @file
///  This is set of useful macros.
///
/// Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
/// This program and the accompanying materials
/// are licensed and made available under the terms and conditions of the BSD License
/// which accompanies this distribution.  The full text of the license may be found at
/// http://opensource.org/licenses/bsd-license.php
///
/// THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
/// WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
///
/// Module Name: Common.i
///
///


#define NESTED_SETUP(i,l,o,r)               \
         alloc loc1=ar##.##pfs,i,l,o,r ;    \
         mov loc0=b0 ;;


#define NESTED_RETURN                       \
         mov b0=loc0 ;                      \
         mov ar##.##pfs=loc1 ;;             \
         br##.##ret##.##dpnt  b0 ;;

#define MASK(bp,value)  (value << bp)

