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
**/

#pragma once

#include <Base.h>
#include <Library/HiiFormsLib.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** The max value is as per the description in the UEFI specification,
    version 2.11, section 33.3.8.3.26.
*/
#define MAX_FORMSET_CLASS_GUID  3

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
  DynHiiNodeExpression,
  DynHiiNodeMax
} DYN_HII_NODE_TYPE;

/** The DYN_HII_STATEMENT_TYPE enum object defines the
    different Question types that can put put in the
    Formset.
*/
typedef enum {
  DynHiiStAction = 1,
  DynHiiStCheckbox,
  DynHiiStNumeric,
  DynHiiStOneOf,
  DynHiiStOrderedList,
  DynHiiStPassword,
  DynHiiStRef,
  DynHiiStString,
  DynHiiStText,
  DynHiiStSubtitle,
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

/** A header for all the nodes that form
    part of the formset.
*/
typedef struct {
  /// A link to another node
  LIST_ENTRY    Link;

  /// Type of the node. Refer DYN_HII_NODE_TYPE
  UINT16        Type;

  /// Whether the node opens a scope. This
  /// indicates that the node has another
  /// node as it's child
  BOOLEAN       Scope;
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

  /// The type/size of numeric value that is
  /// associated with this question
  UINT8     ValueType;
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
    with a Default for a Question.
*/
typedef struct {
  /// Default node header
  DYN_HII_NODE_HDR      Hdr;

  /// Default store associated with this
  /// default value
  UINT16                DefaultId;

  /// Type of data in the value field
  UINT8                 Type;

  /// The default value
  EFI_IFR_TYPE_VALUE    Value;
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

  /// List of options for the question
  LIST_ENTRY                OptionList;

  /// List of default values for the question
  LIST_ENTRY                DefaultList;
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

  /// List of statements/questions in
  /// the form
  LIST_ENTRY          StatementList;
} DYN_HII_FORM;

/** A structure for holding information associated
    with a Formset.
*/
typedef struct {
  /// Formset node header
  DYN_HII_NODE_HDR    Hdr;

  /// Formset GUID
  EFI_GUID            FormsetGuid;

  /// Formset title string ID
  EFI_STRING_ID       Title;

  /// Formset help string ID
  EFI_STRING_ID       Help;

  /// Number of class identifier GUID's
  /// in the formset
  UINT8               ClassGuidCount;

  /// Class identifier GUID array
  EFI_GUID            ClassGuid[MAX_FORMSET_CLASS_GUID];

  EFI_FORM_ID         FormIdCounter;

  /// List of variable stores associated
  /// with this formset
  LIST_ENTRY          VarstoreList;

  /// List of default stores associated
  /// with this formset
  LIST_ENTRY          DefaultstoreList;

  /// List of forms associated with this
  /// formset
  LIST_ENTRY          FormList;
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
