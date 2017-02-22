/**@file
  Negotiate SMI features with QEMU, and configure UefiCpuPkg/PiSmmCpuDxeSmm
  accordingly.

  Copyright (C) 2016-2017, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __SMI_FEATURES_H__
#define __SMI_FEATURES_H__

#include <Protocol/S3SaveState.h>

/**
  Negotiate SMI features with QEMU.

  @retval FALSE  If SMI feature negotiation is not supported by QEMU. This is
                 not an error, it just means that SaveSmiFeatures() should not
                 be called.

  @retval TRUE   SMI feature negotiation is supported, and it has completed
                 successfully as well. (Failure to negotiate is a fatal error
                 and the function never returns in that case.)
**/
BOOLEAN
NegotiateSmiFeatures (
  VOID
  );

/**
  Append a boot script fragment that will re-select the previously negotiated
  SMI features during S3 resume.
**/
VOID
SaveSmiFeatures (
  VOID
  );

#endif
