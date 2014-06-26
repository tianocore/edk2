/*++

Copyright (c) 2004 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ReportStatusCode.c

Abstract:

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include "PeiHob.h"
#include EFI_PROTOCOL_DEFINITION (DevicePath)
#include EFI_GUID_DEFINITION (Hob)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)
#include EFI_ARCH_PROTOCOL_DEFINITION (StatusCode)

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

EFI_REPORT_STATUS_CODE gReportStatusCode = NULL;

VOID
EFIAPI
OnStatusCodeInstall (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                Status;
  EFI_STATUS_CODE_PROTOCOL  *StatusCode;

  Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID **) &StatusCode);
  if (!EFI_ERROR (Status)) {
    gReportStatusCode = StatusCode->ReportStatusCode;
  }
}

EFI_STATUS
GetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  )
/*++

Routine Description:

  Searches for a Protocol Interface passed from PEI through a HOB

Arguments:

  ProtocolGuid - The Protocol GUID to search for in the HOB List
  Interface    - A pointer to the interface for the Protocol GUID

Returns:

  EFI_SUCCESS   - The Protocol GUID was found and its interface is returned in Interface
  EFI_NOT_FOUND - The Protocol GUID was not found in the HOB List

--*/
{
  EFI_STATUS            Status;
  EFI_PEI_HOB_POINTERS  GuidHob;

  //
  // Get Hob list
  //
  Status = EfiLibGetSystemConfigurationTable (&gEfiHobListGuid, (VOID **)  &GuidHob.Raw);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Status = EFI_NOT_FOUND; EFI_ERROR (Status);) {
    if (END_OF_HOB_LIST (GuidHob)) {
      Status = EFI_NOT_FOUND;
      break;
    }

    if (GET_HOB_TYPE (GuidHob) == EFI_HOB_TYPE_GUID_EXTENSION) {
      if (EfiCompareGuid (ProtocolGuid, &GuidHob.Guid->Name)) {
        Status     = EFI_SUCCESS;
        *Interface = (VOID *) *(UINTN *) (GuidHob.Guid + 1);
      }
    }

    GuidHob.Raw = GET_NEXT_HOB (GuidHob);
  }

  return Status;
}

#endif

EFI_STATUS
EfiLibReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL  
  )
/*++

Routine Description:

  Report device path through status code.

Arguments:

  Type        - Code type
  Value       - Code value
  Instance    - Instance number
  CallerId    - Caller name
  DevicePath  - Device path that to be reported

Returns:

  Status code.

  EFI_OUT_OF_RESOURCES - No enough buffer could be allocated

--*/
{
  EFI_STATUS  Status;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  if (gReportStatusCode == NULL) {
    //
    // Because we've installed the protocol notification on EfiStatusCodeRuntimeProtocol,
    //   running here indicates that the StatusCode driver has not started yet.
    //
    if (gBS == NULL) {
      //
      // Running here only when StatusCode driver never starts.
      //
      return EFI_UNSUPPORTED;
    }

    //
    // Try to get the PEI version of ReportStatusCode.
    //      
    Status = GetPeiProtocol (&gEfiStatusCodeRuntimeProtocolGuid, (VOID **) &gReportStatusCode);
    if (EFI_ERROR (Status) || (gReportStatusCode == NULL)) {
      return EFI_UNSUPPORTED;
    }
  }
  Status = gReportStatusCode (Type, Value, Instance, CallerId, Data);
#else
  if (gRT == NULL) {
    return EFI_UNSUPPORTED;
  }
  //
  // Check whether EFI_RUNTIME_SERVICES has Tiano Extension
  //
  Status = EFI_UNSUPPORTED;
  if (gRT->Hdr.Revision     == EFI_SPECIFICATION_VERSION     &&
      gRT->Hdr.HeaderSize   == sizeof (EFI_RUNTIME_SERVICES) &&
      gRT->ReportStatusCode != NULL) {
    Status = gRT->ReportStatusCode (Type, Value, Instance, CallerId, Data);
  }
#endif
  return Status;
}

EFI_STATUS
ReportStatusCodeWithDevicePath (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId OPTIONAL,
  IN EFI_DEVICE_PATH_PROTOCOL * DevicePath
  )
/*++

Routine Description:

  Report device path through status code.

Arguments:

  Type        - Code type
  Value       - Code value
  Instance    - Instance number
  CallerId    - Caller name
  DevicePath  - Device path that to be reported

Returns:

  Status code.

  EFI_OUT_OF_RESOURCES - No enough buffer could be allocated

--*/
{
  UINT16                    Size;
  UINT16                    DevicePathSize;
  EFI_STATUS_CODE_DATA      *ExtendedData;
  EFI_DEVICE_PATH_PROTOCOL  *ExtendedDevicePath;
  EFI_STATUS                Status;

  DevicePathSize  = (UINT16) EfiDevicePathSize (DevicePath);
  Size            = (UINT16) (DevicePathSize + sizeof (EFI_STATUS_CODE_DATA));
  ExtendedData    = (EFI_STATUS_CODE_DATA *) EfiLibAllocatePool (Size);
  if (ExtendedData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  ExtendedDevicePath = EfiConstructStatusCodeData (Size, &gEfiStatusCodeSpecificDataGuid, ExtendedData);
  EfiCopyMem (ExtendedDevicePath, DevicePath, DevicePathSize);

  Status = EfiLibReportStatusCode (Type, Value, Instance, CallerId, (EFI_STATUS_CODE_DATA *) ExtendedData);

  gBS->FreePool (ExtendedData);
  return Status;
}
