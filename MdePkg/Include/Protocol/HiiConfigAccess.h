/** @file

  The file provides services to forward results to PCOL-based
  handler if EFI HII results processing protocol invokes this
  protocol.
  
  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#ifndef __EFI_HII_CONFIG_ACCESS_H__
#define __EFI_HII_CONFIG_ACCESS_H__

#error "UEFI 2.1 HII is not fully implemented for now, Please don't include this file now."

#define EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID  \
  { 0x330d4706, 0xf2a0, 0x4e4f, { 0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85 } }

typedef struct _EFI_HII_CONFIG_ACCESS_PROTOCOL  EFI_HII_CONFIG_ACCESS_PROTOCOL;

/**
   
  This function allows the caller to request the current
  configuration for one or more named elements. The resulting
  string is in <ConfigAltResp> format. Any and all alternative
  configuration strings shall also be appended to the end of the
  current configuration string. If they are, they must appear
  after the current configuration. They must contain the same
  routing (GUID, NAME, PATH) as the current configuration string.
  They must have an additional description indicating the type of
  alternative configuration the string represents,
  "ALTCFG=<StringToken>". That <StringToken> (when
  converted from Hex UNICODE to binary) is a reference to a
  string in the associated string pack.

  @param This   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Request  A null-terminated Unicode string in
                  <ConfigRequest> format. Note that this
                  includes the routing information as well as
                  the configurable name / value pairs. It is
                  invalid for this string to be in
                  <MultiConfigRequest> format.

  @param Progress   On return, points to a character in the
                    Request string. Points to the string's null
                    terminator if request was successful. Points
                    to the most recent "&" before the first
                    failing name / value pair (or the beginning
                    of the string if the failure is in the first
                    name / value pair) if the request was not
                    successful

  @param Results  A null-terminated Unicode string in
                  <ConfigAltResp> format which has all values
                  filled in for the names in the Request string.
                  String to be allocated by the called function.

  @retval EFI_SUCCESS   The Results string is filled with the
                        values corresponding to all requested
                        names.

  @retval EFI_OUT_OF_MEMORY   Not enough memory to store the
                              parts of the results that must be
                              stored awaiting possible future
                              protocols.

  @retval EFI_INVALID_PARAMETER   For example, passing in a NULL
                                  for the Request parameter
                                  would result in this type of
                                  error. In this case, the
                                  Progress parameter would be
                                  set to NULL. 

  @retval EFI_NOT_FOUND   Routing data doesn't match any
                          known driver. Progress set to the
                          first character in the routing header.
                          Note: There is no requirement that the
                          driver validate the routing data. It
                          must skip the <ConfigHdr> in order to
                          process the names.

  @retval EFI_INVALID_PARAMETER   Illegal syntax. Progress set
                                  to most recent & before the
                                  error or the beginning of the
                                  string.

  @retval EFI_INVALID_PARAMETER   Unknown name. Progress points
                                  to the & before the name in
                                  question.

**/
typedef
EFI_STATUS
(EFIAPI * EFI_HII_ACCESS_EXTRACT_CONFIG ) (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Request,
  OUT       EFI_STRING                      *Progress,
  OUT       EFI_STRING                      *Results
);


/**
   
  This function applies changes in a driver's configuration.
  Input is a Configuration, which has the routing data for this
  driver followed by name / value configuration pairs. The driver
  must apply those pairs to its configurable storage. If the
  driver's configuration is stored in a linear block of data
  and the driver????s name / value pairs are in <BlockConfig>
  format, it may use the ConfigToBlock helper function (above) to
  simplify the job.

  @param This   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param Configuration  A null-terminated Unicode string in
                        <ConfigString> format. Progress a
                        pointer to a string filled in with the
                        offset of the most recent '&' before the
                        first failing name / value pair (or the
                        beginn ing of the string if the failure
                        is in the first name / value pair) or
                        the terminating NULL if all was
                        successful.

  @retval EFI_SUCCESS   The results have been distributed or are
                        awaiting distribution.
  
  @retval EFI_OUT_OF_MEMORY   Not enough memory to store the
                              parts of the results that must be
                              stored awaiting possible future
                              protocols.
  
  @retval EFI_INVALID_PARAMETERS  Passing in a NULL for the
                                  Results parameter would result
                                  in this type of error.
  
  @retval EFI_NOT_FOUND   Target for the specified routing data
                          was not found

**/
typedef
EFI_STATUS
(EFIAPI * EFI_HII_ACCESS_ROUTE_CONFIG ) (
  IN CONST  EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST  EFI_STRING                      Configuration,
  OUT       EFI_STRING                      *Progress
);

/**
   
  This function is called to provide results data to the driver.
  This data consists of a unique key that is used to identify
  which data is either being passed back or being asked for.

  @param This   Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.

  @param KeyValue   A unique value which is sent to the original
                    exporting driver so that it can identify the
                    type of data to expect. The format of the
                    data tends to vary based on the opcode that
                    generated the callback.

  @param Data   A pointer to the data being sent to the original
                exporting driver. The format of the data should
                be the same as that of the question invoking the
                callback and will be known to the recipient.

  @retval EFI_SUCCESS   The firmware has successfully stored the
                        variable and its data as defined by the
                        Attributes.

  @retval EFI_INVALID_PARAMETER   An invalid combination of
                                  Attributes bits was supplied,
                                  or the DataSize exceeds the
                                  maximum allowed.

  @retval EFI_OUT_OF_RESOURCES  Not enough storage is available
                                to hold the variable and its
                                data.

  @retval EFI_DEVICE_ERROR  The variable could not be saved due
                            to a hardware failure.


**/
typedef
EFI_STATUS
(EFIAPI *EFI_HII_ACCESS_FORM_CALLBACK) (
  IN  CONST EFI_HII_CONFIG_ACCESS_PROTOCOL   *This,
  IN  EFI_BROWSER_ACTION                     Action,
  IN  EFI_QUESTION_ID                        QuestionId,
  IN  UINT8                                  Type,
  IN  EFI_IFR_TYPE_VALUE                     *Value,
  OUT EFI_BROWSER_ACTION_REQUEST             *ActionRequest
  )
  ;
/**
   
  This protocol provides a callable interface between the HII and
  drivers. Only drivers which provide IFR data to HII are required
  to publish this protocol.

  @param ExtractConfig  This function breaks apart the UNICODE
                        request strings routing them to the
                        appropriate drivers. This function is
                        analogous to the similarly named
                        function in the HII Routing Protocol.
  
  @param RouteConfig  This function breaks apart the UNICODE
                      results strings and returns configuration
                      information as specified by the request.
  
  @param Callback   This function is called from the
                    configuration browser to communicate certain
                    activities that were initiated by a user.


**/
struct _EFI_HII_CONFIG_ACCESS_PROTOCOL {
  EFI_HII_ACCESS_EXTRACT_CONFIG     ExtractConfig;
  EFI_HII_ACCESS_ROUTE_CONFIG       RouteConfig;
  EFI_HII_ACCESS_FORM_CALLBACK      Callback;
} ;

extern EFI_GUID gEfiHiiConfigAccessProtocolGuid;

#endif

