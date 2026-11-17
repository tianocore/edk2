/** @file
  Data structures for dynamically building Formsets.

  This file contains data structures and other
  related information that is needed for
  generating Formsets and it's underlying nodes
  dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
    - Dyn        - Dynamic
    - Qt         - Question Type
    - Ref        - Cross-link Reference

  @par Reference(s):
  - UEFI specification 2.11, section 33.3.8 Forms Package

**/

#pragma once

#include <Base.h>
#include <Library/HiiFormsLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** The max value is as per the description in the UEFI specification,
    version 2.11, section 33.3.8.3.26.
*/
#define MAX_FORMSET_CLASS_GUID  3

/** Valid flags for the EFI_IFR_ONE_OF_OPTION opcode
*/
#define DYN_HII_VALID_OPTION_FLAGS \
  (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG)

/** The DYN_HII_NODE_TYPE enum object defines the various
    types of nodes that can be present under a Formset.
*/
typedef enum {
  DynHiiNodeFormset = 1,
  DynHiiNodeForm,
  DynHiiNodeDefaultStore,
  DynHiiNodeVarstore,
  DynHiiNodeStatement,
  DynHiiNodeOption,
  DynHiiNodeDefault,
  DynHiiNodeString,
  DynHiiNodeCondition,
  DynHiiNodeExpression,
  DynHiiNodeValue,
  DynHiiNodeMax
} DYN_HII_NODE_TYPE;

/** The DYN_HII_STATEMENT_TYPE enum object defines the
    different Question types that can put put in the
    Formset.
*/
typedef enum {
  DynHiiStCheckbox = 1,
  DynHiiStNumeric,
  DynHiiStOneOf,
  DynHiiStRef,
  DynHiiStSubtitle,
  DynHiiStOrderedList,
  DynHiiStAction,
  DynHiiStString,
  DynHiiStText,
  DynHiiStPassword,
  DynHiiStMax
} DYN_HII_STATEMENT_TYPE;

/** The DYN_HII_VARSTORE_TYPE enum type defines the different
    types of Variable Stores that can be defined in the
    Formset.
*/
typedef enum {
  DynHiiVarstoreBuffer = 1,
  DynHiiVarstoreEfi,
  DynHiiVarstoreNameValue,
  DynHiiVarstoreMax
} DYN_HII_VARSTORE_TYPE;

/** The DYN_HII_REF_QUESTION_TYPE enum type defines the different
    type of allowed cross-reference Question variants.

    There are five variants of the cross-reference question type,
    as described in the UEFI 2.11 specification, section
    33.3.8.3.59
*/
typedef enum {
  DynHiiRef1 = 1,
  DynHiiRef2,
  DynHiiRef3,
  DynHiiRef4,
  DynHiiRef5,
  DynHiiRefMax
} DYN_HII_REF_QUESTION_TYPE;

/** The DYN_HII_DEFAULT_TYPE enum type defines the type
    of default value representation used by a default node.
*/
typedef enum {
  DynHiiDefaultLiteral = 1,
  DynHiiDefaultExpression,
  DynHiiDefaultMax
} DYN_HII_DEFAULT_TYPE;

/** The DYN_HII_CONDITION_TYPE enum type defines scoped
    conditional IFR opcodes.
*/
typedef enum {
  DynHiiConditionSuppressIf = 1,
  DynHiiConditionGrayOutIf,
  DynHiiConditionDisableIf,
  DynHiiConditionMax
} DYN_HII_CONDITION_TYPE;

/** The DYN_HII_EXPRESSION_TYPE enum type defines expression
    opcodes that can be emitted under a condition node.
*/
typedef enum {
  DynHiiExprQuestionRef1 = 1,
  DynHiiExprEqIdVal,
  DynHiiExprEqIdValList,
  DynHiiExprEqual,
  DynHiiExprGreaterThan,
  DynHiiExprAnd,
  DynHiiExprOr,
  DynHiiExprNot,
  DynHiiExprBitwiseAnd,
  DynHiiExprTrue,
  DynHiiExprUint64,
  DynHiiExprMax
} DYN_HII_EXPRESSION_TYPE;

/** Mask for valid numeric flags */
#define NUMERIC_FLAGS_MASK  (UINT8)0x33

/** A header for all the nodes that form
    part of the formset.
*/
typedef struct {
  /// A link to another node
  LIST_ENTRY        Link;

  /// Type of the node. Refer DYN_HII_NODE_TYPE
  UINT16            Type;

  /// Whether the node opens a scope. This
  /// indicates that the node has another
  /// node as it's child
  BOOLEAN           Scope;

  /// Pointer to the formset, which is the
  /// root node of the tree
  DYN_HII_HANDLE    FormsetHandle;
} DYN_HII_NODE_HDR;

/** A structure for Buffer type
    variable store (Varstore).
*/
typedef struct {
  /// The Varstore GUID
  EFI_GUID           Guid;

  /// The Varstore ID associated
  /// with this varstore. Unique
  /// within a formset.
  EFI_VARSTORE_ID    VarstoreId;

  /// Size of the varstore
  UINT16             Size;

  /// Varstore Name as an Ascii string
  CHAR8              *Name;
} DYN_HII_VARSTORE_BUFFER_DATA;

/** A structure for EFI type
    variable store (Varstore).
*/
typedef struct {
  /// The Varstore GUID
  EFI_GUID           Guid;

  /// The Varstore ID associated
  /// with this varstore. Unique
  /// within a formset.
  EFI_VARSTORE_ID    VarstoreId;

  /// Flags for the varstore
  UINT32             Attributes;

  /// Size of the varstore
  UINT16             Size;

  /// Varstore Name as an Ascii string
  CHAR8              *Name;
} DYN_HII_VARSTORE_EFI_DATA;

/** A structure for name-value
    type variable store (Varstore).
*/
typedef struct {
  /// The Varstore GUID
  EFI_GUID           Guid;

  /// The Varstore ID associated
  /// with this varstore. Unique
  /// within a formset.
  EFI_VARSTORE_ID    VarstoreId;
} DYN_HII_VARSTORE_NAME_VALUE_DATA;

/** A union that can hold any type of
    Varstore data type.
*/
typedef union {
  /// A buffer type varstore
  DYN_HII_VARSTORE_BUFFER_DATA        Buffer;

  /// An EFI variable type varstore
  DYN_HII_VARSTORE_EFI_DATA           Efi;

  /// A name-value type varstore
  DYN_HII_VARSTORE_NAME_VALUE_DATA    NameValue;
} DYN_HII_VARSTORE_DATA;

/** A structure to hold all the relevant Varstore
    information.
*/
typedef struct {
  /// Varstore node header
  DYN_HII_NODE_HDR         Hdr;

  /// Type of the varstore
  DYN_HII_VARSTORE_TYPE    VarstoreType;

  /// The varstore data
  DYN_HII_VARSTORE_DATA    Data;
} DYN_HII_VARSTORE;

/** A structure for holding the
    EFI_IFR_QUESTION_REF1_OP expression payload.
*/
typedef struct {
  /// Question ID referenced by the expression.
  EFI_QUESTION_ID    QuestionId;
} DYN_HII_EXPR_QUESTION_REF1_DATA;

/** A structure for holding the EFI_IFR_EQ_ID_VAL_OP
    expression payload.
*/
typedef struct {
  /// Question ID whose value is to be compared.
  EFI_QUESTION_ID    QuestionId;

  /// UINT16 comparison value.
  UINT16             Value;
} DYN_HII_EXPR_EQ_ID_VAL_DATA;

/** A structure for holding the EFI_IFR_EQ_ID_VAL_LIST_OP
    expression payload.
*/
typedef struct {
  /// Question ID whose value is to be searched in ValueList.
  EFI_QUESTION_ID    QuestionId;

  /// Number of UINT16 values in ValueList.
  UINT16             ValueCount;

  /// Dynamically allocated UINT16 value list.
  UINT16             *ValueList;
} DYN_HII_EXPR_EQ_ID_VAL_LIST_DATA;

/** A structure for holding the EFI_IFR_UINT64_OP
    expression payload.
*/
typedef struct {
  /// UINT64 literal value pushed onto the expression stack.
  UINT64    Value;
} DYN_HII_EXPR_UINT64_DATA;

/** A union for holding expression opcode payload data.
*/
typedef union {
  /// EFI_IFR_QUESTION_REF1_OP payload.
  DYN_HII_EXPR_QUESTION_REF1_DATA     QuestionRef1;

  /// EFI_IFR_EQ_ID_VAL_OP payload.
  DYN_HII_EXPR_EQ_ID_VAL_DATA         EqIdVal;

  /// EFI_IFR_EQ_ID_VAL_LIST_OP payload.
  DYN_HII_EXPR_EQ_ID_VAL_LIST_DATA    EqIdValList;

  /// EFI_IFR_UINT64_OP payload.
  DYN_HII_EXPR_UINT64_DATA            Uint64;
} DYN_HII_EXPRESSION_DATA;

/** A structure for holding expression opcode data.

  Expressions are serialized in the order in which
  they are linked into a condition node's
  ExpressionList.
*/
typedef struct {
  /// Expression node header
  DYN_HII_NODE_HDR           Hdr;

  /// Type of expression opcode
  DYN_HII_EXPRESSION_TYPE    ExpressionType;

  /// Expression type specific payload.
  DYN_HII_EXPRESSION_DATA    Data;
} DYN_HII_EXPRESSION;

/** A structure for scoped condition node.

  Conditions will have expression(s) under it,
  followed by other statements/forms/options
  etc.
*/
typedef struct {
  /// Condition node header
  DYN_HII_NODE_HDR          Hdr;

  /// Type of conditional opcode to emit
  DYN_HII_CONDITION_TYPE    ConditionType;

  /// Count of number of expressions under
  /// the condition
  UINT32                    ExpressionCount;

  /// Parent node under which this condition
  /// is attached.
  DYN_HII_HANDLE            ParentNode;

  /// Expression opcodes controlled by this
  /// condition
  LIST_ENTRY                ExpressionList;

  /// Statements or nested condition nodes
  /// under this condition
  LIST_ENTRY                ChildList;
} DYN_HII_CONDITION;

/** A structure for Checkbox Question
    specific information.
*/
typedef struct {
  /// The Flags associated with the
  /// checkbox question
  UINT8    Flags;
} DYN_HII_CHECKBOX_DATA;

/** A structure for Numeric Question
    specific information.
*/
typedef struct {
  /// The flags associated with the
  /// numeric question
  UINT8     Flags;

  /// Minimum value allowed for the
  /// question
  UINT64    MinValue;

  /// Maximum value allowed for the
  /// question
  UINT64    MaxValue;

  /// The step size in which the question's
  /// value can be incremented/decremented
  UINT64    Step;
} DYN_HII_NUMERIC_DATA;

/** A structure for Cross Reference Question
    specific information.
*/
typedef struct {
  /// Type of Ref question
  /// (Ref1/Ref2/Ref3/Ref4/Ref5)
  DYN_HII_REF_QUESTION_TYPE    RefType;

  /// Target form ID to which the question is
  /// referring
  EFI_FORM_ID                  FormId;

  /// Target question ID to which the question
  /// is referring
  EFI_QUESTION_ID              QuestionId;

  /// Target formset to which the question is
  /// referring
  EFI_GUID                     FormSetGuid;

  /// Target device path to which the question is
  /// referring
  EFI_STRING_ID                DevicePath;
} DYN_HII_REF_DATA;

/** A structure for OneOf Question
    specific information.
*/
typedef struct {
  /// The flags associated with the
  /// one-of question
  UINT8      Flags;

  /// Minimum value allowed for the
  /// question
  UINT64     MinValue;

  /// Maximum value allowed for the
  /// question
  UINT64     MaxValue;

  /// The step size in which the question's
  /// value can be incremented/decremented
  UINT64     Step;

  /// The type/size of numeric value that is
  /// associated with this question
  UINT8      ValueType;

  /// Flag to indicate if MinValue and
  /// MaxValue have been set
  BOOLEAN    MinMaxSet;
} DYN_HII_ONE_OF_DATA;

/** A structure for OneOf Question
    specific information.
*/
typedef struct {
  /// The flags associated with the
  /// one-of question
  UINT8            Flags;

  /// Subtitle prompt string ID
  EFI_STRING_ID    Prompt;

  /// Subtitle help string ID
  EFI_STRING_ID    Help;
} DYN_HII_SUBTITLE_DATA;

/** A union for holding data for Question
    data types.
*/
typedef union {
  /// Checkbox question type data
  DYN_HII_CHECKBOX_DATA    Checkbox;

  /// Numeric question type data
  DYN_HII_NUMERIC_DATA     Numeric;

  /// Cross-reference question type data
  DYN_HII_REF_DATA         Ref;

  /// One-Of question type data
  DYN_HII_ONE_OF_DATA      OneOf;

  /// Subtitle statement type data
  DYN_HII_SUBTITLE_DATA    Subtitle;
} DYN_HII_STATEMENT_PAYLOAD;

/** A structure for holding all data associated
    with a Question.
*/
typedef struct {
  /// A header for holding Question metadata
  EFI_IFR_QUESTION_HEADER      QuestionHdr;

  /// Payload of the question depending on the
  /// question type
  DYN_HII_STATEMENT_PAYLOAD    Statement;
} DYN_HII_STATEMENT_DATA;

/** A structure for holding information associated
    with an OneOf Option for a Question.
*/
typedef struct {
  /// Default node header
  DYN_HII_NODE_HDR      Hdr;

  /// String ID for the option text
  EFI_STRING_ID         Text;

  /// Option Flags
  UINT8                 Flags;

  /// Type of option
  UINT8                 Type;

  /// Option Value, governed by the option type
  EFI_IFR_TYPE_VALUE    Value;
} DYN_HII_ONE_OF_OPTION;

/** A structure for holding information associated
    with an EFI_IFR_VALUE_OP node.

  The value node is used as an expression wrapper
  under nodes such as DYN_HII_DEFAULT when the
  value is generated from one or more expression
  opcodes.
*/
typedef struct {
  /// Value node header
  DYN_HII_NODE_HDR    Hdr;

  /// Count of expression nodes under this value
  /// node
  UINT32              ExpressionCount;

  /// Expression opcodes used to compute the value
  LIST_ENTRY          ExpressionList;
} DYN_HII_VALUE;

/** A structure for holding information associated
    with a Default for a Question.
*/
typedef struct {
  /// Default node header
  DYN_HII_NODE_HDR        Hdr;

  /// Default store associated with this
  /// default value
  UINT16                  DefaultId;

  /// Type of default value representation
  DYN_HII_DEFAULT_TYPE    DefaultType;

  /// Type of data in the Value field.
  /// This field is valid if DefaultType is
  /// DynHiiDefaultLiteral
  UINT8                   ValueType;

  /// The default value. This field is valid
  /// if DefaultType is DynHiiDefaultLiteral
  /// The default value
  EFI_IFR_TYPE_VALUE      Value;

  /// List of child nodes. This is used for
  /// expression backed defaults.
  LIST_ENTRY              ChildList;
} DYN_HII_DEFAULT;

/** A structure for holding information associated
    with a Defaultstore for the formset.
*/
typedef struct {
  /// Defaultstore node header
  DYN_HII_NODE_HDR    Hdr;

  /// String ID for the associated default
  /// name string
  EFI_STRING_ID       DefaultName;

  /// Default ID which is unique within
  /// a formset
  UINT16              DefaultId;
} DYN_HII_DEFAULTSTORE;

/** A structure for holding information associated
    with a Statement/Question.
*/
typedef struct {
  /// Statement/Question node header
  DYN_HII_NODE_HDR          Hdr;

  /// Type of question/statement
  DYN_HII_STATEMENT_TYPE    StatementType;

  /// Question/Statement data
  DYN_HII_STATEMENT_DATA    Data;

  /// List of all possible child nodes
  LIST_ENTRY                ChildList;
} DYN_HII_STATEMENT;

/** A structure for holding information associated
    with a Form.
*/
typedef struct {
  /// Form node header
  DYN_HII_NODE_HDR    Hdr;

  /// Form Id for the form
  EFI_FORM_ID         FormId;

  /// Form title string ID
  EFI_STRING_ID       Title;

  /// List of all child nodes of
  /// the form
  LIST_ENTRY          ChildList;
} DYN_HII_FORM;

/** A structure for holding information on
    currently allocated question ID's in
    the formset.
*/
typedef struct {
  /// An array of sorted question ID's that
  /// have been allocated in the formset
  EFI_QUESTION_ID    *Ids;

  /// Current count of number of question
  /// ID's that have been allocated
  UINTN              Count;

  /// Maximum capacity of the question
  /// ID array
  UINTN              Capacity;
} DYN_HII_QUESTION_ID_BUF;

/** A structure for holding information associated
    with a Formset.
*/
typedef struct {
  /// Formset node header
  DYN_HII_NODE_HDR           Hdr;

  /// Formset GUID
  EFI_GUID                   FormsetGuid;

  /// Formset title string ID
  EFI_STRING_ID              Title;

  /// Formset help string ID
  EFI_STRING_ID              Help;

  /// Number of class identifier GUID's
  /// in the formset
  UINT8                      ClassGuidCount;

  /// Class identifier GUID array
  EFI_GUID                   ClassGuid[MAX_FORMSET_CLASS_GUID];

  EFI_FORM_ID                FormIdCounter;

  /// Sorted array of question ID's that have
  /// been allocated for the formset
  DYN_HII_QUESTION_ID_BUF    QuestionIdArr;

  /// List of variable stores associated
  /// with this formset
  LIST_ENTRY                 VarstoreList;

  /// List of default stores associated
  /// with this formset
  LIST_ENTRY                 DefaultstoreList;

  /// List of child nodes associated
  /// with this formset
  LIST_ENTRY                 ChildList;
} DYN_HII_FORMSET;

/** A structure for holding information associated
    with the IFR Form package.
*/
typedef struct {
  /// Pointer to the IFR buffer
  UINT8     *Data;

  /// Size of the IFR Buffer
  UINT32    Size;

  /// Current position of the buffer
  UINT32    Position;
} DYN_HII_IFR_BUFFER;

/** Function pointer for serializing functions.
*/
typedef
EFI_STATUS
(*DYN_HII_SERIALIZE_FN)(
  IN  CONST DYN_HII_NODE_HDR    *Hdr,
  IN        DYN_HII_IFR_BUFFER  *BufInfo
  );
