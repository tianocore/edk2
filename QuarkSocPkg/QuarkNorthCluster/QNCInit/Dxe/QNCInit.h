/** @file
Header file for QNC Initialization Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


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
