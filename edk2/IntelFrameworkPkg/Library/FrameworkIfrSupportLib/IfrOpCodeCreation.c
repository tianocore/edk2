/** @file
  Library Routines to create IFR independent of string data - assume tokens already exist
  Primarily to be used for exporting op-codes at a label in pre-defined forms.

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "IfrSupportLibInternal.h"

/**
  Create a SubTitle opcode independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param  StringToken      StringToken of the subtitle
  @param  FormBuffer       Output of subtitle as a form
  
  @retval EFI_SUCCESS      Subtitle created to be a form
**/
EFI_STATUS
CreateSubTitleOpCode (
  IN      STRING_REF                StringToken,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_SUBTITLE        Subtitle;

  Subtitle.Header.OpCode  = FRAMEWORK_EFI_IFR_SUBTITLE_OP;
  Subtitle.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_SUBTITLE);
  Subtitle.SubTitle       = StringToken;

  CopyMem (FormBuffer, &Subtitle, sizeof (FRAMEWORK_EFI_IFR_SUBTITLE));
  return EFI_SUCCESS;
}

/**
  Create a Text opcode independent of string creation.
  
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  @param StringToken         First string token of the text
  @param StringTokenTwo      Second string token of the text
  @param StringTokenThree    Help string token of the text
  @param Flags               Flag of the text
  @param Key                 Key of the text
  @param FormBuffer          Output of text as a form

  @retval EFI_SUCCESS        Text created to be a form
**/
EFI_STATUS
CreateTextOpCode (
  IN      STRING_REF                StringToken,
  IN      STRING_REF                StringTokenTwo,
  IN      STRING_REF                StringTokenThree,
  IN      UINT8                     Flags,
  IN      UINT16                    Key,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_TEXT            Text;

  Text.Header.OpCode  = FRAMEWORK_EFI_IFR_TEXT_OP;
  Text.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_TEXT);
  Text.Text           = StringToken;

  Text.TextTwo        = StringTokenTwo;
  Text.Help           = StringTokenThree;
  Text.Flags          = Flags;
  Text.Key            = Key;

  CopyMem (FormBuffer, &Text, sizeof (FRAMEWORK_EFI_IFR_TEXT));

  return EFI_SUCCESS;
}

/**
  Create a hyperlink opcode independent of string creation.
  
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param  FormId          Form ID of the hyperlink
  @param  StringToken     Prompt string token of the hyperlink
  @param  StringTokenTwo  Help string token of the hyperlink
  @param  Flags           Flags of the hyperlink
  @param  Key             Key of the hyperlink
  @param  FormBuffer      Output of hyperlink as a form
  
  @retval EFI_SUCCESS     Hyperlink created to be a form
**/
EFI_STATUS
CreateGotoOpCode (
  IN      UINT16                    FormId,
  IN      STRING_REF                StringToken,
  IN      STRING_REF                StringTokenTwo,
  IN      UINT8                     Flags,
  IN      UINT16                    Key,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_REF             Hyperlink;

  Hyperlink.Header.OpCode = FRAMEWORK_EFI_IFR_REF_OP;
  Hyperlink.Header.Length = sizeof (FRAMEWORK_EFI_IFR_REF);
  Hyperlink.FormId        = FormId;
  Hyperlink.Prompt        = StringToken;
  Hyperlink.Help          = StringTokenTwo;
  Hyperlink.Key           = Key;
  Hyperlink.Flags         = Flags;

  CopyMem (FormBuffer, &Hyperlink, sizeof (FRAMEWORK_EFI_IFR_REF));

  return EFI_SUCCESS;
}

/**
  Create a one-of opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
  @param  QuestionId        Question ID of the one-of box
  @param  DataWidth         DataWidth of the one-of box
  @param  PromptToken       Prompt string token of the one-of box
  @param  HelpToken         Help string token of the one-of box
  @param  OptionsList       Each string in it is an option of the one-of box
  @param  OptionCount       Option string count
  @param  FormBuffer        Output of One-Of box as a form
  

  @retval EFI_SUCCESS       One-Of box created to be a form
  @retval EFI_DEVICE_ERROR  DataWidth > 2
**/
EFI_STATUS
CreateOneOfOpCode (
  IN      UINT16                    QuestionId,
  IN      UINT8                     DataWidth,
  IN      STRING_REF                PromptToken,
  IN      STRING_REF                HelpToken,
  IN      IFR_OPTION                *OptionsList,
  IN      UINTN                     OptionCount,
  IN OUT  VOID                      *FormBuffer
  )
{
  UINTN                             Index;
  FRAMEWORK_EFI_IFR_ONE_OF          OneOf;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION   OneOfOption;
  FRAMEWORK_EFI_IFR_END_ONE_OF      EndOneOf;
  UINT8                             *LocalBuffer;

  //
  // We do not create op-code storage widths for one-of in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  OneOf.Header.OpCode = FRAMEWORK_EFI_IFR_ONE_OF_OP;
  OneOf.Header.Length = sizeof (FRAMEWORK_EFI_IFR_ONE_OF);
  OneOf.QuestionId    = QuestionId;
  OneOf.Width         = DataWidth;
  OneOf.Prompt        = PromptToken;
  OneOf.Help          = HelpToken;
  LocalBuffer         = (UINT8 *) FormBuffer;

  CopyMem (LocalBuffer, &OneOf, sizeof (FRAMEWORK_EFI_IFR_ONE_OF));

  LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_ONE_OF));

  for (Index = 0; Index < OptionCount; Index++) {
    OneOfOption.Header.OpCode = FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP;
    OneOfOption.Header.Length = sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION);

    OneOfOption.Option        = OptionsList[Index].StringToken;
    OneOfOption.Value         = OptionsList[Index].Value;
    OneOfOption.Flags         = OptionsList[Index].Flags;
    OneOfOption.Key           = OptionsList[Index].Key;

    CopyMem (LocalBuffer, &OneOfOption, sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION));

    LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION));
  }

  EndOneOf.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF);
  EndOneOf.Header.OpCode  = FRAMEWORK_EFI_IFR_END_ONE_OF_OP;

  CopyMem (LocalBuffer, &EndOneOf, sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF));

  LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF));

  return EFI_SUCCESS;
}

/**
  Create a ordered list opcode with a set of option op-codes to choose from independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  OptionsList is a pointer to a null-terminated list of option descriptions.  Ensure that OptionsList[x].StringToken
  has been filled in since this routine will not generate StringToken values.
  
  @param  QuestionId      Question ID of the ordered list
  @param  MaxEntries      MaxEntries of the ordered list
  @param  PromptToken     Prompt string token of the ordered list
  @param  HelpToken       Help string token of the ordered list
  @param  OptionsList     Each string in it is an option of the ordered list
  @param  OptionCount     Option string count
  @param  FormBuffer      Output of ordered list as a form
  
  @retval EFI_SUCCESS     Ordered list created to be a form
**/
EFI_STATUS
CreateOrderedListOpCode (
  IN      UINT16                    QuestionId,
  IN      UINT8                     MaxEntries,
  IN      STRING_REF                PromptToken,
  IN      STRING_REF                HelpToken,
  IN      IFR_OPTION                *OptionsList,
  IN      UINTN                     OptionCount,
  IN OUT  VOID                      *FormBuffer
  )
{
  UINTN                             Index;
  FRAMEWORK_EFI_IFR_ORDERED_LIST    OrderedList;
  FRAMEWORK_EFI_IFR_ONE_OF_OPTION   OrderedListOption;
  FRAMEWORK_EFI_IFR_END_ONE_OF      EndOrderedList;
  UINT8                             *LocalBuffer;

  OrderedList.Header.OpCode = FRAMEWORK_EFI_IFR_ORDERED_LIST_OP;
  OrderedList.Header.Length = sizeof (FRAMEWORK_EFI_IFR_ORDERED_LIST);
  OrderedList.QuestionId    = QuestionId;
  OrderedList.MaxEntries    = MaxEntries;
  OrderedList.Prompt        = PromptToken;
  OrderedList.Help          = HelpToken;
  LocalBuffer               = (UINT8 *) FormBuffer;

  CopyMem (LocalBuffer, &OrderedList, sizeof (FRAMEWORK_EFI_IFR_ORDERED_LIST));

  LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_ORDERED_LIST));

  for (Index = 0; Index < OptionCount; Index++) {
    OrderedListOption.Header.OpCode = FRAMEWORK_EFI_IFR_ONE_OF_OPTION_OP;
    OrderedListOption.Header.Length = sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION);

    OrderedListOption.Option        = OptionsList[Index].StringToken;
    OrderedListOption.Value         = OptionsList[Index].Value;
    OrderedListOption.Flags         = OptionsList[Index].Flags;
    OrderedListOption.Key           = OptionsList[Index].Key;

    CopyMem (LocalBuffer, &OrderedListOption, sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION));

    LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_ONE_OF_OPTION));
  }

  EndOrderedList.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF);
  EndOrderedList.Header.OpCode  = FRAMEWORK_EFI_IFR_END_ONE_OF_OP;

  CopyMem (LocalBuffer, &EndOrderedList, sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF));

  LocalBuffer = (UINT8 *) (LocalBuffer + sizeof (FRAMEWORK_EFI_IFR_END_ONE_OF));

  return EFI_SUCCESS;
}

/**
  Create a checkbox opcode independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)

  @param  QuestionId        Question ID of the check box
  @param  DataWidth         DataWidth of the check box
  @param  PromptToken       Prompt string token of the check box
  @param  HelpToken         Help string token of the check box
  @param  Flags             Flags of the check box
  @param  Key               Key of the check box
  @param  FormBuffer        Output of the check box as a form

  @retval EFI_SUCCESS       Checkbox created to be a form
  @retval EFI_DEVICE_ERROR  DataWidth > 1
**/
EFI_STATUS
CreateCheckBoxOpCode (
  IN      UINT16                    QuestionId,
  IN      UINT8                     DataWidth,
  IN      STRING_REF                PromptToken,
  IN      STRING_REF                HelpToken,
  IN      UINT8                     Flags,
  IN      UINT16                    Key,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_CHECKBOX        CheckBox;

  //
  // We do not create op-code storage widths for checkbox in excess of 8 bits for now
  //
  if (DataWidth > 1) {
    return EFI_DEVICE_ERROR;
  }

  CheckBox.Header.OpCode  = FRAMEWORK_EFI_IFR_CHECKBOX_OP;
  CheckBox.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_CHECKBOX);
  CheckBox.QuestionId     = QuestionId;
  CheckBox.Width          = DataWidth;
  CheckBox.Prompt         = PromptToken;
  CheckBox.Help           = HelpToken;
  CheckBox.Flags          = Flags;
  CheckBox.Key            = Key;

  CopyMem (FormBuffer, &CheckBox, sizeof (FRAMEWORK_EFI_IFR_CHECKBOX));

  return EFI_SUCCESS;
}

/**
  Create a numeric opcode independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param  QuestionId        Question ID of the numeric
  @param  DataWidth         DataWidth of the numeric
  @param  PromptToken       Prompt string token of the numeric
  @param  HelpToken         Help string token of the numeric
  @param  Minimum           Minumun boundary of the numeric
  @param  Maximum           Maximum boundary of the numeric
  @param  Step              Step of the numeric
  @param  Default           Default value of the numeric
  @param  Flags             Flags of the numeric
  @param  Key               Key of the numeric
  @param  FormBuffer        Output of the numeric as a form
 

  @retval EFI_SUCCESS       The numeric created to be a form.
  @retval EFI_DEVICE_ERROR  DataWidth > 2
**/
EFI_STATUS
CreateNumericOpCode (
  IN      UINT16                    QuestionId,
  IN      UINT8                     DataWidth,
  IN      STRING_REF                PromptToken,
  IN      STRING_REF                HelpToken,
  IN      UINT16                    Minimum,
  IN      UINT16                    Maximum,
  IN      UINT16                    Step,
  IN      UINT16                    Default,
  IN      UINT8                     Flags,
  IN      UINT16                    Key,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_NUMERIC         Numeric;

  //
  // We do not create op-code storage widths for numerics in excess of 16 bits for now
  //
  if (DataWidth > 2) {
    return EFI_DEVICE_ERROR;
  }

  Numeric.Header.OpCode = FRAMEWORK_EFI_IFR_NUMERIC_OP;
  Numeric.Header.Length = sizeof (FRAMEWORK_EFI_IFR_NUMERIC);
  Numeric.QuestionId    = QuestionId;
  Numeric.Width         = DataWidth;
  Numeric.Prompt        = PromptToken;
  Numeric.Help          = HelpToken;
  Numeric.Minimum       = Minimum;
  Numeric.Maximum       = Maximum;
  Numeric.Step          = Step;
  Numeric.Default       = Default;
  Numeric.Flags         = Flags;
  Numeric.Key           = Key;

  CopyMem (FormBuffer, &Numeric, sizeof (FRAMEWORK_EFI_IFR_NUMERIC));

  return EFI_SUCCESS;
}

/**
  Create a numeric opcode independent of string creation.
  This is used primarily by users who need to create just one particular valid op-code and the string
  data will be assumed to exist in the HiiDatabase already.  (Useful when exporting op-codes at a label
  location to pre-defined forms in HII)
  
  @param  QuestionId       Question ID of the string
  @param  DataWidth        DataWidth of the string
  @param  PromptToken      Prompt token of the string
  @param  HelpToken        Help token of the string
  @param  MinSize          Min size boundary of the string
  @param  MaxSize          Max size boundary of the string
  @param  Flags            Flags of the string
  @param  Key              Key of the string
  @param  FormBuffer       Output of the string as a form
  
  @retval EFI_SUCCESS      String created to be a form.
**/
EFI_STATUS
CreateStringOpCode (
  IN      UINT16                    QuestionId,
  IN      UINT8                     DataWidth,
  IN      STRING_REF                PromptToken,
  IN      STRING_REF                HelpToken,
  IN      UINT8                     MinSize,
  IN      UINT8                     MaxSize,
  IN      UINT8                     Flags,
  IN      UINT16                    Key,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_STRING          String;

  String.Header.OpCode  = FRAMEWORK_EFI_IFR_STRING_OP;
  String.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_STRING);
  String.QuestionId     = QuestionId;
  String.Width          = DataWidth;
  String.Prompt         = PromptToken;
  String.Help           = HelpToken;
  String.MinSize        = MinSize;
  String.MaxSize        = MaxSize;
  String.Flags          = Flags;
  String.Key            = Key;

  CopyMem (FormBuffer, &String, sizeof (FRAMEWORK_EFI_IFR_STRING));

  return EFI_SUCCESS;
}

/**
  Create a banner opcode.  This is primarily used by the FrontPage implementation from BDS.
  
  @param  Title        Title of the banner
  @param  LineNumber   LineNumber of the banner
  @param  Alignment    Alignment of the banner
  @param  FormBuffer   Output of banner as a form

  @retval EFI_SUCCESS  Banner created to be a form.
**/
EFI_STATUS
CreateBannerOpCode (
  IN      UINT16                    Title,
  IN      UINT16                    LineNumber,
  IN      UINT8                     Alignment,
  IN OUT  VOID                      *FormBuffer
  )
{
  FRAMEWORK_EFI_IFR_BANNER          Banner;

  Banner.Header.OpCode  = FRAMEWORK_EFI_IFR_BANNER_OP;
  Banner.Header.Length  = sizeof (FRAMEWORK_EFI_IFR_BANNER);
  CopyMem (&Banner.Title, &Title, sizeof (UINT16));
  CopyMem (&Banner.LineNumber, &LineNumber, sizeof (UINT16));
  Banner.Alignment = Alignment;

  CopyMem (FormBuffer, &Banner, sizeof (FRAMEWORK_EFI_IFR_BANNER));

  return EFI_SUCCESS;
}


