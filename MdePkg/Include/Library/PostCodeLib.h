/** @file
  Report Status Code Library public .h file

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __POST_CODE_LIB_H__
#define __POST_CODE_LIB_H__

#define POST_CODE_PROPERTY_POST_CODE_ENABLED              0x00000008
#define POST_CODE_PROPERTY_POST_CODE_DESCRIPTION_ENABLED  0x00000010

/**
  Sends an 32-bit value to a POST card.

  Sends the 32-bit value specified by Value to a POST card, and returns Value.  
  Some implementations of this library function may perform I/O operations 
  directly to a POST card device.  Other implementations may send Value to 
  ReportStatusCode(), and the status code reporting mechanism will eventually 
  display the 32-bit value on the status reporting device.
  
  PostCode() must actively prevent recursion.  If PostCode() is called while 
  processing another any other Report Status Code Library function, then 
  PostCode() must return Value immediately.

  @param  Value  The 32-bit value to write to the POST card.

  @return  Value

**/
UINT32
EFIAPI
PostCode (
  IN UINT32  Value
  );


/**
  Sends an 32-bit value to a POST and associated ASCII string.

  Sends the 32-bit value specified by Value to a POST card, and returns Value.
  If Description is not NULL, then the ASCII string specified by Description is 
  also passed to the handler that displays the POST card value.  Some 
  implementations of this library function may perform I/O operations directly 
  to a POST card device.  Other implementations may send Value to ReportStatusCode(), 
  and the status code reporting mechanism will eventually display the 32-bit 
  value on the status reporting device.  

  PostCodeWithDescription()must actively prevent recursion.  If 
  PostCodeWithDescription() is called while processing another any other Report 
  Status Code Library function, then PostCodeWithDescription() must return Value 
  immediately.

  @param  Value        The 32-bit value to write to the POST card.
  @param  Description  Pointer to an ASCII string that is a description of the 
                       POST code value.  This is an optional parameter that may 
                       be NULL.

  @return  Value

**/
UINT32
EFIAPI
PostCodeWithDescription (
  IN UINT32       Value,
  IN CONST CHAR8  *Description  OPTIONAL
  );


/**
  Returns TRUE if POST Codes are enabled.

  This function returns TRUE if the POST_CODE_PROPERTY_POST_CODE_ENABLED 
  bit of PcdPostCodePropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE   The POST_CODE_PROPERTY_POST_CODE_ENABLED bit of 
                  PcdPostCodeProperyMask is set.
  @retval  FALSE  The POST_CODE_PROPERTY_POST_CODE_ENABLED bit of 
                  PcdPostCodeProperyMask is clear.

**/
BOOLEAN
EFIAPI
PostCodeEnabled (
  VOID
  );


/**
  Returns TRUE if POST code descriptions are enabled.

  This function returns TRUE if the 
  POST_CODE_PROPERTY_POST_CODE_ENABLED bit of 
  PcdPostCodePropertyMask is set.  Otherwise FALSE is returned.

  @retval  TRUE   The POST_CODE_PROPERTY_POST_CODE_ENABLED 
                  bit of PcdPostCodeProperyMask is set.
  @retval  FALSE  The POST_CODE_PROPERTY_POST_CODE_ENABLED 
                  bit of PcdPostCodeProperyMask is clear.

**/
BOOLEAN
EFIAPI
PostCodeDescriptionEnabled (
  VOID
  );


/**
  Sends an 32-bit value to a POST card.

  If POST codes are enabled in PcdPostCodeProperyMask, then call PostCode() 
  passing in Value.  Value is returned.

  @param  Value  The 32-bit value to write to the POST card.

  @return  Value

**/
#define POST_CODE(Value)  ReportPostCodeEnabled() ? PostCode(Value) : Value

/**
  Sends an 32-bit value to a POST and associated ASCII string.

  If POST codes and POST code descriptions are enabled in 
  PcdPostCodeProperyMask, then call PostCodeWithDescription() passing in 
  Value and Description.  If only POST codes are enabled, then call PostCode() 
  passing in Value.  Value is returned.

  @param  Value        The 32-bit value to write to the POST card.
  @param  Description  Pointer to an ASCII string that is a description of the 
                       POST code value.

**/
#define POST_CODE_WITH_DESCRIPTION(Value,Description)  \
  ReportPostCodeEnabled()                        ?     \
    (ReportPostCodeDescriptionEnabled()          ?     \
      PostCodeWithDescription(Value,Description) :     \
      PostCode(Value))                           :     \
    Value

#endif
