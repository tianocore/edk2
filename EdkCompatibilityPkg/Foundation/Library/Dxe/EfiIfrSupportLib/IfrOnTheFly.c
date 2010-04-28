/*++
Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IfrOnTheFly.c

Abstract:

  Library Routines to create IFR on-the-fly

Revision History:

--*/

#include "IfrLibrary.h"

#ifdef SUPPORT_DEPRECATED_IFRSUPPORTLIB_API

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
{
  EFI_STATUS            Status;
  EFI_HII_IFR_PACK      IfrPack;
  EFI_IFR_FORM_SET      FormSet;
  EFI_IFR_END_FORM_SET  EndFormSet;
  UINT8                 *Destination;
  CHAR16                CurrentLanguage[4];
  STRING_REF            StringToken;

  //
  // Pre-allocate a buffer sufficient for us to work from.
  //
  FormBuffer = EfiLibAllocateZeroPool (DEFAULT_FORM_BUFFER_SIZE);
  if (FormBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Pre-allocate a buffer sufficient for us to work from.
  //
  StringBuffer = EfiLibAllocateZeroPool (DEFAULT_STRING_BUFFER_SIZE);
  if (StringBuffer == NULL) {
    gBS->FreePool (FormBuffer);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add the FormSetTitle to the string buffer and get the StringToken
  //
  Status = AddString (*StringBuffer, CurrentLanguage, FormSetTitle, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Initialize the Ifr Package header data
  //
  IfrPack.Header.Length = sizeof (EFI_HII_PACK_HEADER) + sizeof (EFI_IFR_FORM_SET) + sizeof (EFI_IFR_END_FORM_SET);
  IfrPack.Header.Type   = EFI_HII_IFR;

  //
  // Initialize FormSet with the appropriate information
  //
  FormSet.Header.OpCode = EFI_IFR_FORM_SET_OP;
  FormSet.Header.Length = sizeof (EFI_IFR_FORM_SET);
  FormSet.FormSetTitle  = StringToken;
  FormSet.Class         = Class;
  FormSet.SubClass      = SubClass;
  EfiCopyMem (&FormSet.Guid, Guid, sizeof (EFI_GUID));

  //
  // Initialize the end formset data
  //
  EndFormSet.Header.Length  = sizeof (EFI_IFR_END_FORM_SET);
  EndFormSet.Header.OpCode  = EFI_IFR_END_FORM_SET_OP;

  Destination               = (CHAR8 *) *FormBuffer;

  //
  // Copy the formset/endformset data to the form buffer
  //
  EfiCopyMem (Destination, &IfrPack, sizeof (EFI_HII_PACK_HEADER));

  Destination = Destination + sizeof (EFI_HII_PACK_HEADER);

  EfiCopyMem (Destination, &FormSet, sizeof (EFI_IFR_FORM_SET));

  Destination = Destination + sizeof (EFI_IFR_FORM_SET);

  EfiCopyMem (Destination, &EndFormSet, sizeof (EFI_IFR_END_FORM_SET));
  return EFI_SUCCESS;
}


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
{
  EFI_STATUS        Status;
  EFI_IFR_FORM      Form;
  EFI_IFR_END_FORM  EndForm;
  CHAR16            CurrentLanguage[4];
  STRING_REF        StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  Status = AddString (StringBuffer, CurrentLanguage, FormTitle, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Form.Header.OpCode  = EFI_IFR_FORM_OP;
  Form.Header.Length  = sizeof (EFI_IFR_FORM);
  Form.FormId         = FormId;
  Form.FormTitle      = StringToken;

  Status              = AddOpCode (FormBuffer, &Form);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  EndForm.Header.OpCode = EFI_IFR_END_FORM_OP;
  EndForm.Header.Length = sizeof (EFI_IFR_END_FORM);

  Status                = AddOpCode (FormBuffer, &EndForm);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS        Status;
  EFI_IFR_SUBTITLE  Subtitle;
  CHAR16            CurrentLanguage[4];
  STRING_REF        StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  Status = AddString (StringBuffer, CurrentLanguage, SubTitle, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Subtitle.Header.OpCode  = EFI_IFR_SUBTITLE_OP;
  Subtitle.Header.Length  = sizeof (EFI_IFR_SUBTITLE);
  Subtitle.SubTitle       = StringToken;

  Status                  = AddOpCode (FormBuffer, &Subtitle);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS    Status;
  EFI_IFR_TEXT  Text;
  CHAR16        CurrentLanguage[4];
  STRING_REF    StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, String, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Text.Header.OpCode  = EFI_IFR_TEXT_OP;
  Text.Header.Length  = sizeof (EFI_IFR_TEXT);
  Text.Text           = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, String2, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Text.TextTwo  = StringToken;

  Text.Flags    = (UINT8) (Flags | EFI_IFR_FLAG_CREATED);
  Text.Key      = Key;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, String3, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Text.Help = StringToken;

  Status    = AddOpCode (FormBuffer, &Text);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS  Status;
  EFI_IFR_REF Hyperlink;
  CHAR16      CurrentLanguage[4];
  STRING_REF  StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Hyperlink.Header.OpCode = EFI_IFR_REF_OP;
  Hyperlink.Header.Length = sizeof (EFI_IFR_REF);
  Hyperlink.FormId        = FormId;
  Hyperlink.Prompt        = StringToken;

  Status                  = AddOpCode (FormBuffer, &Hyperlink);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_IFR_ONE_OF        OneOf;
  EFI_IFR_ONE_OF_OPTION OneOfOption;
  EFI_IFR_END_ONE_OF    EndOneOf;
  CHAR16                CurrentLanguage[4];
  STRING_REF            StringToken;

  //
  // We do not create op-code storage widths for one-of in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OneOf.Header.OpCode = EFI_IFR_ONE_OF_OP;
  OneOf.Header.Length = sizeof (EFI_IFR_ONE_OF);
  OneOf.QuestionId    = QuestionId;
  OneOf.Width         = DataWidth;
  OneOf.Prompt        = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Help, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OneOf.Help  = StringToken;

  Status      = AddOpCode (FormBuffer, &OneOf);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    OneOfOption.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
    OneOfOption.Header.Length = sizeof (EFI_IFR_ONE_OF_OPTION);

    //
    // Add string and get token back
    //
    Status              = AddString (StringBuffer, CurrentLanguage, OptionsList[Index].OptionString, &StringToken);

    OneOfOption.Option  = StringToken;
    OneOfOption.Value   = OptionsList[Index].Value;
    OneOfOption.Flags   = (UINT8) (OptionsList[Index].Flags | EFI_IFR_FLAG_CREATED);
    OneOfOption.Key     = OptionsList[Index].Key;

    Status              = AddOpCode (FormBuffer, &OneOfOption);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EndOneOf.Header.Length  = sizeof (EFI_IFR_END_ONE_OF);
  EndOneOf.Header.OpCode  = EFI_IFR_END_ONE_OF_OP;

  Status                  = AddOpCode (FormBuffer, &EndOneOf);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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
{
  EFI_STATUS            Status;
  UINTN                 Index;
  EFI_IFR_ORDERED_LIST  OrderedList;
  EFI_IFR_ONE_OF_OPTION OrderedListOption;
  EFI_IFR_END_ONE_OF    EndOrderedList;
  CHAR16                CurrentLanguage[4];
  STRING_REF            StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OrderedList.Header.OpCode = EFI_IFR_ORDERED_LIST_OP;
  OrderedList.Header.Length = sizeof (EFI_IFR_ORDERED_LIST);
  OrderedList.QuestionId    = QuestionId;
  OrderedList.MaxEntries    = MaxEntries;
  OrderedList.Prompt        = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Help, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  OrderedList.Help  = StringToken;

  Status            = AddOpCode (FormBuffer, &OrderedList);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < OptionCount; Index++) {
    OrderedListOption.Header.OpCode = EFI_IFR_ONE_OF_OPTION_OP;
    OrderedListOption.Header.Length = sizeof (EFI_IFR_ONE_OF_OPTION);

    //
    // Add string and get token back
    //
    Status                    = AddString (StringBuffer, CurrentLanguage, OptionsList[Index].OptionString, &StringToken);

    OrderedListOption.Option  = StringToken;
    OrderedListOption.Value   = OptionsList[Index].Value;
    OrderedListOption.Flags   = (UINT8) (OptionsList[Index].Flags | EFI_IFR_FLAG_CREATED);
    OrderedListOption.Key     = OptionsList[Index].Key;

    Status                    = AddOpCode (FormBuffer, &OrderedListOption);

    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  EndOrderedList.Header.Length  = sizeof (EFI_IFR_END_ONE_OF);
  EndOrderedList.Header.OpCode  = EFI_IFR_END_ONE_OF_OP;

  Status                        = AddOpCode (FormBuffer, &EndOrderedList);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS        Status;
  EFI_IFR_CHECK_BOX CheckBox;
  CHAR16            CurrentLanguage[4];
  STRING_REF        StringToken;

  //
  // We do not create op-code storage widths for checkbox in excess of 8 bits for now
  //
  if (DataWidth > 1) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  CheckBox.Header.OpCode  = EFI_IFR_CHECKBOX_OP;
  CheckBox.Header.Length  = sizeof (EFI_IFR_CHECK_BOX);
  CheckBox.QuestionId     = QuestionId;
  CheckBox.Width          = DataWidth;
  CheckBox.Prompt         = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Help, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  CheckBox.Help   = StringToken;
  CheckBox.Flags  = (UINT8) (Flags | EFI_IFR_FLAG_CREATED);

  Status          = AddOpCode (FormBuffer, &CheckBox);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS      Status;
  EFI_IFR_NUMERIC Numeric;
  CHAR16          CurrentLanguage[4];
  STRING_REF      StringToken;

  //
  // We do not create op-code storage widths for numerics in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Numeric.Header.OpCode = EFI_IFR_NUMERIC_OP;
  Numeric.Header.Length = sizeof (EFI_IFR_NUMERIC);
  Numeric.QuestionId    = QuestionId;
  Numeric.Width         = DataWidth;
  Numeric.Prompt        = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Help, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Numeric.Help    = StringToken;
  Numeric.Minimum = Minimum;
  Numeric.Maximum = Maximum;
  Numeric.Step    = Step;
  Numeric.Default = Default;
  Numeric.Flags   = (UINT8) (Flags | EFI_IFR_FLAG_CREATED);
  Numeric.Key     = Key;

  Status          = AddOpCode (FormBuffer, &Numeric);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}


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
{
  EFI_STATUS      Status;
  EFI_IFR_STRING  String;
  CHAR16          CurrentLanguage[4];
  STRING_REF      StringToken;

  //
  // Obtain current language value
  //
  GetCurrentLanguage (CurrentLanguage);

  //
  // Add first string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Prompt, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  String.Header.OpCode  = EFI_IFR_STRING_OP;
  String.Header.Length  = sizeof (EFI_IFR_STRING);
  String.QuestionId     = QuestionId;
  String.Width          = DataWidth;
  String.Prompt         = StringToken;

  //
  // Add second string, get first string's token
  //
  Status = AddString (StringBuffer, CurrentLanguage, Help, &StringToken);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  String.Help     = StringToken;
  String.MinSize  = MinSize;
  String.MaxSize  = MaxSize;
  String.Flags    = (UINT8) (Flags | EFI_IFR_FLAG_CREATED);
  String.Key      = Key;

  Status          = AddOpCode (FormBuffer, &String);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

#endif

