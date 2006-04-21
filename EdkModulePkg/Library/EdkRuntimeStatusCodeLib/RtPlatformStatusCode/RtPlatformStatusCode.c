/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  RtPlatformStatusCode.c 
   
Abstract:

  Contains NT32 specific implementations required to use status codes.

--*/

//
// Globals only work at BootService Time. NOT at Runtime!
//
// 

typedef
EFI_STATUS
(EFIAPI *REPORT_STATUS_CODE_FUNCTION) (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  );

REPORT_STATUS_CODE_FUNCTION  mPeiReportStatusCode;

//
// Function implementations
//
EFI_STATUS
RtPlatformReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Call all status code listeners in the MonoStatusCode.

Arguments:

  Same as ReportStatusCode service
  
Returns:

  EFI_SUCCESS     Always returns success.

--*/
{
  RtMemoryReportStatusCode (CodeType, Value, Instance, CallerId, Data);
  if (EfiAtRuntime ()) {
    //
    // For now all we do is post code at runtime
    //
    return EFI_SUCCESS;
  }

  BsDataHubReportStatusCode (CodeType, Value, Instance, CallerId, Data);

  //
  // Call back into PEI to get status codes.  This is because SecMain contains
  // status code that reports to Win32.
  //
  if (mPeiReportStatusCode != NULL) {
    return mPeiReportStatusCode (CodeType, Value, Instance, CallerId, Data);
  }

  return EFI_SUCCESS;
}

VOID
RtPlatformStatusCodeInitialize (
  VOID
  )
/*++

Routine Description:

  Initialize the status code listeners.

Arguments:

  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  None

--*/
{
  EFI_HOB_GUID_TYPE   *GuidHob;
  void                *Pointer;

  RtMemoryStatusCodeInitialize ();
  BsDataHubStatusCodeInitialize ();

  //
  // Play any prior status codes to the data hub.
  //
  PlaybackStatusCodes (BsDataHubReportStatusCode);

  //
  // If PEI has a ReportStatusCode callback find it and use it before StdErr
  // is connected.
  //
  mPeiReportStatusCode  = NULL;

  GuidHob = GetFirstGuidHob (&gEfiStatusCodeRuntimeProtocolGuid);
  if (NULL == GuidHob) {
    return;
  }
  Pointer = GET_GUID_HOB_DATA (GuidHob);
  mPeiReportStatusCode = (REPORT_STATUS_CODE_FUNCTION) (*(UINTN *) Pointer);
}
