/** @file
Header file for QNC Initialization Driver.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


**/
#ifndef _QNC_INITIALIZATION_DRIVER_H_
#define _QNC_INITIALIZATION_DRIVER_H_

EFI_STATUS
QncInitRootPorts (
  )
/*++

Routine Description:

  Perform Initialization of the Downstream Root Ports.

Arguments:

Returns:

  EFI_STATUS

--*/
;

EFI_STATUS
SetInitRootPortDownstreamS3Item (
  )
/*++

Routine Description:

  Set an Init Root Port Downstream devices S3 dispatch item, this function may assert if any error happend

Arguments:

Returns:

  EFI_SUCCESS             The function completed successfully

--*/
;

#endif
