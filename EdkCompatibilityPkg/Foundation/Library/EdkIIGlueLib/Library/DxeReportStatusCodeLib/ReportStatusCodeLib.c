/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  ReportStatusCodeLib.c
  
Abstract: 

  Report Status Code Library for DXE Phase.

--*/

#include "EdkIIGlueDxe.h"

//
// Global pointer to the Status Code Protocol
//
static EFI_STATUS_CODE_PROTOCOL  *gStatusCode = NULL;


/**
  Internal worker function that reports a status code through the Status Code Protocol

  This function checks to see if a Status Code Protocol is present in the handle 
  database.  If a Status Code Protocol is not present, then EFI_UNSUPPORTED is 
  returned.  If a Status Code Protocol is present, then it is cached in gStatusCode,
  and the ReportStatusCode() service of the Status Code Protocol is called passing in
  Type, Value, Instance, CallerId, and Data.  The result of this call is returned.

  @param  Type              Status code type. 
  @param  Value             Status code value.
  @param  Instance          Status code instance number.
  @param  CallerId          Pointer to a GUID that identifies the caller of this 
                            function.  This is an optional parameter that may be 
                            NULL.
  @param  Data              Pointer to the extended data buffer.  This is an 
                            optional parameter that may be NULL.

  @retval  EFI_SUCCESS           The status code was reported.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to report the status code.
  @retval  EFI_UNSUPPORTED       Status Code Protocol is not available.

**/
STATIC
EFI_STATUS
InternalReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID           *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL  
  )
{
  EFI_STATUS  Status;

  if (gRT == NULL) {
    return EFI_UNSUPPORTED;
  }

  if (gRT->Hdr.Revision >= 0x00020000) {
    //
    // If gStatusCode is NULL, then see if a Status Code Protocol instance is present 
    // in the handle database.
    //
    if (gStatusCode == NULL) {
      Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID **)&gStatusCode);
      if (EFI_ERROR (Status) || gStatusCode == NULL) {
        return EFI_UNSUPPORTED;
      }
    }

    //
    // A Status Code Protocol is present in the handle database, so pass in all the  
    // parameters to the ReportStatusCode() service of the Status Code Protocol
    //
    return (gStatusCode->ReportStatusCode) (Type, Value, Instance, (EFI_GUID *)CallerId, Data);
  } else {
#if (EFI_SPECIFICATION_VERSION < 0x00020000)
    return (gRT->ReportStatusCode) (Type, Value, Instance, (EFI_GUID *)CallerId, Data);
#else
    return EFI_UNSUPPORTED;
#endif
  }

}


/**
  Computes and returns the size, in bytes, of a device path.

  @param  DevicePath  A pointer to a device path.

  @return  The size, in bytes, of DevicePath.

**/
STATIC
UINTN
InternalReportStatusCodeDevicePathSize (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  CONST EFI_DEVICE_PATH_PROTOCOL  *Start;

  //
  // Search for the end of the device path structure
  //
  Start = DevicePath;
  while (!EfiIsDevicePathEnd (DevicePath)) {
    DevicePath = EfiNextDevicePathNode (DevicePath);
  }

  //
  // Subtract the start node from the end node and add in the size of the end node
  //
  return ((UINTN) DevicePath - (UINTN) Start) + DevicePathNodeLength (DevicePath);
}


/**
  Converts a status code to an 8-bit POST code value.

  Converts the status code specified by CodeType and Value to an 8-bit POST code 
  and returns the 8-bit POST code in PostCode.  If CodeType is an 
  EFI_PROGRESS_CODE or CodeType is an EFI_ERROR_CODE, then bits 0..4 of PostCode 
  are set to bits 16..20 of Value, and bits 5..7 of PostCode are set to bits 
  24..26 of Value., and TRUE is returned.  Otherwise, FALSE is returned.  

  If PostCode is NULL, then ASSERT().

  @param  CodeType  The type of status code being converted.
  @param  Value     The status code value being converted.
  @param  PostCode  A pointer to the 8-bit POST code value to return. 

  @retval  TRUE   The status code specified by CodeType and Value was converted 
                  to an 8-bit POST code and returned in  PostCode.
  @retval  FALSE  The status code specified by CodeType and Value could not be 
                  converted to an 8-bit POST code value.

**/
BOOLEAN
EFIAPI
GlueCodeTypeToPostCode (
  IN  EFI_STATUS_CODE_TYPE   CodeType,
  IN  EFI_STATUS_CODE_VALUE  Value,
  OUT UINT8                  *PostCode
  )
{
  //
  // If PostCode is NULL, then ASSERT()
  //
  ASSERT (PostCode != NULL);

  //
  // Convert Value to an 8 bit post code
  //
  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_PROGRESS_CODE) ||
      ((CodeType & EFI_STATUS_CODE_TYPE_MASK) == EFI_ERROR_CODE)       ) {
    *PostCode  = (UINT8) ((((Value & EFI_STATUS_CODE_CLASS_MASK) >> 24) << 5) |
                          (((Value & EFI_STATUS_CODE_SUBCLASS_MASK) >> 16) & 0x1f));
    return TRUE;
  }
  return FALSE;
}


/**
  Extracts ASSERT() information from a status code structure.

  Converts the status code specified by CodeType, Value, and Data to the ASSERT()
  arguments specified by Filename, Description, and LineNumber.  If CodeType is 
  an EFI_ERROR_CODE, and CodeType has a severity of EFI_ERROR_UNRECOVERED, and 
  Value has an operation mask of EFI_SW_EC_ILLEGAL_SOFTWARE_STATE, extract 
  Filename, Description, and LineNumber from the optional data area of the 
  status code buffer specified by Data.  The optional data area of Data contains 
  a Null-terminated ASCII string for the FileName, followed by a Null-terminated 
  ASCII string for the Description, followed by a 32-bit LineNumber.  If the 
  ASSERT() information could be extracted from Data, then return TRUE.  
  Otherwise, FALSE is returned.  

  If Data is NULL, then ASSERT().
  If Filename is NULL, then ASSERT().
  If Description is NULL, then ASSERT().
  If LineNumber is NULL, then ASSERT().

  @param  CodeType     The type of status code being converted.
  @param  Value        The status code value being converted.
  @param  Data         Pointer to status code data buffer. 
  @param  Filename     Pointer to the source file name that generated the ASSERT().
  @param  Description  Pointer to the description of the ASSERT().
  @param  LineNumber   Pointer to source line number that generated the ASSERT().

  @retval  TRUE   The status code specified by CodeType, Value, and Data was 
                  converted ASSERT() arguments specified by Filename, Description, 
                  and LineNumber.
  @retval  FALSE  The status code specified by CodeType, Value, and Data could 
                  not be converted to ASSERT() arguments.

**/
BOOLEAN
EFIAPI
GlueReportStatusCodeExtractAssertInfo (
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,  
  IN CONST EFI_STATUS_CODE_DATA  *Data, 
  OUT CHAR8                      **Filename,
  OUT CHAR8                      **Description,
  OUT UINT32                     *LineNumber
  )
{
  EFI_DEBUG_ASSERT_DATA  *AssertData;

  ASSERT (Data        != NULL);
  ASSERT (Filename    != NULL);
  ASSERT (Description != NULL);
  ASSERT (LineNumber  != NULL);

  if (((CodeType & EFI_STATUS_CODE_TYPE_MASK)      == EFI_ERROR_CODE) && 
      ((CodeType & EFI_STATUS_CODE_SEVERITY_MASK)  == EFI_ERROR_UNRECOVERED) &&
      ((Value    & EFI_STATUS_CODE_OPERATION_MASK) == EFI_SW_EC_ILLEGAL_SOFTWARE_STATE)) {
    AssertData   = (EFI_DEBUG_ASSERT_DATA *)(Data + 1);
    *Filename    = (CHAR8 *)(AssertData + 1);
    *Description = *Filename + AsciiStrLen (*Filename) + 1;
    *LineNumber  = AssertData->LineNumber;
    return TRUE;
  }
  return FALSE;
}


/**
  Extracts DEBUG() information from a status code structure.

  Converts the status code specified by Data to the DEBUG() arguments specified 
  by ErrorLevel, Marker, and Format.  If type GUID in Data is 
  EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID, then extract ErrorLevel, Marker, and 
  Format from the optional data area of the status code buffer specified by Data.  
  The optional data area of Data contains a 32-bit ErrorLevel followed by Marker 
  which is 12 UINTN parameters, followed by a Null-terminated ASCII string for 
  the Format.  If the DEBUG() information could be extracted from Data, then 
  return TRUE.  Otherwise, FALSE is returned.

  If Data is NULL, then ASSERT().
  If ErrorLevel is NULL, then ASSERT().
  If Marker is NULL, then ASSERT().
  If Format is NULL, then ASSERT().

  @param  Data        Pointer to status code data buffer. 
  @param  ErrorLevel  Pointer to error level mask for a debug message.
  @param  Marker      Pointer to the variable argument list associated with Format.
  @param  Format      Pointer to a Null-terminated ASCII format string of a 
                      debug message.

  @retval  TRUE   The status code specified by Data was converted DEBUG() arguments 
                  specified by ErrorLevel, Marker, and Format.
  @retval  FALSE  The status code specified by Data could not be converted to 
                  DEBUG() arguments.

**/
BOOLEAN
EFIAPI
GlueReportStatusCodeExtractDebugInfo (
  IN CONST EFI_STATUS_CODE_DATA  *Data, 
  OUT UINT32                     *ErrorLevel,
  OUT VA_LIST                    *Marker,
  OUT CHAR8                      **Format
  )
{
  EFI_DEBUG_INFO  *DebugInfo;

  ASSERT (Data       != NULL);
  ASSERT (ErrorLevel != NULL);
  ASSERT (Marker     != NULL);
  ASSERT (Format     != NULL);

  //
  // If the GUID type is not EFI_STATUS_CODE_DATA_TYPE_DEBUG_GUID then return FALSE
  //
  if (!CompareGuid (&Data->Type, &gEfiStatusCodeDataTypeDebugGuid)) {
    return FALSE;
  }

  //
  // Retrieve the debug information from the status code record
  //
  DebugInfo = (EFI_DEBUG_INFO *)(Data + 1);

  *ErrorLevel = DebugInfo->ErrorLevel;

#ifdef __APPLE__
  // This is non portable C code you can't assume VA_LIST is pointer
  return FALSE;
#else
  //
  // The first 12 * UINTN bytes of the string are really an 
  // argument stack to support varargs on the Format string.
  //
  *Marker = (VA_LIST) (DebugInfo + 1);
#endif
  *Format = (CHAR8 *)(((UINT64 *)*Marker) + 12);

  return TRUE;
}


/**
  Reports a status code.

  Reports the status code specified by the parameters Type and Value.  Status 
  code also require an instance, caller ID, and extended data.  This function 
  passed in a zero instance, NULL extended data, and a caller ID of 
  gEfiCallerIdGuid, which is the GUID for the module.  
  
  ReportStatusCode()must actively prevent recusrsion.  If ReportStatusCode() 
  is called while processing another any other Report Status Code Library function,
  then ReportStatusCode() must return immediately.

  @param  Type   Status code type. 
  @param  Value  Status code value.

  @retval  EFI_SUCCESS       The status code was reported.
  @retval  EFI_DEVICE_ERROR  There status code could not be reported due to a 
                             device error.
  @retval  EFI_UNSUPPORTED   Report status code is not supported

**/
EFI_STATUS
EFIAPI
GlueReportStatusCode (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value
  )
{
  return InternalReportStatusCode (Type, Value, 0, &gEfiCallerIdGuid, NULL);
}


/**
  Reports a status code with a Device Path Protocol as the extended data.

  Allocates and fills in the extended data section of a status code with the 
  Device Path Protocol specified by DevicePath.  This function is responsible 
  for allocating a buffer large enough for the standard header and the device 
  path.  The standard header is filled in with a GUID of 
  gEfiStatusCodeSpecificDataGuid.  The status code is reported with a zero 
  instance and a caller ID of gEfiCallerIdGuid.

  ReportStatusCodeWithDevicePath()must actively prevent recursion.  If 
  ReportStatusCodeWithDevicePath() is called while processing another any other 
  Report Status Code Library function, then ReportStatusCodeWithDevicePath() 
  must return EFI_DEVICE_ERROR immediately.

  If DevicePath is NULL, then ASSERT().

  @param  Type        Status code type. 
  @param  Value       Status code value.
  @param  DevicePath  Pointer to the Device Path Protocol to be reported.

  @retval  EFI_SUCCESS           The status code was reported with the extended 
                                 data specified by DevicePath.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to allocate the 
                                 extended data section.
  @retval  EFI_UNSUPPORTED       Report status code is not supported

**/
EFI_STATUS
EFIAPI
GlueReportStatusCodeWithDevicePath (
  IN EFI_STATUS_CODE_TYPE            Type,
  IN EFI_STATUS_CODE_VALUE           Value,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  ASSERT (DevicePath != NULL);
  return ReportStatusCodeWithExtendedData (
           Type, 
           Value, 
           (VOID *)DevicePath, 
           InternalReportStatusCodeDevicePathSize (DevicePath)
           );
}


/**
  Reports a status code with an extended data buffer.

  Allocates and fills in the extended data section of a status code with the 
  extended data specified by ExtendedData and ExtendedDataSize.  ExtendedData 
  is assumed to be one of the data structures specified in Related Definitions.  
  These data structure do not have the standard header, so this function is 
  responsible for allocating a buffer large enough for the standard header and 
  the extended data passed into this function.  The standard header is filled 
  in with a GUID of  gEfiStatusCodeSpecificDataGuid.  The status code is reported 
  with a zero instance and a caller ID of gEfiCallerIdGuid.

  ReportStatusCodeWithExtendedData()must actively prevent recursion.  If 
  ReportStatusCodeWithExtendedData() is called while processing another any other 
  Report Status Code Library function, then ReportStatusCodeWithExtendedData() 
  must return EFI_DEVICE_ERROR immediately.

  If ExtendedData is NULL, then ASSERT().
  If ExtendedDataSize is 0, then ASSERT().

  @param  Type              Status code type. 
  @param  Value             Status code value.
  @param  ExtendedData      Pointer to the extended data buffer to be reported.
  @param  ExtendedDataSize  The size, in bytes, of the extended data buffer to 
                            be reported.

  @retval  EFI_SUCCESS           The status code was reported with the extended 
                                 data specified by ExtendedData and ExtendedDataSize.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to allocate the 
                                 extended data section.
  @retval  EFI_UNSUPPORTED       Report status code is not supported

**/
EFI_STATUS
EFIAPI
GlueReportStatusCodeWithExtendedData (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN CONST VOID             *ExtendedData,
  IN UINTN                  ExtendedDataSize
  )
{
  ASSERT (ExtendedData     != NULL);
  ASSERT (ExtendedDataSize != 0);
  return ReportStatusCodeEx (
           Type,
           Value,
           0,
           NULL,
           NULL,
           ExtendedData,
           ExtendedDataSize
           );
}


/**
  Reports a status code with full parameters.

  The function reports a status code.  If ExtendedData is NULL and ExtendedDataSize 
  is 0, then an extended data buffer is not reported.  If ExtendedData is not 
  NULL and ExtendedDataSize is not 0, then an extended data buffer is allocated.  
  ExtendedData is assumed not have the standard status code header, so this function 
  is responsible for allocating a buffer large enough for the standard header and 
  the extended data passed into this function.  The standard header is filled in 
  with a GUID specified by ExtendedDataGuid.  If ExtendedDataGuid is NULL, then a 
  GUID of gEfiStatusCodeSpecificDatauid is used.  The status code is reported with 
  an instance specified by Instance and a caller ID specified by CallerId.  If 
  CallerId is NULL, then a caller ID of gEfiCallerIdGuid is used.

  ReportStatusCodeEx()must actively prevent recursion.  If ReportStatusCodeEx() 
  is called while processing another any other Report Status Code Library function, 
  then ReportStatusCodeEx() must return EFI_DEVICE_ERROR immediately.

  If ExtendedData is NULL and ExtendedDataSize is not zero, then ASSERT().
  If ExtendedData is not NULL and ExtendedDataSize is zero, then ASSERT().

  @param  Type              Status code type. 
  @param  Value             Status code value.
  @param  Instance          Status code instance number.
  @param  CallerId          Pointer to a GUID that identifies the caller of this 
                            function.  If this parameter is NULL, then a caller 
                            ID of gEfiCallerIdGuid is used.
  @param  ExtendedDataGuid  Pointer to the GUID for the extended data buffer.  
                            If this parameter is NULL, then a the status code 
                            standard header is filled in with 
                            gEfiStatusCodeSpecificDataGuid.
  @param  ExtendedData      Pointer to the extended data buffer.  This is an 
                            optional parameter that may be NULL.
  @param  ExtendedDataSize  The size, in bytes, of the extended data buffer.

  @retval  EFI_SUCCESS           The status code was reported.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to allocate 
                                 the extended data section if it was specified.
  @retval  EFI_UNSUPPORTED       Report status code is not supported

**/
EFI_STATUS
EFIAPI
GlueReportStatusCodeEx (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN CONST EFI_GUID         *CallerId          OPTIONAL,
  IN CONST EFI_GUID         *ExtendedDataGuid  OPTIONAL,
  IN CONST VOID             *ExtendedData      OPTIONAL,
  IN UINTN                  ExtendedDataSize
  )
{
  EFI_STATUS            Status;
  EFI_STATUS_CODE_DATA  *StatusCodeData;

  ASSERT (!((ExtendedData == NULL) && (ExtendedDataSize != 0)));
  ASSERT (!((ExtendedData != NULL) && (ExtendedDataSize == 0)));

  //
  // Allocate space for the Status Code Header and its buffer
  //
  StatusCodeData = NULL;
  (gBS->AllocatePool) (EfiBootServicesData, sizeof (EFI_STATUS_CODE_DATA) + ExtendedDataSize, (VOID **)&StatusCodeData);
  if (StatusCodeData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the extended data header
  //
  StatusCodeData->HeaderSize = (UINT16) sizeof (EFI_STATUS_CODE_DATA);
  StatusCodeData->Size = (UINT16)ExtendedDataSize;
  if (ExtendedDataGuid == NULL) {
    ExtendedDataGuid = &gEfiStatusCodeSpecificDataGuid;
  }
  CopyGuid (&StatusCodeData->Type, ExtendedDataGuid);

  //
  // Fill in the extended data buffer
  //
  CopyMem (StatusCodeData + 1, ExtendedData, ExtendedDataSize);

  //
  // Report the status code
  //
  if (CallerId == NULL) {
    CallerId = &gEfiCallerIdGuid;
  }
  Status = InternalReportStatusCode (Type, Value, Instance, CallerId, StatusCodeData);

  //
  // Free the allocated buffer
  //
  (gBS->FreePool) (StatusCodeData);

  return Status;
}


/**
  Returns TRUE if status codes of type EFI_PROGRESS_CODE are enabled

  This function returns TRUE if the REPORT_STATUS_CODE_PROPERTY_PROGRESS_CODE_ENABLED 
  bit of PcdReportStatusCodeProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE   The REPORT_STATUS_CODE_PROPERTY_PROGRESS_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is set.
  @retval  FALSE  The REPORT_STATUS_CODE_PROPERTY_PROGRESS_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is clear.

**/
BOOLEAN
EFIAPI
GlueReportProgressCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdReportStatusCodePropertyMask) & REPORT_STATUS_CODE_PROPERTY_PROGRESS_CODE_ENABLED) != 0);
}


/**
  Returns TRUE if status codes of type EFI_ERROR_CODE are enabled

  This function returns TRUE if the REPORT_STATUS_CODE_PROPERTY_ERROR_CODE_ENABLED 
  bit of PcdReportStatusCodeProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE   The REPORT_STATUS_CODE_PROPERTY_ERROR_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is set.
  @retval  FALSE  The REPORT_STATUS_CODE_PROPERTY_ERROR_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is clear.

**/
BOOLEAN
EFIAPI
GlueReportErrorCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdReportStatusCodePropertyMask) & REPORT_STATUS_CODE_PROPERTY_ERROR_CODE_ENABLED) != 0);
}


/**
  Returns TRUE if status codes of type EFI_DEBUG_CODE are enabled

  This function returns TRUE if the REPORT_STATUS_CODE_PROPERTY_DEBUG_CODE_ENABLED 
  bit of PcdReportStatusCodeProperyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE   The REPORT_STATUS_CODE_PROPERTY_DEBUG_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is set.
  @retval  FALSE  The REPORT_STATUS_CODE_PROPERTY_DEBUG_CODE_ENABLED bit of 
                  PcdReportStatusCodeProperyMask is clear.

**/
BOOLEAN
EFIAPI
GlueReportDebugCodeEnabled (
  VOID
  )
{
  return (BOOLEAN) ((PcdGet8(PcdReportStatusCodePropertyMask) & REPORT_STATUS_CODE_PROPERTY_DEBUG_CODE_ENABLED) != 0);
}
