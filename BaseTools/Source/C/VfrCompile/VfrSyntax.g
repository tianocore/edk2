/*++
Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  VfrSyntax.g

Abstract:

--*/

#header<<

#include "EfiVfr.h"
#include "VfrFormPkg.h"
#include "VfrError.h"
#include "VfrUtilityLib.h"
#include "AToken.h"
#include "ATokPtr.h"
>>

<<
#include "stdio.h"
#include "PBlackBox.h"
#include "DLexerBase.h"
#include "VfrLexer.h"
#include "AToken.h"

#define GET_LINENO(Obj)       ((Obj)->getLine())
#define SET_LINE_INFO(Obj, L) {(Obj).SetLineNo((L)->getLine());} while (0)
#define CRT_END_OP(Obj)       {CIfrEnd EObj; if (Obj != NULL) EObj.SetLineNo ((Obj)->getLine());} while (0)

typedef ANTLRCommonToken ANTLRToken;

class CVfrDLGLexer : public VfrLexer
{
public:
  CVfrDLGLexer (DLGFileInput *F) : VfrLexer (F) {};
  INT32 errstd (char *Text)
  {
    printf ("unrecognized input '%s'\n", Text);
  }
};

UINT8
VfrParserStart (
  IN FILE *File,
  IN INPUT_INFO_TO_SYNTAX *InputInfo
  )
{
  ParserBlackBox<CVfrDLGLexer, EfiVfrParser, ANTLRToken> VfrParser(File);
  VfrParser.parser()->SetCompatibleMode (InputInfo->CompatibleMode);
  VfrParser.parser()->SetOverrideClassGuid (InputInfo->OverrideClassGuid);
  return VfrParser.parser()->vfrProgram();
}
>>

//
// Define a lexical class for parsing quoted strings. Basically
// starts with a double quote, and ends with a double quote that
// is not preceeded with a backslash.
//
#lexclass QUOTED_STRING
#token TheString            "~[\"]*\"" << mode (START); >>

//
// Define a lexclass for skipping over C++ style comments
//
#lexclass CPP_COMMENT
#token "~[\n]*"       << skip (); >>
#token "\n"           << skip (); mode (START); newline (); >>

//
// Standard lexclass is START
//
#lexclass START

//
// Find start of C++ style comments
//
#token "//"       << skip (); mode (CPP_COMMENT); >>

//
// Skip whitespace
//
#token "[\ \t]"   << skip (); >>

//
// Skip over newlines, but count them
//
#token "\n"       << skip (); newline (); >>

//
// Skip over 'extern' in any included .H file
//
#token "extern"   << skip (); mode (CPP_COMMENT); >>

//
// Tokens for the different keywords. Syntax is:
// TokenName("ErrorMessageText")    "TokenString"
//   where:
//     TokenName is the token name (must be capitalized) that is used in the rules
//     ErrorMessageText is the string the compiler emits when it detects a syntax error
//     TokenString is the actual matching string used in the user script
//
#token FormPkgType("formpkgtype")               "formpkgtype"
#token OpenBrace("{")                           "\{"
#token CloseBrace("}")                          "\}"
#token OpenParen("(")                           "\("
#token CloseParen(")")                          "\)"
#token OpenBracket("[")                         "\["
#token CloseBracket("]")                        "\]"

#token LineDefinition                           "#line\ [0-9]+\ \"~[\"]+\"[\ \t]*\n" << gCVfrErrorHandle.ParseFileScopeRecord (begexpr (), line ()); skip (); newline (); >>
#token DevicePath("devicepath")                 "devicepath"
#token FormSet("formset")                       "formset"
#token FormSetId("formsetid")                   "formsetid"
#token EndFormSet("endformset")                 "endformset"
#token Title("title")                           "title"
#token FormId("formid")                         "formid"
#token OneOf("oneof")                           "oneof"
#token EndOneOf("endoneof")                     "endoneof"
#token Prompt("prompt")                         "prompt"
#token OrderedList("orderedlist")               "orderedlist"
#token MaxContainers("maxcontainers")           "maxcontainers"
#token EndList("endlist")                       "endlist"
#token EndForm("endform")                       "endform"
#token Form("form")                             "form"
#token FormMap("formmap")                       "formmap"
#token MapTitle("maptitle")                     "maptitle"
#token MapGuid("mapguid")                       "mapguid"
#token Subtitle("subtitle")                     "subtitle"
#token Help("help")                             "help"
#token Text("text")                             "text"
#token Option("option")                         "option"
#token FLAGS("flags")                           "flags"
#token Date("date")                             "date"
#token EndDate("enddate")                       "enddate"
#token Year("year")                             "year"
#token Month("month")                           "month"
#token Day("day")                               "day"
#token Time("time")                             "time"
#token EndTime("endtime")                       "endtime"
#token Hour("hour")                             "hour"
#token Minute("minute")                         "minute"
#token Second("second")                         "second"
#token GrayOutIf("grayoutif")                   "grayoutif"
#token Label("label")                           "label"
#token Timeout("timeout")                       "timeout"
#token Inventory("inventory")                   "inventory"
#token NonNvDataMap("_NON_NV_DATA_MAP")         "_NON_NV_DATA_MAP"
#token Struct("struct")                         "struct"
#token Boolean("BOOLEAN")                       "BOOLEAN"
#token Uint64("UINT64")                         "UINT64"
#token Uint32("UINT32")                         "UINT32"
#token Uint16("UINT16")                         "UINT16"
#token Char16("CHAR16")                         "CHAR16"
#token Uint8("UINT8")                           "UINT8"
#token Uuid("guid")                             "guid"
#token CheckBox("checkbox")                     "checkbox"
#token EndCheckBox("endcheckbox")               "endcheckbox"
#token Numeric("numeric")                       "numeric"
#token EndNumeric("endnumeric")                 "endnumeric"
#token Minimum("minimum")                       "minimum"
#token Maximum("maximum")                       "maximum"
#token STEP("step")                             "step"
#token Default("default")                       "default"
#token Password("password")                     "password"
#token EndPassword("endpassword")               "endpassword"
#token String("string")                         "string"
#token EndString("endstring")                   "endstring"
#token MinSize("minsize")                       "minsize"
#token MaxSize("maxsize")                       "maxsize"
#token Encoding("encoding")                     "encoding"
#token SuppressIf("suppressif")                 "suppressif"
#token DisableIf("disableif")                   "disableif"
#token Hidden("hidden")                         "hidden"
#token Goto("goto")                             "goto"
#token FormSetGuid("formsetguid")               "formsetguid"
#token InconsistentIf("inconsistentif")         "inconsistentif"
#token NoSubmitIf("nosubmitif")                 "nosubmitif"
#token EndIf("endif")                           "endif"
#token Key("key")                               "key"
#token DefaultFlag("DEFAULT")                   "DEFAULT"
#token ManufacturingFlag("MANUFACTURING")       "MANUFACTURING"
#token InteractiveFlag("INTERACTIVE")           "INTERACTIVE"
#token NVAccessFlag("NV_ACCESS")                "NV_ACCESS"
#token ResetRequiredFlag("RESET_REQUIRED")      "RESET_REQUIRED"
#token LateCheckFlag("LATE_CHECK")              "LATE_CHECK"
#token ReadOnlyFlag("READ_ONLY")                "READ_ONLY"
#token OptionOnlyFlag("OPTIONS_ONLY")           "OPTIONS_ONLY"
#token Class("class")                           "class"
#token Subclass("subclass")                     "subclass"
#token ClassGuid("classguid")                   "classguid"
#token TypeDef("typedef")                       "typedef"
#token Restore("restore")                       "restore"
#token Save("save")                             "save"
#token Defaults("defaults")                     "defaults"
#token Banner("banner")                         "banner"
#token Align("align")                           "align"
#token Left("left")                             "left"
#token Right("right")                           "right"
#token Center("center")                         "center"
#token Line("line")                             "line"
#token Name("name")                             "name"

#token VarId("varid")                           "varid"
#token Question("question")                     "question"
#token QuestionId("questionid")                 "questionid"
#token Image("image")                           "image"
#token Locked("locked")                         "locked"
#token Rule("rule")                             "rule"
#token EndRule("endrule")                       "endrule"
#token Value("value")                           "value"
#token Read("read")                             "read"
#token Write("write")                           "write"
#token ResetButton("resetbutton")               "resetbutton"
#token EndResetButton("endresetbutton")         "endresetbutton"
#token DefaultStore("defaultstore")             "defaultstore"
#token Attribute("attribute")                   "attribute"
#token Varstore("varstore")                     "varstore"
#token Efivarstore("efivarstore")               "efivarstore"
#token VarSize("varsize")                       "varsize"
#token NameValueVarStore("namevaluevarstore")   "namevaluevarstore"
#token Action("action")                         "action"
#token Config("config")                         "config"
#token EndAction("endaction")                   "endaction"
#token Refresh("refresh")                       "refresh"
#token Interval("interval")                     "interval"
#token VarstoreDevice("varstoredevice")         "varstoredevice"
#token GuidOp("guidop")                         "guidop"
#token EndGuidOp("endguidop")                   "endguidop"
#token DataType("datatype")                     "datatype"
#token Data("data")                             "data"
#token Modal("modal")                           "modal"

//
// Define the class and subclass tokens
//
#token ClassNonDevice("NONDEVICE")                        "NON_DEVICE"
#token ClassDiskDevice("DISK_DEVICE")                     "DISK_DEVICE"
#token ClassVideoDevice("VIDEO_DEVICE")                   "VIDEO_DEVICE"
#token ClassNetworkDevice("NETWORK_DEVICE")               "NETWORK_DEVICE"
#token ClassInputDevice("INPUT_DEVICE")                   "INPUT_DEVICE"
#token ClassOnBoardDevice("ONBOARD_DEVICE")               "ONBOARD_DEVICE"
#token ClassOtherDevice("OTHER_DEVICE")                   "OTHER_DEVICE"

#token SubclassSetupApplication("SETUP_APPLICATION")      "SETUP_APPLICATION"
#token SubclassGeneralApplication("GENERAL_APPLICATION")  "GENERAL_APPLICATION"
#token SubclassFrontPage("FRONT_PAGE")                    "FRONT_PAGE"
#token SubclassSingleUse("SINGLE_USE")                    "SINGLE_USE"

//
// This is the overall definition of a VFR form definition script.
//

vfrProgram > [UINT8 Return] :
  <<
     mParserStatus   = 0;
     mCIfrOpHdrIndex = 0;
     mConstantOnlyInExpression = FALSE;
  >>
  (
      vfrPragmaPackDefinition
    | vfrDataStructDefinition
  )*
  vfrFormSetDefinition
  << $Return = mParserStatus; >>
  ;

pragmaPackShowDef :
  L:"show"                                          << gCVfrVarDataTypeDB.Pack (L->getLine(), VFR_PACK_SHOW); >>
  ;

pragmaPackStackDef :
  <<
     UINT32 LineNum;
     UINT8  PackAction;
     CHAR8  *Identifier = NULL;
     UINT32 PackNumber  = DEFAULT_PACK_ALIGN;
  >>
  (
      L1:"push"                                     << LineNum = L1->getLine(); PackAction = VFR_PACK_PUSH; >>
    | L2:"pop"                                      << LineNum = L2->getLine(); PackAction = VFR_PACK_POP; >>
  )
  {
    "," ID:StringIdentifier                         << Identifier = ID->getText(); >>
  }
  {
    "," N:Number                                    << PackAction |= VFR_PACK_ASSIGN; PackNumber = _STOU32(N->getText()); >>
  }
                                                    << gCVfrVarDataTypeDB.Pack (LineNum, PackAction, Identifier, PackNumber); >>
  ;

pragmaPackNumber :
  <<
     UINT32 LineNum;
     UINT32 PackNumber = DEFAULT_PACK_ALIGN;
  >>
  N:Number                                          << LineNum = N->getLine(); PackNumber = _STOU32(N->getText()); >>
                                                    << gCVfrVarDataTypeDB.Pack (LineNum, VFR_PACK_ASSIGN, NULL, PackNumber); >>
  ;

vfrPragmaPackDefinition :
  "\#pragma" "pack" "\("
  {
      pragmaPackShowDef
    | pragmaPackStackDef
    | pragmaPackNumber
  }
  "\)"
  ;

vfrDataStructDefinition :
  { TypeDef } Struct                                << gCVfrVarDataTypeDB.DeclareDataTypeBegin (); >>
  { NonNvDataMap }
  {
    N1:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N1->getText()), N1); >>
  }
  OpenBrace
    vfrDataStructFields
  CloseBrace
  {
    N2:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N2->getText()), N2); >>
  }
  ";"                                               << gCVfrVarDataTypeDB.DeclareDataTypeEnd (); >>
  ;

vfrDataStructFields :
  (
     dataStructField64     |
     dataStructField32     |
     dataStructField16     |
     dataStructField8      |
     dataStructFieldBool   |
     dataStructFieldString |
     dataStructFieldDate   |
     dataStructFieldTime   |
     dataStructFieldRef    |
     dataStructFieldUser
  )*
  ;

dataStructField64 :
  << UINT32 ArrayNum = 0; >>
  D:"UINT64"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructField32 :
  << UINT32 ArrayNum = 0; >>
  D:"UINT32"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructField16 :
  << 
    UINT32 ArrayNum = 0; 
  >>
  ("UINT16" | "CHAR16")
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), (CHAR8 *) "UINT16", ArrayNum), N); >>
  ;

dataStructField8 :
  << UINT32 ArrayNum = 0; >>
  D:"UINT8"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldBool :
  << UINT32 ArrayNum = 0; >>
  D:"BOOLEAN"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldString :
  << UINT32 ArrayNum = 0; >>
  D:"EFI_STRING_ID"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldDate :
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_DATE"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldTime :
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_TIME"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldRef :
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_REF"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum), N); >>
  ;

dataStructFieldUser :
  << UINT32 ArrayNum = 0; >>
  T:StringIdentifier
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), T->getText(), ArrayNum), T); >>
  ;

//*****************************************************************************
//
// the syntax of GUID definition
//
guidSubDefinition [EFI_GUID &Guid] :
  G4:Number "," G5:Number "," G6:Number "," G7:Number "," G8:Number "," G9:Number "," G10:Number "," G11:Number
                                                    <<
                                                       Guid.Data4[0] = _STOU8(G4->getText());
                                                       Guid.Data4[1] = _STOU8(G5->getText());
                                                       Guid.Data4[2] = _STOU8(G6->getText());
                                                       Guid.Data4[3] = _STOU8(G7->getText());
                                                       Guid.Data4[4] = _STOU8(G8->getText());
                                                       Guid.Data4[5] = _STOU8(G9->getText());
                                                       Guid.Data4[6] = _STOU8(G10->getText());
                                                       Guid.Data4[7] = _STOU8(G11->getText());
                                                    >>
  ;

guidDefinition [EFI_GUID &Guid] :
  OpenBrace
    G1:Number "," G2:Number "," G3:Number ","
                                                    <<
                                                       Guid.Data1 = _STOU32 (G1->getText());
                                                       Guid.Data2 = _STOU16 (G2->getText());
                                                       Guid.Data3 = _STOU16 (G3->getText());
                                                    >>
    (
        OpenBrace guidSubDefinition[Guid] CloseBrace
      | guidSubDefinition[Guid]
    )
  CloseBrace
  ;

//*****************************************************************************
//
// the syntax of form set definition
//
vfrFormSetDefinition :
  <<
     EFI_GUID    Guid;
     EFI_GUID    DefaultClassGuid = EFI_HII_PLATFORM_SETUP_FORMSET_GUID;
     EFI_GUID    ClassGuid1, ClassGuid2, ClassGuid3;
     UINT8       ClassGuidNum = 0;
     CIfrFormSet *FSObj = NULL;
     UINT16      C, SC;
  >>
  L:FormSet
  Uuid "=" guidDefinition[Guid] ","
  Title "=" "STRING_TOKEN" "\(" S1:Number "\)" ","
  Help  "=" "STRING_TOKEN" "\(" S2:Number "\)" ","
  {
    ClassGuid "=" guidDefinition[ClassGuid1]        << ++ClassGuidNum; >>
                  {
                   "\|" guidDefinition[ClassGuid2]  << ++ClassGuidNum; >>
                  }
                  {
                   "\|" guidDefinition[ClassGuid3]  << ++ClassGuidNum; >>
                  }
                  ","
  }
                                                    <<
                                                      if (mOverrideClassGuid != NULL && ClassGuidNum >= 3) {
                                                        _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Already has 3 class guids, can't add extra class guid!");
                                                      }
                                                      switch (ClassGuidNum) {
                                                      case 0:
                                                        if (mOverrideClassGuid != NULL) {
                                                          ClassGuidNum = 2;
                                                        } else {
                                                          ClassGuidNum = 1;
                                                        }
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&DefaultClassGuid);
                                                        if (mOverrideClassGuid != NULL) {
                                                          FSObj->SetClassGuid(mOverrideClassGuid);
                                                        }
                                                        break;
                                                      case 1:
                                                        if (mOverrideClassGuid != NULL) {
                                                          ClassGuidNum ++;
                                                        }
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&ClassGuid1);
                                                        if (mOverrideClassGuid != NULL) {
                                                          FSObj->SetClassGuid(mOverrideClassGuid);
                                                        }
                                                        break;
                                                      case 2:
                                                        if (mOverrideClassGuid != NULL) {
                                                          ClassGuidNum ++;
                                                        }
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&ClassGuid1);
                                                        FSObj->SetClassGuid(&ClassGuid2);
                                                        if (mOverrideClassGuid != NULL) {
                                                          FSObj->SetClassGuid(mOverrideClassGuid);
                                                        }
                                                        break;
                                                      case 3:
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&ClassGuid1);
                                                        FSObj->SetClassGuid(&ClassGuid2);
                                                        FSObj->SetClassGuid(&ClassGuid3);
                                                        break;
                                                      default:
                                                        break;
                                                      }

                                                      SET_LINE_INFO (*FSObj, L);
                                                      FSObj->SetGuid (&Guid);
                                                      //
                                                      // for framework vfr to store formset guid used by varstore and efivarstore
                                                      //
                                                      if (mCompatibleMode) {
                                                        memcpy (&mFormsetGuid, &Guid, sizeof (EFI_GUID));
                                                      }
                                                      FSObj->SetFormSetTitle (_STOSID(S1->getText()));
                                                      FSObj->SetHelp (_STOSID(S2->getText()));
                                                    >>
  {
    FC:Class "=" classDefinition[C] ","             << {CIfrClass CObj;SET_LINE_INFO (CObj, FC); CObj.SetClass(C);} >>
  }
  {
    FSC:Subclass "=" subclassDefinition[SC] ","     << {CIfrSubClass SCObj; SET_LINE_INFO (SCObj, FSC); SCObj.SetSubClass(SC);} >>
  }
                                                    <<
                                                       _DeclareStandardDefaultStorage (GET_LINENO (L));
                                                    >>
  vfrFormSetList
  E:EndFormSet                                      <<
                                                      if (mCompatibleMode) {
                                                        //
                                                        // declare all undefined varstore and efivarstore
                                                        //
                                                        _DeclareDefaultFrameworkVarStore (GET_LINENO(E));
                                                      }
                                                      CRT_END_OP (E); if (FSObj != NULL) delete FSObj;
                                                    >>
  ";"
  ;

vfrFormSetList :
  (
    vfrFormDefinition             |
    vfrFormMapDefinition          |
    vfrStatementImage             |
    vfrStatementVarStoreLinear    |
    vfrStatementVarStoreEfi       |
    vfrStatementVarStoreNameValue |
    vfrStatementDefaultStore      |
    vfrStatementDisableIfFormSet  |
    vfrStatementSuppressIfFormSet |
    vfrStatementExtension
  )*
  ;

vfrStatementExtension:
  << 
     EFI_GUID Guid;
     CIfrGuid *GuidObj = NULL;
     CHAR8    *TypeName = NULL;
     UINT32   TypeSize = 0;
     UINT8    *DataBuff = NULL;
     UINT32   Size = 0;
     UINT8    Idx = 0;
     UINT32   LineNum;
     BOOLEAN  IsStruct = FALSE;
     UINT32   ArrayNum = 0;
  >>
  L:GuidOp
  Uuid "=" guidDefinition[Guid]
  {"," DataType "=" 
    (
        U64:"UINT64" {OpenBracket AN1:Number CloseBracket <<ArrayNum = _STOU32(AN1->getText());>>}
                                                      << TypeName = U64->getText(); LineNum = U64->getLine(); >>
      | U32:"UINT32" {OpenBracket AN2:Number CloseBracket <<ArrayNum = _STOU32(AN2->getText());>>}
                                                      << TypeName = U32->getText(); LineNum = U32->getLine(); >>
      | U16:"UINT16" {OpenBracket AN3:Number CloseBracket <<ArrayNum = _STOU32(AN3->getText());>>}
                                                      << TypeName = U16->getText(); LineNum = U16->getLine(); >>
      | U8:"UINT8"   {OpenBracket AN4:Number CloseBracket <<ArrayNum = _STOU32(AN4->getText());>>}
                                                      << TypeName = U8->getText(); LineNum = U8->getLine(); >>
      | BL:"BOOLEAN" {OpenBracket AN5:Number CloseBracket <<ArrayNum = _STOU32(AN5->getText());>>}
                                                      << TypeName = BL->getText(); LineNum = BL->getLine(); >>
      | SI:"EFI_STRING_ID" {OpenBracket AN6:Number CloseBracket <<ArrayNum = _STOU32(AN6->getText());>>}
                                                      << TypeName = SI->getText(); LineNum = SI->getLine(); >>
      | D:"EFI_HII_DATE" {OpenBracket AN7:Number CloseBracket <<ArrayNum = _STOU32(AN7->getText());>>}
                                                      << TypeName = D->getText(); LineNum = D->getLine(); IsStruct = TRUE;>>
      | T:"EFI_HII_TIME" {OpenBracket AN8:Number CloseBracket <<ArrayNum = _STOU32(AN8->getText());>>}
                                                      << TypeName = T->getText(); LineNum = T->getLine(); IsStruct = TRUE;>>
      | R:"EFI_HII_REF" {OpenBracket AN9:Number CloseBracket <<ArrayNum = _STOU32(AN9->getText());>>}
                                                      << TypeName = R->getText(); LineNum = R->getLine(); IsStruct = TRUE;>>                                                
      | TN:StringIdentifier {OpenBracket AN10:Number CloseBracket <<ArrayNum = _STOU32(AN10->getText());>>}
                                                      << TypeName = TN->getText(); LineNum = TN->getLine(); IsStruct = TRUE;>>
    )
                                                      <<
                                                        _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &TypeSize), LineNum);
                                                        if (ArrayNum > 0) {
                                                          Size = TypeSize*ArrayNum;
                                                        } else {
                                                          Size = TypeSize;
                                                        }
                                                        if (Size > (128 - sizeof (EFI_IFR_GUID))) return;
                                                        DataBuff = (UINT8 *)malloc(Size);
                                                        for (Idx = 0; Idx < Size; Idx++) {
                                                          DataBuff[Idx] = 0;
                                                        }
                                                      >>
    vfrExtensionData [DataBuff, Size, TypeName, TypeSize, IsStruct, ArrayNum]
  }
                                                      <<
                                                        {
                                                         GuidObj = new CIfrGuid(Size);
                                                         if (GuidObj != NULL) {
                                                           GuidObj->SetLineNo(L->getLine());
                                                           GuidObj->SetGuid (&Guid);
                                                         }
                                                        }
                                                        if (TypeName != NULL) {
                                                          GuidObj->SetData(DataBuff, Size);
                                                        }
                                                      >>
  {","
    (
      vfrStatementExtension
    )*
  E:EndGuidOp                                         << GuidObj->SetScope(1); CRT_END_OP (E); >>
  }
                                                      <<
                                                         if (GuidObj != NULL) delete GuidObj;
                                                         if (DataBuff != NULL) free(DataBuff);
                                                      >>
  ";"
;

vfrExtensionData[UINT8 *DataBuff, UINT32 Size, CHAR8 *TypeName, UINT32 TypeSize, BOOLEAN IsStruct, UINT32 ArrayNum]:
  <<
     CHAR8    *TFName = NULL;
     UINT32   ArrayIdx = 0;
     UINT16   FieldOffset;
     UINT8    FieldType;
     UINT32   FieldSize;
     UINT64   Data_U64 = 0;
     UINT32   Data_U32 = 0;
     UINT16   Data_U16 = 0;
     UINT8    Data_U8 = 0;
     BOOLEAN  Data_BL = 0;
     EFI_STRING_ID Data_SID = 0;
     BOOLEAN  IsArray = FALSE;
     UINT8    *ByteOffset = NULL;
  >>
(
  ("," "data" {OpenBracket IDX1:Number CloseBracket <<IsArray = TRUE;>>}
          <<
            ArrayIdx = 0;
            if (IsArray == TRUE) {
              ArrayIdx = _STOU8(IDX1->getText());
              if (ArrayIdx >= ArrayNum) return;
              IsArray = FALSE;
            }
            ByteOffset = DataBuff + (ArrayIdx * TypeSize);
            if (IsStruct == TRUE) {
              _STRCAT(&TFName, TypeName);
            }
          >>
    ("." FN:StringIdentifier
          <<
            if (IsStruct == TRUE) {
              _STRCAT(&TFName, ".");
              _STRCAT(&TFName, FN->getText());
            }
          >>
        {
          OpenBracket IDX2:Number CloseBracket
            <<
              if (IsStruct == TRUE) {
                _STRCAT(&TFName, "[");
                _STRCAT(&TFName, IDX2->getText());
                _STRCAT(&TFName, "]");
              }
            >>
        }
    )*
    "=" RD:Number
          <<
            if (IsStruct == FALSE) {
              if (strcmp ("UINT64", TypeName) == 0) {
                Data_U64 = _STOU64(RD->getText());
                memcpy (ByteOffset, &Data_U64, TypeSize);
              }else if (strcmp ("UINT32", TypeName) == 0) {
                Data_U32 = _STOU32(RD->getText());
                memcpy (ByteOffset, &Data_U32, TypeSize);                                                    
              }else if (strcmp ("UINT16", TypeName) == 0) {
                Data_U16 = _STOU16(RD->getText());
                memcpy (ByteOffset, &Data_U16, TypeSize);                                                    
              }else if (strcmp ("UINT8", TypeName) == 0) {
                Data_U8 = _STOU8(RD->getText());
                memcpy (ByteOffset, &Data_U8, TypeSize);                                                    
              }else if (strcmp ("BOOLEAN", TypeName)== 0) {
                Data_BL = _STOU8(RD->getText());
                memcpy (ByteOffset, &Data_BL, TypeSize);                                                    
              }else if (strcmp ("EFI_STRING_ID", TypeName) == 0) {
                Data_SID = _STOSID(RD->getText());
                memcpy (ByteOffset, &Data_SID, TypeSize);                                                    
              }
            } else {
              gCVfrVarDataTypeDB.GetDataFieldInfo(TFName, FieldOffset, FieldType, FieldSize);
              switch (FieldType) {
              case EFI_IFR_TYPE_NUM_SIZE_8:
                 Data_U8 = _STOU8(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_U8, FieldSize);
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_16:
                 Data_U16 = _STOU16(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_U16, FieldSize);
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_32:
                 Data_U32 = _STOU32(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_U32, FieldSize);
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_64:
                 Data_U64 = _STOU64(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_U64, FieldSize);
                 break;
              case EFI_IFR_TYPE_BOOLEAN:
                 Data_BL = _STOU8(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_BL, FieldSize);
                 break;
              case EFI_IFR_TYPE_STRING:
                 Data_SID = _STOSID(RD->getText());
                 memcpy (ByteOffset + FieldOffset, &Data_SID, FieldSize);
                 break;
              default:
                 break;
              }
            }
            if (TFName != NULL) { delete TFName; TFName = NULL; }
          >>
  )*
)
;


vfrStatementDefaultStore :
  << UINT16  DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD; >>
  D:DefaultStore N:StringIdentifier ","
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)"
  {
    "," Attribute "=" A:Number                      << DefaultId = _STOU16(A->getText()); >>
  }
                                                    <<
                                                       if (mCVfrDefaultStore.DefaultIdRegistered (DefaultId) == FALSE) {
                                                         CIfrDefaultStore DSObj;
                                                         _PCATCH(mCVfrDefaultStore.RegisterDefaultStore (DSObj.GetObjBinAddr(), N->getText(), _STOSID(S->getText()), DefaultId)), D->getLine();
                                                         DSObj.SetLineNo(D->getLine());
                                                         DSObj.SetDefaultName (_STOSID(S->getText()));
                                                         DSObj.SetDefaultId (DefaultId);
                                                       } else {
                                                         _PCATCH(mCVfrDefaultStore.ReRegisterDefaultStoreById (DefaultId, N->getText(), _STOSID(S->getText()))), D->getLine();
                                                       }
                                                    >>
  ";"
  ;

vfrStatementVarStoreLinear :
  <<
     EFI_GUID        Guid;
     CIfrVarStore    VSObj;
     CHAR8           *TypeName;
     CHAR8           *StoreName;
     UINT32          LineNum;
     EFI_VARSTORE_ID VarStoreId = EFI_VARSTORE_ID_INVALID;
     UINT32          Size;
  >>
  V:Varstore                                        << VSObj.SetLineNo(V->getLine()); >>
  (
      TN:StringIdentifier ","                       << TypeName = TN->getText(); LineNum = TN->getLine(); >>
    | U8:"UINT8" ","                                << TypeName = U8->getText(); LineNum = U8->getLine(); >>
    | U16:"UINT16" ","                              << TypeName = U16->getText(); LineNum = U16->getLine(); >>
    | C16:"CHAR16" ","                              << TypeName = (CHAR8 *) "UINT16"; LineNum = C16->getLine(); >>
    | U32:"UINT32" ","                              << TypeName = U32->getText(); LineNum = U32->getLine(); >>
    | U64:"UINT64" ","                              << TypeName = U64->getText(); LineNum = U64->getLine(); >>
    | D:"EFI_HII_DATE" ","                          << TypeName = D->getText(); LineNum = D->getLine(); >>
    | T:"EFI_HII_TIME" ","                          << TypeName = T->getText(); LineNum = T->getLine(); >>
    | R:"EFI_HII_REF" ","                           << TypeName = R->getText(); LineNum = R->getLine(); >>
  )
  { Key "=" FID:Number ","                          << // Key is used to assign Varid in Framework VFR but no use in UEFI2.1 VFR
                                                       if (mCompatibleMode) {
                                                         VarStoreId = _STOU16(FID->getText());
                                                       }
                                                    >>
  }
  {
    VarId "=" ID:Number ","                         <<
                                                       _PCATCH(
                                                         (INTN)(VarStoreId = _STOU16(ID->getText())) != 0,
                                                         (INTN)TRUE,
                                                         ID,
                                                         "varid 0 is not allowed."
                                                         );
                                                    >>
  }
  Name "=" SN:StringIdentifier ","
  Uuid "=" guidDefinition[Guid]
                                                    <<
                                                       if (mCompatibleMode) {
                                                         StoreName = TypeName;
                                                       } else {
                                                         StoreName = SN->getText();
                                                       }
                                                       _PCATCH(mCVfrDataStorage.DeclareBufferVarStore (
                                                                                  StoreName,
                                                                                  &Guid,
                                                                                  &gCVfrVarDataTypeDB,
                                                                                  TypeName,
                                                                                  VarStoreId
                                                                                  ), LineNum);
                                                       VSObj.SetGuid (&Guid);
                                                       _PCATCH(mCVfrDataStorage.GetVarStoreId(StoreName, &VarStoreId), SN);
                                                       VSObj.SetVarStoreId (VarStoreId);
                                                       _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &Size), LineNum);
                                                       VSObj.SetSize ((UINT16) Size);
                                                       VSObj.SetName (SN->getText());
                                                    >>
  ";"
  ;

vfrStatementVarStoreEfi :
  <<
     BOOLEAN         IsUEFI23EfiVarstore = TRUE;
     EFI_GUID        Guid;
     CIfrVarStoreEfi VSEObj;
     EFI_VARSTORE_ID VarStoreId = EFI_VARSTORE_ID_INVALID;
     UINT32          Attr = 0;
     UINT32          Size;
     CHAR8           *TypeName;
     UINT32          LineNum;
     CHAR8           *StoreName = NULL;
  >>
  E:Efivarstore                                     << VSEObj.SetLineNo(E->getLine()); >>
  (
      TN:StringIdentifier ","                       << TypeName = TN->getText(); LineNum = TN->getLine(); >>
    | U8:"UINT8" ","                                << TypeName = U8->getText(); LineNum = U8->getLine(); >>
    | U16:"UINT16" ","                              << TypeName = U16->getText(); LineNum = U16->getLine(); >>
    | C16:"CHAR16" ","                              << TypeName = (CHAR8 *) "UINT16"; LineNum = C16->getLine(); >>
    | U32:"UINT32" ","                              << TypeName = U32->getText(); LineNum = U32->getLine(); >>
    | U64:"UINT64" ","                              << TypeName = U64->getText(); LineNum = U64->getLine(); >>
    | D:"EFI_HII_DATE" ","                          << TypeName = D->getText(); LineNum = D->getLine(); >>
    | T:"EFI_HII_TIME" ","                          << TypeName = T->getText(); LineNum = T->getLine(); >>
    | R:"EFI_HII_REF" ","                           << TypeName = R->getText(); LineNum = R->getLine(); >>    
  )
  {
    VarId "=" ID:Number ","                         <<
                                                       _PCATCH(
                                                         (INTN)(VarStoreId = _STOU16(ID->getText())) != 0,
                                                         (INTN)TRUE,
                                                         ID,
                                                         "varid 0 is not allowed."
                                                         );
                                                    >>
  }
  Attribute "=" vfrVarStoreEfiAttr[Attr] ( "\|" vfrVarStoreEfiAttr[Attr] )* ","
                                                    << VSEObj.SetAttributes (Attr); >>

  (
    Name    "=" SN:StringIdentifier ","             << StoreName = SN->getText();   >>
   |
    Name    "=" "STRING_TOKEN" "\(" VN:Number "\)" ","  
    VarSize "=" N:Number ","                        << 
                                                       IsUEFI23EfiVarstore = FALSE;
                                                       StoreName = gCVfrStringDB.GetVarStoreNameFormStringId(_STOSID(VN->getText()));
                                                       if (StoreName == NULL) {
                                                         _PCATCH (VFR_RETURN_UNSUPPORTED, VN->getLine(), "Can't get varstore name for this StringId!");
                                                       }
                                                       Size = _STOU32(N->getText());
                                                       switch (Size) {
                                                       case 1:
                                                        TypeName = (CHAR8 *) "UINT8";
                                                        break;
                                                       case 2:
                                                        TypeName = (CHAR8 *) "UINT16";
                                                        break;
                                                       case 4:
                                                        TypeName = (CHAR8 *) "UINT32";
                                                        break;
                                                       case 8:
                                                        TypeName = (CHAR8 *) "UINT64";
                                                        break; 
                                                       default:
                                                        _PCATCH (VFR_RETURN_UNSUPPORTED, N);
                                                        break;
                                                       }
                                                    >>
  )

  Uuid "=" guidDefinition[Guid]                     << 
                                                       if (IsUEFI23EfiVarstore) {
                                                       _PCATCH(mCVfrDataStorage.DeclareBufferVarStore (
                                                                                  StoreName,
                                                                                  &Guid,
                                                                                  &gCVfrVarDataTypeDB,
                                                                                  TypeName,
                                                                                  VarStoreId
                                                                                  ), LineNum);                                                        
                                                         _PCATCH(mCVfrDataStorage.GetVarStoreId(StoreName, &VarStoreId), SN);
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &Size), LineNum);
                                                       } else {
                                                        _PCATCH(mCVfrDataStorage.DeclareBufferVarStore (
                                                                                  TN->getText(),
                                                                                  &Guid,
                                                                                  &gCVfrVarDataTypeDB,
                                                                                  TypeName,
                                                                                  VarStoreId
                                                                                  ), LineNum);                                                      
                                                         _PCATCH(mCVfrDataStorage.GetVarStoreId(TN->getText(), &VarStoreId), VN);
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &Size), N->getLine());
                                                       }
                                                       VSEObj.SetGuid (&Guid);                                                       
                                                       VSEObj.SetVarStoreId (VarStoreId);
                                                       
                                                       VSEObj.SetSize ((UINT16) Size);
                                                       VSEObj.SetName (StoreName);
                                                       if (IsUEFI23EfiVarstore == FALSE && StoreName != NULL) {
                                                         delete StoreName; 
                                                       }
                                                    >>
  ";"
  ;

vfrVarStoreEfiAttr [UINT32 & Attr] :
  N:Number                                          << $Attr |= _STOU32(N->getText()); >>
  ;

vfrStatementVarStoreNameValue :
  <<
     EFI_GUID              Guid;
     CIfrVarStoreNameValue VSNVObj;
     EFI_VARSTORE_ID       VarStoreId;
  >>
  L:NameValueVarStore                               << VSNVObj.SetLineNo(L->getLine()); >>
  SN:StringIdentifier ","                           << _PCATCH(mCVfrDataStorage.DeclareNameVarStoreBegin (SN->getText()), SN); >>
  (
    Name "=" "STRING_TOKEN" "\(" N:Number "\)" ","  << _PCATCH(mCVfrDataStorage.NameTableAddItem (_STOSID(N->getText())), SN); >>
  )+
  Uuid "=" guidDefinition[Guid]                     << _PCATCH(mCVfrDataStorage.DeclareNameVarStoreEnd (&Guid), SN); >>
                                                    <<
                                                       VSNVObj.SetGuid (&Guid);
                                                       _PCATCH(mCVfrDataStorage.GetVarStoreId(SN->getText(), &VarStoreId), SN);
                                                       VSNVObj.SetVarStoreId (VarStoreId);
                                                    >>
  ";"
  ;

//
// keep classDeinition and validClassNames for compatibility but not generate
// any IFR object
//
classDefinition[UINT16 & Class] :
  << $Class = 0; >>
  validClassNames[$Class] ( "\|" validClassNames[$Class] )*
  ;

validClassNames[UINT16 & Class] :
    ClassNonDevice                                  << $Class |= EFI_NON_DEVICE_CLASS; >>
  | ClassDiskDevice                                 << $Class |= EFI_DISK_DEVICE_CLASS; >>
  | ClassVideoDevice                                << $Class |= EFI_VIDEO_DEVICE_CLASS; >>
  | ClassNetworkDevice                              << $Class |= EFI_NETWORK_DEVICE_CLASS; >>
  | ClassInputDevice                                << $Class |= EFI_INPUT_DEVICE_CLASS; >>
  | ClassOnBoardDevice                              << $Class |= EFI_ON_BOARD_DEVICE_CLASS; >>
  | ClassOtherDevice                                << $Class |= EFI_OTHER_DEVICE_CLASS; >>
  | N:Number                                        << $Class |= _STOU16(N->getText()); >>
  ;

subclassDefinition[UINT16 & SubClass] :
  << $SubClass = 0; >>
    SubclassSetupApplication                        << $SubClass |= EFI_SETUP_APPLICATION_SUBCLASS; >>
  | SubclassGeneralApplication                      << $SubClass |= EFI_GENERAL_APPLICATION_SUBCLASS; >>
  | SubclassFrontPage                               << $SubClass |= EFI_FRONT_PAGE_SUBCLASS; >>
  | SubclassSingleUse                               << $SubClass |= EFI_SINGLE_USE_SUBCLASS; >>
  | N:Number                                        << $SubClass |= _STOU16(N->getText()); >>
  ;

vfrStatementDisableIfFormSet :
  <<
    CIfrDisableIf DIObj;
    mConstantOnlyInExpression = TRUE;
  >>
  D:DisableIf                                       << DIObj.SetLineNo(D->getLine()); >>
  vfrStatementExpression[0] ";"                     << mConstantOnlyInExpression = FALSE; >>
  vfrFormSetList
  E:EndIf                                           << CRT_END_OP (E); >>
  ";"
  ;

vfrStatementSuppressIfFormSet :
  << CIfrSuppressIf SIObj;>>
  L:SuppressIf                                         <<
                                                           if (mCompatibleMode) {
                                                             _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                           }
                                                           SIObj.SetLineNo(L->getLine()); 
                                                       >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0] ";"
  vfrFormSetList
  E: EndIf 
  ";"                                                  << CRT_END_OP (E); >>
  ;

//*****************************************************************************
//
// the syntax of question header and statement header
//
vfrStatementHeader[CIfrStatementHeader *SHObj] :
  Prompt "=" "STRING_TOKEN" "\(" S1:Number "\)" "," << $SHObj->SetPrompt (_STOSID(S1->getText())); >>
  Help   "=" "STRING_TOKEN" "\(" S2:Number "\)"     << $SHObj->SetHelp (_STOSID(S2->getText())); >>
  ;

vfrQuestionHeader[CIfrQuestionHeader & QHObj, EFI_QUESION_TYPE QType = QUESTION_NORMAL]:
  <<
     EFI_VARSTORE_INFO Info;
     EFI_QUESTION_ID   QId       = EFI_QUESTION_ID_INVALID;
     CHAR8             *QName    = NULL;
     CHAR8             *VarIdStr = NULL;
  >>
  {
    Name "=" QN:StringIdentifier ","                <<
                                                       QName = QN->getText();
                                                       _PCATCH(mCVfrQuestionDB.FindQuestion (QName), VFR_RETURN_UNDEFINED, QN, "has already been used please used anther name");
                                                    >>
  }
  { V:VarId "=" vfrStorageVarId[Info, VarIdStr] "," }
  {
    QuestionId "=" ID:Number ","                    <<
                                                       QId = _STOQID(ID->getText());
                                                       _PCATCH(mCVfrQuestionDB.FindQuestion (QId), VFR_RETURN_UNDEFINED, ID, "has already been used please assign another number");
                                                    >>
  }
                                                    <<
                                                       switch (QType) {
                                                       case QUESTION_NORMAL:
                                                         mCVfrQuestionDB.RegisterQuestion (QName, VarIdStr, QId);
                                                         break;
                                                       case QUESTION_DATE:
                                                         mCVfrQuestionDB.RegisterNewDateQuestion (QName, VarIdStr, QId);
                                                         break;
                                                       case QUESTION_TIME:
                                                         mCVfrQuestionDB.RegisterNewTimeQuestion (QName, VarIdStr, QId);
                                                         break;
                                                       case QUESTION_REF:
                                                         //
                                                         // VarIdStr != NULL stand for question with storagae.
                                                         //
                                                         if (VarIdStr != NULL) {
                                                           mCVfrQuestionDB.RegisterRefQuestion (QName, VarIdStr, QId);
                                                         } else {
                                                           mCVfrQuestionDB.RegisterQuestion (QName, NULL, QId);
                                                         }
                                                         break;
                                                       default:
                                                       _PCATCH(VFR_RETURN_FATAL_ERROR);
                                                       }
                                                       $QHObj.SetQuestionId (QId);
                                                       if (VarIdStr != NULL) {
                                                        $QHObj.SetVarStoreInfo (&Info);
                                                       }
                                                    >>
  vfrStatementHeader[&$QHObj]
                                                    << 
                                                       if (VarIdStr != NULL) {
                                                         delete VarIdStr; 
                                                         _SAVE_CURRQEST_VARINFO (Info);
                                                       }
                                                    >>
  ;

questionheaderFlagsField[UINT8 & Flags] :
    ReadOnlyFlag                                    << $Flags |= 0x01; >>
  | InteractiveFlag                                 << $Flags |= 0x04; >>
  | ResetRequiredFlag                               << $Flags |= 0x10; >>
  | OptionOnlyFlag                                  << $Flags |= 0x80; >>
  | NVAccessFlag
  | LateCheckFlag
  ;

vfrStorageVarId[EFI_VARSTORE_INFO & Info, CHAR8 *&QuestVarIdStr, BOOLEAN CheckFlag = TRUE] :
  <<
     UINT32                Idx;
     UINT32                LineNo;
     EFI_VFR_VARSTORE_TYPE VarStoreType = EFI_VFR_VARSTORE_INVALID;
     CHAR8                 *VarIdStr    = NULL;
     CHAR8                 *VarStr      = NULL;
     CHAR8                 *SName       = NULL;
     CHAR8                 *TName       = NULL;
     EFI_VFR_RETURN_CODE   VfrReturnCode = VFR_RETURN_SUCCESS;
     EFI_IFR_TYPE_VALUE    Dummy        = gZeroEfiIfrTypeValue;
  >>
  (
    SN1:StringIdentifier                            << SName = SN1->getText(); _STRCAT(&VarIdStr, SN1->getText()); >>
    OpenBracket I1:Number CloseBracket              <<
                                                       Idx = _STOU32(I1->getText());
                                                       _STRCAT(&VarIdStr, "[");
                                                       _STRCAT(&VarIdStr, I1->getText());
                                                       _STRCAT(&VarIdStr, "]");
                                                    >>
                                                    <<
                                                       VfrReturnCode = mCVfrDataStorage.GetVarStoreType (SName, VarStoreType);
                                                       if (mCompatibleMode && VfrReturnCode == VFR_RETURN_UNDEFINED) {
                                                          mCVfrDataStorage.DeclareBufferVarStore (
                                                                             SName,
                                                                             &mFormsetGuid,
                                                                             &gCVfrVarDataTypeDB,
                                                                             SName,
                                                                             EFI_VARSTORE_ID_INVALID,
                                                                             FALSE
                                                                             );
                                                          VfrReturnCode = mCVfrDataStorage.GetVarStoreType (SName, VarStoreType);
                                                       }
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH(VfrReturnCode, SN1);
                                                         _PCATCH(mCVfrDataStorage.GetVarStoreId (SName, &$Info.mVarStoreId), SN1);
                                                         _PCATCH(mCVfrDataStorage.GetNameVarStoreInfo (&$Info, Idx), SN1);
                                                       }

                                                       QuestVarIdStr = VarIdStr;
                                                    >>
  )
  |
  (
    SN2:StringIdentifier                            << SName = SN2->getText(); _STRCAT(&VarIdStr, SName); >>
                                                    <<
                                                       VfrReturnCode = mCVfrDataStorage.GetVarStoreType (SName, VarStoreType);
                                                       if (mCompatibleMode && VfrReturnCode == VFR_RETURN_UNDEFINED) {
                                                          mCVfrDataStorage.DeclareBufferVarStore (
                                                                             SName,
                                                                             &mFormsetGuid,
                                                                             &gCVfrVarDataTypeDB,
                                                                             SName,
                                                                             EFI_VARSTORE_ID_INVALID,
                                                                             FALSE
                                                                             );
                                                          VfrReturnCode = mCVfrDataStorage.GetVarStoreType (SName, VarStoreType);
                                                       }
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH(VfrReturnCode, SN2);
                                                         _PCATCH(mCVfrDataStorage.GetVarStoreId (SName, &$Info.mVarStoreId), SN2);
                                                         if (VarStoreType == EFI_VFR_VARSTORE_BUFFER) {
                                                           _PCATCH(mCVfrDataStorage.GetBufferVarStoreDataTypeName(SName, &TName), SN2);
                                                           _STRCAT(&VarStr, TName);
                                                         }
                                                       }
                                                    >>

    (
      "."                                           <<
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH(((VarStoreType != EFI_VFR_VARSTORE_BUFFER) ? VFR_RETURN_EFIVARSTORE_USE_ERROR : VFR_RETURN_SUCCESS), SN2);
                                                       }
                                                       _STRCAT(&VarIdStr, "."); _STRCAT(&VarStr, ".");
                                                    >>
      SF:StringIdentifier                           << _STRCAT(&VarIdStr, SF->getText()); _STRCAT(&VarStr, SF->getText()); >>
      {
        OpenBracket I2:Number CloseBracket          <<
                                                       Idx = _STOU32(I2->getText());
                                                       if (mCompatibleMode) Idx --;
                                                       if (Idx > 0) {
                                                         //
                                                         // Idx == 0, [0] can be ignored.
                                                         // Array[0] is same to Array for unify the varid name to cover [0]
                                                         //
                                                         _STRCAT(&VarIdStr, "[");
                                                         _STRCAT(&VarIdStr, I2->getText());
                                                         _STRCAT(&VarIdStr, "]");
                                                       }
                                                       _STRCAT(&VarStr, "[");
                                                       _STRCAT(&VarStr, I2->getText());
                                                       _STRCAT(&VarStr, "]");
                                                    >>
      }
    )*                                              <<
                                                       switch (VarStoreType) {
                                                       case EFI_VFR_VARSTORE_EFI:
                                                         _PCATCH(mCVfrDataStorage.GetEfiVarStoreInfo (&$Info), SN2);
                                                         break;
                                                       case EFI_VFR_VARSTORE_BUFFER:
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataFieldInfo (VarStr, $Info.mInfo.mVarOffset, $Info.mVarType, $Info.mVarTotalSize), SN2->getLine(), VarStr);
                                                         _PCATCH((EFI_VFR_RETURN_CODE)gCVfrBufferConfig.Register (
                                                                    SName,
                                                                    NULL),
                                                                 SN2->getLine());
                                                         _PCATCH((EFI_VFR_RETURN_CODE)gCVfrBufferConfig.Write (
                                                                    'a',
                                                                    SName,
                                                                    NULL,
                                                                    $Info.mVarType,
                                                                    $Info.mInfo.mVarOffset,
                                                                    $Info.mVarTotalSize,
                                                                    Dummy),
                                                                 SN2->getLine());
                                                         break;
                                                       case EFI_VFR_VARSTORE_NAME:
                                                       default: break;
                                                       }

                                                       QuestVarIdStr = VarIdStr;
                                                       if (VarStr != NULL) {delete VarStr;}
                                                    >>
  )
  ;

vfrQuestionDataFieldName [EFI_QUESTION_ID &QId, UINT32 &Mask, CHAR8 *&VarIdStr, UINT32 &LineNo] :
                                                    <<
                                                      UINT32  Idx;
                                                      VarIdStr = NULL; LineNo = 0;
                                                    >>
  (
    SN1:StringIdentifier                            << _STRCAT(&VarIdStr, SN1->getText()); LineNo = SN1->getLine(); >>
    OpenBracket I1:Number CloseBracket              <<
                                                       _STRCAT(&VarIdStr, "[");
                                                       _STRCAT(&VarIdStr, I1->getText());
                                                       _STRCAT(&VarIdStr, "]");
                                                       mCVfrQuestionDB.GetQuestionId (NULL, VarIdStr, $QId, $Mask);
                                                       if (mConstantOnlyInExpression) {
                                                         _PCATCH(VFR_RETURN_CONSTANT_ONLY, LineNo);
                                                       }
                                                    >>
  )
  |
  (
    SN2:StringIdentifier                            << _STRCAT (&VarIdStr, SN2->getText()); LineNo = SN2->getLine(); >>
    (
      "."                                           << 
                                                       _STRCAT (&VarIdStr, ".");
                                                       if (mConstantOnlyInExpression) {
                                                         _PCATCH(VFR_RETURN_CONSTANT_ONLY, LineNo);
                                                       }
                                                    >>
      SF:StringIdentifier                           << _STRCAT (&VarIdStr, SF->getText()); >>
      {
        OpenBracket I2:Number CloseBracket          <<
                                                       Idx = _STOU32(I2->getText());
                                                       if (mCompatibleMode) Idx --;
                                                       if (Idx > 0) {
                                                         //
                                                         // Idx == 0, [0] can be ignored.
                                                         // Array[0] is same to Array
                                                         //
                                                         _STRCAT(&VarIdStr, "[");
                                                         _STRCAT(&VarIdStr, I2->getText());
                                                         _STRCAT(&VarIdStr, "]");
                                                       }
                                                    >>
      }
    )*
                                                    << mCVfrQuestionDB.GetQuestionId (NULL, VarIdStr, $QId, $Mask); >>
  )
  ;

vfrConstantValueField[UINT8 Type] > [EFI_IFR_TYPE_VALUE Value] :
  <<
    EFI_GUID Guid;
  >>
    N1:Number                                       <<
                                                       switch ($Type) {
                                                       case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                         $Value.u8     = _STOU8(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                         $Value.u16    = _STOU16(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                         $Value.u32    = _STOU32(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                         $Value.u64    = _STOU64(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_BOOLEAN :
                                                         $Value.b      = _STOU8(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_STRING :
                                                         $Value.string = _STOU16(N1->getText());
                                                       break;
                                                       case EFI_IFR_TYPE_TIME :
                                                       case EFI_IFR_TYPE_DATE :
                                                       case EFI_IFR_TYPE_REF  :
                                                       default :
                                                       break;
                                                       }
                                                    >>
  | B1:True                                         << $Value.b      = TRUE; >>
  | B2:False                                        << $Value.b      = FALSE; >>
  | O1:One                                          << $Value.u8     = _STOU8(O1->getText()); >>
  | O2:Ones                                         << $Value.u64    = _STOU64(O2->getText()); >>
  | Z:Zero                                          << $Value.u8     = _STOU8(Z->getText()); >>
  | HOUR:Number ":" MINUTE:Number ":" SECOND:Number << $Value.time   = _STOT(HOUR->getText(), MINUTE->getText(), SECOND->getText()); >>
  | YEAR:Number "/" MONTH:Number "/" DAY:Number     << $Value.date   = _STOD(YEAR->getText(), MONTH->getText(), DAY->getText()); >>
  | QI:Number";" FI:Number";" guidDefinition[Guid] ";" "STRING_TOKEN" "\(" DP:Number "\)" 
                                                    << $Value.ref    = _STOR(QI->getText(), FI->getText(), &Guid, DP->getText()); >>
  | "STRING_TOKEN" "\(" S1:Number "\)"              << $Value.string = _STOSID(S1->getText()); >>
  ;

//*****************************************************************************
//
// the syntax of form definition
//
vfrFormDefinition :
  << CIfrForm FObj; >>
  F:Form                                            << FObj.SetLineNo(F->getLine()); >>
  FormId "=" S1:Number ","                          << _PCATCH(FObj.SetFormId (_STOFID(S1->getText())), S1); >>
  Title "=" "STRING_TOKEN" "\(" S2:Number "\)" ";"  << FObj.SetFormTitle (_STOSID(S2->getText())); >>
  (
    vfrStatementImage                        |
    vfrStatementLocked                       |
    vfrStatementRules                        |
    vfrStatementDefault                      |
    vfrStatementStat                         |
    vfrStatementQuestions                    |
    vfrStatementConditional                  |
    vfrStatementLabel                        |
    vfrStatementBanner                       |
    // Just for framework vfr compatibility
    vfrStatementInvalid                      |
    vfrStatementExtension                    |
    vfrStatementModal
  )*
  E:EndForm                                         <<
                                                      if (mCompatibleMode) {
                                                        //
                                                        // Add Label for Framework Vfr
                                                        //
                                                        CIfrLabel LObj1;
                                                        LObj1.SetLineNo(E->getLine());
                                                        LObj1.SetNumber (0xffff);  //add end label for UEFI, label number hardcode 0xffff
                                                        CIfrLabel LObj2;
                                                        LObj2.SetLineNo(E->getLine());
                                                        LObj2.SetNumber (0x0);     //add dummy label for UEFI, label number hardcode 0x0
                                                        CIfrLabel LObj3;
                                                        LObj3.SetLineNo(E->getLine());
                                                        LObj3.SetNumber (0xffff);  //add end label for UEFI, label number hardcode 0xffff
                                                      }

                                                      //
                                                      // Declare undefined Question so that they can be used in expression.
                                                      //
                                                      if (gCFormPkg.HavePendingUnassigned()) {
                                                        gCFormPkg.DeclarePendingQuestion (
                                                                    gCVfrVarDataTypeDB,
                                                                    mCVfrDataStorage,
                                                                    mCVfrQuestionDB,
                                                                    &mFormsetGuid,
                                                                    E->getLine()
                                                                  );
                                                      }

                                                      //
                                                      // mCVfrQuestionDB.PrintAllQuestion();
                                                      //
                                                      CRT_END_OP (E);
                                                    >>
  ";"
  ;

vfrFormMapDefinition :
  << 
    CIfrFormMap *FMapObj = NULL;
    UINT32      FormMapMethodNumber = 0;
    EFI_GUID    Guid;
  >>
  F:FormMap                                         << FMapObj = new CIfrFormMap(); FMapObj->SetLineNo(F->getLine()); >>
  FormId "=" S1:Number ","                          << _PCATCH(FMapObj->SetFormId (_STOFID(S1->getText())), S1); >>
  (
    MapTitle "=" "STRING_TOKEN" "\(" S2:Number "\)" ";"
    MapGuid  "=" guidDefinition[Guid] ";"           << FMapObj->SetFormMapMethod (_STOFID(S2->getText()), &Guid); FormMapMethodNumber ++; >>
  )*                                                << if (FormMapMethodNumber == 0) {_PCATCH (VFR_RETURN_INVALID_PARAMETER, F->getLine(), "No MapMethod is set for FormMap!");} delete FMapObj;>>
  (
    vfrStatementImage                        |
    vfrStatementLocked                       |
    vfrStatementRules                        |
    vfrStatementDefault                      |
    vfrStatementStat                         |
    vfrStatementQuestions                    |
    vfrStatementConditional                  |
    vfrStatementLabel                        |
    vfrStatementBanner                       |
    vfrStatementExtension                    |
    vfrStatementModal
  )*
  E:EndForm                                         << CRT_END_OP (E); >>
  ";"
  ;

vfrStatementRules :
  << CIfrRule RObj; >>
  R:Rule                                            << RObj.SetLineNo(R->getLine()); >>
  S1:StringIdentifier ","                           <<
                                                       mCVfrRulesDB.RegisterRule (S1->getText());
                                                       RObj.SetRuleId (mCVfrRulesDB.GetRuleId(S1->getText()));
                                                    >>
  vfrStatementExpression[0]
  E:EndRule                                         << CRT_END_OP (E); >>
  ";"
  ;

vfrStatementDefault :
  <<
     BOOLEAN               IsExp         = FALSE;
     EFI_IFR_TYPE_VALUE    Val           = gZeroEfiIfrTypeValue;
     CIfrDefault           DObj;
     EFI_DEFAULT_ID        DefaultId     = EFI_HII_DEFAULT_CLASS_STANDARD;
     CHAR8                 *VarStoreName = NULL;
     EFI_VFR_VARSTORE_TYPE VarStoreType  = EFI_VFR_VARSTORE_INVALID;
  >>
  D:Default                                         << DObj.SetLineNo(D->getLine()); >>
  (
    (
        vfrStatementValue ","                       << IsExp = TRUE; DObj.SetScope (1); CIfrEnd EndObj1; EndObj1.SetLineNo(D->getLine()); >>
      | "=" vfrConstantValueField[_GET_CURRQEST_DATATYPE()] > [Val] ","  << 
                                                        if (gCurrentMinMaxData != NULL && gCurrentMinMaxData->IsNumericOpcode()) {
                                                          //check default value is valid for Numeric Opcode
                                                          if (Val.u64 < gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE()) || Val.u64 > gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE())) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                          }
                                                        }
                                                        DObj.SetType (_GET_CURRQEST_DATATYPE()); 
                                                        DObj.SetValue(Val);
                                                    >>
    )
    {
      DefaultStore "=" SN:StringIdentifier ","      << _PCATCH(mCVfrDefaultStore.GetDefaultId (SN->getText(), &DefaultId), SN); DObj.SetDefaultId (DefaultId); >>
    }
                                                    <<
                                                       _PCATCH(mCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), D->getLine());
                                                       _PCATCH(mCVfrDataStorage.GetVarStoreType (VarStoreName, VarStoreType), D->getLine());
                                                       if ((IsExp == FALSE) && (VarStoreType == EFI_VFR_VARSTORE_BUFFER)) {
                                                         _PCATCH(mCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                   DefaultId,
                                                                   _GET_CURRQEST_VARTINFO(),
                                                                   VarStoreName,
                                                                   _GET_CURRQEST_DATATYPE (),
                                                                   Val),
                                                                   D->getLine()
                                                                   );
                                                       }
                                                    >>
  )
  ;

vfrStatementStat :
  vfrStatementSubTitle        |
  vfrStatementStaticText      |
  vfrStatementCrossReference
  ;

vfrStatementQuestions :
  vfrStatementBooleanType     |
  vfrStatementDate            |
  vfrStatementNumericType     |
  vfrStatementStringType      |
  vfrStatementOrderedList     |
  vfrStatementTime
  ;

vfrStatementConditional :
  vfrStatementDisableIfStat   |
  vfrStatementSuppressIfStat  |    //enhance to be compatible for framework endif
  vfrStatementGrayOutIfStat   |
  vfrStatementInconsistentIfStat   //to be compatible for framework
  ;

vfrStatementConditionalNew :
  vfrStatementDisableIfStat      |
  vfrStatementSuppressIfStatNew  |
  vfrStatementGrayOutIfStatNew   |
  vfrStatementInconsistentIfStat   //to be compatible for framework
  ;

vfrStatementSuppressIfStat :
  <<mCompatibleMode>>? vfrStatementSuppressIfStatOld
  | vfrStatementSuppressIfStatNew
  ;

vfrStatementGrayOutIfStat :
  <<mCompatibleMode>>? vfrStatementGrayOutIfStatOld
  | vfrStatementGrayOutIfStatNew
  ;

vfrStatementInvalid :
  (
    vfrStatementInvalidHidden          |
    vfrStatementInvalidInventory       |
    vfrStatementInvalidSaveRestoreDefaults
  )
                                                       << _CRT_OP (TRUE); >>
  ;

flagsField :
  Number | InteractiveFlag | ManufacturingFlag | DefaultFlag |
  NVAccessFlag | ResetRequiredFlag | LateCheckFlag
  ;

vfrStatementValue :
  << CIfrValue VObj; >>
  V:Value                                              << VObj.SetLineNo(V->getLine()); >>
  "=" vfrStatementExpression[0]                        << {CIfrEnd EndObj; EndObj.SetLineNo(V->getLine());} >>
  ;

vfrStatementRead :
  << CIfrRead RObj; >>
  R:Read                                               << RObj.SetLineNo(R->getLine()); >>
  vfrStatementExpression[0] ";" 
  ;

vfrStatementWrite :
  << CIfrWrite WObj; >>
  W:Write                                              << WObj.SetLineNo(W->getLine()); >>
  vfrStatementExpression[0] ";" 
  ;

vfrStatementSubTitle :
  << CIfrSubtitle SObj; >>
  L:Subtitle                                           << SObj.SetLineNo(L->getLine()); >>
  Text "=" "STRING_TOKEN" "\(" S:Number "\)"           << SObj.SetPrompt (_STOSID(S->getText())); >>
  {
    "," FLAGS "=" vfrSubtitleFlags[SObj]
  }
  { vfrStatementStatTagList "," }
  E:";"                                                << CRT_END_OP (E); >>
  ;

vfrSubtitleFlags [CIfrSubtitle & SObj] :
  << UINT8 LFlags = 0; >>
  subtitleFlagsField[LFlags] ( "\|" subtitleFlagsField[LFlags] )*
                                                       << _PCATCH(SObj.SetFlags (LFlags)); >>
  ;

subtitleFlagsField [UINT8 & Flags] :
    N:Number                                           << $Flags |= _STOU8(N->getText()); >>
  | "HORIZONTAL"                                       << $Flags |= 0x01; >>
  ;

vfrStatementStaticText :
  <<
     UINT8           Flags   = 0;
     EFI_QUESTION_ID QId     = EFI_QUESTION_ID_INVALID;
     EFI_STRING_ID   TxtTwo  = EFI_STRING_ID_INVALID;
   >>
  T:Text
  Help "=" "STRING_TOKEN" "\(" S1:Number "\)" ","
  Text "=" "STRING_TOKEN" "\(" S2:Number "\)"
  {
    "," Text "=" "STRING_TOKEN" "\(" S3:Number "\)"    << TxtTwo = _STOSID(S3->getText()); >>
  }
  {
    "," F:FLAGS "=" staticTextFlagsField[Flags] ( "\|" staticTextFlagsField[Flags] )*
    "," Key "=" KN:Number
  }
                                                       <<
                                                          if (Flags & EFI_IFR_FLAG_CALLBACK) {
                                                            CIfrAction AObj;
                                                            mCVfrQuestionDB.RegisterQuestion (NULL, NULL, QId);
                                                            AObj.SetLineNo (F->getLine());
                                                            AObj.SetQuestionId (QId);
                                                            AObj.SetPrompt (_STOSID(S2->getText()));
                                                            AObj.SetHelp (_STOSID(S1->getText()));
                                                            _PCATCH(AObj.SetFlags (Flags), F->getLine());
                                                            AssignQuestionKey (AObj, KN);
                                                            CRT_END_OP (KN);
                                                          } else {
                                                            CIfrText TObj;
                                                            TObj.SetLineNo (T->getLine());
                                                            TObj.SetHelp (_STOSID(S1->getText()));
                                                            TObj.SetPrompt (_STOSID(S2->getText()));
                                                            TObj.SetTextTwo (TxtTwo);
                                                          }
                                                       >>
  { "," vfrStatementStatTagList }
  ";"
  ;

staticTextFlagsField[UINT8 & HFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementCrossReference :
  vfrStatementGoto            |
  vfrStatementResetButton
  ;

vfrStatementGoto :
  <<
     UINT8               RefType = 5;
     EFI_STRING_ID       DevPath = EFI_STRING_ID_INVALID;
     EFI_GUID            FSId = {0,};
     EFI_FORM_ID         FId;
     EFI_QUESTION_ID     QId    = EFI_QUESTION_ID_INVALID;
     UINT32              BitMask;
     CIfrQuestionHeader  *QHObj = NULL;
     CIfrOpHeader        *OHObj = NULL;
     CIfrRef             *R1Obj = NULL;
     CIfrRef2            *R2Obj = NULL;
     CIfrRef3            *R3Obj = NULL;
     CIfrRef4            *R4Obj = NULL;
     CIfrRef5            *R5Obj = NULL;
  >>
  G:Goto
  {
    (
      DevicePath "=" "STRING_TOKEN" "\(" P:Number "\)" ","
      FormSetGuid "=" guidDefinition[FSId] ","
      FormId "=" F1:Number ","
      Question "=" QN1:Number ","
                                                       <<
                                                          RefType = 4;
                                                          DevPath = _STOSID(P->getText());
                                                          FId = _STOFID(F1->getText());
                                                          QId = _STOQID(QN1->getText());
                                                       >>
    )
    |
    (
      FormSetGuid "=" guidDefinition[FSId] ","
      FormId "=" F2:Number ","
      Question "=" QN2:Number ","
                                                       <<
                                                          RefType = 3;
                                                          FId = _STOFID(F2->getText());
                                                          QId = _STOQID(QN2->getText());
                                                       >>
    )
    |
    (
      FormId "=" F3:Number ","                         << RefType = 2; FId = _STOFID(F3->getText()); >>
      Question "="
      (
          QN3:StringIdentifier ","                     << 
                                                          mCVfrQuestionDB.GetQuestionId (QN3->getText (), NULL, QId, BitMask);
                                                          if (QId == EFI_QUESTION_ID_INVALID) {
                                                            _PCATCH(VFR_RETURN_UNDEFINED, QN3);
                                                          }
                                                       >>
        | QN4:Number ","                               << QId = _STOQID(QN4->getText()); >>
      )
    )
    |
    (
      F4:Number ","                                    <<
                                                          RefType = 1;
                                                          FId = _STOFID(F4->getText());
                                                       >>
    )
  }
                                                       <<
                                                          switch (RefType) {
                                                          case 5:
                                                            {
                                                              R5Obj = new CIfrRef5;
                                                              QHObj = R5Obj;
                                                              OHObj = R5Obj;
                                                              R5Obj->SetLineNo(G->getLine());
                                                              break;
                                                            }
                                                          case 4:
                                                            {
                                                              R4Obj = new CIfrRef4;
                                                              QHObj = R4Obj;
                                                              OHObj = R4Obj;
                                                              R4Obj->SetLineNo(G->getLine());
                                                              R4Obj->SetDevicePath (DevPath);
                                                              R4Obj->SetFormSetId (FSId);
                                                              R4Obj->SetFormId (FId);
                                                              R4Obj->SetQuestionId (QId);
                                                              break;
                                                            }
                                                          case 3:
                                                            {
                                                              R3Obj = new CIfrRef3;
                                                              QHObj = R3Obj;
                                                              OHObj = R3Obj;
                                                              R3Obj->SetLineNo(G->getLine());
                                                              R3Obj->SetFormSetId (FSId);
                                                              R3Obj->SetFormId (FId);
                                                              R3Obj->SetQuestionId (QId);
                                                              break;
                                                            }
                                                          case 2:
                                                            {
                                                              R2Obj = new CIfrRef2;
                                                              QHObj = R2Obj;
                                                              OHObj = R2Obj;
                                                              R2Obj->SetLineNo(G->getLine());
                                                              R2Obj->SetFormId (FId);
                                                              R2Obj->SetQuestionId (QId);
                                                              break;
                                                            }
                                                          case 1:
                                                            {
                                                              R1Obj = new CIfrRef;
                                                              QHObj = R1Obj;
                                                              OHObj = R1Obj;
                                                              R1Obj->SetLineNo(G->getLine());
                                                              R1Obj->SetFormId (FId);
                                                              break;
                                                            }
                                                          default: break;
                                                          }
                                                       >>
  vfrQuestionHeader[*QHObj, QUESTION_REF]
  { "," F:FLAGS  "=" vfrGotoFlags[QHObj, F->getLine()] }
  {
    "," Key "=" KN:Number                              << AssignQuestionKey (*QHObj, KN); >>
  }
  {
    E:"," 
      vfrStatementQuestionOptionList                   << OHObj->SetScope(1); CRT_END_OP (E);>>
  }
  ";"                                                  << if (R1Obj != NULL) {delete R1Obj;} if (R2Obj != NULL) {delete R2Obj;} if (R3Obj != NULL) {delete R3Obj;} if (R4Obj != NULL) {delete R4Obj;} if (R5Obj != NULL) {delete R5Obj;}>>
  ;

vfrGotoFlags [CIfrQuestionHeader *QHObj, UINT32 LineNum] :
  << UINT8 HFlags = 0; >>
  gotoFlagsField[HFlags] ( "\|" gotoFlagsField[HFlags] )*
                                                       << _PCATCH(QHObj->SetFlags (HFlags), LineNum); >>
  ;

gotoFlagsField[UINT8 & HFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

getStringId :
  "STRING_TOKEN" "\("
  IdVal:Number
  "\)"
  ;

vfrStatementResetButton :
  <<
     CIfrResetButton RBObj;
     UINT16          DefaultId;
  >>
  L:ResetButton                                        << RBObj.SetLineNo(L->getLine()); >>
  DefaultStore
  "=" N:StringIdentifier ","                           <<
                                                          _PCATCH(mCVfrDefaultStore.GetDefaultId (N->getText(), &DefaultId), N->getLine());
                                                          RBObj.SetDefaultId (DefaultId);
                                                       >>
  vfrStatementHeader[&RBObj] ","
  { vfrStatementStatTagList "," }
  E:EndResetButton                                     << CRT_END_OP (E); >>
  ";"
  ;

vfrStatementBooleanType :
  vfrStatementCheckBox |
  vfrStatementAction
  ;

//*****************************************************
// Syntax of checkbox
//
// Example:
//   checkbox
//     varid       = MySTestData.mField1,
//     prompt      = STRING_TOKEN(STR_CHECK_BOX_PROMPT),
//     help        = STRING_TOKEN(STR_CHECK_BOX_HELP),
//     flags       = CHECKBOX_DEFAULT | CALLBACK,
//     default value = TRUE, defaultstore = MyDefaultStore,
//   endcheckbox;
//
vfrStatementCheckBox :
  <<
     CIfrCheckBox       CBObj;
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
     CHAR8              *VarStoreName = NULL;
     UINT32             DataTypeSize;
  >>
  L:CheckBox                                           << CBObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[CBObj] ","                         << //check data type
                                                          _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "CheckBox varid is not the valid data type");
                                                          if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "CheckBox varid doesn't support array");
                                                          } else if ((mCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId) == EFI_VFR_VARSTORE_BUFFER) &&
                                                                    (_GET_CURRQEST_VARSIZE() != sizeof (BOOLEAN))) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "CheckBox varid only support BOOLEAN data type");
                                                          }
                                                       >>
  {
    F:FLAGS "=" vfrCheckBoxFlags[CBObj, F->getLine()] ","
                                                       <<
                                                          _PCATCH(mCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), VFR_RETURN_SUCCESS, L, "Failed to retrieve varstore name");
                                                          Val.b = TRUE;
                                                          if (CBObj.GetFlags () & 0x01) {
                                                            _PCATCH(
                                                              mCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                                  EFI_HII_DEFAULT_CLASS_STANDARD,
                                                                                  _GET_CURRQEST_VARTINFO(),
                                                                                  VarStoreName,
                                                                                  _GET_CURRQEST_DATATYPE (),
                                                                                  Val
                                                                                  ),
                                                              VFR_RETURN_SUCCESS,
                                                              L,
                                                              "No standard default storage found"
                                                              );
                                                          }
                                                          if (CBObj.GetFlags () & 0x02) {
                                                            _PCATCH(
                                                              mCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                                  EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                                                                                  _GET_CURRQEST_VARTINFO(),
                                                                                  VarStoreName,
                                                                                  _GET_CURRQEST_DATATYPE (),
                                                                                  Val
                                                                                  ),
                                                              VFR_RETURN_SUCCESS,
                                                              L,
                                                              "No manufacturing default storage found"
                                                              );
                                                          }
                                                        >>
  }
  {
    Key "=" KN:Number  ","                             << AssignQuestionKey (CBObj, KN); >>
  }
  vfrStatementQuestionOptionList
  E:EndCheckBox                                        << CRT_END_OP (E); >>
  ";"
  ;

vfrCheckBoxFlags [CIfrCheckBox & CBObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = 0;
     UINT8 HFlags = 0;
  >>
  checkboxFlagsField[LFlags, HFlags] ( "\|" checkboxFlagsField[LFlags, HFlags] )*
                                                       << _PCATCH(CBObj.SetFlags (HFlags, LFlags), LineNum); >>
  ;

checkboxFlagsField[UINT8 & LFlags, UINT8 & HFlags] :
    N:Number                                           <<
                                                          if (mCompatibleMode) {
                                                            //
                                                            // set question flag
                                                            //
                                                            $LFlags |= _STOU8(N->getText());
                                                          } else {
                                                            _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine());
                                                          }
                                                       >>
  | D:"DEFAULT"                                        <<
                                                          if (mCompatibleMode) {
                                                            //
                                                            // set question Default flag
                                                            //
                                                            $LFlags |= 0x01;
                                                          } else {
                                                            _PCATCH (VFR_RETURN_UNSUPPORTED, D);
                                                          }
                                                       >>
  | M:"MANUFACTURING"                                  <<
                                                          if (mCompatibleMode) {
                                                            //
                                                            // set question MFG flag
                                                            //
                                                            $LFlags |= 0x02;
                                                          } else {
                                                            _PCATCH (VFR_RETURN_UNSUPPORTED, M);
                                                          }
                                                       >>
  | "CHECKBOX_DEFAULT"                                 << $LFlags |= 0x01; >>
  | "CHECKBOX_DEFAULT_MFG"                             << $LFlags |= 0x02; >>
  | questionheaderFlagsField[HFlags]
  ;

//*****************************************************
// Syntax of action
//
// Example:
//   action
//     prompt      = STRING_TOKEN(STR_ACTION_PROMPT),
//     help        = STRING_TOKEN(STR_ACTION_HELP),
//     flags       = CALLBACK,
//     config      = STRING_TOKEN(STR_ACTION_CONFIG),
//   endaction;
//
vfrStatementAction :
  << CIfrAction AObj; >>
  L:Action                                             << AObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[AObj] ","
  { F:FLAGS "=" vfrActionFlags[AObj, F->getLine()] "," }
  Config "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << AObj.SetQuestionConfig (_STOSID(S->getText())); >>
  vfrStatementQuestionTagList
  E:EndAction                                          << CRT_END_OP (E); >>
  ";"
  ;

vfrActionFlags[CIfrAction & AObj, UINT32 LineNum] :
  << UINT8 HFlags = 0; >>
  actionFlagsField[HFlags] ( "\|" actionFlagsField[HFlags] )*
                                                       << _PCATCH(AObj.SetFlags (HFlags), LineNum); >>
  ;

actionFlagsField[UINT8 & HFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementDate :
  <<
     EFI_QUESTION_ID    QId          = EFI_QUESTION_ID_INVALID;
     CHAR8              *VarIdStr[3] = {NULL, };
     CIfrDate           DObj;
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
  >>
  L:Date                                               << DObj.SetLineNo(L->getLine()); >>
  (
    (
      vfrQuestionHeader[DObj, QUESTION_DATE] ","
    { F:FLAGS "=" vfrDateFlags[DObj, F->getLine()] "," }
      vfrStatementQuestionOptionList
    )
    |
    (
      Year VarId "=" D1:StringIdentifier "." D1Y:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[0], D1->getText()); _STRCAT(&VarIdStr[0], "."); _STRCAT(&VarIdStr[0], D1Y->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" YP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" YH:Number "\)" ","
      minMaxDateStepDefault[Val.date, 0]

      Month VarId "=" D2:StringIdentifier "." D2M:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[1], D2->getText()); _STRCAT(&VarIdStr[1], "."); _STRCAT(&VarIdStr[1], D2M->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" MP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" MH:Number "\)" ","
      minMaxDateStepDefault[Val.date, 1]

      Day VarId "=" D3:StringIdentifier "." D3D:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[2], D3->getText()); _STRCAT(&VarIdStr[2], "."); _STRCAT(&VarIdStr[2], D3D->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" DP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" DH:Number "\)" ","
      minMaxDateStepDefault[Val.date, 2]
      { G:FLAGS "=" vfrDateFlags[DObj, G->getLine()] "," }
                                                       <<
                                                          mCVfrQuestionDB.RegisterOldDateQuestion (VarIdStr[0], VarIdStr[1], VarIdStr[2], QId);
                                                          DObj.SetQuestionId (QId);
                                                          DObj.SetFlags (EFI_IFR_QUESTION_FLAG_DEFAULT, QF_DATE_STORAGE_TIME);
                                                          DObj.SetPrompt (_STOSID(YP->getText()));
                                                          DObj.SetHelp (_STOSID(YH->getText()));
                                                          if (VarIdStr[0] != NULL) { delete VarIdStr[0]; } if (VarIdStr[1] != NULL) { delete VarIdStr[1]; } if (VarIdStr[2] != NULL) { delete VarIdStr[2]; }
                                                       >>
                                                       << {CIfrDefault DefaultObj(EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_DATE, Val); DefaultObj.SetLineNo(L->getLine());} >>
    )
    ( vfrStatementInconsistentIf )*
  )
  E:EndDate                                            << CRT_END_OP (E); >>
  ";"
  ;

minMaxDateStepDefault[EFI_HII_DATE & D, UINT8 KeyValue] :
  Minimum   "=" MinN:Number ","
  Maximum   "=" MaxN:Number ","
  { "step"    "=" Number "," }
  {
    "default" "=" N:Number ","                         <<
                                                          switch (KeyValue) {
                                                          case 0: 
                                                            D.Year  = _STOU16(N->getText());
                                                            if (D.Year < _STOU16 (MinN->getText()) || D.Year > _STOU16 (MaxN->getText())) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Year default value must be between Min year and Max year.");
                                                            }
                                                            break;
                                                          case 1: 
                                                            D.Month = _STOU8(N->getText()); 
                                                            if (D.Month < 1 || D.Month > 12) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Month default value must be between 1 and 12.");
                                                            }
                                                            break;
                                                          case 2: 
                                                            D.Day = _STOU8(N->getText()); 
                                                            if (D.Day < 1 || D.Day > 31) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Day default value must be between 1 and 31.");
                                                            }
                                                            break;
                                                          }
                                                       >>
  }
  ;

vfrDateFlags [CIfrDate & DObj, UINT32 LineNum] :
  << UINT8 LFlags = 0; >>
  dateFlagsField[LFlags] ( "\|" dateFlagsField[LFlags] )*
                                                       << _PCATCH(DObj.SetFlags (EFI_IFR_QUESTION_FLAG_DEFAULT, LFlags), LineNum); >>
  ;

dateFlagsField [UINT8 & Flags] :
    N:Number                                           << $Flags |= _STOU8(N->getText()); >>
  | "YEAR_SUPPRESS"                                    << $Flags |= 0x01; >>
  | "MONTH_SUPPRESS"                                   << $Flags |= 0x02; >>
  | "DAY_SUPPRESS"                                     << $Flags |= 0x04; >>
  | "STORAGE_NORMAL"                                   << $Flags |= 0x00; >>
  | "STORAGE_TIME"                                     << $Flags |= 0x10; >>
  | "STORAGE_WAKEUP"                                   << $Flags |= 0x20; >>
  ;

vfrStatementNumericType :
  vfrStatementNumeric   |
  vfrStatementOneOf
  ;

vfrSetMinMaxStep[CIfrMinMaxStepData & MMSDObj] :
  <<
     UINT64 MaxU8 = 0, MinU8 = 0, StepU8 = 0;
     UINT32 MaxU4 = 0, MinU4 = 0, StepU4 = 0;
     UINT16 MaxU2 = 0, MinU2 = 0, StepU2 = 0;
     UINT8  MaxU1 = 0, MinU1 = 0, StepU1 = 0;
  >>
  Minimum   "=" I:Number ","
                                                       <<
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                          case EFI_IFR_TYPE_NUM_SIZE_64 : MinU8 = _STOU64(I->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_32 : MinU4 = _STOU32(I->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_16 : MinU2 = _STOU16(I->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_8 :  MinU1 = _STOU8(I->getText());  break;
                                                          }
                                                       >>
  Maximum   "=" A:Number ","
                                                       <<
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                          case EFI_IFR_TYPE_NUM_SIZE_64 : 
                                                            MaxU8 = _STOU64(A->getText()); 
                                                            if (MaxU8 < MinU8) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                            }
                                                            break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_32 : 
                                                            MaxU4 = _STOU32(A->getText()); 
                                                            if (MaxU4 < MinU4) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                            }
                                                            break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_16 : 
                                                            MaxU2 = _STOU16(A->getText()); 
                                                            if (MaxU2 < MinU2) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                            }
                                                            break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_8 :  
                                                            MaxU1 = _STOU8(A->getText());  
                                                            if (MaxU1 < MinU1) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                            }
                                                            break;
                                                          }
                                                       >>
  {
    STEP    "=" S:Number ","
                                                       <<
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                          case EFI_IFR_TYPE_NUM_SIZE_64 : StepU8 = _STOU64(S->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_32 : StepU4 = _STOU32(S->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_16 : StepU2 = _STOU16(S->getText()); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_8 :  StepU1 = _STOU8(S->getText());  break;
                                                          }
                                                       >>
  }
                                                       <<
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                          case EFI_IFR_TYPE_NUM_SIZE_64 : $MMSDObj.SetMinMaxStepData (MinU8, MaxU8, StepU8); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_32 : $MMSDObj.SetMinMaxStepData (MinU4, MaxU4, StepU4); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_16 : $MMSDObj.SetMinMaxStepData (MinU2, MaxU2, StepU2); break;
                                                          case EFI_IFR_TYPE_NUM_SIZE_8 :  $MMSDObj.SetMinMaxStepData (MinU1, MaxU1, StepU1);  break;
                                                          }
                                                       >>
  ;

vfrStatementNumeric :
  <<
     CIfrNumeric NObj;
     UINT32 DataTypeSize;
     BOOLEAN IsSupported;
  >>
  L:Numeric                                            << NObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[NObj] ","                          << // check data type
                                                          _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "Numeric varid is not the valid data type");
                                                          if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Numeric varid doesn't support array");
                                                          }
                                                          _PCATCH(NObj.SetFlags (NObj.FLAGS(), _GET_CURRQEST_DATATYPE()), L->getLine());
                                                       >>
  { F:FLAGS "=" vfrNumericFlags[NObj, F->getLine()] "," }
  {
    Key   "=" KN:Number ","                            << AssignQuestionKey (NObj, KN); >>
  }
  vfrSetMinMaxStep[NObj]
  vfrStatementQuestionOptionList
  E:EndNumeric                                         << 
                                                          IsSupported = FALSE;
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_8:
                                                            case EFI_IFR_TYPE_NUM_SIZE_16:
                                                            case EFI_IFR_TYPE_NUM_SIZE_32:
                                                            case EFI_IFR_TYPE_NUM_SIZE_64:
                                                              IsSupported = TRUE;
                                                              break;
                                                            default:
                                                              break;
                                                          }
                                                          if (!IsSupported) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.");
                                                          }
                                                          CRT_END_OP (E); 
                                                       >>
  ";"
  ;

vfrNumericFlags [CIfrNumeric & NObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     UINT8 HFlags = 0;
     EFI_VFR_VARSTORE_TYPE VarStoreType = EFI_VFR_VARSTORE_INVALID;
  >>
  numericFlagsField[HFlags, LFlags] ( "\|" numericFlagsField[HFlags, LFlags] )*
                                                       <<
                                                          //check data type flag
                                                          VarStoreType = mCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                          if (VarStoreType == EFI_VFR_VARSTORE_BUFFER || VarStoreType == EFI_VFR_VARSTORE_EFI) {
                                                            if (_GET_CURRQEST_DATATYPE() != (LFlags & EFI_IFR_NUMERIC_SIZE)) {
                                                              _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Numeric Flag is not same to Numeric VarData type");
                                                            }
                                                          } else {
                                                            // update data type for name/value store
                                                            UINT32 DataTypeSize;
                                                            _GET_CURRQEST_VARTINFO().mVarType = LFlags & EFI_IFR_NUMERIC_SIZE;
                                                            gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize);
                                                            _GET_CURRQEST_VARTINFO().mVarTotalSize = DataTypeSize;
                                                          }
                                                          _PCATCH(NObj.SetFlags (HFlags, LFlags), LineNum);
                                                       >>
  ;

numericFlagsField [UINT8 & HFlags, UINT8 & LFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | "NUMERIC_SIZE_1"                                   << $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1; >>
  | "NUMERIC_SIZE_2"                                   << $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2; >>
  | "NUMERIC_SIZE_4"                                   << $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4; >>
  | "NUMERIC_SIZE_8"                                   << $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8; >>
  | "DISPLAY_INT_DEC"                                  << $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC; >>
  | "DISPLAY_UINT_DEC"                                 << $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC; >>
  | "DISPLAY_UINT_HEX"                                 << $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX; >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementOneOf :
  <<
     CIfrOneOf OObj;
     UINT32    DataTypeSize;
     BOOLEAN   IsSupported;
  >>
  L:OneOf                                              << OObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[OObj] ","                          << //check data type
                                                          _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "OneOf varid is not the valid data type");
                                                          if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "OneOf varid doesn't support array");
                                                          }
                                                          _PCATCH(OObj.SetFlags (OObj.FLAGS(), _GET_CURRQEST_DATATYPE()), L->getLine());
                                                       >>
  { F:FLAGS "=" vfrOneofFlagsField[OObj, F->getLine()] "," }
  {
    vfrSetMinMaxStep[OObj]
  }
  vfrStatementQuestionOptionList
  E:EndOneOf                                           << 
                                                          IsSupported = FALSE;
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_8:
                                                            case EFI_IFR_TYPE_NUM_SIZE_16:
                                                            case EFI_IFR_TYPE_NUM_SIZE_32:
                                                            case EFI_IFR_TYPE_NUM_SIZE_64:
                                                              IsSupported = TRUE;
                                                              break;
                                                            default:
                                                              break;
                                                          }
                                                          if (!IsSupported) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.");
                                                          }
                                                          CRT_END_OP (E); 
                                                       >>
  ";"
  ;

vfrOneofFlagsField [CIfrOneOf & OObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     UINT8 HFlags = 0;
     EFI_VFR_VARSTORE_TYPE VarStoreType = EFI_VFR_VARSTORE_INVALID;
  >>
  numericFlagsField[HFlags, LFlags] ( "\|" numericFlagsField[HFlags, LFlags] )*
                                                       <<
                                                          //check data type flag
                                                          VarStoreType = mCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                          if (VarStoreType == EFI_VFR_VARSTORE_BUFFER || VarStoreType == EFI_VFR_VARSTORE_EFI) {
                                                            if (_GET_CURRQEST_DATATYPE() != (LFlags & EFI_IFR_NUMERIC_SIZE)) {
                                                              _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Numeric Flag is not same to Numeric VarData type");
                                                            }
                                                          } else {
                                                            // update data type for Name/Value store
                                                            UINT32 DataTypeSize;
                                                            _GET_CURRQEST_VARTINFO().mVarType = LFlags & EFI_IFR_NUMERIC_SIZE;
                                                            gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize);
                                                            _GET_CURRQEST_VARTINFO().mVarTotalSize = DataTypeSize;
                                                          }
                                                          _PCATCH(OObj.SetFlags (HFlags, LFlags), LineNum);
                                                       >>
  ;

vfrStatementStringType :
  vfrStatementString    |
  vfrStatementPassword
  ;

vfrStatementString :
  <<
     CIfrString SObj;
     UINT32 VarArraySize;
     UINT8 StringMinSize;
     UINT8 StringMaxSize;
  >>
  L:String                                             << SObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[SObj] ","
  { F:FLAGS "=" vfrStringFlagsField[SObj, F->getLine()] "," }
  {
    Key "=" KN:Number ","                              << AssignQuestionKey (SObj, KN); >>
  }
  MinSize "=" MIN:Number ","                           << 
                                                          VarArraySize = _GET_CURRQEST_ARRAY_SIZE();
                                                          StringMinSize = _STOU8(MIN->getText());
                                                          if (_STOU64(MIN->getText()) > StringMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "String MinSize takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && StringMinSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "String MinSize can't be larger than the max number of elements in string array.");
                                                          }
                                                          SObj.SetMinSize (StringMinSize);
                                                       >>
  MaxSize "=" MAX:Number ","                           << 
                                                          StringMaxSize = _STOU8(MAX->getText());
                                                          if (_STOU64(MAX->getText()) > StringMaxSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && StringMaxSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize can't be larger than the max number of elements in string array.");
                                                          } else if (StringMaxSize < StringMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize can't be less than String MinSize.");
                                                          }
                                                          SObj.SetMaxSize (StringMaxSize);
                                                       >>
  vfrStatementQuestionOptionList
  E:EndString                                          << CRT_END_OP (E); >>
  ";"
  ;

vfrStringFlagsField [CIfrString & SObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = 0;
     UINT8 HFlags = 0;
  >>
  stringFlagsField[HFlags, LFlags] ( "\|" stringFlagsField[HFlags, LFlags] )*
                                                       << _PCATCH(SObj.SetFlags (HFlags, LFlags), LineNum); >>
  ;

stringFlagsField [UINT8 & HFlags, UINT8 & LFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | "MULTI_LINE"                                       << $LFlags = 0x01; >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementPassword :
  <<
     CIfrPassword PObj;
     UINT32 VarArraySize;
     UINT16 PasswordMinSize;
     UINT16 PasswordMaxSize;
  >>
  L:Password                                           << PObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[PObj] ","
  { F:FLAGS "=" vfrPasswordFlagsField[PObj, F->getLine()] "," }
  {
    Key "=" KN:Number ","                              << AssignQuestionKey (PObj, KN); >>
  }
  MinSize "=" MIN:Number ","                           << 
                                                          VarArraySize = _GET_CURRQEST_ARRAY_SIZE();
                                                          PasswordMinSize = _STOU16(MIN->getText());
                                                          if (_STOU64(MIN->getText()) > PasswordMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "Password MinSize takes only two byte, which can't be larger than 0xFFFF.");
                                                          } else if (VarArraySize != 0 && PasswordMinSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "Password MinSize can't be larger than the max number of elements in password array.");
                                                          }
                                                          PObj.SetMinSize (PasswordMinSize);
                                                       >>
  MaxSize "=" MAX:Number ","                           << 
                                                          PasswordMaxSize = _STOU16(MAX->getText());
                                                          if (_STOU64(MAX->getText()) > PasswordMaxSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "Password MaxSize takes only two byte, which can't be larger than 0xFFFF.");
                                                          } else if (VarArraySize != 0 && PasswordMaxSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "Password MaxSize can't be larger than the max number of elements in password array.");
                                                          } else if (PasswordMaxSize < PasswordMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "Password MaxSize can't be less than Password MinSize.");
                                                          }
                                                          PObj.SetMaxSize (PasswordMaxSize);
                                                       >>
  { Encoding "=" Number "," }
  vfrStatementQuestionOptionList
  E:EndPassword                                        << CRT_END_OP (E); >>
  ";"
  ;

vfrPasswordFlagsField [CIfrPassword & PObj, UINT32 LineNum] :
  << UINT8 HFlags = 0; >>
  passwordFlagsField[HFlags] ( "\|" passwordFlagsField[HFlags] )*
                                                       << _PCATCH(PObj.SetFlags(HFlags), LineNum); >>
  ;

passwordFlagsField [UINT8 & HFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementOrderedList :
  <<
     CIfrOrderedList OLObj;
     UINT32 VarArraySize;
  >>
  L:OrderedList                                        << OLObj.SetLineNo(L->getLine()); >>
  vfrQuestionHeader[OLObj] ","
                                                       << 
                                                          VarArraySize = _GET_CURRQEST_ARRAY_SIZE();
                                                          OLObj.SetMaxContainers ((UINT8) (VarArraySize > 0xFF ? 0xFF : VarArraySize));
                                                       >>
  {
    MaxContainers "=" M:Number ","                     << 
                                                          if (_STOU64(M->getText()) > _STOU8(M->getText())) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, M->getLine(), "OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && _STOU8(M->getText()) > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, M->getLine(), "OrderedList MaxContainers can't be larger than the max number of elements in array.");
                                                          }
                                                          OLObj.SetMaxContainers (_STOU8(M->getText()));
                                                       >>
  }
  { F:FLAGS "=" vfrOrderedListFlags[OLObj, F->getLine()] }
  vfrStatementQuestionOptionList
  E:EndList                                            << CRT_END_OP (E); >>
  ";"
  ;

vfrOrderedListFlags [CIfrOrderedList & OLObj, UINT32 LineNum] :
  <<
     UINT8 HFlags = 0;
     UINT8 LFlags = 0;
  >>
  orderedlistFlagsField[HFlags, LFlags] ( "\|" orderedlistFlagsField[HFlags, LFlags] )*
                                                       << _PCATCH(OLObj.SetFlags (HFlags, LFlags), LineNum); >>
  ;

orderedlistFlagsField [UINT8 & HFlags, UINT8 & LFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | "UNIQUE"                                           << $LFlags |= 0x01; >>
  | "NOEMPTY"                                          << $LFlags |= 0x02; >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementTime :
  <<
     EFI_QUESTION_ID    QId          = EFI_QUESTION_ID_INVALID;
     CHAR8              *VarIdStr[3] = {NULL, };
     CIfrTime           TObj;
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
  >>
  L:Time                                               << TObj.SetLineNo(L->getLine()); >>
  (
    (
      vfrQuestionHeader[TObj, QUESTION_TIME] ","
    { F:FLAGS "=" vfrTimeFlags[TObj, F->getLine()] "," }
      vfrStatementQuestionOptionList
    )
    |
    (
      Hour VarId "=" T1:StringIdentifier "." T1H:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[0], T1->getText()); _STRCAT(&VarIdStr[0], "."); _STRCAT(&VarIdStr[0], T1H->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" HP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" HH:Number "\)" ","
      minMaxTimeStepDefault[Val.time, 0]

      Minute VarId "=" T2:StringIdentifier "." T2M:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[1], T2->getText()); _STRCAT(&VarIdStr[1], "."); _STRCAT(&VarIdStr[1], T2M->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" MP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" MH:Number "\)" ","
      minMaxTimeStepDefault[Val.time, 1]

      Second VarId "=" T3:StringIdentifier "." T3S:StringIdentifier ","
                                                       << _STRCAT(&VarIdStr[2], T3->getText()); _STRCAT(&VarIdStr[2], "."); _STRCAT(&VarIdStr[2], T3S->getText()); >>
      Prompt "=" "STRING_TOKEN" "\(" SP:Number "\)" ","
      Help   "=" "STRING_TOKEN" "\(" SH:Number "\)" ","
      minMaxTimeStepDefault[Val.time, 2]
      { G:FLAGS "=" vfrTimeFlags[TObj, G->getLine()] "," }
                                                       <<
                                                          mCVfrQuestionDB.RegisterOldTimeQuestion (VarIdStr[0], VarIdStr[1], VarIdStr[2], QId);
                                                          TObj.SetQuestionId (QId);
                                                          TObj.SetFlags (EFI_IFR_QUESTION_FLAG_DEFAULT, QF_TIME_STORAGE_TIME);
                                                          TObj.SetPrompt (_STOSID(HP->getText()));
                                                          TObj.SetHelp (_STOSID(HH->getText()));
                                                          if (VarIdStr[0] != NULL) { delete VarIdStr[0]; } if (VarIdStr[1] != NULL) { delete VarIdStr[1]; } if (VarIdStr[2] != NULL) { delete VarIdStr[2]; }
                                                       >>
                                                       << {CIfrDefault DefaultObj(EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_TIME, Val); DefaultObj.SetLineNo(L->getLine());} >>
    )
    ( vfrStatementInconsistentIf )*
  )
  E:EndTime                                            << CRT_END_OP (E); >>
  ";"
  ;

minMaxTimeStepDefault[EFI_HII_TIME & T, UINT8 KeyValue] :
  Minimum   "=" Number ","
  Maximum   "=" Number ","
  { "step"    "=" Number "," }
  {
    "default" "=" N:Number ","                         <<
                                                          switch (KeyValue) {
                                                          case 0: 
                                                            T.Hour   = _STOU8(N->getText()); 
                                                            if (T.Hour > 23) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Hour default value must be between 0 and 23.");
                                                            }
                                                            break;
                                                          case 1: 
                                                            T.Minute = _STOU8(N->getText()); 
                                                            if (T.Minute > 59) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Minute default value must be between 0 and 59.");
                                                            }
                                                            break;
                                                          case 2: 
                                                            T.Second = _STOU8(N->getText());
                                                            if (T.Second > 59) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Second default value must be between 0 and 59.");
                                                            }
                                                            break;
                                                          }
                                                       >>
  }
  ;

vfrTimeFlags [CIfrTime & TObj, UINT32 LineNum] :
  << UINT8 LFlags = 0; >>
  timeFlagsField[LFlags] ( "\|" timeFlagsField[LFlags] )*
                                                       << _PCATCH(TObj.SetFlags(EFI_IFR_QUESTION_FLAG_DEFAULT, LFlags), LineNum); >>
  ;

timeFlagsField [UINT8 & Flags] :
    N:Number                                           << $Flags |= _STOU8(N->getText()); >>
  | "HOUR_SUPPRESS"                                    << $Flags |= 0x01; >>
  | "MINUTE_SUPPRESS"                                  << $Flags |= 0x02; >>
  | "SECOND_SUPPRESS"                                  << $Flags |= 0x04; >>
  | "STORAGE_NORMAL"                                   << $Flags |= 0x00; >>
  | "STORAGE_TIME"                                     << $Flags |= 0x10; >>
  | "STORAGE_WAKEUP"                                   << $Flags |= 0x20; >>
  ;

vfrStatementQuestionTag :
  vfrStatementStatTag ","       |
  vfrStatementInconsistentIf    |
  vfrStatementNoSubmitIf        |
  vfrStatementDisableIfQuest    |
  vfrStatementRefresh           |
  vfrStatementVarstoreDevice    |
  vfrStatementExtension         |
  vfrStatementRefreshEvent
  ;

vfrStatementQuestionTagList :
  ( vfrStatementQuestionTag )*
  ;

vfrStatementQuestionOptionTag :
  vfrStatementSuppressIfQuest   |
  vfrStatementGrayOutIfQuest    |
  vfrStatementValue             |
  vfrStatementDefault           |
  vfrStatementRead              |
  vfrStatementWrite             |
  vfrStatementOptions
  ;

vfrStatementQuestionOptionList :
  (
    vfrStatementQuestionTag     |
    vfrStatementQuestionOptionTag
  )*
  ;

vfrStatementStatList :
  vfrStatementStat                        |
  vfrStatementQuestions                   |
  vfrStatementConditionalNew              |
  vfrStatementLabel                       |
  vfrStatementExtension                   |
  // Just for framework vfr compatibility
  vfrStatementInvalid
  ;

vfrStatementStatListOld :
  vfrStatementStat                        |
  vfrStatementQuestions                   |
  vfrStatementLabel                       |
  // Just for framework vfr compatibility
  vfrStatementInvalid
  ;

vfrStatementDisableIfStat :
  << 
    CIfrDisableIf DIObj; 
  >>
  L:DisableIf                                          << DIObj.SetLineNo(L->getLine()); >>
  vfrStatementExpression[0] ";" 
  ( vfrStatementStatList )*
  E:EndIf                                              << CRT_END_OP (E); >>
  ";"
  ;

vfrStatementInconsistentIfStat :
  << CIfrInconsistentIf IIObj; >>
  L:InconsistentIf                                     <<
                                                          if (!mCompatibleMode) {
                                                            _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                          }
                                                          IIObj.SetLineNo(L->getLine());
                                                       >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << IIObj.SetError (_STOSID(S->getText())); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  E:EndIf                                              << CRT_END_OP (E); >>
  ";"
  ;

//
// Compatible for framework vfr file
//
vfrStatementgrayoutIfSuppressIf:
  << CIfrSuppressIf SIObj; >>
  L:SuppressIf                                         << SIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  ;

vfrStatementsuppressIfGrayOutIf:
  << CIfrGrayOutIf GOIObj; >>
  L:GrayOutIf                                          << GOIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  ;

vfrStatementSuppressIfStatNew :
  << CIfrSuppressIf SIObj;>>
  L:SuppressIf                                         << SIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  ( vfrStatementStatList )*
  E: EndIf ";"                                       << CRT_END_OP (E); >>
  ;

vfrStatementGrayOutIfStatNew :
  << CIfrGrayOutIf GOIObj;>>
  L:GrayOutIf                                          << GOIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  ( vfrStatementStatList )*
  E: EndIf ";"                                       << CRT_END_OP (E); >>
  ;

vfrStatementSuppressIfStatOld :
  <<
    CIfrSuppressIf SIObj;
    BOOLEAN        GrayOutExist = FALSE;
  >>
  L:SuppressIf                                       << SIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  {
    vfrStatementsuppressIfGrayOutIf
                                                     << GrayOutExist = TRUE; >>
  }
  ( vfrStatementStatListOld )*
  E: EndIf ";"                                       << if (GrayOutExist) CRT_END_OP (E); CRT_END_OP (E);>>
  ;

vfrStatementGrayOutIfStatOld :
  <<
    CIfrGrayOutIf  GOIObj;
    BOOLEAN        SuppressExist = FALSE;
  >>
  L:GrayOutIf                                          << GOIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  ";"
  {
    vfrStatementgrayoutIfSuppressIf
                                                       << SuppressExist = TRUE; >>
  }
  ( vfrStatementStatListOld )*
  E: EndIf ";"                                         << if (SuppressExist) CRT_END_OP (E); CRT_END_OP (E); >>
  ;

vfrImageTag :
  << CIfrImage IObj; >>
  L:Image "=" "IMAGE_TOKEN" "\(" S1:Number "\)"        << IObj.SetImageId (_STOSID(S1->getText())); IObj.SetLineNo(L->getLine()); >>
  ;

vfrLockedTag :
  << CIfrLocked LObj; >>
  L:Locked                                             << LObj.SetLineNo(L->getLine()); >>
  ;

vfrModalTag :
  << CIfrModal MObj; >>
  L:Modal                                             << MObj.SetLineNo(L->getLine()); >>
  ;

vfrStatementStatTag :
  vfrImageTag  |
  vfrLockedTag
  ;

vfrStatementStatTagList :
  vfrStatementStatTag ( "," vfrStatementStatTag )*
  ;

vfrStatementImage :
  vfrImageTag
  ";"
  ;

vfrStatementModal :
  vfrModalTag
  ";"
  ;

vfrStatementLocked :
  vfrLockedTag
  ";"
  ;

vfrStatementInconsistentIf :
  << CIfrInconsistentIf IIObj; >>
  L:InconsistentIf                                     << IIObj.SetLineNo(L->getLine()); >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << IIObj.SetError (_STOSID(S->getText())); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  E:EndIf                                              << CRT_END_OP (E); >>
  ;

vfrStatementNoSubmitIf :
  << CIfrNoSubmitIf NSIObj; >>
  L:NoSubmitIf                                         << NSIObj.SetLineNo(L->getLine()); >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << NSIObj.SetError (_STOSID(S->getText())); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  E:EndIf                                              << CRT_END_OP (E); >>
  ;

vfrStatementDisableIfQuest :
  << 
    CIfrDisableIf DIObj; 
  >>
  L:DisableIf                                          << DIObj.SetLineNo(L->getLine()); >>
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf                                              << CRT_END_OP (E); >>
  ;

vfrStatementRefresh :
  << CIfrRefresh RObj; >>
  L:Refresh                                            << RObj.SetLineNo(L->getLine()); >>
  Interval "=" I:Number                                << RObj.SetRefreshInterval (_STOU8(I->getText())); >>
  ;

vfrStatementRefreshEvent :
  <<
    CIfrRefreshId RiObj;
    EFI_GUID      Guid;
  >>
  L:RefreshGuid                                        << RiObj.SetLineNo(L->getLine()); >>
  "="  guidDefinition[Guid] ","                        << RiObj.SetRefreshEventGroutId (&Guid);  >>
  ;

vfrStatementVarstoreDevice :
  << CIfrVarStoreDevice VDObj; >>
  L:VarstoreDevice                                     << VDObj.SetLineNo(L->getLine()); >>
  "=" "STRING_TOKEN" "\(" S:Number "\)" ","            << VDObj.SetDevicePath (_STOSID(S->getText())); >>
  ;

vfrStatementSuppressIfQuest :
  << CIfrSuppressIf SIObj; >>
  L:SuppressIf                                         << SIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf                                              << CRT_END_OP (E); >>
  ;

vfrStatementGrayOutIfQuest :
  << CIfrGrayOutIf GOIObj; >>
  L:GrayOutIf                                          << GOIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf                                              << CRT_END_OP (E); >>
  ;

vfrStatementOptions :
  vfrStatementOneOfOption
  ;

vfrStatementOneOfOption :
  <<
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
     CIfrOneOfOption    OOOObj;
     CHAR8              *VarStoreName = NULL;
  >>
  L:Option                                             << OOOObj.SetLineNo(L->getLine()); >>
  Text  "=" "STRING_TOKEN" "\(" S:Number "\)" ","      << OOOObj.SetOption (_STOSID(S->getText())); >>
  Value "=" vfrConstantValueField[_GET_CURRQEST_DATATYPE()] >[Val] ","    
                                                       << 
                                                          if (gCurrentMinMaxData != NULL) {
                                                            //set min/max value for oneof opcode
                                                            UINT64 Step = gCurrentMinMaxData->GetStepData(_GET_CURRQEST_DATATYPE());
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_64:
                                                              gCurrentMinMaxData->SetMinMaxStepData(Val.u64, Val.u64, Step);
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32:
                                                              gCurrentMinMaxData->SetMinMaxStepData(Val.u32, Val.u32, (UINT32) Step);
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16:
                                                              gCurrentMinMaxData->SetMinMaxStepData(Val.u16, Val.u16, (UINT16) Step);
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_8:
                                                              gCurrentMinMaxData->SetMinMaxStepData(Val.u8, Val.u8, (UINT8) Step);
                                                              break;
                                                            default:
                                                              break;
                                                            }
                                                          }
                                                          OOOObj.SetType (_GET_CURRQEST_DATATYPE()); 
                                                          OOOObj.SetValue (Val); 
                                                       >>
  F:FLAGS "=" vfrOneOfOptionFlags[OOOObj, F->getLine()]
                                                       <<
                                                          _PCATCH(mCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), L->getLine());
                                                          if (OOOObj.GetFlags () & 0x10) {
                                                            _PCATCH(mCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                      EFI_HII_DEFAULT_CLASS_STANDARD,
                                                                       _GET_CURRQEST_VARTINFO(),
                                                                      VarStoreName,
                                                                      _GET_CURRQEST_DATATYPE (),
                                                                      Val
                                                                      ), L->getLine());
                                                          }
                                                          if (OOOObj.GetFlags () & 0x20) {
                                                            _PCATCH(mCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                      EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                                                                       _GET_CURRQEST_VARTINFO(),
                                                                      VarStoreName,
                                                                      _GET_CURRQEST_DATATYPE (),
                                                                      Val
                                                                      ), L->getLine());
                                                          }
                                                       >>
  {
    "," Key "=" KN:Number                              <<
                                                         if (!mCompatibleMode) {
                                                           _PCATCH (VFR_RETURN_UNSUPPORTED, KN);
                                                         }
                                                         //
                                                         // Guid Option Key
                                                         //
                                                         CIfrOptionKey IfrOptionKey (
                                                                         gCurrentQuestion->QUESTION_ID(),
                                                                         Val,
                                                                         _STOQID(KN->getText())
                                                                         );
                                                         SET_LINE_INFO (IfrOptionKey, KN);
                                                       >>
  }
  (
    T:"," vfrImageTag                                  << OOOObj.SetScope (1); CRT_END_OP (T); >>
  )*
  ";"
  ;

vfrOneOfOptionFlags [CIfrOneOfOption & OOOObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = _GET_CURRQEST_DATATYPE();
     UINT8 HFlags = 0;
  >>
  oneofoptionFlagsField[HFlags, LFlags] ( "\|" oneofoptionFlagsField[HFlags, LFlags] )*
                                                       << _PCATCH(gCurrentQuestion->SetFlags(HFlags), LineNum); >>
                                                       << _PCATCH(OOOObj.SetFlags(LFlags), LineNum); >>
  ;

oneofoptionFlagsField [UINT8 & HFlags, UINT8 & LFlags] :
    N:Number                                           << $LFlags |= _STOU8(N->getText()); >>
  | "OPTION_DEFAULT"                                   << $LFlags |= 0x10; >>
  | "OPTION_DEFAULT_MFG"                               << $LFlags |= 0x20; >>
  | InteractiveFlag                                    << $HFlags |= 0x04; >>
  | NVAccessFlag                                       << $HFlags |= 0x08; >>
  | ResetRequiredFlag                                  << $HFlags |= 0x10; >>
  | LateCheckFlag                                      << $HFlags |= 0x20; >>
  | ManufacturingFlag                                  << $LFlags |= 0x20; >>
  | DefaultFlag                                        << $LFlags |= 0x10; >>
  ;

vfrStatementLabel :
  L:Label
  N:Number                                             <<
                                                          if (mCompatibleMode) {
                                                            //
                                                            // Add end Label for Framework Vfr
                                                            //
                                                            CIfrLabel LObj1;
                                                            LObj1.SetLineNo(L->getLine());
                                                            LObj1.SetNumber (0xffff);  //add end label for UEFI, label number hardcode 0xffff
                                                          }

                                                          {
                                                            CIfrLabel LObj2;
                                                            LObj2.SetLineNo(L->getLine());
                                                            LObj2.SetNumber (_STOU16(N->getText()));
                                                          }
                                                       >>
  ";"
  ;

vfrStatementBanner :
  << CIfrBanner BObj; >>
  B:Banner { "," }                                     << BObj.SetLineNo(B->getLine()); >>
  Title "=" "STRING_TOKEN" "\(" S:Number "\)" ","      << BObj.SetTitle (_STOSID(S->getText())); >>
  (
    (
      Line L:Number ","                                << BObj.SetLine (_STOU16(L->getText())); >>
      Align
      (
          Left                                         << BObj.SetAlign (0); >>
        | Center                                       << BObj.SetAlign (1); >>
        | Right                                        << BObj.SetAlign (2); >>
      ) ";"
    )
    |
    (
      Timeout "=" T:Number ";"                         << {CIfrTimeout TObj(_STOU16(T->getText()));} >>
    )
  )
  ;

//******************************************************************************
//
// keep some syntax for compatibility but not generate any IFR object
//
vfrStatementInvalidHidden :
  L:Hidden               <<
                            if (!mCompatibleMode) {
                              _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                            }
                         >>
  Value "=" Number ","
  Key "=" Number ";"
  ;

vfrStatementInvalidInconsistentIf :
  InconsistentIf
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  EndIf
  ";"
  ;

vfrStatementInvalidInventory :
  L:Inventory                                      <<
                                                      if (!mCompatibleMode) {
                                                        _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                      }
                                                   >>
  Help "=" "STRING_TOKEN" "\(" Number "\)" ","
  Text "=" "STRING_TOKEN" "\(" Number "\)" ","
  {
    Text  "=" "STRING_TOKEN" "\(" Number "\)"
  }
  ";"
  ;

vfrStatementInvalidSaveRestoreDefaults :
  (
   L:Save                                          <<
                                                      if (!mCompatibleMode) {
                                                        _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                      }
                                                   >>
  |
   K:Restore                                       <<
                                                      if (!mCompatibleMode) {
                                                        _PCATCH (VFR_RETURN_UNSUPPORTED, K);
                                                      }
                                                   >>
  )
  Defaults ","
  FormId "=" Number  ","
  Prompt "=" "STRING_TOKEN" "\(" Number "\)" ","
  Help   "=" "STRING_TOKEN" "\(" Number "\)"
  { "," FLAGS "=" flagsField ( "\|" flagsField )* }
  { "," Key   "=" Number }
  ";"
  ;

//******************************************************************************
//
// The syntax of expression
//
#token Dup("dup")                               "dup"
#token VarEqVal("vareqval")                     "vareqval"
#token Var("var")                               "var"
#token IdEqVal("ideqval")                       "ideqval"
#token IdEqId("ideqid")                         "ideqid"
#token IdEqValList("ideqvallist")               "ideqvallist"
#token QuestionRef("questionref")               "questionref"
#token RuleRef("ruleref")                       "ruleref"
#token StringRef("stringref")                   "stringref"
#token PushThis("pushthis")                     "pushthis"
#token Security("security")                     "security"
#token Get("get")                               "get"
#token True("TRUE")                             "TRUE"
#token False("FALSE")                           "FALSE"
#token One("ONE")                               "ONE"
#token Ones("ONES")                             "ONES"
#token Zero("ZERO")                             "ZERO"
#token Undefined("UNDEFINED")                   "UNDEFINED"
#token Version("VERSION")                       "VERSION"
#token Length("length")                         "length"
#token AND("AND")                               "AND"
#token OR("OR")                                 "OR"
#token NOT("NOT")                               "NOT"
#token Set("set")                               "set"
#token BitWiseNot("~")                          "\~"
#token BoolVal("boolval")                       "boolval"
#token StringVal("stringval")                   "stringval"
#token UnIntVal("unintval")                     "unintval"
#token ToUpper("toupper")                       "toupper"
#token ToLower("tolower")                       "tolower"
#token Match("match")                           "match"
#token Catenate("catenate")                     "catenate"
#token QuestionRefVal("questionrefval")         "questionrefval"
#token StringRefVal("stringrefval")             "stringrefval"
#token Map("map")                               "map"
#token RefreshGuid("refreshguid")               "refreshguid"

//
// Root expression extension function called by other function.
//
vfrStatementExpression [UINT32 RootLevel, UINT32 ExpOpCount = 0] :
  << if ($RootLevel == 0) {mCIfrOpHdrIndex ++; if (mCIfrOpHdrIndex >= MAX_IFR_EXPRESSION_DEPTH) _PCATCH (VFR_RETURN_INVALID_PARAMETER, 0, "The depth of expression exceeds the max supported level 8!"); _CLEAR_SAVED_OPHDR ();} >>
  andTerm[$RootLevel, $ExpOpCount]
  (
    L:OR andTerm[$RootLevel, $ExpOpCount]              << $ExpOpCount++; CIfrOr OObj(L->getLine()); >>
  )*
                                                       <<
                                                          //
                                                          // Extend OpCode Scope only for the root expression.
                                                          //
                                                          if ($ExpOpCount > 1 && $RootLevel == 0) {
                                                            if (_SET_SAVED_OPHDR_SCOPE()) {
                                                              CIfrEnd EObj;
                                                              if (mCIfrOpHdrLineNo[mCIfrOpHdrIndex] != 0) {
                                                                EObj.SetLineNo (mCIfrOpHdrLineNo[mCIfrOpHdrIndex]);
                                                              }
                                                            }
                                                          }
                                                          
                                                          if ($RootLevel == 0) {
                                                            mCIfrOpHdrIndex --;
                                                          }
                                                       >>
  ;

//
// Add new sub function for the sub expression extension to remember the ExpOpCount
// This funciton is only called by sub expression.
//
vfrStatementExpressionSub [UINT32 RootLevel, UINT32 & ExpOpCount] :
  andTerm[$RootLevel, $ExpOpCount]
  (
    L:OR andTerm[$RootLevel, $ExpOpCount]              << $ExpOpCount++; CIfrOr OObj(L->getLine()); >>
  )*
  ;

andTerm[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  bitwiseorTerm[$RootLevel, $ExpOpCount]
  (
    L:AND bitwiseorTerm [$RootLevel, $ExpOpCount]       << $ExpOpCount++; CIfrAnd AObj(L->getLine()); >>
  )*
  ;

bitwiseorTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  bitwiseandTerm[$RootLevel, $ExpOpCount]
  (
    L:"\|" bitwiseandTerm[$RootLevel, $ExpOpCount]      << $ExpOpCount++; CIfrBitWiseOr BWOObj(L->getLine()); >>
  )*
  ;

bitwiseandTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  equalTerm[$RootLevel, $ExpOpCount]
  (
    L:"&" equalTerm[$RootLevel, $ExpOpCount]            << $ExpOpCount++; CIfrBitWiseAnd BWAObj(L->getLine()); >>
  )*
  ;

equalTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  compareTerm[$RootLevel, $ExpOpCount]
  (
    (
      L1:"==" compareTerm[$RootLevel, $ExpOpCount]      << $ExpOpCount++; CIfrEqual EObj(L1->getLine()); >>
    )
    |
    (
      L2:"!=" compareTerm[$RootLevel, $ExpOpCount]      << $ExpOpCount++; CIfrNotEqual NEObj(L2->getLine()); >>
    )
  )*
  ;

compareTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  shiftTerm[$RootLevel, $ExpOpCount]
  (
    (
      L1:"<" shiftTerm[$RootLevel, $ExpOpCount]         << $ExpOpCount++; CIfrLessThan LTObj(L1->getLine()); >>
    )
    |
    (
      L2:"<=" shiftTerm[$RootLevel, $ExpOpCount]        << $ExpOpCount++; CIfrLessEqual LEObj(L2->getLine()); >>
    )
    |
    (
      L3:">" shiftTerm[$RootLevel, $ExpOpCount]         << $ExpOpCount++; CIfrGreaterThan GTObj(L3->getLine()); >>
    )
    |
    (
      L4:">=" shiftTerm[$RootLevel, $ExpOpCount]        << $ExpOpCount++; CIfrGreaterEqual GEObj(L4->getLine()); >>
    )
  )*
  ;

shiftTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  addMinusTerm[$RootLevel, $ExpOpCount]
  (
    (
      L1:"\<<" addMinusTerm[$RootLevel, $ExpOpCount]    << $ExpOpCount++; CIfrShiftLeft SLObj(L1->getLine()); >>
    )
    |
    (
      L2:"\>>" addMinusTerm[$RootLevel, $ExpOpCount]    << $ExpOpCount++; CIfrShiftRight SRObj(L2->getLine()); >>
    )
  )*
  ;

addMinusTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  multdivmodTerm[$RootLevel, $ExpOpCount]
  (
    (
      L1:"\+" multdivmodTerm[$RootLevel, $ExpOpCount]   << $ExpOpCount++; CIfrAdd AObj(L1->getLine()); >>
    )
    |
    (
      L2:"\-" multdivmodTerm[$RootLevel, $ExpOpCount]   << $ExpOpCount++; CIfrSubtract SObj(L2->getLine()); >>
    )
  )*
  ;

multdivmodTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  castTerm[$RootLevel, $ExpOpCount]
  (
    (
      L1:"\*" castTerm[$RootLevel, $ExpOpCount]         << $ExpOpCount++; CIfrMultiply MObj(L1->getLine()); >>
    )
    |
    (
      L2:"/" castTerm[$RootLevel, $ExpOpCount]          << $ExpOpCount++; CIfrDivide DObj(L2->getLine()); >>
    )
    |
    (
      L3:"%" castTerm[$RootLevel, $ExpOpCount]          << $ExpOpCount++; CIfrModulo MObj(L3->getLine()); >>
    )
  )*
  ;

castTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  << UINT8 CastType = 0xFF; >>
  (
    L:"\("
    (
        Boolean                                         << CastType = 0; >>
      | Uint64                                          << CastType = 1; >>
      | Uint32                                          << CastType = 1; >>
      | Uint16                                          << CastType = 1; >>
      | Uint8                                           << CastType = 1; >>
    )
    "\)"
  )*
  atomTerm[$RootLevel, $ExpOpCount]
                                                        <<
                                                           switch (CastType) {
                                                           case 0: { CIfrToBoolean TBObj(L->getLine()); $ExpOpCount++; } break;
                                                           case 1: { CIfrToUint TUObj(L->getLine()); $ExpOpCount++; } break;
                                                           }
                                                        >>
  ;

atomTerm [UINT32 & RootLevel, UINT32 & ExpOpCount]:
    vfrExpressionCatenate[$RootLevel, $ExpOpCount]
  | vfrExpressionMatch[$RootLevel, $ExpOpCount]
  | vfrExpressionParen[$RootLevel, $ExpOpCount]
  | vfrExpressionBuildInFunction[$RootLevel, $ExpOpCount]
  | vfrExpressionConstant[$RootLevel, $ExpOpCount]
  | vfrExpressionUnaryOp[$RootLevel, $ExpOpCount]
  | vfrExpressionTernaryOp[$RootLevel, $ExpOpCount]
  | vfrExpressionMap[$RootLevel, $ExpOpCount]
  | (
      L:NOT
      atomTerm[$RootLevel, $ExpOpCount]                 << { CIfrNot NObj(L->getLine()); $ExpOpCount++; } >>
    )
  ;

vfrExpressionCatenate [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  L:Catenate
  "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrCatenate CObj(L->getLine()); $ExpOpCount++; } >>
  ;

vfrExpressionMatch [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  L:Match
  "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrMatch MObj(L->getLine()); $ExpOpCount++; } >>
  ;

vfrExpressionParen [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"
  ;

vfrExpressionBuildInFunction [UINT32 & RootLevel, UINT32 & ExpOpCount] :
    dupExp[$RootLevel, $ExpOpCount]
  | vareqvalExp[$RootLevel, $ExpOpCount]  //Compatible for Framework vareqval
  | ideqvalExp[$RootLevel, $ExpOpCount]
  | ideqidExp[$RootLevel, $ExpOpCount]
  | ideqvallistExp[$RootLevel, $ExpOpCount]
  | questionref1Exp[$RootLevel, $ExpOpCount]
  | rulerefExp[$RootLevel, $ExpOpCount]
  | stringref1Exp[$RootLevel, $ExpOpCount]
  | pushthisExp[$RootLevel, $ExpOpCount]
  | securityExp[$RootLevel, $ExpOpCount]
  | getExp[$RootLevel, $ExpOpCount]
  ;

dupExp [UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:Dup                                                << { CIfrDup DObj(L->getLine()); _SAVE_OPHDR_COND(DObj, ($ExpOpCount == 0), L->getLine()); $ExpOpCount++; } >>
  ;

vareqvalExp [UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_QUESTION_ID QId;
     UINT32          Mask;
     UINT16          ConstVal;
     CHAR8           *VarIdStr;
     UINT32          LineNo;
     EFI_VFR_VARSTORE_TYPE VarStoreType = EFI_VFR_VARSTORE_INVALID;
     EFI_VFR_RETURN_CODE   VfrReturnCode = VFR_RETURN_SUCCESS;
  >>
  L:VarEqVal                                          <<
                                                        if (!mCompatibleMode) {
                                                          _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                        }
                                                      >>
  VK:Var
  OpenParen
  VN:Number                                           <<
                                                          VarIdStr = NULL; _STRCAT(&VarIdStr, VK->getText()); _STRCAT(&VarIdStr, VN->getText());
                                                          VfrReturnCode = mCVfrDataStorage.GetVarStoreType (VarIdStr, VarStoreType);
                                                          if (VfrReturnCode == VFR_RETURN_UNDEFINED) {
                                                            _PCATCH (mCVfrDataStorage.DeclareEfiVarStore (
                                                                                        VarIdStr,
                                                                                        &mFormsetGuid,
                                                                                        _STOSID(VN->getText()),
                                                                                        0x2,   //default type is UINT16
                                                                                        FALSE
                                                                                        ), VN);
                                                          } else {
                                                            _PCATCH (VfrReturnCode, VN);
                                                          }
                                                          mCVfrQuestionDB.GetQuestionId (NULL, VarIdStr, QId, Mask);
                                                          LineNo = GET_LINENO(VN);
                                                      >>
  CloseParen
  (
    (
      "=="
      V1:Number                                        << ConstVal = _STOU16(V1->getText()); >>
                                                       <<
                                                          if (Mask == 0) {
                                                            CIfrEqIdVal EIVObj (L->getLine());
                                                            _SAVE_OPHDR_COND (EIVObj, ($ExpOpCount == 0), L->getLine());
                                                            EIVObj.SetQuestionId (QId, VarIdStr, LineNo);
                                                            EIVObj.SetValue (ConstVal);
                                                            $ExpOpCount++;
                                                          } else {
                                                            IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, EQUAL);
                                                          }
                                                       >>
    )
    |
    (
      "<="
      V2:Number                                        << ConstVal = _STOU16(V2->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_EQUAL); >>
    )
    |
    (
      "<"
      V3:Number                                        << ConstVal = _STOU16(V3->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_THAN); >>
    )
    |
    (
      ">="
      V4:Number                                        << ConstVal = _STOU16(V4->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_EQUAL); >>
    )
    |
    (
      ">"
      V5:Number                                        << ConstVal = _STOU16(V5->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_THAN); >>
    )
  )
  ;

ideqvalExp [UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_QUESTION_ID QId;
     UINT32          Mask;
     UINT16          ConstVal;
     CHAR8           *VarIdStr;
     UINT32          LineNo;
  >>
  L:IdEqVal
  vfrQuestionDataFieldName[QId, Mask, VarIdStr, LineNo]
  (
    (
      "=="
      V1:Number                                        << ConstVal = _STOU16(V1->getText()); >>
                                                       <<
                                                          if (Mask == 0) {
                                                            CIfrEqIdVal EIVObj (L->getLine());
                                                            _SAVE_OPHDR_COND (EIVObj, ($ExpOpCount == 0), L->getLine());
                                                            EIVObj.SetQuestionId (QId, VarIdStr, LineNo);
                                                            EIVObj.SetValue (ConstVal);
                                                            $ExpOpCount++;
                                                          } else {
                                                            IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, EQUAL);
                                                          }
                                                       >>
    )
    |
    (
      "<="
      V2:Number                                        << ConstVal = _STOU16(V2->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_EQUAL); >>
    )
    |
    (
      "<"
      V3:Number                                        << ConstVal = _STOU16(V3->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_THAN); >>
    )
    |
    (
      ">="
      V4:Number                                        << ConstVal = _STOU16(V4->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_EQUAL); >>
    )
    |
    (
      ">"
      V5:Number                                        << ConstVal = _STOU16(V5->getText()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_THAN); >>
    )
  )
  ;

ideqidExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_QUESTION_ID QId[2];
     UINT32          Mask[2];
     CHAR8           *VarIdStr[2];
     UINT32          LineNo[2];
  >>
  L:IdEqId
  vfrQuestionDataFieldName[QId[0], Mask[0], VarIdStr[0], LineNo[0]]
  (
    (
      "=="
      vfrQuestionDataFieldName[QId[1], Mask[1], VarIdStr[1], LineNo[1]]
                             <<
                                if (Mask[0] & Mask[1]) {
                                  IdEqIdDoSpecial ($ExpOpCount, L->getLine(), QId[0], VarIdStr[0], Mask[0], QId[1], VarIdStr[1], Mask[1], EQUAL);
                                } else {
                                  CIfrEqIdId      EIIObj(L->getLine());
                                  _SAVE_OPHDR_COND (EIIObj, ($ExpOpCount == 0), L->getLine());
                                  EIIObj.SetQuestionId1 (QId[0], VarIdStr[0], LineNo[0]);
                                  EIIObj.SetQuestionId2 (QId[1], VarIdStr[1], LineNo[1]);
                                  $ExpOpCount++;
                                }
                             >>
    )
    |
    (
      "<="
      vfrQuestionDataFieldName[QId[1], Mask[1], VarIdStr[1], LineNo[1]]
                                                       << IdEqIdDoSpecial ($ExpOpCount, L->getLine(), QId[0], VarIdStr[0], Mask[0], QId[1], VarIdStr[1], Mask[1], LESS_EQUAL); >>
    )
    |
    (
      "<"
      vfrQuestionDataFieldName[QId[1], Mask[1], VarIdStr[1], LineNo[1]]
                                                       << IdEqIdDoSpecial ($ExpOpCount, L->getLine(), QId[0], VarIdStr[0], Mask[0], QId[1], VarIdStr[1], Mask[1], LESS_THAN); >>
    )
    |
    (
      ">="
      vfrQuestionDataFieldName[QId[1], Mask[1], VarIdStr[1], LineNo[1]]
                                                       << IdEqIdDoSpecial ($ExpOpCount, L->getLine(), QId[0], VarIdStr[0], Mask[0], QId[1], VarIdStr[1], Mask[1], GREATER_EQUAL); >>
    )
    |
    (
      ">"
      vfrQuestionDataFieldName[QId[1], Mask[1], VarIdStr[1], LineNo[1]]
                                                       << IdEqIdDoSpecial ($ExpOpCount, L->getLine(), QId[0], VarIdStr[0], Mask[0], QId[1], VarIdStr[1], Mask[1], GREATER_THAN); >>
    )
  )
  ;

ideqvallistExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     UINT16          ListLen = 0;
     EFI_QUESTION_ID QId;
     UINT32          Mask;
     UINT16          ValueList[EFI_IFR_MAX_LENGTH] = {0,};
     CHAR8           *VarIdStr;
     UINT32          LineNo;
  >>
  L:IdEqValList
  vfrQuestionDataFieldName[QId, Mask, VarIdStr, LineNo]
  "=="
  (
    V:Number                                           << ValueList[ListLen] = _STOU16(V->getText()); ListLen++; >>
  )+
                                                       <<
                                                          if (Mask != 0) {
                                                            IdEqListDoSpecial ($ExpOpCount, LineNo, QId, VarIdStr, Mask, ListLen, ValueList);
                                                          } else {
                                                            UINT16       Index;
                                                            CIfrEqIdList EILObj(L->getLine());
                                                            if (QId != EFI_QUESTION_ID_INVALID) {
                                                              EILObj.SetQuestionId (QId, VarIdStr, LineNo);
                                                            }
                                                            EILObj.SetListLength (ListLen);
                                                            for (Index = 0; Index < ListLen; Index++) {
                                                              EILObj.SetValueList (Index, ValueList[Index]);
                                                            }
                                                            
                                                            EILObj.UpdateIfrBuffer();
                                                            _SAVE_OPHDR_COND (EILObj, ($ExpOpCount == 0), L->getLine());                                                            
                                                            
                                                            if (QId == EFI_QUESTION_ID_INVALID) {
                                                              EILObj.SetQuestionId (QId, VarIdStr, LineNo);
                                                            }
                                                            $ExpOpCount++;
                                                          }
                                                        >>
  ;

questionref1Exp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_QUESTION_ID QId = EFI_QUESTION_ID_INVALID;
     UINT32          BitMask;
     CHAR8           *QName = NULL;
     UINT32          LineNo = 0;
  >>
  L:QuestionRef
  "\("
      (
          QN:StringIdentifier                          <<
                                                          QName  = QN->getText();
                                                          LineNo = QN->getLine();
                                                          mCVfrQuestionDB.GetQuestionId (QN->getText(), NULL, QId, BitMask);
                                                       >>
        | ID:Number                                    << QId = _STOQID(ID->getText()); >>
      )
  "\)"
                                                       <<
                                                          { CIfrQuestionRef1 QR1Obj(L->getLine()); _SAVE_OPHDR_COND (QR1Obj, ($ExpOpCount == 0), L->getLine()); QR1Obj.SetQuestionId (QId, QName, LineNo); } $ExpOpCount++; >>
  ;

rulerefExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:RuleRef
  "\(" RN:StringIdentifier "\)"                        << { CIfrRuleRef RRObj(L->getLine()); _SAVE_OPHDR_COND (RRObj, ($ExpOpCount == 0), L->getLine()); RRObj.SetRuleId (mCVfrRulesDB.GetRuleId (RN->getText())); } $ExpOpCount++; >>
  ;

//******************************************************
// PARSE:
//   stringref (STR_FORM_SET_TITLE)
//
stringref1Exp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
    EFI_STRING_ID RefStringId = EFI_STRING_ID_INVALID;
  >>
  L:StringRef
  "\("
      ( 
        "STRING_TOKEN"
        "\(" 
          S:Number << RefStringId = _STOSID(S->getText()); >>
        "\)"
        | I:Number << RefStringId = _STOSID(I->getText()); >>
      )
  "\)" << { CIfrStringRef1 SR1Obj(L->getLine()); _SAVE_OPHDR_COND (SR1Obj, ($ExpOpCount == 0), L->getLine()); SR1Obj.SetStringId (RefStringId); $ExpOpCount++; } >>
  ;

pushthisExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:PushThis                                           << { CIfrThis TObj(L->getLine()); _SAVE_OPHDR_COND (TObj, ($ExpOpCount == 0), L->getLine()); $ExpOpCount++; } >>
  ;

securityExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_GUID Guid;
  >>
  L:Security
  "\(" guidDefinition[Guid] "\)"                       << { CIfrSecurity SObj(L->getLine()); _SAVE_OPHDR_COND (SObj, ($ExpOpCount == 0), L->getLine()); SObj.SetPermissions (&Guid); } $ExpOpCount++; >>
  ;

numericVarStoreType [UINT8 & VarType] :
    "NUMERIC_SIZE_1"                                   << $VarType = EFI_IFR_NUMERIC_SIZE_1; >>
  | "NUMERIC_SIZE_2"                                   << $VarType = EFI_IFR_NUMERIC_SIZE_2; >>
  | "NUMERIC_SIZE_4"                                   << $VarType = EFI_IFR_NUMERIC_SIZE_4; >>
  | "NUMERIC_SIZE_8"                                   << $VarType = EFI_IFR_NUMERIC_SIZE_8; >>
  ;

getExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_VARSTORE_INFO Info;
     CHAR8             *VarIdStr = NULL;
     EFI_QUESTION_ID   QId = EFI_QUESTION_ID_INVALID;
     UINT32            Mask = 0;
     EFI_QUESION_TYPE  QType = QUESTION_NORMAL;
     UINT8             VarType = EFI_IFR_TYPE_UNDEFINED;
     UINT32            VarSize = 0;
     Info.mVarStoreId = 0;
  >>
  L:Get
  "\(" 
      vfrStorageVarId[Info, VarIdStr, FALSE]
      {"\|" FLAGS "=" numericVarStoreType [VarType] }
  "\)"                                                 << 
                                                          {
                                                            if (Info.mVarStoreId == 0) {
                                                              // support Date/Time question
                                                              mCVfrQuestionDB.GetQuestionId (NULL, VarIdStr, QId, Mask, &QType);
                                                              if (QId == EFI_QUESTION_ID_INVALID || Mask == 0 || QType == QUESTION_NORMAL) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode can't get the enough varstore information");
                                                              }
                                                              if (QType == QUESTION_DATE) {
                                                                Info.mVarType = EFI_IFR_TYPE_DATE;
                                                              } else if (QType == QUESTION_TIME) {
                                                                Info.mVarType = EFI_IFR_TYPE_TIME;
                                                              }
                                                              switch (Mask) {
                                                              case DATE_YEAR_BITMASK:
                                                                Info.mInfo.mVarOffset = 0;
                                                                break;
                                                              case DATE_DAY_BITMASK:
                                                                Info.mInfo.mVarOffset = 3;
                                                                break;
                                                              case TIME_HOUR_BITMASK:
                                                                Info.mInfo.mVarOffset = 0;
                                                                break;
                                                              case TIME_MINUTE_BITMASK:
                                                                Info.mInfo.mVarOffset = 1;
                                                                break;
                                                              case TIME_SECOND_BITMASK:
                                                                Info.mInfo.mVarOffset = 2;
                                                                break;
                                                              default:
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode can't get the enough varstore information");
                                                                break;
                                                              }
                                                            } else {
                                                              if ((mCVfrDataStorage.GetVarStoreType(Info.mVarStoreId) == EFI_VFR_VARSTORE_NAME) && (VarType == EFI_IFR_TYPE_UNDEFINED)) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode don't support name string");
                                                              }
                                                              if (VarType != EFI_IFR_TYPE_UNDEFINED) {
                                                                Info.mVarType = VarType;
                                                                _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize (Info.mVarType, &VarSize), L->getLine(), "Get/Set opcode can't get var type size");
                                                                Info.mVarTotalSize = VarSize;
                                                              }
                                                              _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize (Info.mVarType, &VarSize), L->getLine(), "Get/Set opcode can't get var type size");
                                                              if (VarSize != Info.mVarTotalSize) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode don't support data array");
                                                              }
                                                            }
                                                            CIfrGet GObj(L->getLine()); 
                                                            _SAVE_OPHDR_COND (GObj, ($ExpOpCount == 0), L->getLine()); 
                                                            GObj.SetVarInfo (&Info); 
                                                            delete VarIdStr; 
                                                            $ExpOpCount++;
                                                          }
                                                       >>
  ;

vfrExpressionConstant[UINT32 & RootLevel, UINT32 & ExpOpCount] :
    L1:True                                            << CIfrTrue TObj(L1->getLine()); _SAVE_OPHDR_COND (TObj, ($ExpOpCount == 0), L1->getLine()); $ExpOpCount++; >>
  | L2:False                                           << CIfrFalse FObj(L2->getLine()); _SAVE_OPHDR_COND (FObj, ($ExpOpCount == 0), L2->getLine()); $ExpOpCount++; >>
  | L3:One                                             << CIfrOne OObj(L3->getLine()); _SAVE_OPHDR_COND (OObj, ($ExpOpCount == 0), L3->getLine()); $ExpOpCount++; >>
  | L4:Ones                                            << CIfrOnes OObj(L4->getLine()); _SAVE_OPHDR_COND (OObj, ($ExpOpCount == 0), L4->getLine()); $ExpOpCount++; >>
  | L5:Zero                                            << CIfrZero ZObj(L5->getLine()); _SAVE_OPHDR_COND (ZObj, ($ExpOpCount == 0), L5->getLine()); $ExpOpCount++; >>
  | L6:Undefined                                       << CIfrUndefined UObj(L6->getLine()); _SAVE_OPHDR_COND (UObj, ($ExpOpCount == 0), L6->getLine()); $ExpOpCount++; >>
  | L7:Version                                         << CIfrVersion VObj(L7->getLine()); _SAVE_OPHDR_COND (VObj, ($ExpOpCount == 0), L7->getLine()); $ExpOpCount++; >>
  | V:Number                                           << CIfrUint64 U64Obj(V->getLine()); U64Obj.SetValue (_STOU64(V->getText())); _SAVE_OPHDR_COND (U64Obj, ($ExpOpCount == 0), V->getLine()); $ExpOpCount++; >>
  ;

vfrExpressionUnaryOp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
    lengthExp[$RootLevel, $ExpOpCount]
  | bitwisenotExp[$RootLevel, $ExpOpCount]
  | question23refExp[$RootLevel, $ExpOpCount]
  | stringref2Exp[$RootLevel, $ExpOpCount]
  | toboolExp[$RootLevel, $ExpOpCount]
  | tostringExp[$RootLevel, $ExpOpCount]
  | unintExp[$RootLevel, $ExpOpCount]
  | toupperExp[$RootLevel, $ExpOpCount]
  | tolwerExp[$RootLevel, $ExpOpCount]
  | setExp[$RootLevel, $ExpOpCount]
  ;

lengthExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:Length
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrLength LObj(L->getLine()); $ExpOpCount++; } >>
  ;

bitwisenotExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:BitWiseNot
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrBitWiseNot BWNObj(L->getLine()); $ExpOpCount++; } >>
  ;

question23refExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     UINT8           Type = 0x1;
     EFI_STRING_ID   DevPath = EFI_STRING_ID_INVALID;
     EFI_GUID        Guid = {0,};
  >>
  L:QuestionRefVal
  "\("
      {
        DevicePath "=" "STRING_TOKEN" "\(" S:Number "\)" ","    << Type = 0x2; DevPath = _STOSID(S->getText()); >>
      }
      {
        Uuid "=" guidDefinition[Guid] ","                       << Type = 0x3; >>
      }
      vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] 
  "\)"
                                                       <<
                                                          switch (Type) {
                                                          case 0x1: {CIfrQuestionRef2 QR2Obj(L->getLine()); _SAVE_OPHDR_COND (QR2Obj, ($ExpOpCount == 0), L->getLine()); break;}
                                                          case 0x2: {CIfrQuestionRef3_2 QR3_2Obj(L->getLine()); _SAVE_OPHDR_COND (QR3_2Obj, ($ExpOpCount == 0), L->getLine()); QR3_2Obj.SetDevicePath (DevPath); break;}
                                                          case 0x3: {CIfrQuestionRef3_3 QR3_3Obj(L->getLine()); _SAVE_OPHDR_COND (QR3_3Obj, ($ExpOpCount == 0), L->getLine()); QR3_3Obj.SetDevicePath (DevPath); QR3_3Obj.SetGuid (&Guid); break;}
                                                          }
                                                          $ExpOpCount++;
                                                       >>
  ;

stringref2Exp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:StringRefVal
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrStringRef2 SR2Obj(L->getLine()); $ExpOpCount++; } >>
  ;

toboolExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:BoolVal
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrToBoolean TBObj(L->getLine()); $ExpOpCount++; } >>
  ;

tostringExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  << UINT8 Fmt = 0; >>
  L:StringVal
  {
    Format "=" F:Number ","                            << Fmt = _STOU8(F->getText()); >>
  }
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrToString TSObj(L->getLine()); TSObj.SetFormat (Fmt); $ExpOpCount++; } >>
  ;

unintExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:UnIntVal
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrToUint TUObj(L->getLine()); $ExpOpCount++; } >>
  ;

toupperExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:ToUpper
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrToUpper TUObj(L->getLine()); $ExpOpCount++; } >>
  ;

tolwerExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:ToLower
  "\(" vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] "\)"
                                                       << { CIfrToLower TLObj(L->getLine()); $ExpOpCount++; } >>
  ;

setExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  <<
     EFI_VARSTORE_INFO Info;
     CHAR8             *VarIdStr = NULL;
     EFI_QUESTION_ID   QId = EFI_QUESTION_ID_INVALID;
     UINT32            Mask = 0;
     EFI_QUESION_TYPE  QType = QUESTION_NORMAL;
     UINT8             VarType = EFI_IFR_TYPE_UNDEFINED;
     UINT32            VarSize = 0;
     Info.mVarStoreId = 0;
  >>
  L:Set
  "\("
     vfrStorageVarId[Info, VarIdStr, FALSE]
     {"\|" FLAG "=" numericVarStoreType [VarType] }
     "," vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount] 
  "\)"
                                                       << 
                                                          {
                                                            if (Info.mVarStoreId == 0) {
                                                              // support Date/Time question
                                                              mCVfrQuestionDB.GetQuestionId (NULL, VarIdStr, QId, Mask, &QType);
                                                              if (QId == EFI_QUESTION_ID_INVALID || Mask == 0 || QType == QUESTION_NORMAL) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode can't get the enough varstore information");
                                                              }
                                                              if (QType == QUESTION_DATE) {
                                                                Info.mVarType = EFI_IFR_TYPE_DATE;
                                                              } else if (QType == QUESTION_TIME) {
                                                                Info.mVarType = EFI_IFR_TYPE_TIME;
                                                              }
                                                              switch (Mask) {
                                                              case DATE_YEAR_BITMASK:
                                                                Info.mInfo.mVarOffset = 0;
                                                                break;
                                                              case DATE_DAY_BITMASK:
                                                                Info.mInfo.mVarOffset = 3;
                                                                break;
                                                              case TIME_HOUR_BITMASK:
                                                                Info.mInfo.mVarOffset = 0;
                                                                break;
                                                              case TIME_MINUTE_BITMASK:
                                                                Info.mInfo.mVarOffset = 1;
                                                                break;
                                                              case TIME_SECOND_BITMASK:
                                                                Info.mInfo.mVarOffset = 2;
                                                                break;
                                                              default:
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode can't get the enough varstore information");
                                                                break;
                                                              }
                                                            } else {
                                                              if ((mCVfrDataStorage.GetVarStoreType(Info.mVarStoreId) == EFI_VFR_VARSTORE_NAME) && (VarType == EFI_IFR_TYPE_UNDEFINED)) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode don't support name string");
                                                              }
                                                              if (VarType != EFI_IFR_TYPE_UNDEFINED) {
                                                                Info.mVarType = VarType;
                                                                _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize (Info.mVarType, &VarSize), L->getLine(), "Get/Set opcode can't get var type size");
                                                                Info.mVarTotalSize = VarSize;
                                                              }
                                                              _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize (Info.mVarType, &VarSize), L->getLine(), "Get/Set opcode can't get var type size");
                                                              if (VarSize != Info.mVarTotalSize) {
                                                                _PCATCH(VFR_RETURN_UNSUPPORTED, L->getLine(), "Get/Set opcode don't support data array");
                                                              }
                                                            }
                                                            CIfrSet TSObj(L->getLine()); 
                                                            TSObj.SetVarInfo (&Info); 
                                                            delete VarIdStr; 
                                                            $ExpOpCount++;
                                                          }
                                                       >>
  ;

vfrExpressionTernaryOp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
    conditionalExp[$RootLevel, $ExpOpCount]
  | findExp[$RootLevel, $ExpOpCount]
  | midExp[$RootLevel, $ExpOpCount]
  | tokenExp[$RootLevel, $ExpOpCount]
  | spanExp[$RootLevel, $ExpOpCount]
  ;

#token Cond("cond")                                    "cond"
#token Find("find")                                    "find"
#token Mid("mid")                                      "mid"
#token Tok("token")                                    "token"
#token Span("span")                                    "span"

conditionalExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:Cond "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "?"
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ":"
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrConditional CObj(L->getLine()); $ExpOpCount++; } >>
  ;

findExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  << UINT8 Format; >>
  L:Find "\("
  findFormat[Format] ( "\|" findFormat[Format] )*
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrFind FObj(L->getLine()); FObj.SetFormat (Format); $ExpOpCount++; } >>
  ;

findFormat [UINT8 & Format] :
    "SENSITIVE"                                        << $Format = 0x00; >>
  | "INSENSITIVE"                                      << $Format = 0x01; >>
  ;

midExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:Mid "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrMid MObj(L->getLine()); $ExpOpCount++; } >>
  ;

tokenExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  L:Tok "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrToken TObj(L->getLine()); $ExpOpCount++; } >>
  ;

spanExp[UINT32 & RootLevel, UINT32 & ExpOpCount] :
  << UINT8 Flags = 0; >>
  S:Span "\("
  FLAGS "=" spanFlags[Flags] ( "\|" spanFlags[Flags] )*
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  "\)"                                                 << { CIfrSpan SObj(S->getLine()); SObj.SetFlags(Flags); $ExpOpCount++; } >>
  ;

vfrExpressionMap [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  L:Map                                                   
  "\(" 
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]  
  ":"                                                     << { CIfrMap MObj(L->getLine()); } >>
  (
    vfrStatementExpression[0]
    ","
    vfrStatementExpression[0]
    ";"
  ) *
  E:"\)"                                                  << { CIfrEnd EObj; EObj.SetLineNo(E->getLine()); $ExpOpCount++; } >>
  ;

spanFlags [UINT8 & Flags] :
    N:Number                                           << $Flags |= _STOU8(N->getText()); >>
  | "LAST_NON_MATCH"                                   << $Flags |= 0x00; >>
  | "FIRST_NON_MATCH"                                  << $Flags |= 0x01; >>
  ;

#token StringIdentifier("string identifier")    "[A-Za-z_][A-Za-z_0-9]*"
#token Number("numeric value")                  "(0x[0-9A-Fa-f]+) | [0-9]+"

//******************************************************************************
//
// Parser class definition.
//
class EfiVfrParser {
<<
private:
  UINT8               mParserStatus;
  BOOLEAN             mConstantOnlyInExpression;

  CVfrDefaultStore    mCVfrDefaultStore;
  CVfrDataStorage     mCVfrDataStorage;
  CVfrQuestionDB      mCVfrQuestionDB;
  CVfrRulesDB         mCVfrRulesDB;

  CIfrOpHeader *      mCIfrOpHdr[MAX_IFR_EXPRESSION_DEPTH];
  UINT32              mCIfrOpHdrLineNo[MAX_IFR_EXPRESSION_DEPTH];
  UINT8               mCIfrOpHdrIndex;
  VOID                _SAVE_OPHDR_COND (IN CIfrOpHeader &, IN BOOLEAN, UINT32 LineNo = 0);
  VOID                _CLEAR_SAVED_OPHDR (VOID);
  BOOLEAN             _SET_SAVED_OPHDR_SCOPE (VOID);


  EFI_VARSTORE_INFO   mCurrQestVarInfo;
  EFI_GUID            *mOverrideClassGuid;

//
// For framework vfr compatibility
//
  BOOLEAN             mCompatibleMode;
  EFI_GUID            mFormsetGuid;

  VOID                _CRT_OP (IN BOOLEAN);

  VOID                _SAVE_CURRQEST_VARINFO (IN EFI_VARSTORE_INFO &);
  EFI_VARSTORE_INFO & _GET_CURRQEST_VARTINFO (VOID);

  UINT8               _GET_CURRQEST_DATATYPE ();
  UINT32              _GET_CURRQEST_VARSIZE ();
  UINT32              _GET_CURRQEST_ARRAY_SIZE();

public:
  VOID                _PCATCH (IN INTN, IN INTN, IN ANTLRTokenPtr, IN CONST CHAR8 *);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN ANTLRTokenPtr);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN UINT32);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN UINT32, IN CONST CHAR8 *);

  VOID                syn     (ANTLRAbstractToken  *, ANTLRChar *, SetWordType *, ANTLRTokenType, INT32);

  CHAR8*              TrimHex (IN CHAR8 *, OUT BOOLEAN *);
  CHAR8*              _U32TOS (IN UINT32);
  UINT8               _STOU8  (IN CHAR8 *);
  UINT16              _STOU16 (IN CHAR8 *);
  UINT32              _STOU32 (IN CHAR8 *);
  UINT64              _STOU64 (IN CHAR8 *);
  EFI_HII_DATE        _STOD   (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *);
  EFI_HII_TIME        _STOT   (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *);
  EFI_HII_REF         _STOR   (IN CHAR8 *, IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *);

  EFI_STRING_ID       _STOSID (IN CHAR8 *);
  EFI_FORM_ID         _STOFID (IN CHAR8 *);
  EFI_QUESTION_ID     _STOQID (IN CHAR8 *);

  VOID                _STRCAT (IN OUT CHAR8 **, IN CONST CHAR8 *);

  VOID                _DeclareDefaultLinearVarStore (IN UINT32);
  VOID                _DeclareStandardDefaultStorage (IN UINT32);
  VOID                _DeclareDefaultFrameworkVarStore (IN UINT32);

  VOID                AssignQuestionKey (IN CIfrQuestionHeader &, IN ANTLRTokenPtr);

  VOID                ConvertIdExpr         (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32);
  VOID                IdEqValDoSpecial      (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN UINT16, IN EFI_COMPARE_TYPE);
  VOID                IdEqIdDoSpecial       (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN EFI_COMPARE_TYPE);
  VOID                IdEqListDoSpecial     (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN UINT16, IN UINT16 *);
  VOID                SetOverrideClassGuid  (IN EFI_GUID *);
//
// For framework vfr compatibility
//
  VOID                SetCompatibleMode (IN BOOLEAN);
>>
}

<<
VOID
EfiVfrParser::_SAVE_OPHDR_COND (
  IN CIfrOpHeader &OpHdr,
  IN BOOLEAN      Cond,
  IN UINT32       LineNo
  )
{
  if (Cond == TRUE) {
    if (mCIfrOpHdr[mCIfrOpHdrIndex] != NULL) {
      return ;
    }
    mCIfrOpHdr[mCIfrOpHdrIndex]       = new CIfrOpHeader(OpHdr);
    mCIfrOpHdrLineNo[mCIfrOpHdrIndex] = LineNo;
  }
}

VOID
EfiVfrParser::_CLEAR_SAVED_OPHDR (
  VOID
  )
{
  mCIfrOpHdr[mCIfrOpHdrIndex]       = NULL;
  mCIfrOpHdrLineNo[mCIfrOpHdrIndex] = 0;
}

BOOLEAN
EfiVfrParser::_SET_SAVED_OPHDR_SCOPE (
  VOID
  )
{
  if (mCIfrOpHdr[mCIfrOpHdrIndex] != NULL) {
    mCIfrOpHdr[mCIfrOpHdrIndex]->SetScope (1);
    return TRUE;
  }

  //
  // IfrOpHdr is not set, FALSE is return.
  //
  return FALSE;
}

VOID
EfiVfrParser::_CRT_OP (
  IN BOOLEAN Crt
  )
{
  gCreateOp = Crt;
}

VOID
EfiVfrParser::_SAVE_CURRQEST_VARINFO (
  IN EFI_VARSTORE_INFO &Info
  )
{
  mCurrQestVarInfo = Info;
}

EFI_VARSTORE_INFO &
EfiVfrParser::_GET_CURRQEST_VARTINFO (
  VOID
  )
{
  return mCurrQestVarInfo;
}

UINT32
EfiVfrParser::_GET_CURRQEST_ARRAY_SIZE (
  VOID
  )
{
  UINT8 Size = 1;

  switch (mCurrQestVarInfo.mVarType) {
  case EFI_IFR_TYPE_NUM_SIZE_8:
    Size = 1;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_16:
    Size = 2;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_32:
    Size = 4;
    break;

  case EFI_IFR_TYPE_NUM_SIZE_64:
    Size = 8;
    break;

  default:
    break;
  }

  return (mCurrQestVarInfo.mVarTotalSize / Size);
}

UINT8
EfiVfrParser::_GET_CURRQEST_DATATYPE (
  VOID
  )
{
  return mCurrQestVarInfo.mVarType;
}

UINT32
EfiVfrParser::_GET_CURRQEST_VARSIZE (
  VOID
  )
{
  return mCurrQestVarInfo.mVarTotalSize;
}

VOID
EfiVfrParser::_PCATCH (
  IN INTN                ReturnCode,
  IN INTN                ExpectCode,
  IN ANTLRTokenPtr       Tok,
  IN CONST CHAR8         *ErrorMsg
  )
{
  if (ReturnCode != ExpectCode) {
    mParserStatus++;
    gCVfrErrorHandle.PrintMsg (Tok->getLine(), Tok->getText(), "Error", ErrorMsg);
  }
}

VOID
EfiVfrParser::_PCATCH (
  IN EFI_VFR_RETURN_CODE ReturnCode
  )
{
  mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (ReturnCode);
}

VOID
EfiVfrParser::_PCATCH (
  IN EFI_VFR_RETURN_CODE ReturnCode,
  IN ANTLRTokenPtr       Tok
  )
{
  mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (ReturnCode, Tok->getLine(), Tok->getText());
}

VOID
EfiVfrParser::_PCATCH (
  IN EFI_VFR_RETURN_CODE ReturnCode,
  IN UINT32              LineNum
  )
{
  mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (ReturnCode, LineNum);
}

VOID
EfiVfrParser::_PCATCH (
  IN EFI_VFR_RETURN_CODE ReturnCode,
  IN UINT32              LineNum,
  IN CONST CHAR8         *ErrorMsg
  )
{
  mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (ReturnCode, LineNum, (CHAR8 *) ErrorMsg);
}

VOID
EfiVfrParser::syn (
  ANTLRAbstractToken  *Tok,
  ANTLRChar           *Egroup,
  SetWordType         *Eset,
  ANTLRTokenType      ETok,
  INT32               Huh
  )
{
  gCVfrErrorHandle.HandleError (VFR_RETURN_MISMATCHED, Tok->getLine(), Tok->getText());

  mParserStatus += 1;
}

CHAR8 *
EfiVfrParser::TrimHex (
  IN  CHAR8   *Str,
  OUT BOOLEAN *IsHex
  )
{
  *IsHex = FALSE;

  while (*Str && *Str == ' ') {
    Str++;
  }
  while (*Str && *Str == '0') {
    Str++;
  }
  if (*Str && (*Str == 'x' || *Str == 'X')) {
    Str++;
    *IsHex = TRUE;
  }

  return Str;
}

CHAR8 *
EfiVfrParser::_U32TOS (
  IN UINT32 Value
  )
{
  CHAR8 *Str;
  Str = new CHAR8[20];
  sprintf (Str, "%d", Value);
  return Str;
}

UINT8
EfiVfrParser::_STOU8 (
  IN CHAR8*Str
  )
{
  BOOLEAN IsHex;
  UINT8   Value;
  CHAR8   c;

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    //
    // BUG: does not handle overflow here
    //
    (IsHex == TRUE) ? (Value <<= 4) : (Value *= 10);

    if ((IsHex == TRUE) && (c >= 'a') && (c <= 'f')) {
      Value += (c - 'a' + 10);
    }
    if ((IsHex == TRUE) && (c >= 'A') && (c <= 'F')) {
      Value += (c - 'A' + 10);
    }
    if (c >= '0' && c <= '9') {
      Value += (c - '0');
    }
  }

  return Value;
}

UINT16
EfiVfrParser::_STOU16 (
  IN CHAR8*Str
  )
{
  BOOLEAN IsHex;
  UINT16  Value;
  CHAR8   c;

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    //
    // BUG: does not handle overflow here
    //
    (IsHex == TRUE) ? (Value <<= 4) : (Value *= 10);

    if ((IsHex == TRUE) && (c >= 'a') && (c <= 'f')) {
      Value += (c - 'a' + 10);
    }
    if ((IsHex == TRUE) && (c >= 'A') && (c <= 'F')) {
      Value += (c - 'A' + 10);
    }
    if (c >= '0' && c <= '9') {
      Value += (c - '0');
    }
  }

  return Value;
}

UINT32
EfiVfrParser::_STOU32 (
  IN CHAR8*Str
  )
{
  BOOLEAN IsHex;
  UINT32  Value;
  CHAR8   c;

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    //
    // BUG: does not handle overflow here
    //
    (IsHex == TRUE) ? (Value <<= 4) : (Value *= 10);

    if ((IsHex == TRUE) && (c >= 'a') && (c <= 'f')) {
      Value += (c - 'a' + 10);
    }
    if ((IsHex == TRUE) && (c >= 'A') && (c <= 'F')) {
      Value += (c - 'A' + 10);
    }
    if (c >= '0' && c <= '9') {
      Value += (c - '0');
    }
  }

  return Value;
}

UINT64
EfiVfrParser::_STOU64 (
  IN CHAR8*Str
  )
{
  BOOLEAN IsHex;
  UINT64  Value;
  CHAR8   c;

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    //
    // BUG: does not handle overflow here
    //
    (IsHex == TRUE) ? (Value <<= 4) : (Value *= 10);

    if ((IsHex == TRUE) && (c >= 'a') && (c <= 'f')) {
      Value += (c - 'a' + 10);
    }
    if ((IsHex == TRUE) && (c >= 'A') && (c <= 'F')) {
      Value += (c - 'A' + 10);
    }
    if (c >= '0' && c <= '9') {
      Value += (c - '0');
    }
  }

  return Value;
}

EFI_HII_DATE
EfiVfrParser::_STOD (
  IN CHAR8 *Year,
  IN CHAR8 *Month,
  IN CHAR8 *Day
  )
{
  EFI_HII_DATE Date;

  Date.Year  = _STOU16 (Year);
  Date.Month = _STOU8 (Month);
  Date.Day   = _STOU8 (Day);

  return Date;
}

EFI_HII_TIME
EfiVfrParser::_STOT (
  IN CHAR8 *Hour,
  IN CHAR8 *Minute,
  IN CHAR8 *Second
  )
{
  EFI_HII_TIME Time;

  Time.Hour   = _STOU8 (Hour);
  Time.Minute = _STOU8 (Minute);
  Time.Second = _STOU8 (Second);

  return Time;
}

EFI_STRING_ID
EfiVfrParser::_STOSID (
  IN CHAR8 *Str
  )
{
  return (EFI_STRING_ID)_STOU16(Str);
}

EFI_FORM_ID
EfiVfrParser::_STOFID (
  IN CHAR8 *Str
  )
{
  return (EFI_FORM_ID)_STOU16(Str);
}

EFI_QUESTION_ID
EfiVfrParser::_STOQID (
  IN CHAR8 *Str
  )
{
  return (EFI_QUESTION_ID)_STOU16(Str);
}

VOID
EfiVfrParser::_STRCAT (
  IN OUT CHAR8 **Dest,
  IN CONST CHAR8 *Src
  )
{
  CHAR8   *NewStr;
  UINT32 Len;

  if ((Dest == NULL) || (Src == NULL)) {
    return;
  }

  Len = (*Dest == NULL) ? 0 : strlen (*Dest);
  Len += strlen (Src);
  if ((NewStr = new CHAR8[Len + 1]) == NULL) {
    return;
  }
  NewStr[0] = '\0';
  if (*Dest != NULL) {
    strcpy (NewStr, *Dest);
    delete *Dest;
  }
  strcat (NewStr, Src);

  *Dest = NewStr;
}

EFI_HII_REF
EfiVfrParser::_STOR (
  IN CHAR8    *QuestionId,
  IN CHAR8    *FormId,
  IN EFI_GUID *FormSetGuid,
  IN CHAR8    *DevicePath
  )
{
  EFI_HII_REF Ref;
  UINT32      Index;

  memcpy (&Ref.FormSetGuid, FormSetGuid, sizeof (EFI_GUID));
  Ref.QuestionId  = _STOQID (QuestionId);
  Ref.FormId      = _STOFID (FormId);
  Ref.DevicePath  = _STOSID (DevicePath);

  return Ref;
}

//
// framework vfr to default declare varstore for each structure
//
VOID
EfiVfrParser::_DeclareDefaultFrameworkVarStore (
  IN UINT32 LineNo
  )
{
  SVfrVarStorageNode    *pNode;
  UINT32                TypeSize;
  BOOLEAN               FirstNode;
  CONST CHAR8           VarName[] = "Setup";

  FirstNode = TRUE;
  pNode = mCVfrDataStorage.GetBufferVarStoreList();
  if (pNode == NULL && gCVfrVarDataTypeDB.mFirstNewDataTypeName != NULL) {
    //
    // Create the default Buffer Var Store when no VarStore is defined.
    // its name should be "Setup"
    //
    gCVfrVarDataTypeDB.GetDataTypeSize (gCVfrVarDataTypeDB.mFirstNewDataTypeName, &TypeSize);
    CIfrVarStore      VSObj;
    VSObj.SetLineNo (LineNo);
    VSObj.SetVarStoreId (0x1); //the first and only one Buffer Var Store
    VSObj.SetSize ((UINT16) TypeSize);
    //VSObj.SetName (gCVfrVarDataTypeDB.mFirstNewDataTypeName);
    VSObj.SetName ((CHAR8 *) VarName);
    VSObj.SetGuid (&mFormsetGuid);
#ifdef VFREXP_DEBUG
    printf ("Create the default VarStoreName is %s\n", gCVfrVarDataTypeDB.mFirstNewDataTypeName);
#endif
  } else {
    for (; pNode != NULL; pNode = pNode->mNext) {
      //
      // create the default varstore opcode for not declared varstore
      // the first varstore name should be "Setup"
      //
      if (!pNode->mAssignedFlag) {
        CIfrVarStore      VSObj;
        VSObj.SetLineNo (LineNo);
        VSObj.SetVarStoreId (pNode->mVarStoreId);
        VSObj.SetSize ((UINT16) pNode->mStorageInfo.mDataType->mTotalSize);
        if (FirstNode) {
          VSObj.SetName ((CHAR8 *) VarName);
          FirstNode = FALSE;
        } else {
          VSObj.SetName (pNode->mVarStoreName);
        }
        VSObj.SetGuid (&pNode->mGuid);
#ifdef VFREXP_DEBUG
        printf ("undefined VarStoreName is %s and Id is 0x%x\n", pNode->mVarStoreName, pNode->mVarStoreId);
#endif
      }
    }
  }

  pNode = mCVfrDataStorage.GetEfiVarStoreList();
  for (; pNode != NULL; pNode = pNode->mNext) {
    //
    // create the default efi varstore opcode for not exist varstore
    //
    if (!pNode->mAssignedFlag) {
      CIfrVarStoreEfi VSEObj;
      VSEObj.SetLineNo (LineNo);
      VSEObj.SetAttributes (0x00000002); //hardcode EFI_VARIABLE_BOOTSERVICE_ACCESS attribute
      VSEObj.SetGuid (&pNode->mGuid);
      VSEObj.SetVarStoreId (pNode->mVarStoreId);
      // Generate old efi varstore storage structure for compatiable with old "VarEqVal" opcode,
      // which is 3 bytes less than new structure define in UEFI Spec 2.3.1.
      VSEObj.SetBinaryLength (sizeof (EFI_IFR_VARSTORE_EFI) - 3);
#ifdef VFREXP_DEBUG
      printf ("undefined Efi VarStoreName is %s and Id is 0x%x\n", pNode->mVarStoreName, pNode->mVarStoreId);
#endif
    }
  }

}

VOID
EfiVfrParser::_DeclareDefaultLinearVarStore (
  IN UINT32 LineNo
  )
{
  UINT32            Index;
  CHAR8             **TypeNameList;
  UINT32            ListSize;
  CONST CHAR8       DateName[] = "Date";
  CONST CHAR8       TimeName[] = "Time";
  CONST CHAR8       DateType[] = "EFI_HII_DATE";
  CONST CHAR8       TimeType[] = "EFI_HII_TIME";

  gCVfrVarDataTypeDB.GetUserDefinedTypeNameList (&TypeNameList, &ListSize);

  for (Index = 0; Index < ListSize; Index++) {
    UINT32            Size;
    EFI_VARSTORE_ID   VarStoreId;
    CIfrVarStore      VSObj;

    VSObj.SetLineNo (LineNo);
    mCVfrDataStorage.DeclareBufferVarStore (
                       TypeNameList[Index],
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       TypeNameList[Index],
                       EFI_VARSTORE_ID_INVALID
                       );
    mCVfrDataStorage.GetVarStoreId(TypeNameList[Index], &VarStoreId);
    VSObj.SetVarStoreId (VarStoreId);
    gCVfrVarDataTypeDB.GetDataTypeSize(TypeNameList[Index], &Size);
    VSObj.SetSize ((UINT16) Size);
    VSObj.SetName (TypeNameList[Index]);
    VSObj.SetGuid (&mFormsetGuid);
  }

//
// not required to declare Date and Time VarStore,
// because code to support old format Data and Time
//
  if (gCVfrVarDataTypeDB.IsTypeNameDefined ((CHAR8 *) DateName) == FALSE) {
    UINT32            Size;
    EFI_VARSTORE_ID   VarStoreId;
    CIfrVarStore      VSObj;

    VSObj.SetLineNo (LineNo);
    mCVfrDataStorage.DeclareBufferVarStore (
                       (CHAR8 *) DateName,
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       (CHAR8 *) DateType,
                       EFI_VARSTORE_ID_INVALID
                       );
    mCVfrDataStorage.GetVarStoreId((CHAR8 *) DateName, &VarStoreId);
    VSObj.SetVarStoreId (VarStoreId);
    gCVfrVarDataTypeDB.GetDataTypeSize((CHAR8 *) DateType, &Size);
    VSObj.SetSize ((UINT16) Size);
    VSObj.SetName ((CHAR8 *) DateName);
    VSObj.SetGuid (&mFormsetGuid);
  }

  if (gCVfrVarDataTypeDB.IsTypeNameDefined ((CHAR8 *) TimeName) == FALSE) {
    UINT32            Size;
    EFI_VARSTORE_ID   VarStoreId;
    CIfrVarStore      VSObj;

    VSObj.SetLineNo (LineNo);
    mCVfrDataStorage.DeclareBufferVarStore (
                       (CHAR8 *) TimeName,
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       (CHAR8 *) TimeType,
                       EFI_VARSTORE_ID_INVALID
                       );
    mCVfrDataStorage.GetVarStoreId((CHAR8 *) TimeName, &VarStoreId);
    VSObj.SetVarStoreId (VarStoreId);
    gCVfrVarDataTypeDB.GetDataTypeSize((CHAR8 *) TimeType, &Size);
    VSObj.SetSize ((UINT16) Size);
    VSObj.SetName ((CHAR8 *) TimeName);
    VSObj.SetGuid (&mFormsetGuid);
  }
}

VOID
EfiVfrParser::_DeclareStandardDefaultStorage (
  IN UINT32 LineNo
  )
{
  //
  // Default Store is declared.
  //
  CIfrDefaultStore DSObj;

  mCVfrDefaultStore.RegisterDefaultStore (DSObj.GetObjBinAddr(), (CHAR8 *) "Standard Defaults", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_STANDARD);
  DSObj.SetLineNo (LineNo);
  DSObj.SetDefaultName (EFI_STRING_ID_INVALID);
  DSObj.SetDefaultId (EFI_HII_DEFAULT_CLASS_STANDARD);

  //
  // Default MANUFACTURING Store is declared.
  //
  CIfrDefaultStore DSObjMF;

  mCVfrDefaultStore.RegisterDefaultStore (DSObjMF.GetObjBinAddr(), (CHAR8 *) "Standard ManuFacturing", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_MANUFACTURING);
  DSObjMF.SetLineNo (LineNo);
  DSObjMF.SetDefaultName (EFI_STRING_ID_INVALID);
  DSObjMF.SetDefaultId (EFI_HII_DEFAULT_CLASS_MANUFACTURING);
}

VOID
EfiVfrParser::AssignQuestionKey (
  IN CIfrQuestionHeader   &QHObj,
  IN ANTLRTokenPtr        KeyTok
  )
{
  UINT16 KeyValue;

  if (KeyTok == NULL) {
    return;
  }

  KeyValue = _STOU16 (KeyTok->getText());

  if (QHObj.FLAGS () & EFI_IFR_FLAG_CALLBACK) {
    /*
     * if the question is not CALLBACK ignore the key.
    */
    _PCATCH(mCVfrQuestionDB.UpdateQuestionId (QHObj.QUESTION_ID(), KeyValue), KeyTok);
    QHObj.SetQuestionId (KeyValue);
  }
}

VOID
EfiVfrParser::ConvertIdExpr (
  IN UINT32          &ExpOpCount,
  IN UINT32          LineNo,
  IN EFI_QUESTION_ID QId,
  IN CHAR8           *VarIdStr,
  IN UINT32          BitMask
  )
{
  CIfrQuestionRef1 QR1Obj(LineNo);
  QR1Obj.SetQuestionId (QId, VarIdStr, LineNo);
  _SAVE_OPHDR_COND (QR1Obj, (ExpOpCount == 0));

  if (BitMask != 0) {
    CIfrUint32       U32Obj(LineNo);
    U32Obj.SetValue (BitMask);

    CIfrBitWiseAnd   BWAObj(LineNo);

    CIfrUint8        U8Obj(LineNo);
    switch (BitMask) {
    case DATE_YEAR_BITMASK   : U8Obj.SetValue (0); break;
    case TIME_SECOND_BITMASK : U8Obj.SetValue (0x10); break;
    case DATE_DAY_BITMASK    : U8Obj.SetValue (0x18); break;
    case TIME_HOUR_BITMASK   : U8Obj.SetValue (0); break;
    case TIME_MINUTE_BITMASK : U8Obj.SetValue (0x8); break;
    }

    CIfrShiftRight   SRObj(LineNo);
  }

  ExpOpCount += 4;
}

VOID
EfiVfrParser::IdEqValDoSpecial (
  IN UINT32           &ExpOpCount,
  IN UINT32           LineNo,
  IN EFI_QUESTION_ID  QId,
  IN CHAR8            *VarIdStr,
  IN UINT32           BitMask,
  IN UINT16           ConstVal,
  IN EFI_COMPARE_TYPE CompareType
  )
{
  ConvertIdExpr (ExpOpCount, LineNo, QId, VarIdStr, BitMask);

  if (ConstVal > 0xFF) {
    CIfrUint16       U16Obj(LineNo);
    U16Obj.SetValue (ConstVal);
  } else {
    CIfrUint8        U8Obj(LineNo);
    U8Obj.SetValue ((UINT8)ConstVal);
  }

  switch (CompareType) {
  case EQUAL :
    {
      CIfrEqual EObj(LineNo);
      break;
    }
  case LESS_EQUAL :
    {
      CIfrLessEqual LEObj(LineNo);
      break;
    }
  case LESS_THAN :
    {
      CIfrLessThan LTObj(LineNo);
      break;
    }
  case GREATER_EQUAL :
    {
      CIfrGreaterEqual GEObj(LineNo);
      break;
    }
  case GREATER_THAN :
    {
      CIfrGreaterThan GTObj(LineNo);
      break;
    }
  }

  ExpOpCount += 2;
}

VOID
EfiVfrParser::IdEqIdDoSpecial (
  IN UINT32           &ExpOpCount,
  IN UINT32           LineNo,
  IN EFI_QUESTION_ID  QId1,
  IN CHAR8            *VarId1Str,
  IN UINT32           BitMask1,
  IN EFI_QUESTION_ID  QId2,
  IN CHAR8            *VarId2Str,
  IN UINT32           BitMask2,
  IN EFI_COMPARE_TYPE CompareType
  )
{
  ConvertIdExpr (ExpOpCount, LineNo, QId1, VarId1Str, BitMask1);
  ConvertIdExpr (ExpOpCount, LineNo, QId2, VarId2Str, BitMask2);

  switch (CompareType) {
  case EQUAL :
    {
      CIfrEqual EObj(LineNo);
      break;
    }
  case LESS_EQUAL :
    {
      CIfrLessEqual LEObj(LineNo);
      break;
    }
  case LESS_THAN :
    {
      CIfrLessThan LTObj(LineNo);
      break;
    }
  case GREATER_EQUAL :
    {
      CIfrGreaterEqual GEObj(LineNo);
      break;
    }
  case GREATER_THAN :
    {
      CIfrGreaterThan GTObj(LineNo);
      break;
    }
  }

  ExpOpCount++;
}

VOID
EfiVfrParser::IdEqListDoSpecial (
  IN UINT32          &ExpOpCount,
  IN UINT32          LineNo,
  IN EFI_QUESTION_ID QId,
  IN CHAR8           *VarIdStr,
  IN UINT32          BitMask,
  IN UINT16          ListLen,
  IN UINT16          *ValueList
  )
{
  UINT16 Index;

  if (ListLen == 0) {
    return;
  }

  IdEqValDoSpecial (ExpOpCount, LineNo, QId, VarIdStr, BitMask, ValueList[0], EQUAL);
  for (Index = 1; Index < ListLen; Index++) {
    IdEqValDoSpecial (ExpOpCount, LineNo, QId, VarIdStr, BitMask, ValueList[Index], EQUAL);
    CIfrOr OObj (LineNo);
    ExpOpCount++;
  }
}

VOID 
EfiVfrParser::SetOverrideClassGuid (IN EFI_GUID *OverrideClassGuid)
{
  mOverrideClassGuid = OverrideClassGuid;
}

//
// For framework vfr compatibility
//
VOID
EfiVfrParser::SetCompatibleMode (IN BOOLEAN Mode)
{
  mCompatibleMode = Mode;
  mCVfrQuestionDB.SetCompatibleMode (Mode);
}
>>
