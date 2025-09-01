/*++ @file
Vfr Syntax

Copyright (c) 2004 - 2025, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2025, Loongson Technology Corporation Limited. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

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
#ifdef UINT8_MAX
#undef UINT8_MAX
#endif
#include "stdio.h"
#include "PBlackBox.h"
#include "DLexerBase.h"
#include "VfrLexer.h"
#include "AToken.h"

#define GET_LINENO(Obj)       ((Obj)->getLine())
#define SET_LINE_INFO(Obj, L) do {(Obj).SetLineNo((L)->getLine());} while (0)
#define CRT_END_OP(Obj)       do {CIfrEnd EObj; if (Obj != NULL) EObj.SetLineNo ((Obj)->getLine());} while (0)

typedef ANTLRCommonToken ANTLRToken;

class CVfrDLGLexer : public VfrLexer
{
public:
  CVfrDLGLexer (DLGFileInput *F) : VfrLexer (F) {};
  void errstd (const char *Text)
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
  VfrParser.parser()->SetOverrideClassGuid (InputInfo->OverrideClassGuid);
  return VfrParser.parser()->vfrProgram();
}
>>

//
// Define a lexical class for parsing quoted strings. Basically
// starts with a double quote, and ends with a double quote that
// is not preceded with a backslash.
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
#token GccLineDefinition                        "#\ [0-9]+\ \"~[\"]+\"[\ \t]*([1234][\ \t]*)*\n" << gCVfrErrorHandle.ParseFileScopeRecord (begexpr (), line ()); skip (); newline (); >>
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
#token EndSubtitle("endsubtitle")               "endsubtitle"
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
#token Union("union")                           "union"
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
#token WarningIf("warningif")                   "warningif"
#token NoSubmitIf("nosubmitif")                 "nosubmitif"
#token EndIf("endif")                           "endif"
#token Key("key")                               "key"
#token DefaultFlag("DEFAULT")                   "DEFAULT"
#token ManufacturingFlag("MANUFACTURING")       "MANUFACTURING"
#token InteractiveFlag("INTERACTIVE")           "INTERACTIVE"
#token NVAccessFlag("NV_ACCESS")                "NV_ACCESS"
#token ResetRequiredFlag("RESET_REQUIRED")      "RESET_REQUIRED"
#token ReconnectRequiredFlag("RECONNECT_REQUIRED") "RECONNECT_REQUIRED"
#token LateCheckFlag("LATE_CHECK")              "LATE_CHECK"
#token ReadOnlyFlag("READ_ONLY")                "READ_ONLY"
#token OptionOnlyFlag("OPTIONS_ONLY")           "OPTIONS_ONLY"
#token RestStyleFlag("REST_STYLE")              "REST_STYLE"
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
    | vfrDataUnionDefinition
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
    "," N:Number                                    << PackAction |= VFR_PACK_ASSIGN; PackNumber = _STOU32(N->getText(), N->getLine()); >>
  }
                                                    << gCVfrVarDataTypeDB.Pack (LineNum, PackAction, Identifier, PackNumber); >>
  ;

pragmaPackNumber :
  <<
     UINT32 LineNum;
     UINT32 PackNumber = DEFAULT_PACK_ALIGN;
  >>
  N:Number                                          << LineNum = N->getLine(); PackNumber = _STOU32(N->getText(), N->getLine()); >>
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

  vfrDataUnionDefinition :
  { TypeDef } Union                                << gCVfrVarDataTypeDB.DeclareDataTypeBegin (); >>
  { NonNvDataMap }
  {
    N1:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N1->getText()), N1); >>
  }
  OpenBrace
    vfrDataStructFields[TRUE]
  CloseBrace
  {
    N2:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N2->getText()), N2); >>
  }
  ";"                                               << gCVfrVarDataTypeDB.DeclareDataTypeEnd ();>>
  ;

vfrDataStructDefinition :
  { TypeDef } Struct                                << gCVfrVarDataTypeDB.DeclareDataTypeBegin (); >>
  { NonNvDataMap }
  {
    N1:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N1->getText()), N1); >>
  }
  OpenBrace
    vfrDataStructFields[FALSE]
  CloseBrace
  {
    N2:StringIdentifier                             << _PCATCH(gCVfrVarDataTypeDB.SetNewTypeName (N2->getText()), N2); >>
  }
  ";"                                               << gCVfrVarDataTypeDB.DeclareDataTypeEnd (); >>
  ;

vfrDataStructFields [BOOLEAN  FieldInUnion]:
  (
     dataStructField64 [FieldInUnion]    |
     dataStructField32 [FieldInUnion]    |
     dataStructField16 [FieldInUnion]    |
     dataStructField8  [FieldInUnion]    |
     dataStructFieldBool [FieldInUnion]  |
     dataStructFieldString [FieldInUnion]|
     dataStructFieldDate  [FieldInUnion] |
     dataStructFieldTime  [FieldInUnion] |
     dataStructFieldRef   [FieldInUnion] |
     dataStructFieldUser  [FieldInUnion] |
     dataStructBitField64 [FieldInUnion] |
     dataStructBitField32 [FieldInUnion] |
     dataStructBitField16 [FieldInUnion] |
     dataStructBitField8  [FieldInUnion]
  )*
  ;

dataStructField64 [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"UINT64"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructField32 [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"UINT32"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructField16 [BOOLEAN  FieldInUnion]:
  << 
    UINT32 ArrayNum = 0; 
  >>
  ("UINT16" | "CHAR16")
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), (CHAR8 *) "UINT16", ArrayNum, FieldInUnion), N); >>
  ;

dataStructField8 [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"UINT8"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldBool [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"BOOLEAN"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldString [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"EFI_STRING_ID"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldDate [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_DATE"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldTime [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_TIME"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldRef [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  D:"EFI_HII_REF"
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), D->getText(), ArrayNum, FieldInUnion), N); >>
  ;

dataStructFieldUser [BOOLEAN  FieldInUnion]:
  << UINT32 ArrayNum = 0; >>
  T:StringIdentifier
  N:StringIdentifier
  {
    OpenBracket I:Number CloseBracket               << ArrayNum = _STOU32(I->getText(), I->getLine()); >>
  }
  ";"                                               << _PCATCH(gCVfrVarDataTypeDB.DataTypeAddField (N->getText(), T->getText(), ArrayNum, FieldInUnion), T); >>
  ;

dataStructBitField64[BOOLEAN  FieldInUnion]:
  <<
    UINT32 Width = 0;
    BOOLEAN HasBitFieldName = FALSE;
  >>
  D:"UINT64"
  {
    N:StringIdentifier      << HasBitFieldName = TRUE;>>
  }
  ":" I:Number              << Width = _STOU32(I->getText(), I->getLine());>>

  ";"                       << if (HasBitFieldName) {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (N->getText(), D->getText(), Width, FieldInUnion), N);
                               } else {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (NULL, D->getText(), Width, FieldInUnion), D);
                               }
                            >>
  ;

dataStructBitField32[BOOLEAN  FieldInUnion]:
  <<
    UINT32 Width = 0;
    BOOLEAN HasBitFieldName = FALSE;
  >>
  D:"UINT32"
  {
    N:StringIdentifier      << HasBitFieldName = TRUE;>>
  }

  ":" I:Number              << Width = _STOU32(I->getText(), I->getLine());>>

  ";"                       << if (HasBitFieldName) {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (N->getText(), D->getText(), Width, FieldInUnion), N);
                               } else {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (NULL, D->getText(), Width, FieldInUnion), D);
                               }
                            >>
  ;

dataStructBitField16[BOOLEAN  FieldInUnion]:
  <<
    UINT32 Width = 0;
    BOOLEAN HasBitFieldName = FALSE;
  >>
  D:"UINT16"
  {
    N:StringIdentifier      << HasBitFieldName = TRUE;>>
  }
  ":" I:Number              << Width = _STOU32(I->getText(), I->getLine());>>

  ";"                       << if (HasBitFieldName) {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (N->getText(), D->getText(), Width, FieldInUnion), N);
                               } else {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (NULL, D->getText(), Width, FieldInUnion), D);
                               }
                            >>
  ;

dataStructBitField8[BOOLEAN  FieldInUnion]:
  <<
    UINT32 Width = 0;
    BOOLEAN  HasBitFieldName = FALSE;
  >>
  D:"UINT8"
  {
    N:StringIdentifier      << HasBitFieldName = TRUE;>>
  }
  ":" I:Number              << Width = _STOU32(I->getText(), I->getLine());>>

  ";"                       << if (HasBitFieldName) {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (N->getText(), D->getText(), Width, FieldInUnion), N);
                               } else {
                                 _PCATCH(gCVfrVarDataTypeDB.DataTypeAddBitField (NULL, D->getText(), Width, FieldInUnion), D);
                               }
                            >>
  ;

//*****************************************************************************
//
// the syntax of GUID definition
//
guidSubDefinition [EFI_GUID &Guid] :
  G4:Number "," G5:Number "," G6:Number "," G7:Number "," G8:Number "," G9:Number "," G10:Number "," G11:Number
                                                    <<
                                                       Guid.Data4[0] = _STOU8(G4->getText(), G4->getLine());
                                                       Guid.Data4[1] = _STOU8(G5->getText(), G5->getLine());
                                                       Guid.Data4[2] = _STOU8(G6->getText(), G6->getLine());
                                                       Guid.Data4[3] = _STOU8(G7->getText(), G7->getLine());
                                                       Guid.Data4[4] = _STOU8(G8->getText(), G8->getLine());
                                                       Guid.Data4[5] = _STOU8(G9->getText(), G9->getLine());
                                                       Guid.Data4[6] = _STOU8(G10->getText(), G10->getLine());
                                                       Guid.Data4[7] = _STOU8(G11->getText(), G11->getLine());
                                                    >>
  ;

guidDefinition [EFI_GUID &Guid] :
  OpenBrace
    G1:Number "," G2:Number "," G3:Number ","
                                                    <<
                                                       Guid.Data1 = _STOU32 (G1->getText(), G1->getLine());
                                                       Guid.Data2 = _STOU16 (G2->getText(), G2->getLine());
                                                       Guid.Data3 = _STOU16 (G3->getText(), G3->getLine());
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
     EFI_GUID    ClassGuid1, ClassGuid2, ClassGuid3, ClassGuid4;
     UINT8       ClassGuidNum = 0;
     CIfrFormSet *FSObj = NULL;
     UINT16      C, SC;
     CHAR8*      InsertOpcodeAddr = NULL;
  >>
  L:FormSet
  Uuid "=" guidDefinition[Guid] ","
  Title "=" "STRING_TOKEN" "\(" S1:Number "\)" ","
  Help  "=" "STRING_TOKEN" "\(" S2:Number "\)" ","
  {
    ClassGuid "=" guidDefinition[ClassGuid1]        << ++ClassGuidNum; >>
                  {
                     "\|" guidDefinition[ClassGuid2]  << ++ClassGuidNum; >>
                     {
                      "\|" guidDefinition[ClassGuid3]  << ++ClassGuidNum; >>
                       {
                         "\|" guidDefinition[ClassGuid4]  << ++ClassGuidNum; >>
                       }
                     }
                  }
                  ","
  }
                                                    <<
                                                      if (mOverrideClassGuid != NULL && ClassGuidNum >= 4) {
                                                        _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Already has 4 class guids, can't add extra class guid!");
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
                                                        if (mOverrideClassGuid != NULL) {
                                                          ClassGuidNum ++;
                                                        }
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&ClassGuid1);
                                                        FSObj->SetClassGuid(&ClassGuid2);
                                                        FSObj->SetClassGuid(&ClassGuid3);
                                                        if (mOverrideClassGuid != NULL) {
                                                          FSObj->SetClassGuid(mOverrideClassGuid);
                                                        }
                                                        break;
                                                      case 4:
                                                        FSObj = new CIfrFormSet(sizeof(EFI_IFR_FORM_SET) + ClassGuidNum * sizeof(EFI_GUID));
                                                        FSObj->SetClassGuid(&ClassGuid1);
                                                        FSObj->SetClassGuid(&ClassGuid2);
                                                        FSObj->SetClassGuid(&ClassGuid3);
                                                        FSObj->SetClassGuid(&ClassGuid4);
                                                        break;
                                                      default:
                                                        break;
                                                      }

                                                      SET_LINE_INFO (*FSObj, L);
                                                      FSObj->SetGuid (&Guid);
                                                      FSObj->SetFormSetTitle (_STOSID(S1->getText(), S1->getLine()));
                                                      FSObj->SetHelp (_STOSID(S2->getText(), S2->getLine()));
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
                                                      //
                                                      // Declare undefined Question so that they can be used in expression.
                                                      //
                                                      if (gCFormPkg.HavePendingUnassigned()) {
                                                        mParserStatus += gCFormPkg.DeclarePendingQuestion (
                                                                    gCVfrVarDataTypeDB,
                                                                    gCVfrDataStorage,
                                                                    mCVfrQuestionDB,
                                                                    &mFormsetGuid,
                                                                    E->getLine(),
                                                                    &InsertOpcodeAddr
                                                                  );
                                                        gNeedAdjustOpcode = TRUE;
                                                      }

                                                      CRT_END_OP (E);

                                                      //
                                                      // Adjust the pending question position.
                                                      // Move the position from current to before the end of the last form in the form set.
                                                      //
                                                      if (gNeedAdjustOpcode) {
                                                        gCFormPkg.AdjustDynamicInsertOpcode (
                                                          mLastFormEndAddr,
                                                          InsertOpcodeAddr,
                                                          FALSE
                                                        );
                                                      }

                                                      if (FSObj != NULL) {
                                                        delete FSObj;
                                                      }
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
        U64:"UINT64" {OpenBracket AN1:Number CloseBracket <<ArrayNum = _STOU32(AN1->getText(), AN1->getLine());>>}
                                                      << TypeName = U64->getText(); LineNum = U64->getLine(); >>
      | U32:"UINT32" {OpenBracket AN2:Number CloseBracket <<ArrayNum = _STOU32(AN2->getText(), AN2->getLine());>>}
                                                      << TypeName = U32->getText(); LineNum = U32->getLine(); >>
      | U16:"UINT16" {OpenBracket AN3:Number CloseBracket <<ArrayNum = _STOU32(AN3->getText(), AN3->getLine());>>}
                                                      << TypeName = U16->getText(); LineNum = U16->getLine(); >>
      | U8:"UINT8"   {OpenBracket AN4:Number CloseBracket <<ArrayNum = _STOU32(AN4->getText(), AN4->getLine());>>}
                                                      << TypeName = U8->getText(); LineNum = U8->getLine(); >>
      | BL:"BOOLEAN" {OpenBracket AN5:Number CloseBracket <<ArrayNum = _STOU32(AN5->getText(), AN5->getLine());>>}
                                                      << TypeName = BL->getText(); LineNum = BL->getLine(); >>
      | SI:"EFI_STRING_ID" {OpenBracket AN6:Number CloseBracket <<ArrayNum = _STOU32(AN6->getText(), AN6->getLine());>>}
                                                      << TypeName = SI->getText(); LineNum = SI->getLine(); >>
      | D:"EFI_HII_DATE" {OpenBracket AN7:Number CloseBracket <<ArrayNum = _STOU32(AN7->getText(), AN7->getLine());>>}
                                                      << TypeName = D->getText(); LineNum = D->getLine(); IsStruct = TRUE;>>
      | T:"EFI_HII_TIME" {OpenBracket AN8:Number CloseBracket <<ArrayNum = _STOU32(AN8->getText(), AN8->getLine());>>}
                                                      << TypeName = T->getText(); LineNum = T->getLine(); IsStruct = TRUE;>>
      | R:"EFI_HII_REF" {OpenBracket AN9:Number CloseBracket <<ArrayNum = _STOU32(AN9->getText(), AN9->getLine());>>}
                                                      << TypeName = R->getText(); LineNum = R->getLine(); IsStruct = TRUE;>>                                                
      | TN:StringIdentifier {OpenBracket AN10:Number CloseBracket <<ArrayNum = _STOU32(AN10->getText(), AN10->getLine());>>}
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
     BOOLEAN  BitField = FALSE;
     UINT64   Value;
     UINT64   Mask;
     UINT16   Offset;
     UINT8    PreBits;
  >>
(
  ("," "data" {OpenBracket IDX1:Number CloseBracket <<IsArray = TRUE;>>}
          <<
            ArrayIdx = 0;
            if (IsArray == TRUE) {
              ArrayIdx = _STOU8(IDX1->getText(), IDX1->getLine());
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
                Data_U64 = _STOU64(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_U64, TypeSize);
              }else if (strcmp ("UINT32", TypeName) == 0) {
                Data_U32 = _STOU32(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_U32, TypeSize);                                                    
              }else if (strcmp ("UINT16", TypeName) == 0) {
                Data_U16 = _STOU16(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_U16, TypeSize);                                                    
              }else if (strcmp ("UINT8", TypeName) == 0) {
                Data_U8 = _STOU8(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_U8, TypeSize);                                                    
              }else if (strcmp ("BOOLEAN", TypeName)== 0) {
                Data_BL = _STOU8(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_BL, TypeSize);                                                    
              }else if (strcmp ("EFI_STRING_ID", TypeName) == 0) {
                Data_SID = _STOSID(RD->getText(), RD->getLine());
                memcpy (ByteOffset, &Data_SID, TypeSize);                                                    
              }
            } else {
              gCVfrVarDataTypeDB.GetDataFieldInfo(TFName, FieldOffset, FieldType, FieldSize, BitField);
              if (BitField) {
                Mask = (1 << FieldSize) - 1;
                Offset = FieldOffset / 8;
                PreBits = FieldOffset % 8;
                Mask <<= PreBits;
              }
              switch (FieldType) {
              case EFI_IFR_TYPE_NUM_SIZE_8:
                 Data_U8 = _STOU8(RD->getText(), RD->getLine());
                 if (BitField) {
                   //
                   // Set the value to the bit fileds.
                   //
                   Value = *(UINT8*) (ByteOffset + Offset);
                   Data_U8 <<= PreBits;
                   Value = (Value & (~Mask)) | Data_U8;
                   memcpy (ByteOffset + Offset, &Value, sizeof (UINT8));
                 } else {
                   memcpy (ByteOffset + FieldOffset, &Data_U8, FieldSize);
                 }
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_16:
                 Data_U16 = _STOU16(RD->getText(), RD->getLine());
                 if (BitField) {
                   //
                   // Set the value to the bit fileds.
                   //
                   Value = *(UINT16*) (ByteOffset + Offset);
                   Data_U16 <<= PreBits;
                   Value = (Value & (~Mask)) | Data_U16;
                   memcpy (ByteOffset + Offset, &Value, sizeof (UINT16));
                 } else {
                   memcpy (ByteOffset + FieldOffset, &Data_U16, FieldSize);
                 }
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_32:
                 Data_U32 = _STOU32(RD->getText(), RD->getLine());
                 if (BitField) {
                   //
                   // Set the value to the bit fileds.
                   //
                   Value = *(UINT32*) (ByteOffset + Offset);
                   Data_U32 <<= PreBits;
                   Value = (Value & (~Mask)) | Data_U32;
                   memcpy (ByteOffset + Offset, &Value, sizeof (UINT32));
                 } else {
                   memcpy (ByteOffset + FieldOffset, &Data_U32, FieldSize);
                 }
                 break;
              case EFI_IFR_TYPE_NUM_SIZE_64:
                 Data_U64 = _STOU64(RD->getText(), RD->getLine());
                 if (BitField) {
                   //
                   // Set the value to the bit fileds.
                   //
                   Value = *(UINT64*) (ByteOffset + Offset);
                   Data_U64 <<= PreBits;
                   Value = (Value & (~Mask)) | Data_U64;
                   memcpy (ByteOffset + Offset, &Value, sizeof (UINT64));
                 } else {
                   memcpy (ByteOffset + FieldOffset, &Data_U64, FieldSize);
                 }
                 break;
              case EFI_IFR_TYPE_BOOLEAN:
                 Data_BL = _STOU8(RD->getText(), RD->getLine());
                 memcpy (ByteOffset + FieldOffset, &Data_BL, FieldSize);
                 break;
              case EFI_IFR_TYPE_STRING:
                 Data_SID = _STOSID(RD->getText(), RD->getLine());
                 memcpy (ByteOffset + FieldOffset, &Data_SID, FieldSize);
                 break;
              default:
                 break;
              }
            }
            if (TFName != NULL) { delete[] TFName; TFName = NULL; }
          >>
  )*
)
;


vfrStatementDefaultStore :
  << UINT16  DefaultId = EFI_HII_DEFAULT_CLASS_STANDARD; >>
  D:DefaultStore N:StringIdentifier ","
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)"
  {
    "," Attribute "=" A:Number                      << DefaultId = _STOU16(A->getText(), A->getLine()); >>
  }
                                                    <<
                                                       if (gCVfrDefaultStore.DefaultIdRegistered (DefaultId) == FALSE) {
                                                         CIfrDefaultStore DSObj;
                                                         _PCATCH(gCVfrDefaultStore.RegisterDefaultStore (DSObj.GetObjBinAddr<CHAR8>(), N->getText(), _STOSID(S->getText(), S->getLine()), DefaultId)), D->getLine();
                                                         DSObj.SetLineNo(D->getLine());
                                                         DSObj.SetDefaultName (_STOSID(S->getText(), S->getLine()));
                                                         DSObj.SetDefaultId (DefaultId);
                                                       } else {
                                                         _PCATCH(gCVfrDefaultStore.ReRegisterDefaultStoreById (DefaultId, N->getText(), _STOSID(S->getText(), S->getLine()))), D->getLine();
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
     BOOLEAN         IsBitVarStore = FALSE;
  >>
  V:Varstore                                        << VSObj.SetLineNo(V->getLine()); >>
  (
      TN:StringIdentifier ","                       << TypeName = TN->getText(); LineNum = TN->getLine(); IsBitVarStore = gCVfrVarDataTypeDB.DataTypeHasBitField (TN->getText());>>
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
                                                         (INTN)(VarStoreId = _STOU16(ID->getText(), ID->getLine())) != 0,
                                                         (INTN)TRUE,
                                                         ID,
                                                         "varid 0 is not allowed."
                                                         );
                                                    >>
  }
  Name "=" SN:StringIdentifier ","
  Uuid "=" guidDefinition[Guid]
                                                    <<

                                                       StoreName = SN->getText();
                                                       _PCATCH(gCVfrDataStorage.DeclareBufferVarStore (
                                                                                StoreName,
                                                                                &Guid,
                                                                                &gCVfrVarDataTypeDB,
                                                                                TypeName,
                                                                                VarStoreId,
                                                                                IsBitVarStore
                                                                                ), LineNum);
                                                       VSObj.SetGuid (&Guid);
                                                       _PCATCH(gCVfrDataStorage.GetVarStoreId(StoreName, &VarStoreId, &Guid), SN);
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
     BOOLEAN         CustomizedName = FALSE;
     BOOLEAN         IsBitVarStore = FALSE;
  >>
  E:Efivarstore                                     << VSEObj.SetLineNo(E->getLine()); >>
  (
      TN:StringIdentifier ","                       << TypeName = TN->getText(); LineNum = TN->getLine(); CustomizedName = TRUE; IsBitVarStore = gCVfrVarDataTypeDB.DataTypeHasBitField (TN->getText());>>
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
                                                         (INTN)(VarStoreId = _STOU16(ID->getText(), ID->getLine())) != 0,
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
                                                       StoreName = gCVfrStringDB.GetVarStoreNameFormStringId(_STOSID(VN->getText(), VN->getLine()));
                                                       if (StoreName == NULL) {
                                                         _PCATCH (VFR_RETURN_UNSUPPORTED, VN->getLine(), "Can't get varstore name for this StringId!");
                                                       }
                                                       if (!CustomizedName) {
                                                         _PCATCH (VFR_RETURN_UNSUPPORTED, E->getLine(), "Old style efivarstore must have String Identifier!");
                                                         return;
                                                       }
                                                       Size = _STOU32(N->getText(), N->getLine());
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
                                                       _PCATCH(gCVfrDataStorage.DeclareBufferVarStore (
                                                                                    StoreName,
                                                                                    &Guid,
                                                                                    &gCVfrVarDataTypeDB,
                                                                                    TypeName,
                                                                                    VarStoreId,
                                                                                    IsBitVarStore
                                                                                    ), LineNum);
                                                         _PCATCH(gCVfrDataStorage.GetVarStoreId(StoreName, &VarStoreId, &Guid), SN);
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &Size), LineNum);
                                                       } else {
                                                       _PCATCH(gCVfrDataStorage.DeclareBufferVarStore (
                                                                                  TN->getText(),
                                                                                  &Guid,
                                                                                  &gCVfrVarDataTypeDB,
                                                                                  TypeName,
                                                                                  VarStoreId,
                                                                                  FALSE
                                                                                  ), LineNum);
                                                         _PCATCH(gCVfrDataStorage.GetVarStoreId(TN->getText(), &VarStoreId, &Guid), VN);
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataTypeSize(TypeName, &Size), N->getLine());
                                                       }
                                                       VSEObj.SetGuid (&Guid);
                                                       VSEObj.SetVarStoreId (VarStoreId);
                                                       
                                                       VSEObj.SetSize ((UINT16) Size);
                                                       VSEObj.SetName (StoreName);
                                                       if (IsUEFI23EfiVarstore == FALSE && StoreName != NULL) {
                                                         delete[] StoreName;
                                                       }
                                                    >>
  ";"
  ;

vfrVarStoreEfiAttr [UINT32 & Attr] :
  N:Number                                          << $Attr |= _STOU32(N->getText(), N->getLine()); >>
  ;

vfrStatementVarStoreNameValue :
  <<
     EFI_GUID              Guid;
     CIfrVarStoreNameValue VSNVObj;
     EFI_VARSTORE_ID       VarStoreId = EFI_VARSTORE_ID_INVALID;
     BOOLEAN               Created    = FALSE;
  >>
  L:NameValueVarStore                               << VSNVObj.SetLineNo(L->getLine()); >>
  SN:StringIdentifier ","
  {
    VarId "=" ID:Number ","                         <<
                                                       _PCATCH(
                                                         (INTN)(VarStoreId = _STOU16(ID->getText(), ID->getLine())) != 0,
                                                         (INTN)TRUE,
                                                         ID,
                                                         "varid 0 is not allowed."
                                                         );
                                                    >>
  }
  (
    Name "=" "STRING_TOKEN" "\(" N:Number "\)" ","  << 
                                                       if (!Created) {
                                                         _PCATCH(gCVfrDataStorage.DeclareNameVarStoreBegin (SN->getText(), VarStoreId), SN);
                                                         Created = TRUE;
                                                       }
                                                       _PCATCH(gCVfrDataStorage.NameTableAddItem (_STOSID(N->getText(), N->getLine())), SN);
                                                    >>
  )+
  Uuid "=" guidDefinition[Guid]                     << _PCATCH(gCVfrDataStorage.DeclareNameVarStoreEnd (&Guid), SN); >>
                                                    <<
                                                       VSNVObj.SetGuid (&Guid);
                                                       _PCATCH(gCVfrDataStorage.GetVarStoreId(SN->getText(), &VarStoreId, &Guid), SN);
                                                       VSNVObj.SetVarStoreId (VarStoreId);
                                                    >>
  ";"
  ;

//
// keep classDefinition and validClassNames for compatibility but not generate
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
  | N:Number                                        << $Class |= _STOU16(N->getText(), N->getLine()); >>
  ;

subclassDefinition[UINT16 & SubClass] :
  << $SubClass = 0; >>
    SubclassSetupApplication                        << $SubClass |= EFI_SETUP_APPLICATION_SUBCLASS; >>
  | SubclassGeneralApplication                      << $SubClass |= EFI_GENERAL_APPLICATION_SUBCLASS; >>
  | SubclassFrontPage                               << $SubClass |= EFI_FRONT_PAGE_SUBCLASS; >>
  | SubclassSingleUse                               << $SubClass |= EFI_SINGLE_USE_SUBCLASS; >>
  | N:Number                                        << $SubClass |= _STOU16(N->getText(), N->getLine()); >>
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
  Prompt "=" "STRING_TOKEN" "\(" S1:Number "\)" "," << $SHObj->SetPrompt (_STOSID(S1->getText(), S1->getLine())); >>
  Help   "=" "STRING_TOKEN" "\(" S2:Number "\)"     << $SHObj->SetHelp (_STOSID(S2->getText(), S2->getLine())); >>
  ;

vfrQuestionBaseInfo[EFI_VARSTORE_INFO & Info, EFI_QUESTION_ID & QId, EFI_QUESION_TYPE QType = QUESTION_NORMAL]:
 <<
    CHAR8             *QName    = NULL;
    CHAR8             *VarIdStr = NULL;
    mUsedDefaultCount           = 0;
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
                                                      QId = _STOQID(ID->getText(), ID->getLine());
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
                                                   >>
                                                   <<
                                                      if (VarIdStr != NULL) {
                                                        delete[] VarIdStr;
                                                      }
                                                      _SAVE_CURRQEST_VARINFO (Info);
                                                   >>
  ;

vfrQuestionHeader[CIfrQuestionHeader & QHObj, EFI_QUESION_TYPE QType = QUESTION_NORMAL]:
  <<
     EFI_VARSTORE_INFO Info;
     Info.mVarType               = EFI_IFR_TYPE_OTHER;
     Info.mVarTotalSize          = 0;
     Info.mInfo.mVarOffset       = EFI_VAROFFSET_INVALID;
     Info.mVarStoreId            = EFI_VARSTORE_ID_INVALID;
     Info.mIsBitVar              = FALSE;
     EFI_QUESTION_ID   QId       = EFI_QUESTION_ID_INVALID;
  >>
  vfrQuestionBaseInfo[Info, QId, QType]
                                                    << $QHObj.SetQuestionId (QId);
                                                        if (Info.mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                          $QHObj.SetVarStoreInfo (&Info);
                                                        }
                                                    >>
  vfrStatementHeader[&$QHObj]
  ;

questionheaderFlagsField[UINT8 & Flags] :
    ReadOnlyFlag                                    << $Flags |= 0x01; >>
  | InteractiveFlag                                 << $Flags |= 0x04; >>
  | ResetRequiredFlag                               << $Flags |= 0x10; >>
  | RestStyleFlag                                   << $Flags |= 0x20; >>
  | ReconnectRequiredFlag                           << $Flags |= 0x40; >>
  | O:OptionOnlyFlag                                <<
                                                       gCVfrErrorHandle.HandleWarning (
                                                          VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                          O->getLine(),
                                                          O->getText()
                                                          );
                                                    >>
  | N:NVAccessFlag                                  <<
                                                        gCVfrErrorHandle.HandleWarning (
                                                          VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                          N->getLine(),
                                                          N->getText()
                                                          );
                                                    >>
  | L:LateCheckFlag                                 <<
                                                        gCVfrErrorHandle.HandleWarning (
                                                          VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                          L->getLine(),
                                                          L->getText()
                                                          );
                                                    >>
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
     EFI_GUID              *VarGuid     = NULL;
  >>
  (
    SN1:StringIdentifier                            << SName = SN1->getText(); _STRCAT(&VarIdStr, SN1->getText()); >>
    OpenBracket I1:Number CloseBracket              <<
                                                       Idx = _STOU32(I1->getText(), I1->getLine());
                                                       _STRCAT(&VarIdStr, "[");
                                                       _STRCAT(&VarIdStr, I1->getText());
                                                       _STRCAT(&VarIdStr, "]");
                                                    >>
                                                    <<
                                                       VfrReturnCode = gCVfrDataStorage.GetVarStoreId(SName, &$Info.mVarStoreId);
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH(VfrReturnCode, SN1);
                                                         _PCATCH(gCVfrDataStorage.GetNameVarStoreInfo (&$Info, Idx), SN1);
                                                       }

                                                       QuestVarIdStr = VarIdStr;
                                                    >>
  )
  |
  (
    SN2:StringIdentifier                            << SName = SN2->getText(); _STRCAT(&VarIdStr, SName); >>
                                                    <<
                                                       VfrReturnCode = gCVfrDataStorage.GetVarStoreId(SName, &$Info.mVarStoreId);
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH(VfrReturnCode, SN2);
                                                         VarStoreType = gCVfrDataStorage.GetVarStoreType ($Info.mVarStoreId);
                                                         if (VarStoreType == EFI_VFR_VARSTORE_BUFFER || VarStoreType == EFI_VFR_VARSTORE_BUFFER_BITS) {
                                                           _PCATCH(gCVfrDataStorage.GetBufferVarStoreDataTypeName(Info.mVarStoreId, &TName), SN2);
                                                           _STRCAT(&VarStr, TName);
                                                         }
                                                       }
                                                    >>

    (
      "."                                           <<
                                                       if (CheckFlag || VfrReturnCode == VFR_RETURN_SUCCESS) {
                                                         _PCATCH((((VarStoreType != EFI_VFR_VARSTORE_BUFFER) && (VarStoreType != EFI_VFR_VARSTORE_BUFFER_BITS))? VFR_RETURN_EFIVARSTORE_USE_ERROR : VFR_RETURN_SUCCESS), SN2);
                                                       }
                                                       _STRCAT(&VarIdStr, "."); _STRCAT(&VarStr, ".");
                                                    >>
      SF:StringIdentifier                           << _STRCAT(&VarIdStr, SF->getText()); _STRCAT(&VarStr, SF->getText()); >>
      {
        OpenBracket I2:Number CloseBracket          <<
                                                       Idx = _STOU32(I2->getText(), I2->getLine());
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
                                                         _PCATCH(gCVfrDataStorage.GetEfiVarStoreInfo (&$Info), SN2);
                                                         break;
                                                       case EFI_VFR_VARSTORE_BUFFER:
                                                       case EFI_VFR_VARSTORE_BUFFER_BITS:
                                                         _PCATCH(gCVfrVarDataTypeDB.GetDataFieldInfo (VarStr, $Info.mInfo.mVarOffset, $Info.mVarType, $Info.mVarTotalSize, $Info.mIsBitVar), SN2->getLine(), VarStr);
                                                         VarGuid = gCVfrDataStorage.GetVarStoreGuid($Info.mVarStoreId);
                                                         _PCATCH((EFI_VFR_RETURN_CODE)gCVfrBufferConfig.Register (
                                                                    SName,
                                                                    VarGuid,
                                                                    NULL),
                                                                 SN2->getLine());
                                                         _PCATCH((EFI_VFR_RETURN_CODE)gCVfrBufferConfig.Write (
                                                                    'a',
                                                                    SName,
                                                                    VarGuid,
                                                                    NULL,
                                                                    $Info.mVarType,
                                                                    $Info.mInfo.mVarOffset,
                                                                    $Info.mVarTotalSize,
                                                                    Dummy),
                                                                 SN2->getLine());
                                                         _PCATCH(gCVfrDataStorage.AddBufferVarStoreFieldInfo(&$Info ),SN2->getLine());
                                                         break;
                                                       case EFI_VFR_VARSTORE_NAME:
                                                       default: break;
                                                       }

                                                       QuestVarIdStr = VarIdStr;
                                                       if (VarStr != NULL) {delete[] VarStr;}
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
                                                       Idx = _STOU32(I2->getText(), I2->getLine());
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

vfrConstantValueField[UINT8 Type, EFI_IFR_TYPE_VALUE &Value, BOOLEAN &ListType] :
  <<  
    EFI_GUID    Guid;
    BOOLEAN     Negative = FALSE;
    BOOLEAN     IntDecStyle = FALSE;
    CIfrNumeric *NumericQst = NULL;
    if (gCurrentMinMaxData != NULL && gCurrentMinMaxData->IsNumericOpcode()) {
      NumericQst = (CIfrNumeric *) gCurrentQuestion;
      IntDecStyle = (NumericQst->GetNumericFlags() & EFI_IFR_DISPLAY) == 0 ? TRUE : FALSE;
    }
    UINT8    *Type8  = (UINT8  *) &Value;
    UINT16   *Type16 = (UINT16 *) &Value;
    UINT32   *Type32 = (UINT32 *) &Value;
    UINT64   *Type64 = (UINT64 *) &Value;
    UINT16   Index = 0;
    ListType = FALSE;
  >>
    {
      "\-"                                          << Negative = TRUE;  >>
    }
    N1:Number                                       <<
                                                       //
                                                       // The value stored in bit fields is always set to UINT32 type.
                                                       //
                                                       if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                         $Value.u32 = _STOU32(N1->getText(), N1->getLine());
                                                       } else {
                                                         switch ($Type) {
                                                         case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                           $Value.u8 = _STOU8(N1->getText(), N1->getLine());
                                                           if (IntDecStyle) {
                                                             if (Negative) {
                                                               if ($Value.u8 > 0x80) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT8 type can't big than 0x7F, small than -0x80");
                                                               }
                                                             } else {
                                                               if ($Value.u8 > 0x7F) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT8 type can't big than 0x7F, small than -0x80");
                                                               }
                                                             }
                                                           }
                                                           if (Negative) {
                                                             $Value.u8 = ~$Value.u8 + 1;
                                                           }
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                           $Value.u16 = _STOU16(N1->getText(), N1->getLine());
                                                           if (IntDecStyle) {
                                                             if (Negative) {
                                                               if ($Value.u16 > 0x8000) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT16 type can't big than 0x7FFF, small than -0x8000");
                                                               }
                                                             } else {
                                                               if ($Value.u16 > 0x7FFF) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT16 type can't big than 0x7FFF, small than -0x8000");
                                                               }
                                                             }
                                                           }
                                                           if (Negative) {
                                                             $Value.u16 = ~$Value.u16 + 1;
                                                           }
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                           $Value.u32    = _STOU32(N1->getText(), N1->getLine());
                                                           if (IntDecStyle) {
                                                             if (Negative) {
                                                               if ($Value.u32 > 0x80000000) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT32 type can't big than 0x7FFFFFFF, small than -0x80000000");
                                                               }
                                                             } else {
                                                               if ($Value.u32 > 0X7FFFFFFF) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT32 type can't big than 0x7FFFFFFF, small than -0x80000000");
                                                               }
                                                             }
                                                           }
                                                           if (Negative) {
                                                             $Value.u32 = ~$Value.u32 + 1;
                                                           }
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                           $Value.u64    = _STOU64(N1->getText(), N1->getLine());
                                                           if (IntDecStyle) {
                                                             if (Negative) {
                                                               if ($Value.u64 > 0x8000000000000000) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000");
                                                               }
                                                             } else {
                                                               if ($Value.u64 > 0x7FFFFFFFFFFFFFFF) {
                                                                 _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "INT64 type can't big than 0x7FFFFFFFFFFFFFFF, small than -0x8000000000000000");
                                                               }
                                                             }
                                                           }
                                                           if (Negative) {
                                                             $Value.u64 = ~$Value.u64 + 1;
                                                           }
                                                         break;
                                                         case EFI_IFR_TYPE_BOOLEAN :
                                                           $Value.b      = _STOU8(N1->getText(), N1->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_STRING :
                                                           _PCATCH (VFR_RETURN_INVALID_PARAMETER, N1->getLine(), "string type can't be numeric constant.");
                                                         break;
                                                         case EFI_IFR_TYPE_TIME :
                                                         case EFI_IFR_TYPE_DATE :
                                                         case EFI_IFR_TYPE_REF  :
                                                         default :
                                                         break;
                                                         }
                                                       }
                                                    >>
  | B1:True                                         << $Value.b      = TRUE; >>
  | B2:False                                        << $Value.b      = FALSE; >>
  | O1:One                                          << $Value.u8     = _STOU8(O1->getText(), O1->getLine()); >>
  | O2:Ones                                         << $Value.u64    = _STOU64(O2->getText(), O2->getLine()); >>
  | Z:Zero                                          << $Value.u8     = _STOU8(Z->getText(), Z->getLine()); >>
  | HOUR:Number ":" MINUTE:Number ":" SECOND:Number << $Value.time   = _STOT(HOUR->getText(), MINUTE->getText(),SECOND->getText(), HOUR->getLine()); >>
  | YEAR:Number "/" MONTH:Number "/" DAY:Number     << $Value.date   = _STOD(YEAR->getText(), MONTH->getText(), DAY->getText(), YEAR->getLine()); >>
  | QI:Number";" FI:Number";" guidDefinition[Guid] ";" "STRING_TOKEN" "\(" DP:Number "\)" 
                                                    << $Value.ref    = _STOR(QI->getText(), FI->getText(), &Guid, DP->getText(), QI->getLine()); >>
  | "STRING_TOKEN" "\(" S1:Number "\)"              << $Value.string = _STOSID(S1->getText(), S1->getLine()); >>
  | "\{"                                            << ListType = TRUE; >>
      L1:Number                                     << 
                                                       switch (Type) {
                                                         case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                           Type8[Index]  = _STOU8(L1->getText(), L1->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                           Type16[Index] = _STOU16(L1->getText(), L1->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                           Type32[Index] = _STOU32(L1->getText(), L1->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                           Type64[Index] = _STOU64(L1->getText(), L1->getLine());
                                                         break;
                                                         default:
                                                         break;
                                                       }
                                                       Index++;
                                                    >>
      (
        "," 
        L2:Number                                   << 
                                                       switch (Type) {
                                                         case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                           Type8[Index]  = _STOU8(L2->getText(), L2->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                           Type16[Index] = _STOU16(L2->getText(), L2->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                           Type32[Index] = _STOU32(L2->getText(), L2->getLine());
                                                         break;
                                                         case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                           Type64[Index] = _STOU64(L2->getText(), L2->getLine());
                                                         break;
                                                         default:
                                                         break;
                                                       }
                                                       Index++;
                                                    >>
      )*
    "\}"                                           
  ;

//*****************************************************************************
//
// the syntax of form definition
//
vfrFormDefinition :
  << CIfrForm FObj; >>
  F:Form                                            << FObj.SetLineNo(F->getLine()); >>
  FormId "=" S1:Number ","                          << _PCATCH(FObj.SetFormId (_STOFID(S1->getText(), S1->getLine())), S1); >>
  Title "=" "STRING_TOKEN" "\(" S2:Number "\)" ";"  << FObj.SetFormTitle (_STOSID(S2->getText(), S2->getLine())); >>
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
    vfrStatementModal                        |
    vfrStatementRefreshEvent ";"
  )*
  E:EndForm                                         <<
                                                      {CIfrEnd EObj; EObj.SetLineNo (E->getLine()); mLastFormEndAddr = EObj.GetObjBinAddr<CHAR8>(); gAdjustOpcodeOffset = EObj.GetObjBinOffset ();}
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
  FormId "=" S1:Number ","                          << _PCATCH(FMapObj->SetFormId (_STOFID(S1->getText(), S1->getLine())), S1); >>
  (
    MapTitle "=" "STRING_TOKEN" "\(" S2:Number "\)" ";"
    MapGuid  "=" guidDefinition[Guid] ";"           << FMapObj->SetFormMapMethod (_STOFID(S2->getText(), S2->getLine()), &Guid); FormMapMethodNumber ++; >>
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
    vfrStatementModal                        |
    vfrStatementRefreshEvent ";"
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
     UINT64                ValueList[EFI_IFR_MAX_LENGTH] = {0,};
     EFI_IFR_TYPE_VALUE    *Val           = (EFI_IFR_TYPE_VALUE *) ValueList;
     CIfrDefault           *DObj         = NULL;
     CIfrDefault2          *DObj2        = NULL;
     EFI_DEFAULT_ID        DefaultId     = EFI_HII_DEFAULT_CLASS_STANDARD;
     CHAR8                 *VarStoreName = NULL;
     EFI_VFR_VARSTORE_TYPE VarStoreType  = EFI_VFR_VARSTORE_INVALID;
     UINT32                Size          = 0;
     EFI_GUID              *VarGuid      = NULL;
     BOOLEAN               ArrayType     = FALSE;
     UINT8                 *Type8        = (UINT8  *) ValueList;
     UINT16                *Type16       = (UINT16 *) ValueList;
     UINT32                *Type32       = (UINT32 *) ValueList;
     UINT64                *Type64       = (UINT64 *) ValueList;
     CIfrNumeric           *NumericQst   = NULL;

  >>
  D:Default                                         
  (
    (
      "=" vfrConstantValueField[_GET_CURRQEST_DATATYPE(), *Val, ArrayType] ","  
                                                    << 
                                                        if (gCurrentMinMaxData != NULL && gCurrentMinMaxData->IsNumericOpcode()) {
                                                          //check default value is valid for Numeric Opcode
                                                          NumericQst = (CIfrNumeric *) gCurrentQuestion;
                                                          if ((NumericQst->GetNumericFlags() & EFI_IFR_DISPLAY) == 0 && !(_GET_CURRQEST_VARTINFO().mIsBitVar)) {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_8:
                                                              if (((INT8) Val->u8 < (INT8) gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), FALSE)) ||
                                                                  ((INT8) Val->u8 > (INT8) gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), FALSE))) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16:
                                                              if (((INT16) Val->u16 < (INT16) gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), FALSE)) ||
                                                                  ((INT16) Val->u16 > (INT16) gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), FALSE))) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32:
                                                              if (((INT32) Val->u32 < (INT32) gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), FALSE)) ||
                                                                  ((INT32) Val->u32 > (INT32) gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), FALSE))) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_64:
                                                              if (((INT64) Val->u64 < (INT64) gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), FALSE)) ||
                                                                  ((INT64) Val->u64 > (INT64) gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), FALSE))) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                              break;
                                                            default:
                                                              break;
                                                            }
                                                          } else {
                                                            //
                                                            // Value for question stored in bit fields is always set to UINT32 type.
                                                            //
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                              if (Val->u32 < gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), TRUE) || Val->u32 > gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), TRUE)) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                            } else {
                                                              if (Val->u64 < gCurrentMinMaxData->GetMinData(_GET_CURRQEST_DATATYPE(), FALSE) || Val->u64 > gCurrentMinMaxData->GetMaxData(_GET_CURRQEST_DATATYPE(), FALSE)) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, D->getLine(), "Numeric default value must be between MinValue and MaxValue.");
                                                              }
                                                            }
                                                          }
                                                        }
                                                        if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                          _PCATCH (VFR_RETURN_FATAL_ERROR, D->getLine(), "Default data type error.");
                                                          Size = sizeof (EFI_IFR_TYPE_VALUE);
                                                        } else if (ArrayType) {
                                                          switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                              while (Type8[Size] != 0) {
                                                                Size++;
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                              while (Type16[Size] != 0) {
                                                                Size++;
                                                              }
                                                              Size *= sizeof (UINT16);
                                                              break;

                                                            case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                              while (Type32[Size] != 0) {
                                                                Size++;
                                                              }
                                                              Size *= sizeof (UINT32);
                                                              break;

                                                            case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                              while (Type64[Size] != 0) {
                                                                Size++;
                                                              }
                                                              Size *= sizeof (UINT64);
                                                              break;

                                                            default:
                                                              break;
                                                          }
                                                        } else {
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            Size = sizeof (UINT32);
                                                          } else {
                                                            _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &Size), D->getLine());
                                                          }
                                                        }
                                                        Size += OFFSET_OF (EFI_IFR_DEFAULT, Value);
                                                        DObj = new CIfrDefault ((UINT8)Size);
                                                        DObj->SetLineNo(D->getLine());
                                                        if (ArrayType) {
                                                          DObj->SetType (EFI_IFR_TYPE_BUFFER);
                                                        } else if (gIsStringOp) {
                                                          DObj->SetType (EFI_IFR_TYPE_STRING);
                                                        } else {
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            DObj->SetType (EFI_IFR_TYPE_NUM_SIZE_32);
                                                          } else {
                                                            DObj->SetType (_GET_CURRQEST_DATATYPE());
                                                          }
                                                        }
                                                        DObj->SetValue(*Val);
                                                    >>
      |                                             << IsExp = TRUE; DObj2 = new CIfrDefault2; DObj2->SetLineNo(D->getLine()); DObj2->SetScope (1); >>
        vfrStatementValue ","                       << CIfrEnd EndObj1; EndObj1.SetLineNo(D->getLine()); >>
    )
    {
      DefaultStore "=" SN:StringIdentifier ","      << 
                                                        _PCATCH(gCVfrDefaultStore.GetDefaultId (SN->getText(), &DefaultId), SN);
                                                        if (DObj != NULL) {
                                                          DObj->SetDefaultId (DefaultId); 
                                                        } 

                                                        if (DObj2 != NULL) {
                                                          DObj2->SetDefaultId (DefaultId); 
                                                        }
                                                    >>
    }
                                                    <<
                                                      CheckDuplicateDefaultValue (DefaultId, D);
                                                      if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                       _PCATCH(gCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), D->getLine());
                                                       VarGuid = gCVfrDataStorage.GetVarStoreGuid(_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                       VarStoreType = gCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                       if ((IsExp == FALSE) && (VarStoreType == EFI_VFR_VARSTORE_BUFFER)) {
                                                         _PCATCH(gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                   DefaultId,
                                                                   _GET_CURRQEST_VARTINFO(),
                                                                   VarStoreName,
                                                                   VarGuid,
                                                                   _GET_CURRQEST_DATATYPE (),
                                                                   *Val),
                                                                   D->getLine()
                                                                   );
                                                         }
                                                       }
                                                       if (DObj  != NULL) {delete DObj;} 
                                                       if (DObj2 != NULL) {delete DObj2;} 
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
  vfrStatementSuppressIfStatNew
  ;

vfrStatementGrayOutIfStat :
  vfrStatementGrayOutIfStatNew
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
  Number 
  | InteractiveFlag 
  | ManufacturingFlag 
  | DefaultFlag 
  | ResetRequiredFlag 
  | ReconnectRequiredFlag
  | N:NVAccessFlag                                     <<
                                                          gCVfrErrorHandle.HandleWarning (
                                                            VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                            N->getLine(),
                                                            N->getText()
                                                            );
                                                       >>
  | L:LateCheckFlag                                    <<
                                                          gCVfrErrorHandle.HandleWarning (
                                                            VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                            L->getLine(),
                                                            L->getText()
                                                            );
                                                       >> 
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
  Text "=" "STRING_TOKEN" "\(" S:Number "\)"           << SObj.SetPrompt (_STOSID(S->getText(), S->getLine())); >>
  {
    "," FLAGS "=" vfrSubtitleFlags[SObj]
  }
  (
    {vfrStatementStatTagList "," }
    E:";"                                               << CRT_END_OP (E); >>
  |
    { "," vfrStatementStatTagList}
    { "," (vfrStatementStat | vfrStatementQuestions)*}
    D: EndSubtitle ";"                                  << CRT_END_OP (D); >>
  )
  ;

vfrSubtitleFlags [CIfrSubtitle & SObj] :
  << UINT8 LFlags = 0; >>
  subtitleFlagsField[LFlags] ( "\|" subtitleFlagsField[LFlags] )*
                                                       << _PCATCH(SObj.SetFlags (LFlags)); >>
  ;

subtitleFlagsField [UINT8 & Flags] :
    N:Number                                           << $Flags |= _STOU8(N->getText(), N->getLine()); >>
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
    "," Text "=" "STRING_TOKEN" "\(" S3:Number "\)"    << TxtTwo = _STOSID(S3->getText(), S3->getLine()); >>
  }
  {
    "," F:FLAGS "=" staticTextFlagsField[Flags] ( "\|" staticTextFlagsField[Flags] )*
    "," Key "=" KN:Number
  }
                                                       <<
                                                          if (Flags & EFI_IFR_FLAG_CALLBACK) {
                                                            if (TxtTwo != EFI_STRING_ID_INVALID) {
                                                              gCVfrErrorHandle.HandleWarning (
                                                                                VFR_WARNING_ACTION_WITH_TEXT_TWO,
                                                                                S3->getLine(),
                                                                                S3->getText()
                                                                                );
                                                            }
                                                            CIfrAction AObj;
                                                            mCVfrQuestionDB.RegisterQuestion (NULL, NULL, QId);
                                                            AObj.SetLineNo (F->getLine());
                                                            AObj.SetQuestionId (QId);
                                                            AObj.SetPrompt (_STOSID(S2->getText(), S2->getLine()));
                                                            AObj.SetHelp (_STOSID(S1->getText(), S1->getLine()));
                                                            _PCATCH(AObj.SetFlags (Flags), F->getLine());
                                                            AssignQuestionKey (AObj, KN);
                                                            CRT_END_OP (KN);
                                                          } else {
                                                            CIfrText TObj;
                                                            TObj.SetLineNo (T->getLine());
                                                            TObj.SetHelp (_STOSID(S1->getText(), S1->getLine()));
                                                            TObj.SetPrompt (_STOSID(S2->getText(), S2->getLine()));
                                                            TObj.SetTextTwo (TxtTwo);
                                                          }
                                                       >>
  { "," vfrStatementStatTagList }
  ";"
  ;

staticTextFlagsField[UINT8 & HFlags] :
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
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
                                                          DevPath = _STOSID(P->getText(), P->getLine());
                                                          FId = _STOFID(F1->getText(), F1->getLine());
                                                          QId = _STOQID(QN1->getText(), QN1->getLine());
                                                       >>
    )
    |
    (
      FormSetGuid "=" guidDefinition[FSId] ","
      FormId "=" F2:Number ","
      Question "=" QN2:Number ","
                                                       <<
                                                          RefType = 3;
                                                          FId = _STOFID(F2->getText(), F2->getLine());
                                                          QId = _STOQID(QN2->getText(), QN2->getLine());
                                                       >>
    )
    |
    (
      FormId "=" F3:Number ","                         << RefType = 2; FId = _STOFID(F3->getText(), F3->getLine()); >>
      Question "="
      (
          QN3:StringIdentifier ","                     << 
                                                          mCVfrQuestionDB.GetQuestionId (QN3->getText (), NULL, QId, BitMask);
                                                          if (QId == EFI_QUESTION_ID_INVALID) {
                                                            _PCATCH(VFR_RETURN_UNDEFINED, QN3);
                                                          }
                                                       >>
        | QN4:Number ","                               << QId = _STOQID(QN4->getText(), QN4->getLine()); >>
      )
    )
    |
    (
      F4:Number ","                                    <<
                                                          RefType = 1;
                                                          FId = _STOFID(F4->getText(), F4->getLine());
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
  vfrQuestionHeader[*QHObj, QUESTION_REF]              <<
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            _GET_CURRQEST_VARTINFO().mVarType = EFI_IFR_TYPE_REF;
                                                          }
                                                       >>
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
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
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
                                                          _PCATCH(gCVfrDefaultStore.GetDefaultId (N->getText(), &DefaultId), N->getLine());
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
     CIfrCheckBox       *CBObj = NULL;
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
     CHAR8              *VarStoreName = NULL;
     UINT32             DataTypeSize;
     EFI_GUID           *VarStoreGuid = NULL;
     CIfrGuid           *GuidObj = NULL;
     EFI_QUESTION_ID   QId = EFI_QUESTION_ID_INVALID;;
     EFI_VARSTORE_INFO Info;
     Info.mVarType          = EFI_IFR_TYPE_OTHER;
     Info.mVarTotalSize     = 0;
     Info.mInfo.mVarOffset  = EFI_VAROFFSET_INVALID;
     Info.mVarStoreId       = EFI_VARSTORE_ID_INVALID;
     Info.mIsBitVar         = FALSE;
  >>
  L:CheckBox
  vfrQuestionBaseInfo[Info, QId]                       <<
                                                         //
                                                         // Create a GUID opcode to wrap the checkbox opcode, if it refer to bit varstore.
                                                         //
                                                         if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                           GuidObj = new CIfrGuid(0);
                                                           GuidObj->SetGuid (&gEdkiiIfrBitVarGuid);
                                                           GuidObj->SetLineNo(L->getLine());
                                                         }
                                                         CBObj = new CIfrCheckBox;
                                                         CBObj->SetLineNo(L->getLine());
                                                         CBObj->SetQuestionId (QId);
                                                         CBObj->SetVarStoreInfo (&Info);
                                                        >>
  vfrStatementHeader[CBObj]","                           << //check data type
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            _GET_CURRQEST_VARTINFO().mVarType = EFI_IFR_TYPE_BOOLEAN;
                                                          }
                                                          if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            //
                                                            // Check whether the question refers to a bit field, if yes. create a Guid to indicate the question refers to a bit field.
                                                            //
                                                            if (_GET_CURRQEST_VARTINFO ().mIsBitVar) {
                                                              _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "CheckBox varid is not the valid data type");
                                                              if ((gCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId) == EFI_VFR_VARSTORE_BUFFER_BITS) &&
                                                                  (_GET_CURRQEST_VARSIZE() != 1)) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "CheckBox varid only occupy 1 bit in Bit Varstore");
                                                              }
                                                            } else {
                                                              _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "CheckBox varid is not the valid data type");
                                                              if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "CheckBox varid doesn't support array");
                                                              } else if ((gCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId) == EFI_VFR_VARSTORE_BUFFER) &&
                                                                        (_GET_CURRQEST_VARSIZE() != sizeof (BOOLEAN))) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "CheckBox varid only support BOOLEAN data type");
                                                              }
                                                            }
                                                          }
                                                       >>
  {
    F:FLAGS "=" vfrCheckBoxFlags[*CBObj, F->getLine()] ","
                                                       <<
                                                         if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            _PCATCH(gCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), VFR_RETURN_SUCCESS, L, "Failed to retrieve varstore name");
                                                            VarStoreGuid = gCVfrDataStorage.GetVarStoreGuid(_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                            Val.b = TRUE;
                                                            if (CBObj->GetFlags () & 0x01) {
                                                              CheckDuplicateDefaultValue (EFI_HII_DEFAULT_CLASS_STANDARD, F);
                                                              _PCATCH(
                                                                gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                                    EFI_HII_DEFAULT_CLASS_STANDARD,
                                                                                    _GET_CURRQEST_VARTINFO(),
                                                                                    VarStoreName,
                                                                                    VarStoreGuid,
                                                                                    _GET_CURRQEST_DATATYPE (),
                                                                                    Val
                                                                                    ),
                                                                VFR_RETURN_SUCCESS,
                                                                L,
                                                                "No standard default storage found"
                                                                );
                                                            }
                                                            if (CBObj->GetFlags () & 0x02) {
                                                              CheckDuplicateDefaultValue (EFI_HII_DEFAULT_CLASS_MANUFACTURING, F);
                                                              _PCATCH(
                                                                gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                                    EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                                                                                    _GET_CURRQEST_VARTINFO(),
                                                                                    VarStoreName,
                                                                                    VarStoreGuid,
                                                                                    _GET_CURRQEST_DATATYPE (),
                                                                                    Val
                                                                                    ),
                                                                VFR_RETURN_SUCCESS,
                                                                L,
                                                                "No manufacturing default storage found"
                                                                );
                                                            }
                                                          }
                                                        >>
  }
  {
    Key "=" KN:Number  ","                             << AssignQuestionKey (*CBObj, KN); >>
  }
  vfrStatementQuestionOptionList
  E:EndCheckBox                                        << CRT_END_OP (E);
                                                          if (GuidObj != NULL) {
                                                            GuidObj->SetScope(1);
                                                            CRT_END_OP (E);
                                                            delete GuidObj;
                                                          }
                                                          if (CBObj != NULL) delete CBObj;
                                                       >>
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
                                                          _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine());
                                                       >>
  | D:"DEFAULT"                                        <<
                                                          _PCATCH (VFR_RETURN_UNSUPPORTED, D);
                                                       >>
  | M:"MANUFACTURING"                                  <<
                                                          _PCATCH (VFR_RETURN_UNSUPPORTED, M);
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
  Config "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << AObj.SetQuestionConfig (_STOSID(S->getText(), S->getLine())); >>
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
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementDate :
  <<
     EFI_QUESTION_ID    QId          = EFI_QUESTION_ID_INVALID;
     CHAR8              *VarIdStr[3] = {NULL, };
     CIfrDate           DObj;
     EFI_IFR_TYPE_VALUE Val = gZeroEfiIfrTypeValue;
     UINT8              Size = OFFSET_OF (EFI_IFR_DEFAULT, Value) + sizeof (EFI_HII_DATE);
  >>
  L:Date                                               << DObj.SetLineNo(L->getLine()); >>
  (
    (
      vfrQuestionHeader[DObj, QUESTION_DATE] ","       <<
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            _GET_CURRQEST_VARTINFO().mVarType = EFI_IFR_TYPE_DATE;
                                                          }
                                                       >>
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
                                                          DObj.SetPrompt (_STOSID(YP->getText(), YP->getLine()));
                                                          DObj.SetHelp (_STOSID(YH->getText(), YH->getLine()));
                                                          if (VarIdStr[0] != NULL) { delete VarIdStr[0]; } if (VarIdStr[1] != NULL) { delete VarIdStr[1]; } if (VarIdStr[2] != NULL) { delete VarIdStr[2]; }
                                                       >>
                                                       << {CIfrDefault DefaultObj(Size, EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_DATE, Val); DefaultObj.SetLineNo(L->getLine());} >>
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
                                                            D.Year  = _STOU16(N->getText(), N->getLine());
                                                            if (D.Year < _STOU16 (MinN->getText(), MinN->getLine()) || D.Year > _STOU16 (MaxN->getText(), MaxN->getLine())) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Year default value must be between Min year and Max year.");
                                                            }
                                                            break;
                                                          case 1: 
                                                            D.Month = _STOU8(N->getText(), N->getLine()); 
                                                            if (D.Month < 1 || D.Month > 12) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Month default value must be between 1 and 12.");
                                                            }
                                                            break;
                                                          case 2: 
                                                            D.Day = _STOU8(N->getText(), N->getLine()); 
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
    N:Number                                           << $Flags |= _STOU8(N->getText(), N->getLine()); >>
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
     BOOLEAN IntDecStyle = FALSE;
     CIfrNumeric *NObj = (CIfrNumeric *) (&MMSDObj);
     if (((_GET_CURRQEST_VARTINFO().mIsBitVar) && (NObj->GetOpCode() == EFI_IFR_NUMERIC_OP) && ((NObj->GetNumericFlags() &  EDKII_IFR_DISPLAY_BIT) == 0)) ||
     (!(_GET_CURRQEST_VARTINFO().mIsBitVar) && (NObj->GetOpCode() == EFI_IFR_NUMERIC_OP) && ((NObj->GetNumericFlags() & EFI_IFR_DISPLAY) == 0))) {
       IntDecStyle = TRUE;
     }
     BOOLEAN MinNegative = FALSE;
     BOOLEAN MaxNegative = FALSE;
  >>
  Minimum   "=" 
  {
    "\-"                                               << MinNegative = TRUE; >>
  }
  I:Number ","                                         <<
                                                          if (!IntDecStyle &&  MinNegative) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "\"-\" can't be used when not in int decimal type. ");
                                                          }
                                                          //
                                                          // Value for question stored in bit fields is always set to UINT32 type.
                                                          //
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            MinU4 = _STOU32(I->getText(), I->getLine());
                                                            if (!IntDecStyle && MinU4 > (1<< _GET_CURRQEST_VARTINFO().mVarTotalSize) -1) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "BIT type minimum can't small than 0, bigger than 2^BitWidth -1");
                                                            }
                                                          } else {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                              MinU8 = _STOU64(I->getText(), I->getLine());
                                                              if (IntDecStyle) {
                                                                if (MinNegative) {
                                                                  if (MinU8 > 0x8000000000000000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF");
                                                                  }
                                                                } else {
                                                                  if (MinU8 > 0x7FFFFFFFFFFFFFFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT64 type minimum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MinNegative) {
                                                                MinU8 = ~MinU8 + 1;
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                              MinU4 = _STOU32(I->getText(), I->getLine());
                                                              if (IntDecStyle) {
                                                                if (MinNegative) {
                                                                  if (MinU4 > 0x80000000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF");
                                                                  }
                                                                } else {
                                                                  if (MinU4 > 0x7FFFFFFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT32 type minimum can't small than -0x80000000, big than 0x7FFFFFFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MinNegative) {
                                                                MinU4 = ~MinU4 + 1;
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                              MinU2 = _STOU16(I->getText(), I->getLine());
                                                              if (IntDecStyle) {
                                                                if (MinNegative) {
                                                                  if (MinU2 > 0x8000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT16 type minimum can't small than -0x8000, big than 0x7FFF");
                                                                  }
                                                                } else {
                                                                  if (MinU2 > 0x7FFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT16 type minimum can't small than -0x8000, big than 0x7FFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MinNegative) {
                                                                MinU2 = ~MinU2 + 1;
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                              MinU1 = _STOU8(I->getText(), I->getLine());
                                                              if (IntDecStyle) {
                                                                if (MinNegative) {
                                                                  if (MinU1 > 0x80) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT8 type minimum can't small than -0x80, big than 0x7F");
                                                                  }
                                                                } else {
                                                                  if (MinU1 > 0x7F) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, I->getLine(), "INT8 type minimum can't small than -0x80, big than 0x7F");
                                                                  }
                                                                }
                                                              }
                                                              if (MinNegative) {
                                                                MinU1 = ~MinU1 + 1;
                                                              }
                                                              break;
                                                            }
                                                          }
                                                       >>
  Maximum   "=" 
  { 
    "\-"                                               << MaxNegative = TRUE; >>
  }
  A:Number ","                                         <<
                                                          if (!IntDecStyle && MaxNegative) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "\"-\" can't be used when not in int decimal type. ");
                                                          }
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            MaxU4 = _STOU32(A->getText(), A->getLine());
                                                            if (!IntDecStyle && MaxU4 > (1<< _GET_CURRQEST_VARTINFO().mVarTotalSize) -1) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "BIT type maximum can't bigger than 2^BitWidth -1");
                                                            }
                                                          } else {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_64 :
                                                              MaxU8 = _STOU64(A->getText(), A->getLine());
                                                              if (IntDecStyle) {
                                                                if (MaxNegative) {
                                                                  if (MaxU8 > 0x8000000000000000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT64 type maximum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF");
                                                                  }
                                                                } else {
                                                                  if (MaxU8 > 0x7FFFFFFFFFFFFFFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT64 type maximum can't small than -0x8000000000000000, big than 0x7FFFFFFFFFFFFFFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MaxNegative) {
                                                                MaxU8 = ~MaxU8 + 1;
                                                              }
                                                              if (IntDecStyle) {
                                                                if ((INT64) MaxU8 < (INT64) MinU8) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              } else {
                                                                if (MaxU8 < MinU8) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32 :
                                                              MaxU4 = _STOU32(A->getText(), A->getLine());
                                                              if (IntDecStyle) {
                                                                if (MaxNegative) {
                                                                  if (MaxU4 > 0x80000000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT32 type maximum can't small than -0x80000000, big than 0x7FFFFFFF");
                                                                  }
                                                                } else {
                                                                  if (MaxU4 > 0x7FFFFFFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT32 type maximum can't small than -0x80000000, big than 0x7FFFFFFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MaxNegative) {
                                                                MaxU4 = ~MaxU4 + 1;
                                                              }
                                                              if (IntDecStyle) {
                                                                if ((INT32) MaxU4 < (INT32) MinU4) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              } else {
                                                                if (MaxU4 < MinU4) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16 :
                                                              MaxU2 = _STOU16(A->getText(), A->getLine());
                                                              if (IntDecStyle) {
                                                                if (MaxNegative) {
                                                                  if (MaxU2 > 0x8000) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT16 type maximum can't small than -0x8000, big than 0x7FFF");
                                                                  }
                                                                } else {
                                                                  if (MaxU2 > 0x7FFF) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT16 type maximum can't small than -0x8000, big than 0x7FFF");
                                                                  }
                                                                }
                                                              }
                                                              if (MaxNegative) {
                                                                MaxU2 = ~MaxU2 + 1;
                                                              }
                                                              if (IntDecStyle) {
                                                                if ((INT16) MaxU2 < (INT16) MinU2) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              } else {
                                                                if (MaxU2 < MinU2) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              }
                                                              break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_8 :
                                                              MaxU1 = _STOU8(A->getText(), A->getLine());
                                                              if (IntDecStyle) {
                                                                if (MaxNegative) {
                                                                  if (MaxU1 > 0x80) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT8 type maximum can't small than -0x80, big than 0x7F");
                                                                  }
                                                                } else {
                                                                  if (MaxU1 > 0x7F) {
                                                                    _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "INT8 type maximum can't small than -0x80, big than 0x7F");
                                                                  }
                                                                }
                                                              }
                                                              if (MaxNegative) {
                                                                MaxU1 = ~MaxU1 + 1;
                                                              }
                                                              if (IntDecStyle) {
                                                                if ((INT8) MaxU1 < (INT8) MinU1) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              } else {
                                                                if (MaxU1 < MinU1) {
                                                                  _PCATCH (VFR_RETURN_INVALID_PARAMETER, A->getLine(), "Maximum can't be less than Minimum");
                                                                }
                                                              }
                                                              break;
                                                            }
                                                          }
                                                       >>
  {
    STEP    "=" S:Number ","
                                                       <<
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            StepU4 = _STOU32(S->getText(), S->getLine());
                                                          } else {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_64 : StepU8 = _STOU64(S->getText(), S->getLine()); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32 : StepU4 = _STOU32(S->getText(), S->getLine()); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16 : StepU2 = _STOU16(S->getText(), S->getLine()); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_8 :  StepU1 = _STOU8(S->getText(), S->getLine());  break;
                                                            }
                                                          }
                                                       >>
  }
                                                       <<
                                                          if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $MMSDObj.SetMinMaxStepData (MinU4, MaxU4, StepU4);
                                                          } else {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                            case EFI_IFR_TYPE_NUM_SIZE_64 : $MMSDObj.SetMinMaxStepData (MinU8, MaxU8, StepU8); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_32 : $MMSDObj.SetMinMaxStepData (MinU4, MaxU4, StepU4); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_16 : $MMSDObj.SetMinMaxStepData (MinU2, MaxU2, StepU2); break;
                                                            case EFI_IFR_TYPE_NUM_SIZE_8 :  $MMSDObj.SetMinMaxStepData (MinU1, MaxU1, StepU1);  break;
                                                            }
                                                          }
                                                       >>
  ;

vfrStatementNumeric :
  <<
     CIfrNumeric *NObj = NULL;
     UINT32      DataTypeSize;
     BOOLEAN     IsSupported = TRUE;
     UINT8       ShrinkSize  = 0;
     CIfrGuid    *GuidObj = NULL;
     UINT8       LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     EFI_QUESTION_ID   QId = EFI_QUESTION_ID_INVALID;
     EFI_VARSTORE_INFO Info;
     Info.mVarType          = EFI_IFR_TYPE_OTHER;
     Info.mVarTotalSize     = 0;
     Info.mInfo.mVarOffset  = EFI_VAROFFSET_INVALID;
     Info.mVarStoreId       = EFI_VARSTORE_ID_INVALID;
     Info.mIsBitVar         = FALSE;
  >>
  L:Numeric
  vfrQuestionBaseInfo[Info, QId]                       <<
                                                         //
                                                         // Create a GUID opcode to wrap the numeric opcode, if it refer to bit varstore.
                                                         //
                                                         if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                           GuidObj = new CIfrGuid(0);
                                                           GuidObj->SetGuid (&gEdkiiIfrBitVarGuid);
                                                           GuidObj->SetLineNo(L->getLine());
                                                         }
                                                         NObj = new CIfrNumeric;
                                                         NObj->SetLineNo(L->getLine());
                                                         NObj->SetQuestionId (QId);
                                                         NObj->SetVarStoreInfo (&Info);
                                                        >>
  vfrStatementHeader[NObj]","
                                                         <<
                                                          // check data type
                                                          if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                               LFlags = (EDKII_IFR_NUMERIC_SIZE_BIT & (_GET_CURRQEST_VARSIZE()));
                                                              _PCATCH(NObj->SetFlagsForBitField (NObj->FLAGS(), LFlags), L->getLine());
                                                            } else {
                                                              _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "Numeric varid is not the valid data type");
                                                              if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Numeric varid doesn't support array");
                                                              }
                                                              _PCATCH(NObj->SetFlags (NObj->FLAGS(), _GET_CURRQEST_DATATYPE()), L->getLine());
                                                            }
                                                          }
                                                       >>
  { F:FLAGS "=" vfrNumericFlags[*NObj, F->getLine()] "," }
  {
    Key   "=" KN:Number ","                            << AssignQuestionKey (*NObj, KN); >>
  }
  vfrSetMinMaxStep[*NObj]                               <<
                                                          if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                              //
                                                              // Base on the type to know the actual used size,shrink the buffer
                                                              // size allocate before.
                                                              //
                                                              case EFI_IFR_TYPE_NUM_SIZE_8: ShrinkSize = 21;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_16:ShrinkSize = 18;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_32:ShrinkSize = 12;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_64:break;
                                                              default:
                                                                IsSupported = FALSE;
                                                                break;
                                                            }
                                                          } else {
                                                            //
                                                            // Question stored in bit fields saved as UINT32 type, so the ShrinkSize same as EFI_IFR_TYPE_NUM_SIZE_32.
                                                            //
                                                            ShrinkSize = 12;
                                                          }
                                                          NObj->ShrinkBinSize (ShrinkSize);

                                                          if (!IsSupported) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "Numeric question only support UINT8, UINT16, UINT32 and UINT64 data type.");
                                                          }
                                                       >>
  vfrStatementQuestionOptionList
  E:EndNumeric                                         << 
                                                          CRT_END_OP (E);
                                                          if (GuidObj != NULL) {
                                                            GuidObj->SetScope(1);
                                                            CRT_END_OP (E);
                                                            delete GuidObj;
                                                          }
                                                          if (NObj != NULL) delete NObj;
                                                       >>
  ";"
  ;

vfrNumericFlags [CIfrNumeric & NObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     UINT8 HFlags = 0;
     BOOLEAN IsSetType = FALSE;
     BOOLEAN IsDisplaySpecified = FALSE;
     EFI_VFR_VARSTORE_TYPE VarStoreType = gCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId);
  >>
  numericFlagsField[HFlags, LFlags, IsSetType, IsDisplaySpecified, LineNum] ( "\|" numericFlagsField[HFlags, LFlags, IsSetType, IsDisplaySpecified, LineNum] )*
                                                       <<
                                                          //check data type flag
                                                          if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
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
                                                            } else if (IsSetType){
                                                              _GET_CURRQEST_VARTINFO().mVarType = LFlags & EFI_IFR_NUMERIC_SIZE;
                                                            }
                                                            _PCATCH(NObj.SetFlags (HFlags, LFlags, IsDisplaySpecified), LineNum);
                                                          } else if ((_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) && (_GET_CURRQEST_VARTINFO().mIsBitVar)) {
                                                            LFlags &= EDKII_IFR_DISPLAY_BIT;
                                                            LFlags |= (EDKII_IFR_NUMERIC_SIZE_BIT & (_GET_CURRQEST_VARSIZE()));
                                                            _PCATCH(NObj.SetFlagsForBitField (HFlags, LFlags, IsDisplaySpecified), LineNum);
                                                          }
                                                       >>
  ;

numericFlagsField [UINT8 & HFlags, UINT8 & LFlags, BOOLEAN & IsSetType, BOOLEAN & IsDisplaySpecified, UINT32 LineNum] :
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | "NUMERIC_SIZE_1"                                   << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_1;IsSetType = TRUE;
                                                          } else {
                                                            _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Can not specify the size of the numeric value for BIT field");
                                                          }
                                                       >>
  | "NUMERIC_SIZE_2"                                   << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_2;IsSetType = TRUE;
                                                          } else {
                                                            _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Can not specify the size of the numeric value for BIT field");
                                                          }
                                                       >>
  | "NUMERIC_SIZE_4"                                   << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_4; IsSetType = TRUE;
                                                          } else {
                                                            _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Can not specify the size of the numeric value for BIT field");
                                                          }
                                                       >>
  | "NUMERIC_SIZE_8"                                   << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_NUMERIC_SIZE) | EFI_IFR_NUMERIC_SIZE_8; IsSetType = TRUE;
                                                          } else {
                                                            _PCATCH(VFR_RETURN_INVALID_PARAMETER, LineNum, "Can not specify the size of the numeric value for BIT field");
                                                          }
                                                       >>
  | "DISPLAY_INT_DEC"                                  << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_INT_DEC;
                                                          } else {
                                                            $LFlags = ($LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_INT_DEC_BIT;
                                                          }
                                                          IsDisplaySpecified = TRUE;
                                                       >>
  | "DISPLAY_UINT_DEC"                                 << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_DEC;
                                                          } else {
                                                            $LFlags = ($LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_DEC_BIT;
                                                          }
                                                          IsDisplaySpecified = TRUE;
                                                       >>
  | "DISPLAY_UINT_HEX"                                 << if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            $LFlags = ($LFlags & ~EFI_IFR_DISPLAY) | EFI_IFR_DISPLAY_UINT_HEX;
                                                          } else {
                                                            $LFlags = ($LFlags & ~EDKII_IFR_DISPLAY_BIT) | EDKII_IFR_DISPLAY_UINT_HEX_BIT;
                                                          }
                                                          IsDisplaySpecified = TRUE;
                                                       >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementOneOf :
  <<
     CIfrOneOf *OObj = NULL;
     UINT32    DataTypeSize;
     BOOLEAN   IsSupported = TRUE;
     UINT8     ShrinkSize  = 0;
     CIfrGuid  *GuidObj = NULL;
     UINT8 LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     EFI_QUESTION_ID   QId = EFI_QUESTION_ID_INVALID;;
     EFI_VARSTORE_INFO Info;
     Info.mVarType               = EFI_IFR_TYPE_OTHER;
     Info.mVarTotalSize          = 0;
     Info.mInfo.mVarOffset       = EFI_VAROFFSET_INVALID;
     Info.mVarStoreId            = EFI_VARSTORE_ID_INVALID;
     Info.mIsBitVar              = FALSE;
  >>
  L:OneOf
  vfrQuestionBaseInfo[Info, QId]                       <<
                                                         //
                                                         // Create a GUID opcode to wrap the oneof opcode, if it refer to bit varstore.
                                                         //
                                                         if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                           GuidObj = new CIfrGuid(0);
                                                           GuidObj->SetGuid (&gEdkiiIfrBitVarGuid);
                                                           GuidObj->SetLineNo(L->getLine());
                                                         }
                                                         OObj = new CIfrOneOf;
                                                         OObj->SetLineNo(L->getLine());
                                                         OObj->SetQuestionId (QId);
                                                         OObj->SetVarStoreInfo (&Info);
                                                        >>
  vfrStatementHeader[OObj]","
                                                        << //check data type
                                                          if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                               LFlags = (EDKII_IFR_NUMERIC_SIZE_BIT & (_GET_CURRQEST_VARSIZE()));
                                                              _PCATCH(OObj->SetFlagsForBitField (OObj->FLAGS(), LFlags), L->getLine());
                                                            } else {
                                                              _PCATCH (gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &DataTypeSize), L->getLine(), "OneOf varid is not the valid data type");
                                                              if (DataTypeSize != 0 && DataTypeSize != _GET_CURRQEST_VARSIZE()) {
                                                                _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "OneOf varid doesn't support array");
                                                              }
                                                              _PCATCH(OObj->SetFlags (OObj->FLAGS(), _GET_CURRQEST_DATATYPE()), L->getLine());
                                                            }
                                                          }
                                                       >>
  { F:FLAGS "=" vfrOneofFlagsField[*OObj, F->getLine()] "," }
  {
    vfrSetMinMaxStep[*OObj]
  }
                                                       <<
                                                          if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
                                                              //
                                                              // Base on the type to know the actual used size,shrink the buffer
                                                              // size allocate before.
                                                              //
                                                              case EFI_IFR_TYPE_NUM_SIZE_8: ShrinkSize = 21;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_16:ShrinkSize = 18;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_32:ShrinkSize = 12;break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_64:break;
                                                              default:
                                                                IsSupported = FALSE;
                                                                break;
                                                            }
                                                          } else {
                                                            //
                                                            // Question stored in bit fields saved as UINT32 type, so the ShrinkSize same as EFI_IFR_TYPE_NUM_SIZE_32.
                                                            //
                                                            ShrinkSize = 12;
                                                          }
                                                          OObj->ShrinkBinSize (ShrinkSize);

                                                          if (!IsSupported) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, L->getLine(), "OneOf question only support UINT8, UINT16, UINT32 and UINT64 data type.");
                                                          }
                                                       >>
  vfrStatementQuestionOptionList
  E:EndOneOf                                           <<
                                                          CRT_END_OP (E);
                                                          if (GuidObj != NULL) {
                                                            GuidObj->SetScope(1);
                                                            CRT_END_OP (E);
                                                            delete GuidObj;
                                                          }
                                                          if (OObj != NULL) delete OObj;
                                                       >>
  ";"
  ;

vfrOneofFlagsField [CIfrOneOf & OObj, UINT32 LineNum] :
  <<
     UINT8 LFlags = _GET_CURRQEST_DATATYPE() & EFI_IFR_NUMERIC_SIZE;
     UINT8 HFlags = 0;
     BOOLEAN IsSetType = FALSE;
     BOOLEAN IsDisplaySpecified = FALSE;
     EFI_VFR_VARSTORE_TYPE VarStoreType = gCVfrDataStorage.GetVarStoreType (_GET_CURRQEST_VARTINFO().mVarStoreId);
  >>
  numericFlagsField[HFlags, LFlags, IsSetType, IsDisplaySpecified, LineNum] ( "\|" numericFlagsField[HFlags, LFlags, IsSetType, IsDisplaySpecified, LineNum] )*
                                                       <<
                                                          //check data type flag
                                                          if (!_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                            if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
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
                                                            } else if (IsSetType){
                                                              _GET_CURRQEST_VARTINFO().mVarType = LFlags & EFI_IFR_NUMERIC_SIZE;
                                                            }
                                                            _PCATCH(OObj.SetFlags (HFlags, LFlags), LineNum);
                                                          } else if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            LFlags &= EDKII_IFR_DISPLAY_BIT;
                                                            LFlags |= (EDKII_IFR_NUMERIC_SIZE_BIT & (_GET_CURRQEST_VARSIZE()));
                                                            _PCATCH(OObj.SetFlagsForBitField (HFlags, LFlags), LineNum);
                                                          }
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
  L:String                                             << SObj.SetLineNo(L->getLine()); gIsStringOp = TRUE;>>
  vfrQuestionHeader[SObj] ","                          << _GET_CURRQEST_VARTINFO().mVarType = EFI_IFR_TYPE_STRING;>>
  { F:FLAGS "=" vfrStringFlagsField[SObj, F->getLine()] "," }
  {
    Key "=" KN:Number ","                              << AssignQuestionKey (SObj, KN); >>
  }
  MinSize "=" MIN:Number ","                           << 
                                                          VarArraySize = _GET_CURRQEST_ARRAY_SIZE();
                                                          StringMinSize = _STOU8(MIN->getText(), MIN->getLine());
                                                          if (_STOU64(MIN->getText(), MIN->getLine()) > StringMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "String MinSize takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && StringMinSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "String MinSize can't be larger than the max number of elements in string array.");
                                                          }
                                                          SObj.SetMinSize (StringMinSize);
                                                       >>
  MaxSize "=" MAX:Number ","                           << 
                                                          StringMaxSize = _STOU8(MAX->getText(), MAX->getLine());
                                                          if (_STOU64(MAX->getText(), MAX->getLine()) > StringMaxSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && StringMaxSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize can't be larger than the max number of elements in string array.");
                                                          } else if (StringMaxSize < StringMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MAX->getLine(), "String MaxSize can't be less than String MinSize.");
                                                          }
                                                          SObj.SetMaxSize (StringMaxSize);
                                                       >>
  vfrStatementQuestionOptionList
  E:EndString                                          << CRT_END_OP (E); gIsStringOp = FALSE;>>
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
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
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
                                                          PasswordMinSize = _STOU16(MIN->getText(), MIN->getLine());
                                                          if (_STOU64(MIN->getText(), MIN->getLine()) > PasswordMinSize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "Password MinSize takes only two byte, which can't be larger than 0xFFFF.");
                                                          } else if (VarArraySize != 0 && PasswordMinSize > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, MIN->getLine(), "Password MinSize can't be larger than the max number of elements in password array.");
                                                          }
                                                          PObj.SetMinSize (PasswordMinSize);
                                                       >>
  MaxSize "=" MAX:Number ","                           << 
                                                          PasswordMaxSize = _STOU16(MAX->getText(), MAX->getLine());
                                                          if (_STOU64(MAX->getText(), MAX->getLine()) > PasswordMaxSize) {
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
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
  | questionheaderFlagsField[HFlags]
  ;

vfrStatementOrderedList :
  <<
     CIfrOrderedList OLObj;
     UINT32 VarArraySize;
  >>
  L:OrderedList                                        << OLObj.SetLineNo(L->getLine()); gIsOrderedList = TRUE;>>
  vfrQuestionHeader[OLObj] ","
                                                       << 
                                                          VarArraySize = _GET_CURRQEST_ARRAY_SIZE();
                                                          OLObj.SetMaxContainers ((UINT8) (VarArraySize > 0xFF ? 0xFF : VarArraySize));
                                                       >>
  {
    MaxContainers "=" M:Number ","                     << 
                                                          if (_STOU64(M->getText(), M->getLine()) > _STOU8(M->getText(), M->getLine())) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, M->getLine(), "OrderedList MaxContainers takes only one byte, which can't be larger than 0xFF.");
                                                          } else if (VarArraySize != 0 && _STOU8(M->getText(), M->getLine()) > VarArraySize) {
                                                            _PCATCH (VFR_RETURN_INVALID_PARAMETER, M->getLine(), "OrderedList MaxContainers can't be larger than the max number of elements in array.");
                                                          }
                                                          OLObj.SetMaxContainers (_STOU8(M->getText(), M->getLine()));
                                                       >>
  }
  { F:FLAGS "=" vfrOrderedListFlags[OLObj, F->getLine()] {","}}
  vfrStatementQuestionOptionList
  E:EndList                                            << CRT_END_OP (E); gIsOrderedList = FALSE;>>
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
    N:Number                                           << _PCATCH(_STOU8(N->getText(), N->getLine()) == 0 ? VFR_RETURN_SUCCESS : VFR_RETURN_UNSUPPORTED, N->getLine()); >>
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
     UINT8              Size = OFFSET_OF (EFI_IFR_DEFAULT, Value) + sizeof (EFI_HII_TIME);
  >>
  L:Time                                               << TObj.SetLineNo(L->getLine()); >>
  (
    (
      vfrQuestionHeader[TObj, QUESTION_TIME] ","       <<
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            _GET_CURRQEST_VARTINFO().mVarType = EFI_IFR_TYPE_TIME;
                                                          }
                                                       >>
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
                                                          TObj.SetPrompt (_STOSID(HP->getText(), HP->getLine()));
                                                          TObj.SetHelp (_STOSID(HH->getText(), HH->getLine()));
                                                          if (VarIdStr[0] != NULL) { delete VarIdStr[0]; } if (VarIdStr[1] != NULL) { delete VarIdStr[1]; } if (VarIdStr[2] != NULL) { delete VarIdStr[2]; }
                                                       >>
                                                       << {CIfrDefault DefaultObj(Size, EFI_HII_DEFAULT_CLASS_STANDARD, EFI_IFR_TYPE_TIME, Val); DefaultObj.SetLineNo(L->getLine());} >>
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
                                                            T.Hour   = _STOU8(N->getText(), N->getLine()); 
                                                            if (T.Hour > 23) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Hour default value must be between 0 and 23.");
                                                            }
                                                            break;
                                                          case 1: 
                                                            T.Minute = _STOU8(N->getText(), N->getLine()); 
                                                            if (T.Minute > 59) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, N->getLine(), "Minute default value must be between 0 and 59.");
                                                            }
                                                            break;
                                                          case 2: 
                                                            T.Second = _STOU8(N->getText(), N->getLine());
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
    N:Number                                           << $Flags |= _STOU8(N->getText(), N->getLine()); >>
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
  vfrStatementRefreshEvent ","  |
  vfrStatementWarningIf
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
                                                          _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                          IIObj.SetLineNo(L->getLine());
                                                       >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << IIObj.SetError (_STOSID(S->getText(), S->getLine())); >>
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

vfrImageTag :
  << CIfrImage IObj; >>
  L:Image "=" "IMAGE_TOKEN" "\(" S1:Number "\)"        << IObj.SetImageId (_STOSID(S1->getText(), S1->getLine())); IObj.SetLineNo(L->getLine()); >>
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
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << IIObj.SetError (_STOSID(S->getText(), S->getLine())); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementNoSubmitIf :
  << CIfrNoSubmitIf NSIObj; >>
  L:NoSubmitIf                                         << NSIObj.SetLineNo(L->getLine()); >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << NSIObj.SetError (_STOSID(S->getText(), S->getLine())); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0]
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementWarningIf :
  << CIfrWarningIf WIObj; >>
  L:WarningIf                                          << WIObj.SetLineNo(L->getLine()); >>
  Prompt "=" "STRING_TOKEN" "\(" S:Number "\)" ","     << WIObj.SetWarning (_STOSID(S->getText(), S->getLine())); >>
  {Timeout "=" T:Number ","                            << WIObj.SetTimeOut (_STOU8(T->getText(), T->getLine())); >>}
  vfrStatementExpression[0]
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementDisableIfQuest :
  << 
    CIfrDisableIf DIObj; 
  >>
  L:DisableIf                                          << DIObj.SetLineNo(L->getLine()); >>
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementRefresh :
  << CIfrRefresh RObj; >>
  L:Refresh                                            << RObj.SetLineNo(L->getLine()); >>
  Interval "=" I:Number                                << RObj.SetRefreshInterval (_STOU8(I->getText(), I->getLine())); >>
  ;

vfrStatementRefreshEvent :
  <<
    CIfrRefreshId RiObj;
    EFI_GUID      Guid;
  >>
  L:RefreshGuid                                        << RiObj.SetLineNo(L->getLine()); >>
  "="  guidDefinition[Guid]                            << RiObj.SetRefreshEventGroutId (&Guid);  >>
  ;

vfrStatementVarstoreDevice :
  << CIfrVarStoreDevice VDObj; >>
  L:VarstoreDevice                                     << VDObj.SetLineNo(L->getLine()); >>
  "=" "STRING_TOKEN" "\(" S:Number "\)" ","            << VDObj.SetDevicePath (_STOSID(S->getText(), S->getLine())); >>
  ;

vfrStatementSuppressIfQuest :
  << CIfrSuppressIf SIObj; >>
  L:SuppressIf                                         << SIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementGrayOutIfQuest :
  << CIfrGrayOutIf GOIObj; >>
  L:GrayOutIf                                          << GOIObj.SetLineNo(L->getLine()); >>
  { FLAGS "=" flagsField ( "\|" flagsField )* "," }
  vfrStatementExpression[0] ";"
  vfrStatementQuestionOptionList
  E:EndIf {";"}                                        << CRT_END_OP (E); >>
  ;

vfrStatementOptions :
  vfrStatementOneOfOption
  ;

vfrStatementOneOfOption :
  <<
     UINT8              ValueList[EFI_IFR_MAX_LENGTH] = {0,};
     EFI_IFR_TYPE_VALUE *Val          = (EFI_IFR_TYPE_VALUE *) ValueList;
     CHAR8              *VarStoreName = NULL;
     UINT32             Size          = 0;
     BOOLEAN            TypeError     = FALSE;
     EFI_VFR_RETURN_CODE ReturnCode   = VFR_RETURN_SUCCESS;
     EFI_GUID           *VarStoreGuid = NULL;
     BOOLEAN            ArrayType     = FALSE;
     CIfrOneOfOption    *OOOObj;
     UINT8              *Type8        = (UINT8  *) ValueList;
     UINT16             *Type16       = (UINT16 *) ValueList;
     UINT32             *Type32       = (UINT32 *) ValueList;
     UINT64             *Type64       = (UINT64 *) ValueList;
  >>
  L:Option                                             <<      
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            _PCATCH (VFR_RETURN_FATAL_ERROR, L->getLine(), "Get data type error.");
                                                          }

                                                       >>
  Text  "=" "STRING_TOKEN" "\(" S:Number "\)" ","      
  Value "=" vfrConstantValueField[_GET_CURRQEST_DATATYPE(), *Val, ArrayType] ","
                                                       << 
                                                          if (gCurrentMinMaxData != NULL) {
                                                            //set min/max value for oneof opcode
                                                            UINT64 Step = gCurrentMinMaxData->GetStepData(_GET_CURRQEST_DATATYPE(), _GET_CURRQEST_VARTINFO().mIsBitVar);
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                              gCurrentMinMaxData->SetMinMaxStepData(Val->u32, Val->u32, (UINT32) Step);
                                                            } else {
                                                              switch (_GET_CURRQEST_DATATYPE()) {
                                                              case EFI_IFR_TYPE_NUM_SIZE_64:
                                                                gCurrentMinMaxData->SetMinMaxStepData(Val->u64, Val->u64, Step);
                                                                break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_32:
                                                                gCurrentMinMaxData->SetMinMaxStepData(Val->u32, Val->u32, (UINT32) Step);
                                                                break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_16:
                                                                gCurrentMinMaxData->SetMinMaxStepData(Val->u16, Val->u16, (UINT16) Step);
                                                                break;
                                                              case EFI_IFR_TYPE_NUM_SIZE_8:
                                                                gCurrentMinMaxData->SetMinMaxStepData(Val->u8, Val->u8, (UINT8) Step);
                                                                break;
                                                              default:
                                                                break;
                                                              }
                                                            }
                                                          }
                                                          if (_GET_CURRQEST_DATATYPE() == EFI_IFR_TYPE_OTHER) {
                                                            Size = sizeof (EFI_IFR_TYPE_VALUE);
                                                          } else if (ArrayType) {
                                                            switch (_GET_CURRQEST_DATATYPE()) {
	                                                          case EFI_IFR_TYPE_NUM_SIZE_8 :
    	                                                        while (Type8[Size] != 0) {
    	                                                          Size++;
    	                                                        }
    	                                                        break;
    	                                                      case EFI_IFR_TYPE_NUM_SIZE_16 :
    	                                                        while (Type16[Size] != 0) {
    	                                                          Size++;
    	                                                        }
    	                                                        Size *= sizeof (UINT16);
    	                                                        break;
    	                                                      case EFI_IFR_TYPE_NUM_SIZE_32 :
    	                                                        while (Type32[Size] != 0) {
    	                                                          Size++;
    	                                                        }
    	                                                        Size *= sizeof (UINT32);
    	                                                        break;
    	                                                      case EFI_IFR_TYPE_NUM_SIZE_64 :
    	                                                        while (Type64[Size] != 0) {
    	                                                          Size++;
    	                                                        }
    	                                                        Size *= sizeof (UINT64);
    	                                                        break;
    	                                                      default:
    	                                                        break;
                                                            }
                                                          } else {
                                                            //
                                                            // For the oneof stored in bit fields, set the option type as UINT32.
                                                            //
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                              Size = sizeof (UINT32);
                                                            } else {
                                                              ReturnCode = gCVfrVarDataTypeDB.GetDataTypeSize (_GET_CURRQEST_DATATYPE(), &Size);
                                                            }
                                                          }
                                                          if (ReturnCode != VFR_RETURN_SUCCESS) {
                                                            _PCATCH (ReturnCode, L->getLine());
                                                          }

                                                          Size += OFFSET_OF (EFI_IFR_ONE_OF_OPTION, Value);
                                                          OOOObj = new CIfrOneOfOption((UINT8)Size);
                                                          OOOObj->SetLineNo(L->getLine());
                                                          OOOObj->SetOption (_STOSID(S->getText(), S->getLine())); 
                                                          if (ArrayType) {
                                                            OOOObj->SetType (EFI_IFR_TYPE_BUFFER);
                                                          } else {
                                                            if (_GET_CURRQEST_VARTINFO().mIsBitVar) {
                                                              OOOObj->SetType ( EFI_IFR_TYPE_NUM_SIZE_32);
                                                            } else {
                                                              OOOObj->SetType (_GET_CURRQEST_DATATYPE());
                                                            }
                                                          }
                                                          OOOObj->SetValue (*Val); 
                                                       >>
  F:FLAGS "=" vfrOneOfOptionFlags[*OOOObj, F->getLine()]
                                                       <<
                                                          //
                                                          // Array type only for default type OneOfOption.
                                                          //
                                                          if ((OOOObj->GetFlags () & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) == 0 && ArrayType) {
                                                            _PCATCH (VFR_RETURN_FATAL_ERROR, L->getLine(), "Default keyword should with array value type!");
                                                          }

                                                          //
                                                          // Clear the default flag if the option not use array value but has default flag.
                                                          //
                                                          if ((OOOObj->GetFlags () & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)) != 0 && !ArrayType && gIsOrderedList) {
                                                            OOOObj->SetFlags(OOOObj->GetFlags () & ~(EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG));
                                                          }

                                                          if (_GET_CURRQEST_VARTINFO().mVarStoreId != EFI_VARSTORE_ID_INVALID) {
                                                            _PCATCH(gCVfrDataStorage.GetVarStoreName (_GET_CURRQEST_VARTINFO().mVarStoreId, &VarStoreName), L->getLine());
                                                            VarStoreGuid = gCVfrDataStorage.GetVarStoreGuid(_GET_CURRQEST_VARTINFO().mVarStoreId);
                                                            if (OOOObj->GetFlags () & EFI_IFR_OPTION_DEFAULT) {
                                                              CheckDuplicateDefaultValue (EFI_HII_DEFAULT_CLASS_STANDARD, F);
                                                              _PCATCH(gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                        EFI_HII_DEFAULT_CLASS_STANDARD,
                                                                         _GET_CURRQEST_VARTINFO(),
                                                                        VarStoreName,
                                                                        VarStoreGuid,
                                                                        _GET_CURRQEST_DATATYPE (),
                                                                        *Val
                                                                        ), L->getLine());
                                                            }
                                                            if (OOOObj->GetFlags () & EFI_IFR_OPTION_DEFAULT_MFG) {
                                                              CheckDuplicateDefaultValue (EFI_HII_DEFAULT_CLASS_MANUFACTURING, F);
                                                              _PCATCH(gCVfrDefaultStore.BufferVarStoreAltConfigAdd (
                                                                        EFI_HII_DEFAULT_CLASS_MANUFACTURING,
                                                                         _GET_CURRQEST_VARTINFO(),
                                                                        VarStoreName,
                                                                        VarStoreGuid,
                                                                        _GET_CURRQEST_DATATYPE (),
                                                                        *Val
                                                                        ), L->getLine());
                                                            }
                                                          }
                                                       >>
  {
    "," Key "=" KN:Number                              <<
                                                         _PCATCH (VFR_RETURN_UNSUPPORTED, KN);
                                                         //
                                                         // Guid Option Key
                                                         //
                                                         CIfrOptionKey IfrOptionKey (
                                                                         gCurrentQuestion->QUESTION_ID(),
                                                                         *Val,
                                                                         _STOQID(KN->getText(), KN->getLine())
                                                                         );
                                                         SET_LINE_INFO (IfrOptionKey, KN);
                                                       >>
  }
  (
    T:"," vfrImageTag                                  << OOOObj->SetScope (1); CRT_END_OP (T); >>
  )*
  ";"                                                  << if (OOOObj != NULL) {delete OOOObj;} >>
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
    N:Number                                           << $LFlags |= _STOU8(N->getText(), N->getLine()); >>
  | "OPTION_DEFAULT"                                   << $LFlags |= 0x10; >>
  | "OPTION_DEFAULT_MFG"                               << $LFlags |= 0x20; >>
  | InteractiveFlag                                    << $HFlags |= 0x04; >>
  | ResetRequiredFlag                                  << $HFlags |= 0x10; >>
  | RestStyleFlag                                      << $HFlags |= 0x20; >>
  | ReconnectRequiredFlag                              << $HFlags |= 0x40; >>
  | ManufacturingFlag                                  << $LFlags |= 0x20; >>
  | DefaultFlag                                        << $LFlags |= 0x10; >>
  | A:NVAccessFlag                                     <<
                                                          gCVfrErrorHandle.HandleWarning (
                                                            VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                            A->getLine(),
                                                            A->getText()
                                                            );
                                                       >>
  | L:LateCheckFlag                                    <<
                                                          gCVfrErrorHandle.HandleWarning (
                                                            VFR_WARNING_OBSOLETED_FRAMEWORK_OPCODE,
                                                            L->getLine(),
                                                            L->getText()
                                                            );
                                                       >>
  ;

vfrStatementLabel :
  L:Label
  N:Number                                             <<
                                                          {
                                                            CIfrLabel LObj2;
                                                            LObj2.SetLineNo(L->getLine());
                                                            LObj2.SetNumber (_STOU16(N->getText(), N->getLine()));
                                                          }
                                                       >>
  ";"
  ;

vfrStatementBanner :
  << CIfrBanner BObj; >>
  B:Banner { "," }                                     << BObj.SetLineNo(B->getLine()); >>
  Title "=" "STRING_TOKEN" "\(" S:Number "\)" ","      << BObj.SetTitle (_STOSID(S->getText(), S->getLine())); >>
  (
    (
      Line L:Number ","                                << BObj.SetLine (_STOU16(L->getText(), L->getLine())); >>
      Align
      (
          Left                                         << BObj.SetAlign (0); >>
        | Center                                       << BObj.SetAlign (1); >>
        | Right                                        << BObj.SetAlign (2); >>
      ) ";"
    )
    |
    (
      Timeout "=" T:Number ";"                         << {CIfrTimeout TObj(_STOU16(T->getText(), T->getLine()));} >>
    )
  )
  ;

//******************************************************************************
//
// keep some syntax for compatibility but not generate any IFR object
//
vfrStatementInvalidHidden :
  L:Hidden               <<
                            _PCATCH (VFR_RETURN_UNSUPPORTED, L);
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
                                                      _PCATCH (VFR_RETURN_UNSUPPORTED, L);
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
                                                      _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                   >>
  |
   K:Restore                                       <<
                                                      _PCATCH (VFR_RETURN_UNSUPPORTED, K);
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
#token Match2("match2")                         "match2"
#token Catenate("catenate")                     "catenate"
#token QuestionRefVal("questionrefval")         "questionrefval"
#token StringRefVal("stringrefval")             "stringrefval"
#token Map("map")                               "map"
#token RefreshGuid("refreshguid")               "refreshguid"

//
// Root expression extension function called by other function.
//
vfrStatementExpression [UINT32 RootLevel, UINT32 ExpOpCount = 0] :
                                                       <<
                                                          if ($RootLevel == 0) {
                                                            mCIfrOpHdrIndex ++;
                                                            if (mCIfrOpHdrIndex >= MAX_IFR_EXPRESSION_DEPTH) {
                                                              _PCATCH (VFR_RETURN_INVALID_PARAMETER, 0, "The depth of expression exceeds the max supported level 8!");
                                                            }
                                                            _INIT_OPHDR_COND ();
                                                          }
                                                       >>
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
                                                            _CLEAR_SAVED_OPHDR ();
                                                            mCIfrOpHdrIndex --;
                                                          }
                                                       >>
  ;

//
// Add new sub function for the sub expression extension to remember the ExpOpCount
// This function is only called by sub expression.
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
  | vfrExpressionMatch2[$RootLevel, $ExpOpCount]
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

vfrExpressionMatch2 [UINT32 & RootLevel, UINT32 & ExpOpCount]:
  <<
    EFI_GUID      Guid;
  >>
  L:Match2
  "\("
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  vfrStatementExpressionSub[$RootLevel + 1, $ExpOpCount]
  ","
  guidDefinition[Guid]
  "\)"                                                 << { CIfrMatch2 M2Obj(L->getLine(), &Guid); $ExpOpCount++; } >>
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
     EFI_VFR_RETURN_CODE   VfrReturnCode = VFR_RETURN_SUCCESS;
     EFI_VARSTORE_ID       VarStoreId   = EFI_VARSTORE_ID_INVALID;
  >>
  L:VarEqVal                                          <<
                                                          _PCATCH (VFR_RETURN_UNSUPPORTED, L);
                                                      >>
  VK:Var
  OpenParen
  VN:Number                                           <<
                                                          VarIdStr = NULL; _STRCAT(&VarIdStr, VK->getText()); _STRCAT(&VarIdStr, VN->getText());
                                                          VfrReturnCode = gCVfrDataStorage.GetVarStoreId (VarIdStr, &VarStoreId);
                                                          if (VfrReturnCode == VFR_RETURN_UNDEFINED) {
                                                            _PCATCH (gCVfrDataStorage.DeclareEfiVarStore (
                                                                                        VarIdStr,
                                                                                        &mFormsetGuid,
                                                                                        _STOSID(VN->getText(), VN->getLine()),
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
      V1:Number                                        << ConstVal = _STOU16(V1->getText(), V1->getLine()); >>
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
      V2:Number                                        << ConstVal = _STOU16(V2->getText(), V2->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_EQUAL); >>
    )
    |
    (
      "<"
      V3:Number                                        << ConstVal = _STOU16(V3->getText(), V3->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_THAN); >>
    )
    |
    (
      ">="
      V4:Number                                        << ConstVal = _STOU16(V4->getText(), V4->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_EQUAL); >>
    )
    |
    (
      ">"
      V5:Number                                        << ConstVal = _STOU16(V5->getText(), V5->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_THAN); >>
    )
  )
  <<
     if (VarIdStr != NULL) {
       delete[] VarIdStr;
       VarIdStr = NULL;
     }
  >>
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
      V1:Number                                        << ConstVal = _STOU16(V1->getText(), V1->getLine()); >>
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
      V2:Number                                        << ConstVal = _STOU16(V2->getText(), V2->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_EQUAL); >>
    )
    |
    (
      "<"
      V3:Number                                        << ConstVal = _STOU16(V3->getText(), V3->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, LESS_THAN); >>
    )
    |
    (
      ">="
      V4:Number                                        << ConstVal = _STOU16(V4->getText(), V4->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_EQUAL); >>
    )
    |
    (
      ">"
      V5:Number                                        << ConstVal = _STOU16(V5->getText(), V5->getLine()); >>
                                                       << IdEqValDoSpecial ($ExpOpCount, L->getLine(), QId, VarIdStr, Mask, ConstVal, GREATER_THAN); >>
    )
  )
  <<
     if (VarIdStr != NULL) {
       delete[] VarIdStr;
       VarIdStr = NULL;
     }
  >>
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
  <<
     if (VarIdStr[0] != NULL) {
       delete[] VarIdStr[0];
       VarIdStr[0] = NULL;
     }
     if (VarIdStr[1] != NULL) {
       delete[] VarIdStr[1];
       VarIdStr[1] = NULL;
     }
  >>
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
    V:Number                                           << ValueList[ListLen] = _STOU16(V->getText(), V->getLine()); ListLen++; >>
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
                                                          if (VarIdStr != NULL) {
                                                            delete[] VarIdStr;
                                                            VarIdStr = NULL;
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
        | ID:Number                                    << QId = _STOQID(ID->getText(), ID->getLine()); >>
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
          S:Number << RefStringId = _STOSID(S->getText(), S->getLine()); >>
        "\)"
        | I:Number << RefStringId = _STOSID(I->getText(), I->getLine()); >>
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
                                                              if ((gCVfrDataStorage.GetVarStoreType(Info.mVarStoreId) == EFI_VFR_VARSTORE_NAME) && (VarType == EFI_IFR_TYPE_UNDEFINED)) {
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
                                                            delete[] VarIdStr;
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
  | V:Number                                           << CIfrUint64 U64Obj(V->getLine()); U64Obj.SetValue (_STOU64(V->getText(), V->getLine())); _SAVE_OPHDR_COND (U64Obj, ($ExpOpCount == 0), V->getLine()); $ExpOpCount++; >>
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
        DevicePath "=" "STRING_TOKEN" "\(" S:Number "\)" ","    << Type = 0x2; DevPath = _STOSID(S->getText(), S->getLine()); >>
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
    Format "=" F:Number ","                            << Fmt = _STOU8(F->getText(), F->getLine()); >>
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
                                                              if ((gCVfrDataStorage.GetVarStoreType(Info.mVarStoreId) == EFI_VFR_VARSTORE_NAME) && (VarType == EFI_IFR_TYPE_UNDEFINED)) {
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
                                                            delete[] VarIdStr;
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
    N:Number                                           << $Flags |= _STOU8(N->getText(), N->getLine()); >>
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

  CVfrQuestionDB      mCVfrQuestionDB;
  CVfrRulesDB         mCVfrRulesDB;

  CIfrOpHeader *      mCIfrOpHdr[MAX_IFR_EXPRESSION_DEPTH];
  UINT32              mCIfrOpHdrLineNo[MAX_IFR_EXPRESSION_DEPTH];
  UINT8               mCIfrOpHdrIndex;
  VOID                _SAVE_OPHDR_COND (IN CIfrOpHeader &, IN BOOLEAN, UINT32 LineNo = 0);
  VOID                _CLEAR_SAVED_OPHDR (VOID);
  VOID                _INIT_OPHDR_COND (VOID);
  BOOLEAN             _SET_SAVED_OPHDR_SCOPE (VOID);


  EFI_VARSTORE_INFO   mCurrQestVarInfo;
  EFI_GUID            *mOverrideClassGuid;
  CHAR8*              mLastFormEndAddr;

//
// Whether the question already has default value.
//
  UINT16              mUsedDefaultArray[EFI_IFR_MAX_DEFAULT_TYPE];
  UINT16              mUsedDefaultCount;

  EFI_GUID            mFormsetGuid;

  VOID                _CRT_OP (IN BOOLEAN);

  VOID                _SAVE_CURRQEST_VARINFO (IN EFI_VARSTORE_INFO &);
  EFI_VARSTORE_INFO & _GET_CURRQEST_VARTINFO (VOID);

  UINT8               _GET_CURRQEST_DATATYPE ();
  UINT32              _GET_CURRQEST_VARSIZE ();
  UINT32              _GET_CURRQEST_ARRAY_SIZE();
  VOID                CheckDuplicateDefaultValue (IN EFI_DEFAULT_ID, IN ANTLRTokenPtr);

public:
  VOID                _PCATCH (IN INTN, IN INTN, IN ANTLRTokenPtr, IN CONST CHAR8 *);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN ANTLRTokenPtr);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN UINT32);
  VOID                _PCATCH (IN EFI_VFR_RETURN_CODE, IN UINT32, IN CONST CHAR8 *);

  VOID                syn     (ANTLRAbstractToken  *, ANTLRChar *, SetWordType *, ANTLRTokenType, INT32);

  CHAR8*              TrimHex (IN CHAR8 *, OUT BOOLEAN *);
  CHAR8*              _U32TOS (IN UINT32);
  UINT8               _STOU8  (IN CHAR8 *, IN UINT32);
  UINT16              _STOU16 (IN CHAR8 *, IN UINT32);
  UINT32              _STOU32 (IN CHAR8 *, IN UINT32);
  UINT64              _STOU64 (IN CHAR8 *, IN UINT32);
  EFI_HII_DATE        _STOD   (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *, IN UINT32);
  EFI_HII_TIME        _STOT   (IN CHAR8 *, IN CHAR8 *, IN CHAR8 *, IN UINT32);
  EFI_HII_REF         _STOR   (IN CHAR8 *, IN CHAR8 *, IN EFI_GUID *, IN CHAR8 *, IN UINT32);

  EFI_STRING_ID       _STOSID (IN CHAR8 *, IN UINT32);
  EFI_FORM_ID         _STOFID (IN CHAR8 *, IN UINT32);
  EFI_QUESTION_ID     _STOQID (IN CHAR8 *, IN UINT32);

  VOID                _STRCAT (IN OUT CHAR8 **, IN CONST CHAR8 *);

  VOID                _DeclareDefaultLinearVarStore (IN UINT32);
  VOID                _DeclareStandardDefaultStorage (IN UINT32);

  VOID                AssignQuestionKey (IN CIfrQuestionHeader &, IN ANTLRTokenPtr);

  VOID                ConvertIdExpr         (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32);
  VOID                IdEqValDoSpecial      (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN UINT16, IN EFI_COMPARE_TYPE);
  VOID                IdEqIdDoSpecial       (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN EFI_COMPARE_TYPE);
  VOID                IdEqListDoSpecial     (IN UINT32 &, IN UINT32, IN EFI_QUESTION_ID, IN CHAR8 *, IN UINT32, IN UINT16, IN UINT16 *);
  VOID                SetOverrideClassGuid  (IN EFI_GUID *);
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
EfiVfrParser::_INIT_OPHDR_COND (
  VOID
  )
{
  mCIfrOpHdr[mCIfrOpHdrIndex]       = NULL;
  mCIfrOpHdrLineNo[mCIfrOpHdrIndex] = 0;
}

VOID
EfiVfrParser::_CLEAR_SAVED_OPHDR (
  VOID
  )
{
  if (mCIfrOpHdr[mCIfrOpHdrIndex] != NULL) {
    delete mCIfrOpHdr[mCIfrOpHdrIndex];
    mCIfrOpHdr[mCIfrOpHdrIndex] = NULL;
  }
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
  IN CHAR8  *Str,
  IN UINT32 LineNum
  )
{
  BOOLEAN IsHex;
  UINT8   Value;
  CHAR8   c;

  UINT8 PreviousValue;
  CHAR8 *OrigString = Str;
  CHAR8 ErrorMsg[100];

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    PreviousValue = Value;
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
    if((IsHex && ((Value/16) != PreviousValue)) || (!IsHex && ((Value/10) != PreviousValue))) {
      sprintf(ErrorMsg, "Overflow: Value %s is too large to store in a UINT8", OrigString);
      mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (VFR_RETURN_STRING_TO_UINT_OVERFLOW, LineNum, ErrorMsg);
    }
  }

  return Value;
}

UINT16
EfiVfrParser::_STOU16 (
  IN CHAR8  *Str,
  IN UINT32 LineNum
  )
{
  BOOLEAN IsHex;
  UINT16  Value;
  CHAR8   c;

  UINT16 PreviousValue;
  CHAR8 *OrigString = Str;
  CHAR8 ErrorMsg[100];

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    PreviousValue = Value;
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
    if((IsHex && ((Value/16) != PreviousValue)) || (!IsHex && ((Value/10) != PreviousValue))) {
      sprintf(ErrorMsg, "Overflow: Value %s is too large to store in a UINT16", OrigString);
      mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (VFR_RETURN_STRING_TO_UINT_OVERFLOW, LineNum, ErrorMsg);
    }
  }

  return Value;
}

UINT32
EfiVfrParser::_STOU32 (
  IN CHAR8  *Str,
  IN UINT32 LineNum
  )
{
  BOOLEAN IsHex;
  UINT32  Value;
  CHAR8   c;

  UINT32 PreviousValue;
  CHAR8 *OrigString = Str;
  CHAR8 ErrorMsg[100];

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    PreviousValue = Value;
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
    if((IsHex && ((Value/16) != PreviousValue)) || (!IsHex && ((Value/10) != PreviousValue ))) {
      sprintf(ErrorMsg, "Overflow: Value %s is too large to store in a UINT32", OrigString);
      mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (VFR_RETURN_STRING_TO_UINT_OVERFLOW, LineNum, ErrorMsg);
    }
  }

  return Value;
}

UINT64
EfiVfrParser::_STOU64 (
  IN CHAR8  *Str,
  IN UINT32 LineNum
  )
{
  BOOLEAN IsHex;
  UINT64  Value;
  CHAR8   c;
  UINT64 PreviousValue;
  CHAR8 *OrigString = Str;
  CHAR8 ErrorMsg[100];

  Str = TrimHex (Str, &IsHex);
  for (Value = 0; (c = *Str) != '\0'; Str++) {
    PreviousValue = Value;
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
    if((IsHex && ((Value/16) != PreviousValue)) || ((!IsHex && (Value/10) != PreviousValue))) {
      sprintf(ErrorMsg, "Overflow: Value %s is too large to store in a UINT64", OrigString);
      mParserStatus = mParserStatus + gCVfrErrorHandle.HandleError (VFR_RETURN_STRING_TO_UINT_OVERFLOW, LineNum, ErrorMsg);
    }
  }

  return Value;
}

EFI_HII_DATE
EfiVfrParser::_STOD (
  IN CHAR8 *Year,
  IN CHAR8 *Month,
  IN CHAR8 *Day,
  IN UINT32 LineNum
  )
{
  EFI_HII_DATE Date;

  Date.Year  = _STOU16 (Year, LineNum);
  Date.Month = _STOU8 (Month, LineNum);
  Date.Day   = _STOU8 (Day, LineNum);

  return Date;
}

EFI_HII_TIME
EfiVfrParser::_STOT (
  IN CHAR8 *Hour,
  IN CHAR8 *Minute,
  IN CHAR8 *Second,
  IN UINT32 LineNum
  )
{
  EFI_HII_TIME Time;

  Time.Hour   = _STOU8 (Hour, LineNum);
  Time.Minute = _STOU8 (Minute, LineNum);
  Time.Second = _STOU8 (Second, LineNum);

  return Time;
}

EFI_STRING_ID
EfiVfrParser::_STOSID (
  IN CHAR8  *Str,
  IN UINT32 LineNum
  )
{
  return (EFI_STRING_ID)_STOU16(Str, LineNum);
}

EFI_FORM_ID
EfiVfrParser::_STOFID (
  IN CHAR8 *Str,
  IN UINT32 LineNum
  )
{
  return (EFI_FORM_ID)_STOU16(Str, LineNum);
}

EFI_QUESTION_ID
EfiVfrParser::_STOQID (
  IN CHAR8 *Str,
  IN UINT32 LineNum
  )
{
  return (EFI_QUESTION_ID)_STOU16(Str, LineNum);
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
    delete[] *Dest;
  }
  strcat (NewStr, Src);

  *Dest = NewStr;
}

EFI_HII_REF
EfiVfrParser::_STOR (
  IN CHAR8    *QuestionId,
  IN CHAR8    *FormId,
  IN EFI_GUID *FormSetGuid,
  IN CHAR8    *DevicePath,
  IN UINT32   LineNum
  )
{
  EFI_HII_REF Ref;
  UINT32      Index;

  memcpy (&Ref.FormSetGuid, FormSetGuid, sizeof (EFI_GUID));
  Ref.QuestionId  = _STOQID (QuestionId, LineNum);
  Ref.FormId      = _STOFID (FormId, LineNum);
  Ref.DevicePath  = _STOSID (DevicePath, LineNum);

  return Ref;
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
    gCVfrDataStorage.DeclareBufferVarStore (
                       TypeNameList[Index],
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       TypeNameList[Index],
                       EFI_VARSTORE_ID_INVALID,
                       FALSE
                       );
    gCVfrDataStorage.GetVarStoreId(TypeNameList[Index], &VarStoreId, &mFormsetGuid);
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
    gCVfrDataStorage.DeclareBufferVarStore (
                       (CHAR8 *) DateName,
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       (CHAR8 *) DateType,
                       EFI_VARSTORE_ID_INVALID,
                       FALSE
                       );
    gCVfrDataStorage.GetVarStoreId((CHAR8 *) DateName, &VarStoreId, &mFormsetGuid);
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
    gCVfrDataStorage.DeclareBufferVarStore (
                       (CHAR8 *) TimeName,
                       &mFormsetGuid,
                       &gCVfrVarDataTypeDB,
                       (CHAR8 *) TimeType,
                       EFI_VARSTORE_ID_INVALID,
                       FALSE
                       );
    gCVfrDataStorage.GetVarStoreId((CHAR8 *) TimeName, &VarStoreId, &mFormsetGuid);
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

  gCVfrDefaultStore.RegisterDefaultStore (DSObj.GetObjBinAddr<CHAR8>(), (CHAR8 *) "Standard Defaults", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_STANDARD);
  DSObj.SetLineNo (LineNo);
  DSObj.SetDefaultName (EFI_STRING_ID_INVALID);
  DSObj.SetDefaultId (EFI_HII_DEFAULT_CLASS_STANDARD);

  //
  // Default MANUFACTURING Store is declared.
  //
  CIfrDefaultStore DSObjMF;

  gCVfrDefaultStore.RegisterDefaultStore (DSObjMF.GetObjBinAddr<CHAR8>(), (CHAR8 *) "Standard ManuFacturing", EFI_STRING_ID_INVALID, EFI_HII_DEFAULT_CLASS_MANUFACTURING);
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

  KeyValue = _STOU16 (KeyTok->getText(), KeyTok->getLine());

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

VOID
EfiVfrParser::CheckDuplicateDefaultValue (
  IN EFI_DEFAULT_ID      DefaultId,
  IN ANTLRTokenPtr       Tok
  )
{
  UINT16    Index;

  for(Index = 0; Index < mUsedDefaultCount; Index++) {
    if (mUsedDefaultArray[Index] == DefaultId) {
      gCVfrErrorHandle.HandleWarning (VFR_WARNING_DEFAULT_VALUE_REDEFINED, Tok->getLine(), Tok->getText());
    }
  }

  if (mUsedDefaultCount >= EFI_IFR_MAX_DEFAULT_TYPE - 1) {
    gCVfrErrorHandle.HandleError (VFR_RETURN_FATAL_ERROR, Tok->getLine(), Tok->getText());
  }

  mUsedDefaultArray[mUsedDefaultCount++] = DefaultId;
}
>>
