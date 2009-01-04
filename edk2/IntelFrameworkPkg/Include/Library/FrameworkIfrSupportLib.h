/** @file
  Library class name: FrameworkIfrSupportLib

  FrameworkIfrSupportLib is designed for produce IFR operation interface .
  The IFR format follows framework specification.
  
Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef _IFRSUPPORTLIBRARY_H_
#define _IFRSUPPORTLIBRARY_H_

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

/**
  Determine what is the current language setting.
  
  The setting is stored in language variable in flash. This routine
  will get setting by accesssing that variable. If failed to access
  language variable, then use default setting that 'eng' as current
  language setting.
  
  @param  Lang       Pointer of system language
  
  @return            whether success to get setting from variable
**/
EFI_STATUS
EFIAPI
GetCurrentLanguage (
  OUT     CHAR16              *Lang
  );

/**
  Add a string to the incoming buffer and return the token and offset data.
  
  @param StringBuffer      The incoming buffer
  @param Language          Currrent language
  @param String            The string to be added
  @param StringToken       The index where the string placed  
  
  @retval EFI_OUT_OF_RESOURCES No enough buffer to allocate
  @retval EFI_SUCCESS          String successfully added to the incoming buffer
**/
EFI_STATUS
EFIAPI
AddString (
  IN      VOID                *StringBuffer,
  IN      CHAR16              *Language,
  IN      CHAR16              *String,
  IN OUT  STRING_REF          *StringToken
  );

/**
  Add op-code data to the FormBuffer.
  
  @param FormBuffer        Form buffer to be inserted to
  @param OpCodeData        Op-code data to be inserted
  
  @retval EFI_OUT_OF_RESOURCES    No enough buffer to allocate
  @retval EFI_SUCCESS             Op-code data successfully inserted
**/
EFI_STATUS
EFIAPI
AddOpCode (
  IN      VOID                *FormBuffer,
  IN OUT  VOID                *OpCodeData
  );

/**
  Create a formset
  
  The form package is a collection of forms that are intended to describe the pages that will be
  displayed to the user.
  
  @param FormSetTitle      Title of formset
  @param Guid              Guid of formset
  @param Class             Class of formset
  @param SubClass          Sub class of formset
  @param FormBuffer        Pointer of the formset created
  @param StringBuffer      Pointer of FormSetTitile string created
  
  @retval EFI_OUT_OF_RESOURCES     No enough buffer to allocate
  @retval EFI_SUCCESS              Formset successfully created  
**/
EFI_STATUS
EFIAPI
CreateFormSet (
  IN      CHAR16              *FormSetTitle,
  IN      EFI_GUID            *Guid,
  IN      UINT8               Class,
  IN      UINT8               SubClass,
  IN OUT  VOID                **FormBuffer,
  IN OUT  VOID                **StringBuffer
  );

/**
  Create a form.
  A form is the encapsulation of what amounts to a browser page. The header defines a FormId,
  which is referenced by the form package, among others. It also defines a FormTitle, which is a
  string to be used as the title for the form
  
  @param FormTitle         Title of the form
  @param FormId            Id of the form
  @param FormBuffer        Pointer of the form created
  @param StringBuffer      Pointer of FormTitil string created
  
  @retval EFI_SUCCESS      Form successfully created
**/
EFI_STATUS
EFIAPI
CreateForm (
  IN      CHAR16              *FormTitle,
  IN      UINT16              FormId,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a SubTitle
  
  Subtitle strings are intended to be used by authors to separate sections of questions into semantic
  groups.
  
  @param SubTitle          Sub title to be created
  @param FormBuffer        Where this subtitle to add to
  @param StringBuffer      String buffer created for subtitle
  
  @retval EFI_SUCCESS      Subtitle successfully created
**/
EFI_STATUS
EFIAPI
CreateSubTitle (
  IN      CHAR16              *SubTitle,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a line of text
  Unlike HTML, text is simply another tag. 
  This tag type enables IFR to be more easily localized.
  
  @param String            First string of the text
  @param String2           Second string of the text
  @param String3           Help string of the text
  @param Flags             Flag of the text
  @param Key               Key of the text
  @param FormBuffer        The form where this text adds to
  @param StringBuffer      String buffer created for String, String2 and String3
  
  @retval EFI_SUCCESS      Text successfully created
**/
EFI_STATUS
EFIAPI
CreateText (
  IN      CHAR16              *String,
  IN      CHAR16              *String2,
  IN      CHAR16              *String3,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a hyperlink.
  
  @param FormId            Form ID of the hyperlink
  @param Prompt            Prompt of the hyperlink
  @param FormBuffer        The form where this hyperlink adds to
  @param StringBuffer      String buffer created for Prompt
  
  @retval EFI_SUCCESS      Hyperlink successfully created  
**/
EFI_STATUS
EFIAPI
CreateGoto (
  IN      UINT16              FormId,
  IN      CHAR16              *Prompt,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a one-of question with a set of options to choose from.  The
  OptionsList is a pointer to a null-terminated list of option descriptions.

  @param QuestionId        Question ID of the one-of box
  @param DataWidth         DataWidth of the one-of box
  @param Prompt            Prompt of the one-of box
  @param Help              Help of the one-of box
  @param OptionsList       Each string in it is an option of the one-of box
  @param OptionCount       Option string count
  @param FormBuffer        The form where this one-of box adds to
  @param StringBuffer      String buffer created for Prompt, Help and Option strings
  
  @retval EFI_DEVICE_ERROR DataWidth > 2
  @retval EFI_SUCCESS      One-Of box successfully created.
**/
EFI_STATUS
EFIAPI
CreateOneOf (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a one-of question with a set of options to choose from.  The
  OptionsList is a pointer to a null-terminated list of option descriptions.
  
  @param QuestionId        Question ID of the ordered list
  @param MaxEntries        MaxEntries of the ordered list
  @param Prompt            Prompt of the ordered list
  @param Help              Help of the ordered list
  @param OptionsList       Each string in it is an option of the ordered list
  @param OptionCount       Option string count
  @param FormBuffer        The form where this ordered list adds to
  @param StringBuffer      String buffer created for Prompt, Help and Option strings
  
  @retval EFI_SUCCESS      Ordered list successfully created.
**/
EFI_STATUS
EFIAPI
CreateOrderedList (
  IN      UINT16              QuestionId,
  IN      UINT8               MaxEntries,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a checkbox
  
  @param QuestionId         Question ID of the check box
  @param DataWidth          DataWidth of the check box
  @param Prompt             Prompt of the check box
  @param Help               Help of the check box  
  @param Flags              Flags of the check box
  @param FormBuffer         The form where this check box adds to
  @param StringBuffer       String buffer created for Prompt and Help.
  
  @retval  EFI_DEVICE_ERROR DataWidth > 1
  @retval EFI_SUCCESS       Check box successfully created
**/
EFI_STATUS
EFIAPI
CreateCheckBox (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      CHAR16              *Prompt,
  IN      CHAR16              *Help,
  IN      UINT8               Flags,
  IN OUT  VOID                *FormBuffer,
  IN OUT  VOID                *StringBuffer
  );

/**
  Create a numeric
  
  @param QuestionId        Question ID of the numeric
  @param DataWidth         DataWidth of the numeric
  @param Prompt            Prompt of the numeric
  @param Help              Help of the numeric
  @param Minimum           Minumun boundary of the numeric
  @param Maximum           Maximum boundary of the numeric
  @param Step              Step of the numeric
  @param Default           Default value
  @param Flags             Flags of the numeric
  @param Key               Key of the numeric
  @param FormBuffer        The form where this numeric adds to
  @param StringBuffer      String buffer created for Prompt and Help.

  @retval EFI_DEVICE_ERROR DataWidth > 2
  @retval EFI_SUCCESS      Numeric is successfully created  
**/
EFI_STATUS
EFIAPI
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
  );

/**
  Create a string.
  
  @param QuestionId        Question ID of the string
  @param DataWidth         DataWidth of the string
  @param Prompt            Prompt of the string
  @param Help              Help of the string
  @param MinSize           Min size boundary of the string
  @param MaxSize           Max size boundary of the string
  @param Flags             Flags of the string
  @param Key               Key of the string
  @param FormBuffer        The form where this string adds to
  @param StringBuffer      String buffer created for Prompt and Help.
  @retval EFI_SUCCESS      String successfully created.  
**/
EFI_STATUS
EFIAPI
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
  );

/**
  Extract information pertaining to the HiiHandle.
  
  @param HiiHandle         Hii handle
  @param ImageLength       For input, length of DefaultImage;
                           For output, length of actually required
  @param DefaultImage      Image buffer prepared by caller
  @param Guid              Guid information about the form 
  
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate
  @retval EFI_BUFFER_TOO_SMALL  DefualtImage has no enough ImageLength
  @retval EFI_SUCCESS           Successfully extract data from Hii database.
**/
EFI_STATUS
EFIAPI
ExtractDataFromHiiHandle (
  IN      FRAMEWORK_EFI_HII_HANDLE   HiiHandle,
  IN OUT  UINT16              *ImageLength,
  OUT     UINT8               *DefaultImage,
  OUT     EFI_GUID            *Guid
  );

/**
  Finds HII handle for given pack GUID previously registered with the HII.
  
  @param HiiProtocol       pointer to pointer to HII protocol interface.
                           If NULL, the interface will be found but not returned.
                           If it points to NULL, the interface will be found and
                           written back to the pointer that is pointed to.
  @param Guid              The GUID of the pack that registered with the HII.

  @return                  Handle to the HII pack previously registered by the memory driver.
**/
FRAMEWORK_EFI_HII_HANDLE
EFIAPI
FindHiiHandle (
  IN OUT EFI_HII_PROTOCOL    **HiiProtocol, OPTIONAL
  IN     EFI_GUID            *Guid
  );

/**
  Create a SubTitle opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param StringToken       StringToken of the subtitle
  @param FormBuffer        Output of subtitle as a form
  
  @retval EFI_SUCCESS      Subtitle created to be a form
**/
EFI_STATUS
EFIAPI
CreateSubTitleOpCode (
  IN      STRING_REF          StringToken,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a Text opcode independent of string creation.
  
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  @param StringToken       First string token of the text
  @param StringTokenTwo    Second string token of the text
  @param StringTokenThree  Help string token of the text
  @param Flags             Flag of the text
  @param Key               Key of the text
  @param FormBuffer        Output of text as a form

  @retval EFI_SUCCESS      Text created to be a form
**/
EFI_STATUS
EFIAPI
CreateTextOpCode (
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      STRING_REF          StringTokenThree,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a hyperlink opcode independent of string creation.
  
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  

  @param FormId            Form ID of the hyperlink
  @param StringToken       Prompt string token of the hyperlink
  @param StringTokenTwo    Help string token of the hyperlink
  @param Flags             Flags of the hyperlink
  @param Key               Key of the hyperlink
  @param FormBuffer        Output of hyperlink as a form
  @retval EFI_SUCCESS      Hyperlink created to be a form
**/
EFI_STATUS
EFIAPI
CreateGotoOpCode (
  IN      UINT16              FormId,
  IN      STRING_REF          StringToken,
  IN      STRING_REF          StringTokenTwo,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a one-of opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
  @param QuestionId        Question ID of the one-of box
  @param DataWidth         DataWidth of the one-of box
  @param PromptToken       Prompt string token of the one-of box
  @param HelpToken         Help string token of the one-of box
  @param OptionsList       Each string in it is an option of the one-of box
  @param OptionCount       Option string count
  @param FormBuffer        Output of One-Of box as a form
  

  @retval EFI_SUCCESS      One-Of box created to be a form
  @retval EFI_DEVICE_ERROR DataWidth > 2
**/
EFI_STATUS
EFIAPI
CreateOneOfOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a ordered list opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
  @param QuestionId        Question ID of the ordered list
  @param MaxEntries        MaxEntries of the ordered list
  @param PromptToken       Prompt string token of the ordered list
  @param HelpToken         Help string token of the ordered list
  @param OptionsList       Each string in it is an option of the ordered list
  @param OptionCount       Option string count
  @param FormBuffer        Output of ordered list as a form
  
  @retval EFI_SUCCESS      Ordered list created to be a form
**/
EFI_STATUS
EFIAPI
CreateOrderedListOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               MaxEntries,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      IFR_OPTION          *OptionsList,
  IN      UINTN               OptionCount,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a checkbox opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  @param QuestionId        Question ID of the check box
  @param DataWidth         DataWidth of the check box
  @param PromptToken       Prompt string token of the check box
  @param HelpToken         Help string token of the check box
  @param Flags             Flags of the check box
  @param Key               Key of the check box
  @param FormBuffer        Output of the check box as a form

  @retval EFI_SUCCESS      Checkbox created to be a form
  @retval EFI_DEVICE_ERROR DataWidth > 1
**/
EFI_STATUS
EFIAPI
CreateCheckBoxOpCode (
  IN      UINT16              QuestionId,
  IN      UINT8               DataWidth,
  IN      STRING_REF          PromptToken,
  IN      STRING_REF          HelpToken,
  IN      UINT8               Flags,
  IN      UINT16              Key,
  IN OUT  VOID                *FormBuffer
  );

/**
  Create a numeric opcode independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param QuestionId        Question ID of the numeric
  @param DataWidth         DataWidth of the numeric
  @param PromptToken       Prompt string token of the numeric
  @param HelpToken         Help string token of the numeric
  @param Minimum           Minumun boundary of the numeric
  @param Maximum           Maximum boundary of the numeric
  @param Step              Step of the numeric
  @param Default           Default value of the numeric
  @param Flags             Flags of the numeric
  @param Key               Key of the numeric
  @param FormBuffer        Output of the numeric as a form
 

  @retval EFI_SUCCESS       The numeric created to be a form.
  @retval EFI_DEVICE_ERROR  DataWidth > 2
**/
EFI_STATUS
EFIAPI
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
  );

/**
  Create a numeric opcode independent of string creation
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param QuestionId        Question ID of the string
  @param DataWidth         DataWidth of the string
  @param PromptToken       Prompt token of the string
  @param HelpToken         Help token of the string
  @param MinSize           Min size boundary of the string
  @param MaxSize           Max size boundary of the string
  @param Flags             Flags of the string
  @param Key               Key of the string
  @param FormBuffer        Output of the string as a form
   
  @retval EFI_SUCCESS      String created to be a form.
**/
EFI_STATUS
EFIAPI
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
  );

/**
  Validate that the data associated with the HiiHandle in NVRAM is within
  the reasonable parameters for that FormSet.  Values for strings and passwords
  are not verified due to their not having the equivalent of valid range settings.

  @param HiiHandle         Handle of the HII database entry to query

  @param Results           If return Status is EFI_SUCCESS, Results provides valid data
                           TRUE  = NVRAM Data is within parameters
                           FALSE = NVRAM Data is NOT within parameters
  @retval EFI_OUT_OF_RESOURCES  No enough buffer to allocate
  @retval EFI_SUCCESS           Data successfully validated
**/
EFI_STATUS
EFIAPI
ValidateDataFromHiiHandle (
  IN      FRAMEWORK_EFI_HII_HANDLE       HiiHandle,
  OUT     BOOLEAN             *Results
  );

/**
  Create a banner opcode.  This is primarily used by the FrontPage implementation from BDS.
  
  @param Title             Title of the banner
  @param LineNumber        LineNumber of the banner
  @param Alignment         Alignment of the banner
  @param FormBuffer        Output of banner as a form

  @retval EFI_SUCCESS      Banner created to be a form.
**/
EFI_STATUS
EFIAPI
CreateBannerOpCode (
  IN      UINT16              Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT  VOID                *FormBuffer
  );
 
/**
  Extracts a variable form a Pack.

  @param Pack              List of variables
  @param Name              Name of the variable/map
  @param Guid              GUID of the variable/map
  @param Id                The index of the variable/map to retrieve
  @param Var               Pointer to the variable/map
  @param Size              Size of the variable/map in bytes
**/
VOID
EFIAPI
EfiLibHiiVariablePackGetMap (
  IN    EFI_HII_VARIABLE_PACK *Pack,  
  OUT   CHAR16                **Name,  OPTIONAL
  OUT   EFI_GUID              **Guid,  OPTIONAL
  OUT   UINT16                *Id,     OPTIONAL
  OUT   VOID                  **Var,   OPTIONAL
  OUT   UINTN                 *Size    OPTIONAL
  );

/**
  Finds a count of the variables/maps in the List.

  @param List              List of variables

  @return                  The number of map count.
**/
UINTN
EFIAPI
EfiLibHiiVariablePackListGetMapCnt (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List
  );
  
/**
  type definition for the callback to be 
  used with EfiLibHiiVariablePackListForEachVar().

  @param Id                Variable/Map ID
  @param Name              Name of the variable/map
  @param Guid              GUID of the variable/map
  @param Var               Pointer to the variable/map
  @param Size              Size of the variable/map in bytes
**/
typedef VOID (EFI_LIB_HII_VARIABLE_PACK_LIST_CALLBACK) (
  IN CHAR16                   *Name,
  IN EFI_GUID                 *Guid,
  IN UINT16                   Id,
  IN VOID                     *Var,
  IN UINTN                    Size
  );

/**
  Will iterate all variable/maps as appearing 
  in List and for each, it will call the Callback.

  @param List              List of variables
  @param Callback          Routine to be called for each iterated variable.
**/
VOID
EFIAPI
EfiLibHiiVariablePackListForEachVar (
  IN    EFI_HII_VARIABLE_PACK_LIST               *List,
  IN    EFI_LIB_HII_VARIABLE_PACK_LIST_CALLBACK  *Callback
  );
 
/**
  Finds a variable form List given 
  the order number as appears in the List.

  @param Idx               The index of the variable/map to retrieve
  @param List              List of variables
  @param Name              Name of the variable/map
  @param Guid              GUID of the variable/map
  @param Id                Id of the variable/map
  @param Var               Pointer to the variable/map
  @param Size              Size of the variable/map in bytes

  @return EFI_SUCCESS      Variable is found, OUT parameters are valid
  @return EFI_NOT_FOUND    Variable is not found, OUT parameters are not valid
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariablePackListGetMapByIdx (
  IN    UINTN                         Idx,  
  IN    EFI_HII_VARIABLE_PACK_LIST    *List,  
  OUT   CHAR16                        **Name,  OPTIONAL
  OUT   EFI_GUID                      **Guid,  OPTIONAL
  OUT   UINT16                        *Id,     OPTIONAL
  OUT   VOID                          **Var,
  OUT   UINTN                         *Size
  );
 
/**
  Finds a variable form List given the 
  order number as appears in the List.

  @param Id                The ID of the variable/map to retrieve
  @param List              List of variables
  @param Name              Name of the variable/map
  @param Guid              GUID of the variable/map
  @param Var               Pointer to the variable/map
  @param Size              Size of the variable/map in bytes

  @retval EFI_SUCCESS      Variable is found, OUT parameters are valid
  @retval EFI_NOT_FOUND    Variable is not found, OUT parameters are not valid
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariablePackListGetMapById (
  IN    UINT16                        Id,  
  IN    EFI_HII_VARIABLE_PACK_LIST    *List,
  OUT   CHAR16                        **Name,  OPTIONAL
  OUT   EFI_GUID                      **Guid,  OPTIONAL
  OUT   VOID                          **Var,
  OUT   UINTN                         *Size
  );

/**
  Finds a variable form EFI_HII_VARIABLE_PACK_LIST given name and GUID.

  @param List              List of variables
  @param Name              Name of the variable/map to be found
  @param Guid              GUID of the variable/map to be found
  @param Id                Id of the variable/map to be found
  @param Var               Pointer to the variable/map found
  @param Size              Size of the variable/map in bytes found

  @retval EFI_SUCCESS      variable is found, OUT parameters are valid
  @retval EFI_NOT_FOUND    variable is not found, OUT parameters are not valid
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariablePackListGetMap (
  IN    EFI_HII_VARIABLE_PACK_LIST   *List,
  IN    CHAR16                       *Name,
  IN    EFI_GUID                     *Guid,
  OUT   UINT16                       *Id,
  OUT   VOID                         **Var, 
  OUT   UINTN                        *Size
  );

/**
  Finds out if a variable of specific Name/Guid/Size exists in NV. 
  If it does, it will retrieve it into the Var. 

  @param Name              Parameters of the variable to retrieve. Must match exactly.
  @param Guid              Parameters of the variable to retrieve. Must match exactly.
  @param Size              Parameters of the variable to retrieve. Must match exactly.
  @param Var               Variable will be retrieved into buffer pointed by this pointer.
                           If pointing to NULL, the buffer will be allocated. Caller is responsible for releasing the buffer.

  @retval EFI_SUCCESS      The variable of exact Name/Guid/Size parameters was retrieved and written to Var.
  @retval EFI_NOT_FOUND    The variable of this Name/Guid was not found in the NV.
  @retval EFI_LOAD_ERROR   The variable in the NV was of different size, or NV API returned error.
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariableRetrieveFromNv (
  IN  CHAR16                     *Name,
  IN  EFI_GUID                   *Guid,
  IN  UINTN                      Size,
  OUT VOID                       **Var
  );

/**
  Overrrides the variable with NV data if found.
  But it only does it if the Name ends with specified Suffix.
  For example, if Suffix="MyOverride" and the Name="XyzSetupMyOverride",
  the Suffix matches the end of Name, so the variable will be loaded from NV
  provided the variable exists and the GUID and Size matches.

  @param Suffix            Suffix the Name should end with.
  @param Name              Name of the variable to retrieve.
  @param Guid              Guid of the variable to retrieve.
  @param Size              Parameters of the variable to retrieve.
  @param Var               Variable will be retrieved into this buffer.
                           Caller is responsible for providing storage of exactly Size size in bytes.

  @retval EFI_SUCCESS           The variable was overriden with NV variable of same Name/Guid/Size.
  @retval EFI_INVALID_PARAMETER The name of the variable does not end with <Suffix>.
  @retval EFI_NOT_FOUND         The variable of this Name/Guid was not found in the NV.
  @retval EFI_LOAD_ERROR        The variable in the NV was of different size, or NV API returned error.
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariableOverrideIfSuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  );

/**
  Overrrides the variable with NV data if found.
  But it only does it if the NV contains the same variable with Name is appended with Suffix.  
  For example, if Suffix="MyOverride" and the Name="XyzSetup",
  the Suffix will be appended to the end of Name, and the variable with Name="XyzSetupMyOverride"
  will be loaded from NV provided the variable exists and the GUID and Size matches.

  @param Suffix            Suffix the variable will be appended with.
  @param Name              Parameters of the Name variable to retrieve.
  @param Guid              Parameters of the Guid variable to retrieve.
  @param Size              Parameters of the Size variable to retrieve.
  @param Var               Variable will be retrieved into this buffer.
                           Caller is responsible for providing storage of exactly Size size in bytes.

  @retval EFI_SUCCESS      The variable was overriden with NV variable of same Name/Guid/Size.
  @retval EFI_NOT_FOUND    The variable of this Name/Guid was not found in the NV.
  @retval EFI_LOAD_ERROR   The variable in the NV was of different size, or NV API returned error.
**/
EFI_STATUS
EFIAPI
EfiLibHiiVariableOverrideBySuffix (
  IN  CHAR16                 *Suffix,
  IN  CHAR16                 *Name,
  IN  EFI_GUID               *Guid,
  IN  UINTN                   Size,
  OUT VOID                   *Var
  );

#endif
