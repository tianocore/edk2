/** @file
PciHostBridge driver module, part of QNC module.

Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "CommonHeader.h"
#include "QNCInit.h"

UINT32  mS3ParameterRootPortDownstream = 0;
EFI_QNC_S3_DISPATCH_ITEM  mS3DispatchItem = {
    QncS3ItemTypeInitPcieRootPortDownstream,
    &mS3ParameterRootPortDownstream
  };

EFI_STATUS
QncInitRootPorts (
  )
/*++

Routine Description:

  Perform Initialization of the Downstream Root Ports

Arguments:

Returns:

  EFI_SUCCESS             The function completed successfully

--*/
{
  EFI_STATUS                   Status;
  EFI_QNC_S3_SUPPORT_PROTOCOL  *QncS3Support;
  VOID                         *Context;
  VOID                         *S3DispatchEntryPoint;

  Status = PciExpressInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // Get the QNC S3 Support Protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiQncS3SupportProtocolGuid,
                  NULL,
                  (VOID **) &QncS3Support
                  );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get the QNC S3 Support Protocol
  //
  Status = QncS3Support->SetDispatchItem (
                          QncS3Support,
                          &mS3DispatchItem,
                          &S3DispatchEntryPoint,
                          &Context
                          );
  ASSERT_EFI_ERROR (Status);

  //
  // Save the script dispatch item in the Boot Script
  //
  Status = S3BootScriptSaveDispatch2 (S3DispatchEntryPoint, Context);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
