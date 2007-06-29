/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  IfrSupportLib.h

Abstract:

  The file contain all library function for Ifr Operations.

--*/

#ifndef _IFRSUPPORTLIBRARY_H
#define _IFRSUPPORTLIBRARY_H

#define DEFAULT_FORM_BUFFER_SIZE    0xFFFF
#define DEFAULT_STRING_BUFFER_SIZE  0xFFFF

#pragma pack(1)
typedef struct {
  CHAR16      *OptionString;  // Passed in string to generate a token for in a truly dynamic form creation
  STRING_REF  StringToken;    // This is used when creating a single op-code without generating a StringToken (have one already)
  UINT16      Value;
  UINT8       Flags;
  UINT16      Key;
} IFR_OPTION;
#pragma pack()

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR16              *Lang
  )
/*++

Routine Description:

  Determine what is the current language setting
  
Arguments:

  Lang      - Pointer of system language
  
Returns: 
  
  Status code

--*/
;

EFI_STATUS
AddString (
  IN      VOID                *StringBuffer,
  IN      CHAR16              *Language,
  IN      CHAR16              *String,
  IN OUT  STRING_REF          *StringToken
  )
/*++

Routine Description:

  Add a string to the incoming buffer and return the token and offset data
  
Arguments:

  StringBuffer      - The incoming buffer
  
  Language          - Currrent language
  
  String            - The string to be added
  
  StringToken       - The index where the string placed
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_SUCCESS             - String successfully added to the incoming buffer

--*/
;

EFI_STATUS
AddOpCode (
  IN      VOID                *FormBuffer,
  IN OUT  VOID                *OpCodeData
  )
/*++

Routine Description:

  Add op-code data to the FormBuffer
  
Arguments:

  FormBuffer      - Form buffer to be inserted to
  
  OpCodeData      - Op-code data to be inserted
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_SUCCESS             - Op-code data successfully inserted

--*/
;

EFI_STATUS
CreateFormSet (
  IN      CHAR16              *FormSetTitle,
  IN      EFI_GUID            *Guid,
  IN      UINT8               Class,
  IN      UINT8               SubClass,
  IN OUT  VOID                **FormBuffer,
  IN OUT  VOID                **StringBuffer
  )
/*++

Routine Description:

  Create a formset
  
Arguments:

  FormSetTitle        - Title of formset
  
  Guid                - Guid of formset
  
  Class               - Class of formset
  
  SubClass            - Sub class of formset
  
  FormBuffer          - Pointer of the formset created
  
  StringBuffer        - Pointer of FormSetTitile string created
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_SUCCESS             - Formset successfully created

--*/
;

EFI_STATUS
CreateForm (
  IN      CHAR16              *FormTitle,
  IN      UINT16              FormId,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a form
  
Arguments:

  FormTitle       - Title of the form
  
  FormId          - Id of the form
  
  FormBuffer          - Pointer of the form created
  
  StringBuffer        - Pointer of FormTitil string created
  
Returns: 

  EFI_SUCCESS     - Form successfully created

--*/
;

EFI_STATUS
CreateSubTitle (
  IN      CHAR16              *SubTitle,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a SubTitle
  
Arguments:

  SubTitle        - Sub title to be created
  
  FormBuffer      - Where this subtitle to add to
  
  StringBuffer    - String buffer created for subtitle
  
Returns: 

  EFI_SUCCESS     - Subtitle successfully created

--*/
;

EFI_STATUS
CreateText (
  IN      CHAR16              *String,
  IN      CHAR16              *String2,
  IN      CHAR16              *String3,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a line of text
  
Arguments:

  String          - First string of the text
  
  String2         - Second string of the text
  
  String3         - Help string of the text
  
  Flags           - Flag of the text
  
  Key             - Key of the text
  
  FormBuffer      - The form where this text adds to
  
  StringBuffer    - String buffer created for String, String2 and String3
  
Returns: 

  EFI_SUCCESS     - Text successfully created

--*/
;

EFI_STATUS
CreateGoto (
  IN      UINT16              FormId,
  IN      CHAR16              *Prompt,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a hyperlink
  
Arguments:

  FormId        - Form ID of the hyperlink
  
  Prompt        - Prompt of the hyperlink
  
  FormBuffer    - The form where this hyperlink adds to
  
  StringBuffer  - String buffer created for Prompt
  
Returns: 

  EFI_SUCCESS     - Hyperlink successfully created

--*/
;

EFI_STATUS
CreateOneOf (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a one-of question with a set of options to choose from.  The
  OptionsList is a pointer to a null-terminated list of option descriptions.
  
Arguments:

  QuestionId      - Question ID of the one-of box
  
  DataWidth       - DataWidth of the one-of box
  
  Prompt          - Prompt of the one-of box
  
  Help            - Help of the one-of box
  
  OptionsList     - Each string in it is an option of the one-of box
  
  OptionCount     - Option string count
  
  FormBuffer      - The form where this one-of box adds to
  
  StringBuffer    - String buffer created for Prompt, Help and Option strings
  
Returns: 

  EFI_DEVICE_ERROR    - DataWidth > 2

  EFI_SUCCESS         - One-Of box successfully created.

--*/
;

EFI_STATUS
CreateOrderedList (
  IN      UINT16              QuestionId,
  IN      UINT8               MaxEntries,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a one-of question with a set of options to choose from.  The
  OptionsList is a pointer to a null-terminated list of option descriptions.
  
Arguments:

  QuestionId      - Question ID of the ordered list
  
  MaxEntries      - MaxEntries of the ordered list
  
  Prompt          - Prompt of the ordered list
  
  Help            - Help of the ordered list
  
  OptionsList     - Each string in it is an option of the ordered list
  
  OptionCount     - Option string count
  
  FormBuffer      - The form where this ordered list adds to
  
  StringBuffer    - String buffer created for Prompt, Help and Option strings
  
Returns: 

  EFI_SUCCESS     - Ordered list successfully created.

--*/
;

EFI_STATUS
CreateCheckBox (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      UINT8               Flags,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a checkbox
  
Arguments:

  QuestionId      - Question ID of the check box
  
  DataWidth       - DataWidth of the check box
  
  Prompt          - Prompt of the check box
  
  Help            - Help of the check box
  
  Flags           - Flags of the check box
  
  FormBuffer      - The form where this check box adds to
  
  StringBuffer    - String buffer created for Prompt and Help.
  
Returns: 

  EFI_DEVICE_ERROR    - DataWidth > 1

  EFI_SUCCESS         - Check box successfully created

--*/
;

EFI_STATUS
CreateNumeric (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      UINT16              Minimum,
  IN      UINT16              Maximum,
  IN      UINT16              Step,
  IN      UINT16              Default,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a numeric
  
Arguments:

  QuestionId      - Question ID of the numeric
  
  DataWidth       - DataWidth of the numeric
  
  Prompt          - Prompt of the numeric
  
  Help            - Help of the numeric
  
  Minimum         - Minumun boundary of the numeric
  
  Maximum         - Maximum boundary of the numeric
  
  Step            - Step of the numeric
   
  Default         - Default value
  
  Flags           - Flags of the numeric
  
  Key             - Key of the numeric
  
  FormBuffer      - The form where this numeric adds to
  
  StringBuffer    - String buffer created for Prompt and Help.
  
Returns: 

  EFI_DEVICE_ERROR      - DataWidth > 2
  
  EFI_SUCCESS           - Numeric is successfully created

--*/
;

EFI_STATUS
CreateString (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      UINT8               MinSize,
  IN      UINT8               MaxSize,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  )
/*++

Routine Description:

  Create a string
  
Arguments:

  QuestionId      - Question ID of the string
  
  DataWidth       - DataWidth of the string
  
  Prompt          - Prompt of the string
  
  Help            - Help of the string
  
  MinSize         - Min size boundary of the string
  
  MaxSize         - Max size boundary of the string
    
  Flags           - Flags of the string
  
  Key             - Key of the string
  
  FormBuffer      - The form where this string adds to
  
  StringBuffer    - String buffer created for Prompt and Help.
  
Returns: 

  EFI_SUCCESS     - String successfully created.

--*/
;

EFI_STATUS
ExtractDataFromHiiHandle (
  IN      EFI_HII_HANDLE      HiiHandle,
  IN OUT  UINT16              *ImageLength,
  OUT     UINT8               *DefaultImage,
  OUT     EFI_GUID            *Guid
  )
/*++

Routine Description:

  Extract information pertaining to the HiiHandle
  
Arguments:

  HiiHandle       - Hii handle
  
  ImageLength     - For input, length of DefaultImage;
                    For output, length of actually required
                    
  DefaultImage    - Image buffer prepared by caller
  
  Guid            - Guid information about the form
  
Returns: 

  EFI_OUT_OF_RESOURCES    - No enough buffer to allocate
  
  EFI_BUFFER_TOO_SMALL    - DefualtImage has no enough ImageLength
  
  EFI_SUCCESS             - Successfully extract data from Hii database.
  
  
--*/
;

EFI_HII_HANDLE
FindHiiHandle (
  IN OUT EFI_HII_PROTOCOL    **HiiProtocol, OPTIONAL
  IN     EFI_GUID            *Guid
  )
/*++

Routine Description:
  Finds HII handle for given pack GUID previously registered with the HII.

Arguments:
  HiiProtocol - pointer to pointer to HII protocol interface. 
                If NULL, the interface will be found but not returned.
                If it points to NULL, the interface will be found and 
                written back to the pointer that is pointed to.
  Guid        - The GUID of the pack that registered with the HII.

Returns:
  Handle to the HII pack previously registered by the memory driver.

--*/
;

EFI_STATUS
CreateSubTitleOpCode (
  IN      STRING_REF          StringToken,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a SubTitle opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  StringToken     - StringToken of the subtitle
  
  FormBuffer      - Output of subtitle as a form
  
Returns: 

  EFI_SUCCESS     - Subtitle created to be a form

--*/
;

EFI_STATUS
CreateTextOpCode (
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      STRING_REF          StringTokenThree,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a Text opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  StringToken               - First string token of the text
  
  StringTokenTwo            - Second string token of the text
  
  StringTokenThree          - Help string token of the text
  
  Flags                     - Flag of the text
  
  Key                       - Key of the text
  
  FormBuffer                - Output of text as a form
  
Returns: 

  EFI_SUCCESS       - Text created to be a form

--*/
;

EFI_STATUS
CreateGotoOpCode (
  IN      UINT16              FormId,
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a hyperlink opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  FormId          - Form ID of the hyperlink
  
  StringToken     - Prompt string token of the hyperlink
  
  StringTokenTwo  - Help string token of the hyperlink
  
  Flags           - Flags of the hyperlink
  
  Key             - Key of the hyperlink
  
  FormBuffer      - Output of hyperlink as a form
  
Returns: 

  EFI_SUCCESS   - Hyperlink created to be a form

--*/
;

EFI_STATUS
CreateOneOfOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a one-of opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
Arguments:

  QuestionId      - Question ID of the one-of box
  
  DataWidth       - DataWidth of the one-of box
  
  PromptToken     - Prompt string token of the one-of box
  
  HelpToken       - Help string token of the one-of box
  
  OptionsList     - Each string in it is an option of the one-of box
  
  OptionCount     - Option string count
  
  FormBuffer      - Output of One-Of box as a form
  
Returns: 

  EFI_SUCCESS         - One-Of box created to be a form
  
  EFI_DEVICE_ERROR    - DataWidth > 2

--*/
;

EFI_STATUS
CreateOrderedListOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               MaxEntries,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a ordered list opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
Arguments:

  QuestionId      - Question ID of the ordered list
  
  MaxEntries      - MaxEntries of the ordered list
  
  PromptToken     - Prompt string token of the ordered list
  
  HelpToken       - Help string token of the ordered list
  
  OptionsList     - Each string in it is an option of the ordered list
  
  OptionCount     - Option string count
  
  FormBuffer      - Output of ordered list as a form
  
Returns: 

  EFI_SUCCESS     - Ordered list created to be a form

--*/
;

EFI_STATUS
CreateCheckBoxOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a checkbox opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  QuestionId      - Question ID of the check box
  
  DataWidth       - DataWidth of the check box
  
  PromptToken     - Prompt string token of the check box
  
  HelpToken       - Help string token of the check box
  
  Flags           - Flags of the check box
  
  Key             - Key of the check box
  
  FormBuffer      - Output of the check box as a form
  
Returns: 

  EFI_SUCCESS       - Checkbox created to be a form
  
  EFI_DEVICE_ERROR  - DataWidth > 1

--*/
;

EFI_STATUS
CreateNumericOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT16              Minimum,
  IN      UINT16              Maximum,
  IN      UINT16              Step,
  IN      UINT16              Default,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a numeric opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  QuestionId      - Question ID of the numeric
  
  DataWidth       - DataWidth of the numeric
  
  PromptToken     - Prompt string token of the numeric
  
  HelpToken       - Help string token of the numeric
  
  Minimum         - Minumun boundary of the numeric
  
  Maximum         - Maximum boundary of the numeric
  
  Step            - Step of the numeric
  
  Default         - Default value of the numeric
  
  Flags           - Flags of the numeric
  
  Key             - Key of the numeric
  
  FormBuffer      - Output of the numeric as a form
  
Returns: 

  EFI_SUCCESS       - The numeric created to be a form.
  
  EFI_DEVICE_ERROR  - DataWidth > 2

--*/
;

EFI_STATUS
CreateStringOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT8               MinSize,
  IN      UINT8               MaxSize,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a numeric opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
Arguments:

  QuestionId      - Question ID of the string
  
  DataWidth       - DataWidth of the string
  
  PromptToken     - Prompt token of the string
  
  HelpToken       - Help token of the string
  
  MinSize         - Min size boundary of the string
  
  MaxSize         - Max size boundary of the string
    
  Flags           - Flags of the string
  
  Key             - Key of the string
  
  FormBuffer      - Output of the string as a form
  
Returns: 

  EFI_SUCCESS       - String created to be a form.

--*/
;

EFI_STATUS
ValidateDataFromHiiHandle (
  IN      EFI_HII_HANDLE      HiiHandle,
  OUT     BOOLEAN             *Results
  )
/*++

Routine Description:

  Validate that the data associated with the HiiHandle in NVRAM is within
  the reasonable parameters for that FormSet.  Values for strings and passwords
  are not verified due to their not having the equivalent of valid range settings.
  
Arguments:

  HiiHandle -   Handle of the HII database entry to query

  Results -     If return Status is EFI_SUCCESS, Results provides valid data
                TRUE  = NVRAM Data is within parameters
                FALSE = NVRAM Data is NOT within parameters
  
Returns: 

  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Data successfully validated
--*/
;

EFI_STATUS
CreateBannerOpCode (
  IN      UINT16              Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT  VOID                *FormBuffer
  )
/*++

Routine Description:

  Create a banner opcode.  This is primarily used by the FrontPage implementation from BDS.
  
Arguments:

  Title       - Title of the banner
  
  LineNumber  - LineNumber of the banner
  
  Alignment   - Alignment of the banner
  
  FormBuffer  - Output of banner as a form
  
Returns: 

  EFI_SUCCESS     - Banner created to be a form.

--*/
;

VOID
EfiLibHiiVariablePackGetMap (
  IN    EFI_HII_VARIABLE_PACK        *Pack,  
  OUT   CHAR16                       **Name,  OPTIONAL
  OUT   EFI_GUID                     **Guid,  OPTIONAL
  OUT   UINT16                       *Id,     OPTIONAL
  OUT   VOID                         **Var,   OPTIONAL
  OUT   UINTN                        *Size    OPTIONAL
  ) 
/*++

Routine Description:

  Extracts a variable form a Pack.

Arguments:

  Pack - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns: 

  VOID.

--*/
;

UINTN
EfiLibHiiVariablePackListGetMapCnt (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List
  )
/*++

Routine Description:

  Finds a count of the variables/maps in the List.

Arguments:

  List - List of variables

Returns: 

  Number of Map in the variable pack list.

--*/
;

typedef VOID (EFI_LIB_HII_VARIABLE_PACK_LIST_CALLBACK) (
  IN CHAR16                      *Name,
  IN EFI_GUID                    *Guid,
  IN UINT16                      Id,
  IN VOID                        *Var,
  IN UINTN                       Size
  )  
/*++

Routine Description:

  type definition for the callback to be 
  used with EfiLibHiiVariablePackListForEachVar().

Arguments:

  Id   - Variable/Map ID
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns: 

  VOID

--*/
;

VOID
EfiLibHiiVariablePackListForEachVar (
  IN    EFI_HII_VARIABLE_PACK_LIST               *List,
  IN    EFI_LIB_HII_VARIABLE_PACK_LIST_CALLBACK  *Callback
  )
/*++

Routine Description:

  Will iterate all variable/maps as appearing 
  in List and for each, it will call the Callback.

Arguments:

  List     - List of variables
  Callback - Routine to be called for each iterated variable.

Returns: 

  VOID

--*/
;

EFI_STATUS
EfiLibHiiVariablePackListGetMapByIdx (
  IN    UINTN                         Idx,  
  IN    EFI_HII_VARIABLE_PACK_LIST    *List,  
  OUT   CHAR16                        **Name,  OPTIONAL
  OUT   EFI_GUID                      **Guid,  OPTIONAL
  OUT   UINT16                        *Id,    OPTIONAL
  OUT   VOID                          **Var,
  OUT   UINTN                         *Size
  ) 
/*++

Routine Description:

  Finds a variable form List given 
  the order number as appears in the List.

Arguments:

  Idx  - The index of the variable/map to retrieve
  List - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns:

  EFI_SUCCESS   - Variable is found, OUT parameters are valid
  EFI_NOT_FOUND - Variable is not found, OUT parameters are not valid

--*/
;

EFI_STATUS
EfiLibHiiVariablePackListGetMapById (
  IN    UINT16                        Id,  
  IN    EFI_HII_VARIABLE_PACK_LIST    *List,
  OUT   CHAR16                        **Name,  OPTIONAL
  OUT   EFI_GUID                      **Guid,  OPTIONAL
  OUT   VOID                          **Var,
  OUT   UINTN                         *Size
  ) 
/*++

Routine Description:

  Finds a variable form List given the 
  order number as appears in the List.

Arguments:

  Id   - The ID of the variable/map to retrieve
  List - List of variables
  Name - Name of the variable/map
  Guid - GUID of the variable/map
  Var  - Pointer to the variable/map
  Size - Size of the variable/map in bytes

Returns:

  EFI_SUCCESS   - Variable is found, OUT parameters are valid
  EFI_NOT_FOUND - Variable is not found, OUT parameters are not valid

--*/
;

EFI_STATUS
EfiLibHiiVariablePackListGetMap (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List,
  IN    CHAR16                       *Name,
  IN    EFI_GUID                     *Guid,
  OUT   UINT16                       *Id,
  OUT   VOID                         **Var, 
  OUT   UINTN                        *Size
  ) 
/*++

Routine Description:

  Finds a variable form EFI_HII_VARIABLE_PACK_LIST given name and GUID.

Arguments:

  List - List of variables
  Name - Name of the variable/map to be found
  Guid - GUID of the variable/map to be found
  Var  - Pointer to the variable/map found
  Size - Size of the variable/map in bytes found

Returns:

  EFI_SUCCESS   - variable is found, OUT parameters are valid
  EFI_NOT_FOUND - variable is not found, OUT parameters are not valid

--*/
;

EFI_STATUS
EfiLibHiiVariableRetrieveFromNv (
  IN  CHAR16                     *Name,
  IN  EFI_GUID                   *Guid,
  IN  UINTN                      Size,
  OUT VOID                       **Var
  )
/*++

Routine Description:
  Finds out if a variable of specific Name/Guid/Size exists in NV. 
  If it does, it will retrieve it into the Var. 

Arguments:
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into buffer pointed by this pointer.
                     If pointing to NULL, the buffer will be allocated. Caller is responsible for releasing the buffer.
Returns:
  EFI_SUCCESS    - The variable of exact Name/Guid/Size parameters was retrieved and written to Var.
  EFI_NOT_FOUND  - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR - The variable in the NV was of different size, or NV API returned error.

--*/
;

////
//// Variable override support.
////

EFI_STATUS
EfiLibHiiVariableOverrideIfSuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  )  
/*++

Routine Description:
  Overrrides the variable with NV data if found.
  But it only does it if the Name ends with specified Suffix.
  For example, if Suffix="MyOverride" and the Name="XyzSetupMyOverride",
  the Suffix matches the end of Name, so the variable will be loaded from NV
  provided the variable exists and the GUID and Size matches.

Arguments:
  Suffix           - Suffix the Name should end with.
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into this buffer.
                     Caller is responsible for providing storage of exactly Size size in bytes.
Returns:
  EFI_SUCCESS           - The variable was overriden with NV variable of same Name/Guid/Size.
  EFI_INVALID_PARAMETER - The name of the variable does not end with <Suffix>.
  EFI_NOT_FOUND         - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR        - The variable in the NV was of different size, or NV API returned error.

--*/
;

EFI_STATUS
EfiLibHiiVariableOverrideBySuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  ) 
/*++

Routine Description:
  Overrrides the variable with NV data if found.
  But it only does it if the NV contains the same variable with Name is appended with Suffix.  
  For example, if Suffix="MyOverride" and the Name="XyzSetup",
  the Suffix will be appended to the end of Name, and the variable with Name="XyzSetupMyOverride"
  will be loaded from NV provided the variable exists and the GUID and Size matches.

Arguments:
  Suffix           - Suffix the variable will be appended with.
  Name, Guid, Size - Parameters of the variable to retrieve. Must match exactly.
  Var              - Variable will be retrieved into this buffer.
                     Caller is responsible for providing storage of exactly Size size in bytes.

Returns:
  EFI_SUCCESS    - The variable was overriden with NV variable of same Name/Guid/Size.
  EFI_NOT_FOUND  - The variable of this Name/Guid was not found in the NV.
  EFI_LOAD_ERROR - The variable in the NV was of different size, or NV API returned error.

--*/
;

#endif
