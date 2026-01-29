/** @file
  Definitions of RedfishPlatformConfigLib.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef HII_UTILITY_LIB_
#define HII_UTILITY_LIB_

#include <Protocol/DisplayProtocol.h>
#include <Protocol/HiiConfigAccess.h>

//
// IFR relative definition
//
#define EFI_HII_EXPRESSION_INCONSISTENT_IF  0
#define EFI_HII_EXPRESSION_NO_SUBMIT_IF     1
#define EFI_HII_EXPRESSION_GRAY_OUT_IF      2
#define EFI_HII_EXPRESSION_SUPPRESS_IF      3
#define EFI_HII_EXPRESSION_DISABLE_IF       4
#define EFI_HII_EXPRESSION_VALUE            5
#define EFI_HII_EXPRESSION_RULE             6
#define EFI_HII_EXPRESSION_READ             7
#define EFI_HII_EXPRESSION_WRITE            8
#define EFI_HII_EXPRESSION_WARNING_IF       9

#define EFI_HII_VARSTORE_BUFFER               0
#define EFI_HII_VARSTORE_NAME_VALUE           1
#define EFI_HII_VARSTORE_EFI_VARIABLE         2   // EFI Varstore type follow UEFI spec before 2.3.1.
#define EFI_HII_VARSTORE_EFI_VARIABLE_BUFFER  3   // EFI varstore type follow UEFI spec 2.3.1 and later.

///
/// HII_NAME_VALUE_NODE for name/value storage
///
typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Link;
  CHAR16        *Name;
  CHAR16        *Value;
} HII_NAME_VALUE_NODE;

#define HII_NAME_VALUE_NODE_SIGNATURE  SIGNATURE_32 ('N', 'V', 'S', 'T')
#define HII_NAME_VALUE_NODE_FROM_LINK(a)  CR (a, HII_NAME_VALUE_NODE, Link, HII_NAME_VALUE_NODE_SIGNATURE)

///
/// Storage info
///
typedef union {
  EFI_STRING_ID    VarName;
  UINT16           VarOffset;
} HII_VAR_STORE_INFO;

///
/// FormSet storage
///
typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;

  UINT8             Type;          ///< Storage type
  EFI_HII_HANDLE    HiiHandle;     ///< HiiHandle for this varstore.

  ///
  /// For all type of storages.
  ///
  UINT16            VarStoreId;    ///< VarStore ID.
  EFI_GUID          Guid;          ///< VarStore Guid.

  ///
  /// For EFI_IFR_VARSTORE, EFI_IFR_VARSTORE_EFI
  ///
  CHAR16            *Name;         ///< VarStore name
  UINT16            Size;          ///< VarStore size.
  UINT8             *Buffer;       ///< Buffer storage.
  UINT8             *EditBuffer;   ///< Edit copy for Buffer Storage

  ///
  /// For EFI_IFR_VARSTORE_EFI: EFI Variable.
  ///
  UINT32            Attributes;

  ///
  /// For EFI_IFR_VARSTORE_NAME_VALUE.
  ///
  LIST_ENTRY        NameValueList;  ///< List of NAME_VALUE_NODE

  CHAR16            *ConfigHdr;     ///< <ConfigHdr>
  CHAR16            *ConfigRequest; ///< <ConfigRequest> = <ConfigHdr> + <RequestElement>
  UINTN             ElementCount;   ///< Number of <RequestElement> in the <ConfigRequest>
  UINTN             SpareStrLen;    ///< Spare length of ConfigRequest string buffer
} HII_FORMSET_STORAGE;

#define HII_STORAGE_SIGNATURE  SIGNATURE_32 ('B', 'S', 'T', 'G')
#define HII_STORAGE_FROM_LINK(a)  CR (a, HII_FORMSET_STORAGE, Link, HII_STORAGE_SIGNATURE)

///
/// Definition of EXPRESS_RESULT
///
typedef enum {
  ExpressFalse = 0,
  ExpressGrayOut,
  ExpressSuppress,
  ExpressDisable
} EXPRESS_RESULT;

///
/// Definition of EXPRESS_LEVEL
///
typedef enum {
  ExpressNone = 0,
  ExpressForm,
  ExpressStatement,
  ExpressOption
} EXPRESS_LEVEL;

///
/// Definition of HII_EXPRESSION_OPCODE_EXTRA
///
typedef union {
  EFI_HII_VALUE    Value;         ///< EFI_IFR_UINT64, EFI_IFR_UINT32, EFI_IFR_UINT16, EFI_IFR_UINT8, EFI_IFR_STRING_REF1
  UINT8            Format;        ///< For EFI_IFR_TO_STRING, EFI_IFR_FIND
  UINT8            Flags;         ///< For EFI_IFR_SPAN
  UINT8            RuleId;        ///< For EFI_IFR_RULE_REF
  EFI_GUID         Guid;          ///< For EFI_IFR_SECURITY, EFI_IFR_MATCH2

  struct {
    EFI_QUESTION_ID    QuestionId;
    EFI_HII_VALUE      Value;
  } EqIdValData;

  struct {
    EFI_QUESTION_ID    QuestionId1;
    EFI_QUESTION_ID    QuestionId2;
  } EqIdIdData;

  struct {
    EFI_QUESTION_ID    QuestionId; ///< For EFI_IFR_EQ_ID_VAL_LIST
    UINT16             ListLength;
    UINT16             *ValueList;
  } EqIdListData;

  struct {
    EFI_QUESTION_ID    QuestionId;
  } QuestionRef1Data;

  struct {
    EFI_STRING_ID    DevicePath;  ///< For EFI_IFR_QUESTION_REF3_3
    EFI_GUID         Guid;
  } QuestionRef3Data;

  struct {
    HII_FORMSET_STORAGE    *VarStorage;
    HII_VAR_STORE_INFO     VarStoreInfo;
    UINT8                  ValueType;
    UINT8                  ValueWidth;
    CHAR16                 *ValueName;
  } GetSetData;
} HII_EXPRESSION_OPCODE_EXTRA;

typedef union _HII_DEPENDENCY_EXPRESSION HII_DEPENDENCY_EXPRESSION;

///
/// Definition of HII_EXPRESSION_CONSTANT
///
/// Operand:
///
/// EFI_IFR_TRUE
/// EFI_IFR_FALSE
/// EFI_IFR_ONE
/// EFI_IFR_ONES
/// EFI_IFR_ZERO
/// EFI_IFR_UNDEFINED
/// EFI_IFR_VERSION
/// EFI_IFR_UINT8
/// EFI_IFR_UINT16
/// EFI_IFR_UINT32
/// EFI_IFR_UINT64
///
typedef struct {
  UINT8            Operand;
  EFI_HII_VALUE    Value;
} HII_EXPRESSION_CONSTANT;

///
/// Definition of HII_DEPENDENCY_DUP
///
typedef struct {
  UINT8    Operand;
} HII_DEPENDENCY_DUP;

///
/// Definition of HII_DEPENDENCY_EQ_ID_VAL
///
typedef struct {
  UINT8              Operand;
  EFI_QUESTION_ID    QuestionId;
  EFI_HII_VALUE      Value;
} HII_DEPENDENCY_EQ_ID_VAL;

///
/// Definition of HII_DEPENDENCY_EQ_ID_VAL
///
typedef struct {
  UINT8              Operand;
  EFI_QUESTION_ID    QuestionId1;
  EFI_QUESTION_ID    QuestionId2;
} HII_DEPENDENCY_EQ_ID_ID;

///
/// Definition of HII_DEPENDENCY_EQ_ID_VAL_LIST
///
typedef struct {
  UINT8              Operand;
  EFI_QUESTION_ID    QuestionId;
  UINT16             ListLength;
  UINT16             *ValueList;
} HII_DEPENDENCY_EQ_ID_VAL_LIST;

///
/// Definition of HII_DEPENDENCY_QUESTION_REF1
///
typedef struct {
  UINT8              Operand;
  EFI_QUESTION_ID    QuestionId;
} HII_DEPENDENCY_QUESTION_REF1;

///
/// Definition of HII_DEPENDENCY_RULE_REF
///
typedef struct {
  UINT8    Operand;
  UINT8    RuleId;
} HII_DEPENDENCY_RULE_REF;

///
/// Definition of HII_DEPENDENCY_STRING_REF1
///
typedef struct {
  UINT8            Operand;
  EFI_HII_VALUE    Value;
} HII_DEPENDENCY_STRING_REF1;

///
/// Definition of HII_DEPENDENCY_THIS
///
typedef struct {
  UINT8              Operand;
  EFI_QUESTION_ID    QuestionId;
} HII_DEPENDENCY_THIS;

///
/// Definition of HII_DEPENDENCY_SECURITY
///
typedef struct {
  UINT8       Operand;
  EFI_GUID    Permissions;
} HII_DEPENDENCY_SECURITY;

///
/// Definition of HII_DEPENDENCY_GET
///
typedef struct  {
  UINT8                  Operand;
  HII_FORMSET_STORAGE    *VarStorage;
  HII_VAR_STORE_INFO     VarStoreInfo;
  UINT8                  ValueType;
  UINT8                  ValueWidth;
  CHAR16                 *ValueName;
} HII_DEPENDENCY_GET;

///
/// Definition of HII_DEPENDENCY_LENGTH
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_LENGTH;

///
/// Definition of HII_DEPENDENCY_BITWISE_NOT
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_BITWISE_NOT;

///
/// Definition of HII_DEPENDENCY_STRING_REF2
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_STRING_REF2;

///
/// Definition of HII_DEPENDENCY_QUESTION_REF2
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_QUESTION_REF2;

///
/// Definition of HII_DEPENDENCY_QUESTION_REF3
///
typedef struct  {
  UINT8                        Operand;
  EFI_STRING_ID                DevicePath;
  EFI_GUID                     Guid;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_QUESTION_REF3;

///
/// Definition of HII_DEPENDENCY_TO_BOOLEAN
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_TO_BOOLEAN;

///
/// Definition of HII_DEPENDENCY_TO_STRING
///
typedef struct  {
  UINT8                        Operand;
  UINT8                        Format;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_TO_STRING;

///
/// Definition of HII_DEPENDENCY_TO_UINT
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_TO_UINT;

///
/// Definition of HII_DEPENDENCY_TO_UPPER
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_TO_UPPER;

///
/// Definition of HII_DEPENDENCY_TO_LOWER
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_TO_LOWER;

///
/// Definition of HII_DEPENDENCY_SET
///
typedef struct  {
  UINT8                        Operand;
  HII_FORMSET_STORAGE          *VarStorage;
  HII_VAR_STORE_INFO           VarStoreInfo;
  UINT8                        ValueType;
  UINT8                        ValueWidth;
  CHAR16                       *ValueName;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_SET;

///
/// Definition of HII_DEPENDENCY_NOT
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression;
} HII_DEPENDENCY_NOT;

///
/// Definition of HII_DEPENDENCY_CATENATE
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *LeftStringExp;
  HII_DEPENDENCY_EXPRESSION    *RightStringExp;
} HII_DEPENDENCY_CATENATE;

///
/// Definition of HII_DEPENDENCY_MATCH
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *StringExp;
  HII_DEPENDENCY_EXPRESSION    *PatternExp;
} HII_DEPENDENCY_MATCH;

///
/// Definition of HII_DEPENDENCY_MATCH2
///
typedef struct {
  UINT8                        Operand;
  EFI_GUID                     SyntaxType;
  HII_DEPENDENCY_EXPRESSION    *StringExp;
  HII_DEPENDENCY_EXPRESSION    *PatternExp;
} HII_DEPENDENCY_MATCH2;

///
/// Definition of HII_DEPENDENCY_MULT
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_MULT;

///
/// Definition of HII_DEPENDENCY_DIV
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;    ///< right value
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;     ///< left value
} HII_DEPENDENCY_DIV;

///
/// Definition of HII_DEPENDENCY_MOD
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;    ///< right value
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;     ///< left value
} HII_DEPENDENCY_MOD;

///
/// Definition of HII_DEPENDENCY_ADD
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;     ///< right value
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;      ///< left value
} HII_DEPENDENCY_ADD;

///
/// Definition of HII_DEPENDENCY_SUBTRACT
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;    ///< right value
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;     ///< left value
} HII_DEPENDENCY_SUBTRACT;

///
/// Definition of HII_DEPENDENCY_SHIFT_LEFT
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_SHIFT_LEFT;

///
/// Definition of HII_DEPENDENCY_SHIFT_RIGHT
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_SHIFT_RIGHT;

///
/// Definition of HII_DEPENDENCY_GREATER_THAN
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_GREATER_THAN;

///
/// Definition of HII_DEPENDENCY_GREATER_EQUAL
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_GREATER_EQUAL;

///
/// Definition of HII_DEPENDENCY_LESS_THAN
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_LESS_THAN;

///
/// Definition of HII_DEPENDENCY_LESS_EQUAL
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *RightHandExp;
  HII_DEPENDENCY_EXPRESSION    *LeftHandExp;
} HII_DEPENDENCY_LESS_EQUAL;

///
/// Definition of HII_DEPENDENCY_EQUAL
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_EQUAL;

///
/// Definition of HII_DEPENDENCY_NOT_EQUAL
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_NOT_EQUAL;

///
/// Definition of HII_DEPENDENCY_BITWISE_AND
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_BITWISE_AND;

///
/// Definition of HII_DEPENDENCY_BITWISE_OR
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_BITWISE_OR;

///
/// Definition of HII_DEPENDENCY_AND
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_AND;

///
/// Definition of HII_DEPENDENCY_OR
///
typedef struct {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *SubExpression1;
  HII_DEPENDENCY_EXPRESSION    *SubExpression2;
} HII_DEPENDENCY_OR;

///
/// Definition of HII_DEPENDENCY_CONDITIONAL
///
/// Ternary expression
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *CondTrueValExp;  ///< right value
  HII_DEPENDENCY_EXPRESSION    *CondFalseValExp; ///< middle value
  HII_DEPENDENCY_EXPRESSION    *ConditionExp;    ///< left value
} HII_DEPENDENCY_CONDITIONAL;

///
/// Definition of HII_DEPENDENCY_FIND
///
typedef struct  {
  UINT8                        Operand;
  UINT8                        Format;
  HII_DEPENDENCY_EXPRESSION    *IndexExp;             ///< right value
  HII_DEPENDENCY_EXPRESSION    *StringToCompWithExp;  ///< middle value
  HII_DEPENDENCY_EXPRESSION    *StringToSearchExp;    ///< left value
} HII_DEPENDENCY_FIND;

///
/// Definition of HII_DEPENDENCY_MID
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *LengthExp;         ///< right value
  HII_DEPENDENCY_EXPRESSION    *IndexExp;          ///< middle value
  HII_DEPENDENCY_EXPRESSION    *StringOrBufferExp; ///< left value
} HII_DEPENDENCY_MID;

///
/// Definition of HII_DEPENDENCY_TOKEN
///
typedef struct  {
  UINT8                        Operand;
  HII_DEPENDENCY_EXPRESSION    *IndexExp;           ///< right value
  HII_DEPENDENCY_EXPRESSION    *DelimiterExp;       ///< middle value
  HII_DEPENDENCY_EXPRESSION    *StringToSearchExp;  ///< left value
} HII_DEPENDENCY_TOKEN;

///
/// Definition of HII_DEPENDENCY_SPAN
///
typedef struct  {
  UINT8                        Operand;
  UINT8                        Flags;
  HII_DEPENDENCY_EXPRESSION    *IndexExp;            ///< right value
  HII_DEPENDENCY_EXPRESSION    *CharsetExp;          ///< middle value
  HII_DEPENDENCY_EXPRESSION    *StringToSearchExp;   ///< left value
} HII_DEPENDENCY_SPAN;

///
/// Map expression
///
typedef struct {
  HII_DEPENDENCY_EXPRESSION    *MatchExp;
  HII_DEPENDENCY_EXPRESSION    *ReturnExp;
} HII_DEPENDENCY_EXPRESSION_PAIR;

///
/// Definition of HII_DEPENDENCY_MAP
///
typedef struct  {
  UINT8                             Operand;
  HII_DEPENDENCY_EXPRESSION         *SubExp;
  HII_DEPENDENCY_EXPRESSION_PAIR    *ExpPair;
  UINT8                             ExpPairNo;
} HII_DEPENDENCY_MAP;

///
/// Definition of HII_DEPENDENCY_EXPRESSION
///
union _HII_DEPENDENCY_EXPRESSION {
  ///
  /// Constant
  ///
  HII_EXPRESSION_CONSTANT          ConstantExp;
  ///
  /// build-in expression
  ///
  HII_DEPENDENCY_DUP               DupExp;
  HII_DEPENDENCY_EQ_ID_VAL         EqIdValExp;
  HII_DEPENDENCY_EQ_ID_ID          EqIdIdExp;
  HII_DEPENDENCY_EQ_ID_VAL_LIST    EqIdListExp;
  HII_DEPENDENCY_QUESTION_REF1     QuestionRef1Exp;
  HII_DEPENDENCY_RULE_REF          RuleRefExp;
  HII_DEPENDENCY_STRING_REF1       StringRef1Exp;
  HII_DEPENDENCY_THIS              ThisExp;
  HII_DEPENDENCY_SECURITY          SecurityExp;
  HII_DEPENDENCY_GET               GetExp;

  ///
  /// unary expression
  ///
  HII_DEPENDENCY_LENGTH            LengthExp;
  HII_DEPENDENCY_BITWISE_NOT       BitWiseNotExp;
  HII_DEPENDENCY_STRING_REF2       StringRef2Exp;
  HII_DEPENDENCY_QUESTION_REF2     QuestionRef2Exp;
  HII_DEPENDENCY_QUESTION_REF3     QuestionRef3Exp;
  HII_DEPENDENCY_TO_BOOLEAN        ToBooleanExp;
  HII_DEPENDENCY_TO_STRING         ToStringExp;
  HII_DEPENDENCY_TO_UINT           ToUintExp;
  HII_DEPENDENCY_TO_UPPER          ToUpperExp;
  HII_DEPENDENCY_TO_LOWER          ToLowerExp;
  HII_DEPENDENCY_SET               SetExp;
  HII_DEPENDENCY_NOT               NotExp;

  ///
  /// Binary expression
  ///
  HII_DEPENDENCY_CATENATE          CatenateExp;
  HII_DEPENDENCY_MATCH             MatchExp;
  HII_DEPENDENCY_MATCH2            Match2Exp;
  HII_DEPENDENCY_MULT              MultExp;
  HII_DEPENDENCY_DIV               DivExp;
  HII_DEPENDENCY_MOD               ModExp;
  HII_DEPENDENCY_ADD               AddExp;
  HII_DEPENDENCY_SUBTRACT          SubtractExp;
  HII_DEPENDENCY_SHIFT_LEFT        ShiftLeftExp;
  HII_DEPENDENCY_SHIFT_RIGHT       ShiftRightExp;
  HII_DEPENDENCY_GREATER_THAN      GreaterThanExp;
  HII_DEPENDENCY_GREATER_EQUAL     GreaterEqualExp;
  HII_DEPENDENCY_LESS_THAN         LessThanExp;
  HII_DEPENDENCY_LESS_EQUAL        LessEqualExp;
  HII_DEPENDENCY_EQUAL             EqualExp;
  HII_DEPENDENCY_NOT_EQUAL         NotEqualExp;
  HII_DEPENDENCY_BITWISE_AND       BitwiseAndExp;
  HII_DEPENDENCY_BITWISE_OR        BitwiseOrExp;
  HII_DEPENDENCY_AND               AndExp;
  HII_DEPENDENCY_OR                OrExp;

  ///
  /// ternary expression
  ///
  HII_DEPENDENCY_CONDITIONAL       ConditionalExp;
  HII_DEPENDENCY_FIND              FindExp;
  HII_DEPENDENCY_MID               MidExp;
  HII_DEPENDENCY_TOKEN             TokenExp;
  HII_DEPENDENCY_SPAN              SpanExp;
  HII_DEPENDENCY_MAP               MapExp;
};

///
/// Definition of HII_EXPRESSION_OPCODE
///
typedef struct {
  UINTN                          Signature;
  LIST_ENTRY                     Link;
  UINT8                          Operand;
  HII_EXPRESSION_OPCODE_EXTRA    ExtraData;
  LIST_ENTRY                     MapExpressionList; ///< nested expressions inside of Map opcode.
} HII_EXPRESSION_OPCODE;

#define HII_EXPRESSION_OPCODE_SIGNATURE  SIGNATURE_32 ('E', 'X', 'O', 'P')
#define HII_EXPRESSION_OPCODE_FROM_LINK(a)  CR (a, HII_EXPRESSION_OPCODE, Link, HII_EXPRESSION_OPCODE_SIGNATURE)

///
/// Definition of HII_WARNING_IF_DATA
///
typedef struct {
  EFI_STRING_ID    WarningIfError;
  UINT8            TimeOut;
} HII_WARNING_IF_DATA;

///
/// Definition of HII_EXTRA_DATA
///
typedef union {
  UINT8                  RuleId;      ///< For EFI_IFR_RULE only
  EFI_STRING_ID          Error;       ///< For EFI_IFR_NO_SUBMIT_IF, EFI_IFR_INCONSISTENT_IF only
  HII_WARNING_IF_DATA    WarningIfData;
} HII_EXTRA_DATA;

///
/// Definition of HII_EXPRESSION
///
typedef struct {
  UINTN                        Signature;
  LIST_ENTRY                   Link;
  UINT8                        Type;               ///< Type for this expression
  EFI_IFR_OP_HEADER            *OpCode;            ///< Save the opcode buffer.
  LIST_ENTRY                   OpCodeListHead;     ///< OpCodes consist of this expression (HII_EXPRESSION_OPCODE)
  HII_DEPENDENCY_EXPRESSION    *RootDependencyExp; ///< Expression OpCodes tree layout to describe dependency of this expression.
  HII_EXTRA_DATA               ExtraData;
  EFI_HII_VALUE                Result;             ///< Expression evaluation result
} HII_EXPRESSION;

#define HII_EXPRESSION_SIGNATURE  SIGNATURE_32 ('F', 'E', 'X', 'P')
#define HII_EXPRESSION_FROM_LINK(a)  CR (a, HII_EXPRESSION, Link, HII_EXPRESSION_SIGNATURE)

///
/// Definition of HII_EXPRESSION_LIST
///
typedef struct {
  UINTN             Signature;
  UINTN             Count;
  HII_EXPRESSION    *Expression[1];    ///< Array[Count] of expressions
} HII_EXPRESSION_LIST;

#define HII_EXPRESSION_LIST_SIGNATURE  SIGNATURE_32 ('F', 'E', 'X', 'R')

///
/// Definition of HII_STATEMENT_VALUE
///
typedef struct {
  ///
  /// HII Data Type
  ///
  UINT8                 Type;
  EFI_IFR_TYPE_VALUE    Value;
  ///
  /// Buffer Data and Length if Type is EFI_IFR_TYPE_BUFFER or EFI_IFR_TYPE_STRING
  ///
  UINT8                 *Buffer;
  UINT16                BufferLen;
  UINT8                 BufferValueType; ///< Data type for buffer internal data, currently only for orderedlist
} HII_STATEMENT_VALUE;

///
/// Default value
///
typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;

  UINT16                 DefaultId;
  HII_STATEMENT_VALUE    Value;            ///< Default value

  HII_EXPRESSION         *ValueExpression; ///< Not-NULL indicates default value is provided by EFI_IFR_VALUE
} HII_QUESTION_DEFAULT;

#define HII_QUESTION_DEFAULT_SIGNATURE  SIGNATURE_32 ('Q', 'D', 'F', 'T')
#define HII_QUESTION_DEFAULT_FROM_LINK(a)  CR (a, HII_QUESTION_DEFAULT, Link, HII_QUESTION_DEFAULT_SIGNATURE)

#define HII_QUESTION_OPTION_SIGNATURE  SIGNATURE_32 ('Q', 'O', 'P', 'T')

///
/// Option value
///
typedef struct {
  UINTN                    Signature;
  LIST_ENTRY               Link;

  EFI_IFR_ONE_OF_OPTION    *OpCode;             ///< OneOfOption Data

  EFI_STRING_ID            Text;
  UINT8                    Flags;
  HII_STATEMENT_VALUE      Value;
  EFI_IMAGE_ID             ImageId;

  HII_EXPRESSION_LIST      *SuppressExpression; ///< Non-NULL indicates nested inside of SuppressIf
} HII_QUESTION_OPTION;

#define HII_QUESTION_OPTION_FROM_LINK(a)  CR (a, HII_QUESTION_OPTION, Link, HII_QUESTION_OPTION_SIGNATURE)

///
/// class of default
///
typedef struct {
  UINTN            Signature;
  LIST_ENTRY       Link;

  UINT16           DefaultId;
  EFI_STRING_ID    DefaultName;
} HII_FORMSET_DEFAULTSTORE;

#define HII_FORMSET_DEFAULTSTORE_SIGNATURE  SIGNATURE_32 ('F', 'D', 'F', 'S')
#define HII_FORMSET_DEFAULTSTORE_FROM_LINK(a)  CR (a, HII_FORMSET_DEFAULTSTORE, Link, HII_FORMSET_DEFAULTSTORE_SIGNATURE)

///
/// Definition of HII_STATEMENT_EXTRA
///
typedef union {
  UINT8             Flags;
  EFI_STRING_ID     TextTwo;
  EFI_DEFAULT_ID    DefaultId;
  EFI_STRING_ID     QuestionConfig;
  EFI_GUID          Guid;

  struct {
    UINT8       Flags;
    UINT64      Minimum;            ///< for EFI_IFR_ONE_OF/EFI_IFR_NUMERIC, it's Min/Max value
    UINT64      Maximum;            ///< for EFI_IFR_STRING/EFI_IFR_PASSWORD, it's Min/Max length
    UINT64      Step;
    EFI_GUID    Guid;
  } NumData;

  struct {
    UINT8    Flags;
    UINT8    MaxContainers;         ///< for EFI_IFR_ORDERED_LIST
  } OrderListData;

  struct {
    UINT8    Flags;
    UINT8    MinSize;
    UINT8    MaxSize;
  } StrData;

  struct {
    UINT16    MinSize;
    UINT16    MaxSize;
  } PwdData;
} HII_STATEMENT_EXTRA;

///
/// Statement (Question)
///
typedef struct _HII_STATEMENT HII_STATEMENT;
struct _HII_STATEMENT {
  UINTN                  Signature;
  LIST_ENTRY             Link;

  UINT8                  Operand;         ///< The operand (first byte) of this Statement or Question
  EFI_IFR_OP_HEADER      *OpCode;

  ///
  /// Statement Header
  ///
  EFI_STRING_ID          Prompt;
  EFI_STRING_ID          Help;

  ///
  /// Question Header
  ///
  EFI_QUESTION_ID        QuestionId;      ///< Question id, the value of zero is reserved
  EFI_VARSTORE_ID        VarStoreId;      ///< VarStore id, a value of zero indicates no variable storage
  HII_VAR_STORE_INFO     VarStoreInfo;    ///< VarStoreInfoIf VarStoreId refers to Buffer Storage (EFI_IFR_VARSTORE or EFI_IFR_VARSTORE_EFI), then VarStoreInfo contains a 16-bit Buffer Storage offset (VarOffset).
                                          ///< If VarStoreId refers to Name/Value Storage (EFI_IFR_VARSTORE_NAME_VALUE), then VarStoreInfo contains the String ID of the name (VarName) for this name/value pair.
  UINT8                  QuestionFlags;   ///< The flag of this Question.(Readonly, reset required, callback attribute....)

  BOOLEAN                QuestionReferToBitField;   ///< Whether the question is stored in a bit field.
  UINT16                 StorageWidth;              ///< The storage width of this Question.
  UINT16                 BitStorageWidth;           ///< The Storage width of this Question in bit level.
  UINT16                 BitVarOffset;              ///< The storage offset of this Question in bit level.
  CHAR16                 *VariableName;             ///< Name/Value or EFI Variable name
  CHAR16                 *BlockName;                ///< Buffer storage block name: "OFFSET=...WIDTH=..."

  HII_FORMSET_STORAGE    *Storage;                  ///< Point to the storage that store this question.
  HII_STATEMENT_EXTRA    ExtraData;

  BOOLEAN                Locked;                    ///< Whether this statement is locked.

  HII_STATEMENT_VALUE    Value;

  ///
  /// Get from IFR parsing
  ///

  HII_STATEMENT          *ParentStatement;     ///< Parent Statement of current statement.
  HII_EXPRESSION_LIST    *ExpressionList;      ///< nesting inside of GrayedOutIf/DisableIf/SuppressIf
  HII_EXPRESSION         *ValueExpression;     ///< nested EFI_IFR_VALUE, provide Question value and indicate Question is ReadOnly

  EFI_IMAGE_ID           ImageId;              ///< nested EFI_IFR_IMAGE
  UINT8                  RefreshInterval;      ///< nested EFI_IFR_REFRESH, refresh interval(in seconds) for Question value, 0 means no refresh

  LIST_ENTRY             DefaultListHead;      ///< nested EFI_IFR_DEFAULT list (HII_QUESTION_DEFAULT), provide default values
  LIST_ENTRY             OptionListHead;       ///< nested EFI_IFR_ONE_OF_OPTION list (HII_QUESTION_OPTION)
  LIST_ENTRY             InconsistentListHead; ///< nested inconsistent expression list (HII_EXPRESSION)
  LIST_ENTRY             NoSubmitListHead;     ///< nested nosubmit expression list (HII_EXPRESSION)
  LIST_ENTRY             WarningListHead;      ///< nested warning expression list (HII_EXPRESSION)

  HII_EXPRESSION         *ReadExpression;       ///< nested EFI_IFR_READ, provide this question value by read expression.
  HII_EXPRESSION         *WriteExpression;      ///< nested EFI_IFR_WRITE, evaluate write expression after this question value is set.
};

#define HII_STATEMENT_SIGNATURE  SIGNATURE_32 ('H', 'S', 'T', 'A')
#define HII_STATEMENT_FROM_LINK(a)  CR (a, HII_STATEMENT, Link, HII_STATEMENT_SIGNATURE)

///
/// Form
///
#define STANDARD_MAP_FORM_TYPE  0x01

typedef struct {
  UINTN                  Signature;
  LIST_ENTRY             Link;

  UINT16                 FormId;              ///< FormId of normal form or formmap form.
  EFI_STRING_ID          FormTitle;           ///< FormTile of normal form, or FormMapMethod title of formmap form.
  UINT16                 FormType;            ///< Specific form type for the different form.

  EFI_IMAGE_ID           ImageId;             ///< The image id.

  BOOLEAN                ModalForm;           ///< Whether this is a modal form.
  BOOLEAN                Locked;              ///< Whether this form is locked.
  EFI_GUID               RefreshGuid;         ///< Form refresh event guid.

  LIST_ENTRY             StatementListHead;   ///< List of Statements and Questions (HII_STATEMENT)
  LIST_ENTRY             ConfigRequestHead;   ///< List of configrequest for all storage.
  LIST_ENTRY             RuleListHead;        ///< nested EFI_IFR_RULE list, pre-defined expressions attached to the form.
  HII_EXPRESSION_LIST    *SuppressExpression; ///< nesting inside of SuppressIf
} HII_FORM;

#define HII_FORM_SIGNATURE  SIGNATURE_32 ('F', 'F', 'R', 'M')
#define HII_FORM_FROM_LINK(a)  CR (a, HII_FORM, Link, HII_FORM_SIGNATURE)

///
/// FormSet
///
typedef struct {
  UINTN                             Signature;
  LIST_ENTRY                        Link;

  EFI_HII_HANDLE                    HiiHandle;             ///< Unique id for formset, HII Handle of this FormSet package.
  EFI_HANDLE                        DriverHandle;          ///< EFI_HANDLE which was registered with the package list in NewPackageList().
  EFI_HII_CONFIG_ACCESS_PROTOCOL    *ConfigAccess;         ///< ConfigAccess Protocol associated with this HiiPackageList
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  UINTN                             IfrBinaryLength;       ///< Ifr binary data length of this formset.
  UINT8                             *IfrBinaryData;        ///< Point to the Ifr binary data.

  EFI_GUID                          Guid;                  ///< Formset Guid.
  EFI_STRING_ID                     FormSetTitle;          ///< String Id of Formset title.
  EFI_STRING_ID                     Help;                  ///< String Id of Formset title.

  UINT8                             NumberOfClassGuid;     ///< Class Guid name
  EFI_GUID                          ClassGuid[3];          ///< Up to three ClassGuid

  EFI_IMAGE_ID                      ImageId;               ///< The image id.

  LIST_ENTRY                        StatementListOSF;      ///< Statement list out side of the form.
  LIST_ENTRY                        StorageListHead;       ///< Storage list (HII_FORMSET_STORAGE)
  LIST_ENTRY                        DefaultStoreListHead;  ///< DefaultStore list (HII_FORMSET_DEFAULTSTORE)
  LIST_ENTRY                        FormListHead;          ///< Form list (HII_FORM_BROWSER_FORM)
} HII_FORMSET;

#define HII_FORMSET_SIGNATURE  SIGNATURE_32 ('H', 'I', 'F', 'S')
#define HII_FORMSET_FROM_LINK(a)  CR (a, HII_FORMSET, Link, HII_FORMSET_SIGNATURE)

///
/// Get/set question value from/to.
///
typedef enum {
  GetSetValueWithBuffer = 0,       ///< Get/Set question value from/to buffer in the storage.
  GetSetValueWithHiiDriver,        ///< Get/Set question value from/to hii driver.
  GetSetValueWithBoth,             ///< Compare the editbuffer with buffer for this question, not use the question value.
  GetSetValueWithMax               ///< Invalid value.
} GET_SET_QUESTION_VALUE_WITH;

/**
  Initialize the internal data structure of a FormSet.

  @param[in]      Handle         PackageList Handle
  @param[in,out]  FormSetGuid    On input, GUID or class GUID of a formset. If not
                                 specified (NULL or zero GUID), take the first
                                 FormSet with class GUID EFI_HII_PLATFORM_SETUP_FORMSET_GUID
                                 found in package list.
                                 On output, GUID of the formset found(if not NULL).
  @param[out]     FormSet        FormSet data structure.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The specified FormSet could not be found.

**/
EFI_STATUS
CreateFormSetFromHiiHandle (
  IN     EFI_HII_HANDLE  Handle,
  IN OUT EFI_GUID        *FormSetGuid,
  OUT HII_FORMSET        *FormSet
  );

/**
  Initialize a Formset and get current setting for Questions.

  @param[in,out]  FormSet                FormSet data structure.

**/
VOID
InitializeFormSet (
  IN OUT HII_FORMSET  *FormSet
  );

/**
  Free resources allocated for a FormSet.

  @param[in,out]  FormSet                Pointer of the FormSet

**/
VOID
DestroyFormSet (
  IN OUT HII_FORMSET  *FormSet
  );

/**
  Save Question Value to the memory, but not to storage.

  @param[in]     FormSet                FormSet data structure.
  @param[in]     Form                   Form data structure.
  @param[in,out] Question               Pointer to the Question.
  @param[in]     QuestionValue          New Question Value to be set.

  @retval EFI_SUCCESS            The question value has been set successfully.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.

**/
EFI_STATUS
SetQuestionValue (
  IN     HII_FORMSET          *FormSet,
  IN     HII_FORM             *Form,
  IN OUT HII_STATEMENT        *Question,
  IN     HII_STATEMENT_VALUE  *QuestionValue
  );

/**
  Get Question's current Value from storage.

  @param[in]     FormSet                FormSet data structure.
  @param[in]     Form                   Form data structure.
  @param[in,out] Question               Question to be initialized.

  @return the current Question Value in storage if success.
  @return NULL if Question is not found or any error occurs.

**/
HII_STATEMENT_VALUE *
RetrieveQuestion (
  IN     HII_FORMSET    *FormSet,
  IN     HII_FORM       *Form,
  IN OUT HII_STATEMENT  *Question
  );

/**
  Get Question's current Value.

  @param[in]   FormSet                FormSet data structure.
  @param[in]   Form                   Form data structure.
  @param[out]  Question               Question to be initialized.
  @param[in]   GetValueFrom           Where to get value, may from editbuffer, buffer or hii driver.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_INVALID_PARAMETER  Formset, Form or Question is NULL.

**/
EFI_STATUS
GetQuestionValue (
  IN HII_FORMSET                  *FormSet,
  IN HII_FORM                     *Form,
  IN OUT HII_STATEMENT            *Question,
  IN GET_SET_QUESTION_VALUE_WITH  GetValueFrom
  );

/**
  Submit data for a form.

  @param[in]  FormSet                FormSet which contains the Form.
  @param[in]  Form                   Form to submit.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval Others                 Other errors occur.

**/
EFI_STATUS
SubmitForm (
  IN     HII_FORMSET  *FormSet,
  IN     HII_FORM     *Form
  );

/**
  Evaluate the result of a HII expression.

  If Expression is NULL, then ASSERT.

  @param[in]     FormSet                FormSet associated with this expression.
  @param[in]     Form                   Form associated with this expression.
  @param[in,out] Expression             Expression to be evaluated.

  @retval EFI_SUCCESS            The expression evaluated successfully
  @retval EFI_NOT_FOUND          The Question which referenced by a QuestionId
                                 could not be found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to grow the
                                 stack.
  @retval EFI_ACCESS_DENIED      The pop operation underflowed the stack
  @retval EFI_INVALID_PARAMETER  Syntax error with the Expression

**/
EFI_STATUS
EvaluateHiiExpression (
  IN     HII_FORMSET     *FormSet,
  IN     HII_FORM        *Form,
  IN OUT HII_EXPRESSION  *Expression
  );

/**
  Retrieve dependencies within an expression. These dependencies can express how
  this expression will be evaluated.

  @param[in]  Expression             Expression to retrieve dependencies.

  @retval EFI_SUCCESS            The dependencies were successfully retrieved.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory.

**/
EFI_STATUS
GetHiiExpressionDependency (
  IN HII_EXPRESSION  *Expression
  );

/**
  Get default value of question.

  @param[in]  FormSet                The form set.
  @param[in]  Form                   The form.
  @param[in]  Question               The question.
  @param[in]  DefaultId              The Class of the default.
  @param[out] DefaultValue           The default value of given question.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetQuestionDefault (
  IN HII_FORMSET           *FormSet,
  IN HII_FORM              *Form,
  IN HII_STATEMENT         *Question,
  IN UINT16                DefaultId,
  OUT HII_STATEMENT_VALUE  *DefaultValue
  );

/**
  Return the result of the expression list. Check the expression list and
  return the highest priority express result.
  Priority: DisableIf > SuppressIf > GrayOutIf > FALSE

  @param[in]  ExpList         The input expression list.
  @param[in]  Evaluate        Whether need to evaluate the expression first.
  @param[in]  FormSet         FormSet associated with this expression.
  @param[in]  Form            Form associated with this expression.

  @retval EXPRESS_RESULT      Return the higher priority express result.
                              DisableIf > SuppressIf > GrayOutIf > FALSE

**/
EXPRESS_RESULT
EvaluateExpressionList (
  IN HII_EXPRESSION_LIST  *ExpList,
  IN BOOLEAN              Evaluate,
  IN HII_FORMSET          *FormSet OPTIONAL,
  IN HII_FORM             *Form OPTIONAL
  );

#endif // HII_UTILITY_LIB_
