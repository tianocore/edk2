/** @file

  The file provides services to call for drivers to leverage the
  EFI configuration driver interface.
  
  Copyright (c) 2006 - 2007, Intel Corporation
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_FORM_BROWSER_H__
#define __EFI_FORM_BROWSER_H__

#define EFI_FORM_BROWSER2_PROTOCOL_GUID \
  {0xb9d4c360, 0xbcfb, 0x4f9b, {0x92, 0x98, 0x53, 0xc1, 0x36, 0x98, 0x22, 0x58 }}


typedef struct _EFI_FORM_BROWSER2_PROTOCOL   EFI_FORM_BROWSER2_PROTOCOL;



/**
   
  @param LeftColumn   Value that designates the text column
                      where the browser window will begin from
                      the left-hand side of the screen
                      RightColumn Value that designates the text
                      column where the browser window will end
                      on the right-hand side of the screen.

  @param TopRow   Value that designates the text row from the
                  top of the screen where the browser window
                  will start.

  @param BottomRow  Value that designates the text row from the
                    bottom of the screen where the browser
                    window will end. 
**/
typedef struct {
  UINTN   LeftColumn;
  UINTN   RightColumn;
  UINTN   TopRow;
  UINTN   BottomRow;
} EFI_SCREEN_DESCRIPTOR;

typedef UINTN EFI_BROWSER_ACTION_REQUEST;

#define EFI_BROWSER_ACTION_REQUEST_NONE   0
#define EFI_BROWSER_ACTION_REQUEST_RESET  1
#define EFI_BROWSER_ACTION_REQUEST_SUBMIT 2
#define EFI_BROWSER_ACTION_REQUEST_EXIT   3


/**
   
  This function is the primary interface to the internal
  forms-based browser. By calling this routine, one is directing
  the browser to use a variety of passed-in information or
  primarily use the HII database as the source of information.

  @param This   A pointer to the EFI_FORM_BROWSER2_PROTOCOL
                instance.

  @param Handle   A pointer to an array of HII handles to
                  display. This value should correspond to the
                  value of the HII form package that is required
                  to be displayed.

  @param HandleCount  The number of handles in the array
                      specified by Handle.

  @param SingleUse  If FALSE, the browser operates as a standard
                    forms processor and exits only when
                    explicitly requested by the user. If TRUE,
                    the browser will return immediately after
                    processing the first user-generated
                    selection.

  @param ScreenDimensions   Allows the browser to be called so
                            that it occupies a portion of the
                            physical screen instead of
                            dynamically determining the screen
                            dimensions. If the input values
                            violate the platform policy then the
                            dimensions will be dynamically
                            adjusted to comply.

  @param ResetRequired  This BOOLEAN value will tell the caller
                        if a reset is required based on the data
                        that might have been changed. The
                        ResetRequired parameter is primarily
                        applicable for configuration
                        applications, and is an optional
                        parameter.

  @retval EFI_SUCCESS   The function completed successfully
  
  @retval EFI_NOT_FOUND   The variable was not found.
  
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for
                                the result. DataSize has been
                                updated with the size needed to
                                complete the request.
  
  @retval EFI_INVALID_PARAMETER   One of the parameters has an
                                  invalid value.
  
  @retval EFI_DEVICE_ERROR  The variable could not be saved due
                            to a hardware failure.
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_SEND_FORM2) (
  IN CONST  EFI_FORM_BROWSER2_PROTOCOL  *This,
  IN        EFI_HII_HANDLE              *Handle,
  IN        UINTN                      HandleCount,
  IN        EFI_GUID                   *FormSetGuid, OPTIONAL
  IN        EFI_FORM_ID                FormId, OPTIONAL
  IN CONST  EFI_SCREEN_DESCRIPTOR      *ScreenDimensions, OPTIONAL
  OUT       EFI_BROWSER_ACTION_REQUEST *ActionRequest  OPTIONAL
);


/**
   
  This routine is called by a routine which was called by the
  browser. This routine called this service in the browser to
  retrieve or set certain uncommitted state information.

  @param This   A pointer to the EFI_FORM_BROWSER2_PROTOCOL
                instance.

  @param ResultsDataSize  A pointer to the size of the buffer
                          associated with ResultsData. 

  @param ResultsData  A string returned from an IFR browser or
                      equivalent. The results string will have
                      no routing information in them.

  @param RetrieveData   A BOOLEAN field which allows an agent to
                        retrieve (if RetrieveData = TRUE) data
                        from the uncommitted browser state
                        information or set (if RetrieveData =
                        FALSE) data in the uncommitted browser
                        state information.

  @param VariableGuid   An optional field to indicate the target
                        variable GUID name to use.

  @param VariableName   An optional field to indicate the target
                        human-readable variable name.


  @retval EFI_SUCCESS   The results have been distributed or are
                        awaiting distribution.
  
  @retval EFI_OUT_OF_RESOURCES  The ResultsDataSize specified
                                was too small to contain the
                                results data.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_BROWSER_CALLBACK2 ) (
  IN CONST  EFI_FORM_BROWSER2_PROTOCOL *This,
  IN OUT    UINTN                     *ResultsDataSize,
  IN OUT    EFI_STRING                ResultsData,
  IN CONST  BOOLEAN                   RetrieveData,
  IN CONST  EFI_GUID                  *VariableGuid, OPTIONAL
  IN CONST  CHAR16                    *VariableName OPTIONAL
);

/**
   
  This protocol is the interface to call for drivers to leverage
  the EFI configuration driver interface.

  @param SendForm   Provides direction to the configuration
                    driver whether to use the HII database or to
                    use a passed-in set of data. This functions
                    also establishes a pointer to the calling
                    driver's callback interface. See the
                    SendForm() function description.

  @param BrowserCallback  Routine used to expose internal
                          configuration state of the browser.
                          This is primarily used by callback
                          handler routines which were called by
                          the browser and in-turn need to get
                          additional information from the
                          browser itself. See the
                          BrowserCallback() function
                          description.

**/
struct _EFI_FORM_BROWSER2_PROTOCOL {
  EFI_SEND_FORM2         SendForm;
  EFI_BROWSER_CALLBACK2  BrowserCallback;
} ;


extern EFI_GUID gEfiFormBrowser2ProtocolGuid;

#endif

