/** @file

  Root include file for IntelFrameworkModule Package Base type modules

  This is the include file for any module of type base. Base modules only use 
  types defined via this include file and can be ported easily to any 
  environment. There are a set of base libraries in the Mde Package that can
  be used to implement base modules.

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FRAMEWORK_MODULE_BASE_H_
#define _FRAMEWORK_MODULE_BASE_H_
///
/// This is the max data size including all the headers which can be passed
/// as Status Code data. This data should be multiple of 8 byte
/// to avoid any kind of boundary issue. Also, sum of this data size (inclusive
/// of size of EFI_STATUS_CODE_DATA should not exceed the max record size of
/// data hub
///
#define EFI_STATUS_CODE_DATA_MAX_SIZE 200

#endif //_FRAMEWORK_MODULE_BASE_H_
