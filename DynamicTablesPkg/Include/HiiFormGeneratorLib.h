/** @file
  Functions and Data structures for building Formsets.

  This file contains data structures and function
  prototypes that are needed for generating Formsets
  and it's underlying nodes dynamically.

  Copyright (c) 2026, ARM Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <Base.h>
#include <Uefi.h>
#include <Protocol/DisplayProtocol.h>
#include <Protocol/HiiDatabase.h>
#include <Uefi/UefiBaseType.h>
#include <Uefi/UefiInternalFormRepresentation.h>

/** The max value is as per the description in the UEFI specification,
    version 2.11, section 33.3.8.3.26.
*/
#define MAX_FORMSET_CLASS_GUID  3

/** The max length of an HII opcode.
*/
#define MAX_HII_OPCODE_LENGTH  128

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

/** The DYN_HII_QUESTION_TYPE enum object defines the
    different Question types that can put put in the
    Formset.
*/
typedef enum {
  DynHiiQtAction = 1,
  DynHiiQtCheckbox,
  DynHiiQtNumeric,
  DynHiiQtOneOf,
  DynHiiQtOrderedList,
  DynHiiQtPassword,
  DynHiiQtRef,
  DynHiiQtString,
  DynHiiQtText,
  DynHiiQtMax
} DYN_HII_QUESTION_TYPE;

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
  DynHiiRefOp = 1,
  DynHiiRef2Op,
  DynHiiRef3Op,
  DynHiiRef4Op,
  DynHiiRef5Op,
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
  EFI_STRING_ID      Name;
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
  /// Checkbox Question
  UINT8    Flags;
} DYN_HII_CHECKBOX_DATA;

/** A structure for Numeric Question
    specific information.
*/
typedef struct {
  /// The Flags associated with the
  /// Numeric Question
  UINT8     Flags;

  /// Minimum value allowed for the
  /// Question
  UINT64    MinValue;

  /// Maximum value allowed for the
  /// Question
  UINT64    MaxValue;

  /// The step size in which the Question's
  /// value can be incremented/decremented
  UINT64    Step;
} DYN_HII_NUMERIC_DATA;

/** A structure for Cross Reference Question
    specific information.
*/
typedef struct {
  /// Type of Ref Question
  /// (Ref1/Ref2/Ref3/Ref4/Ref5)
  DYN_HII_REF_QUESTION_TYPE    RefType;

  /// Target Form ID to which the Question is
  /// referring
  EFI_FORM_ID                  FormId;

  /// Target Question ID to which the Question
  /// is referring
  EFI_QUESTION_ID              QuestionId;

  /// Target Formset to which the Question is
  /// referring
  EFI_GUID                     FormSetGuid;

  /// Target Device Path to which the Question is
  /// referring
  EFI_STRING_ID                DevicePath;
} DYN_HII_REF_DATA;

/** A structure for OneOf Question
    specific information.
*/
typedef struct {
  /// The Flags associated with the
  /// OneOf Question
  UINT8     Flags;

  /// Minimum value allowed for the
  /// Question
  UINT64    MinValue;

  /// Maximum value allowed for the
  /// Question
  UINT64    MaxValue;

  /// The step size in which the Question's
  /// value can be incremented/decremented
  UINT64    Step;
} DYN_HII_ONE_OF_DATA;

/** A union for holding data for Question
    data types.
*/
typedef union {
  /// Checkbox Question type Data
  DYN_HII_CHECKBOX_DATA    Checkbox;

  /// Numeric Question type Data
  DYN_HII_NUMERIC_DATA     Numeric;

  /// Cross Reference Question type Data
  DYN_HII_REF_DATA         Ref;

  /// One-Of Question type Data
  DYN_HII_ONE_OF_DATA      OneOf;
} DYN_HII_QUESTION_PAYLOAD;

/** A structure for holding all data associated
    with a Question.
*/
typedef struct {
  /// A Header for holding Question metadata
  EFI_IFR_QUESTION_HEADER     QuestionHdr;

  /// Payload of the Question depending on the
  /// question type
  DYN_HII_QUESTION_PAYLOAD    Question;
} DYN_HII_QUESTION_DATA;

/** A structure for holding information associated
    with an OneOf Option for a Question.
*/
typedef struct {
  /// Default node header
  DYN_HII_NODE_HDR      Hdr;

  /// String ID for the Option text
  EFI_STRING_ID         Text;

  /// Option Flags
  UINT8                 Flags;

  /// Type of Option
  UINT8                 Type;

  /// Option Value, governed by the Option Type
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

  /// Type of data in the Value field
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
  DYN_HII_NODE_HDR         Hdr;

  /// Type of Question
  DYN_HII_QUESTION_TYPE    QuestionType;

  /// Question Data
  DYN_HII_QUESTION_DATA    Data;

  /// List of options for the Question
  LIST_ENTRY               OptionList;

  /// List of default values for the Question
  LIST_ENTRY               DefaultList;
} DYN_HII_STATEMENT;

/** A structure for holding information associated
    with a Form.
*/
typedef struct {
  /// Form node header
  DYN_HII_NODE_HDR    Hdr;

  /// Form Id for the form
  EFI_FORM_ID         FormId;

  /// Form Title String ID
  EFI_STRING_ID       Title;

  /// List of Statements/Questions in
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

  /// Formset Title String ID
  EFI_STRING_ID       Title;

  /// Formset Help String ID
  EFI_STRING_ID       Help;

  /// Number of Class identifier GUID's
  /// in the Formset
  UINT8               ClassGuidCount;

  /// Class Identifier GUID array
  EFI_GUID            ClassGuid[MAX_FORMSET_CLASS_GUID];

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

/** Create a HII Formset.

  Create a HII Formset under which other HII nodes can
  be added subsequently.

  @param [in]  FormsetGuid      Formset GUID
  @param [in]  ClassGuid        Pointer to an array of Class GUIDs, if any.
  @param [in]  Title            String ID to the Formset title text.
  @param [in]  Help             String ID to the Formset help text.
  @param [in]  ClassGuidCount   Number of Class GUIDs(can be 0 - 3).
  @param [out] Formset          The Formset object created by the function.


  @retval  EFI_SUCCESS            The Formset was created successfully.
  @retval  EFI_INVALID_PARAMETER  The FormsetGuid or Formset is NULL, or the
                                  ClassGuidCount > 3.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiCreateFormSet (
  IN  CONST EFI_GUID   *FormsetGuid,
  IN  CONST EFI_GUID   *ClassGuid,
  IN  EFI_STRING_ID    Title,
  IN  EFI_STRING_ID    Help,
  IN  UINT8            ClassGuidCount,
  OUT DYN_HII_FORMSET  **Formset
  );

/** Add a form to a Formset.

  Create a HII Form and add it to the given Formset.

  @param [in]  Formset          Formset under which Form is to be added.
  @param [in]  FormId           Form Identifier.
  @param [in]  Title            String ID to the Form's title text.

  @retval  EFI_SUCCESS            The Form was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_ALREADY_STARTED    The Form has already been added to the Formset.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddForm (
  IN  DYN_HII_FORMSET  *Formset,
  IN  EFI_FORM_ID      FormId,
  IN  EFI_STRING_ID    Title
  );

/** Add a Statement/Question to a Form.

  Create a HII Statement/Question and add it to the given Form.

  @param [in]  Form             Form under which Statement is to be added.
  @param [in]  QuestionType     Type of Question to be added.
  @param [in]  QuestionData     Data of the corresponding Statement to be added.

  @retval  EFI_SUCCESS            The Statement was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Formset is NULL.
  @retval  EFI_ALREADY_STARTED    The Form has already been added to the Formset.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddStatement (
  IN  DYN_HII_FORM           *Form,
  IN  DYN_HII_QUESTION_TYPE  QuestionType,
  IN  DYN_HII_QUESTION_DATA  *QuestionData
  );

/** Add an Option to a Question.

  Create a HII Option and add it to the given Statement.

  @param [in]  Statement        Question/Statement under which Option is to be
                                added.
  @param [in]  Text             The String ID for the option description.
  @param [in]  Flags            Flags associated with the option.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Option.

  @retval  EFI_SUCCESS            The Option was created successfully.
  @retval  EFI_INVALID_PARAMETER  The any input parameter is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddOption (
  IN  DYN_HII_STATEMENT   *Statement,
  IN  EFI_STRING_ID       Text,
  IN  UINT8               Flags,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  );

/** Add a Default to a Question.

  Create a HII Default and add it to the given Statement.

  @param [in]  Form             Question/Statement under which Default is to be
                                added.
  @param [in]  DefaultId        The Default Store ID for this Default.
  @param [in]  Type             Type of data in the value field.
  @param [in]  Value            Value of the Default.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_INVALID_PARAMETER  The Statement is NULL.
  @retval  EFI_ALREADY_STARTED    The Default for the corresponding DefaultId
                                  has already been added for the Statement.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddDefault (
  IN  DYN_HII_STATEMENT   *Statement,
  IN  UINT16              DefaultId,
  IN  UINT8               Type,
  IN  EFI_IFR_TYPE_VALUE  Value
  );

/** Add a Variable Store (Varstore) under a Formset.

  Create a HII Varstore and add it to the given Formset.

  @param [in]  Formset          Formset under which the Varstore is to be added.
  @param [in]  VarstoreType     Type of Varstore being added.
  @param [in]  Data             Data of the corresponding Varstore to be added.

  @retval  EFI_SUCCESS            The varstore was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  passed through Data is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiAddFormsetVarStore (
  IN  DYN_HII_FORMSET        *Formset,
  IN  DYN_HII_VARSTORE_TYPE  VarstoreType,
  IN  DYN_HII_VARSTORE_DATA  *Data
  );

/** Generate the IFR byte-stream for the Formset.

  Serialize the Formset and it's child nodes to generate an IFR byte-stream
  based Form package. This can then be added to the HII database through the
  HiiAddPackages() call.

  @param [in]  Formset          The formset hierarchy that needs to be
                                serialized.
  @param [out] IfrBuf           Pointer to the IFR buffer.

  @retval  EFI_SUCCESS            The Default was created successfully.
  @retval  EFI_UNSUPPORTED        Adding Varstore of the given type is currently
                                  not supported.
  @retval  EFI_INVALID_PARAMETER  The VarstoreType, or any other input parameter
                                  passed through Data is invalid.
  @retval  EFI_OUT_OF_RESOURCES   Unable to allocate memory.

**/
EFI_STATUS
EFIAPI
DynHiiGenerateFormPackage (
  IN  CONST DYN_HII_FORMSET     *Formset,
  OUT       DYN_HII_IFR_BUFFER  **IfrBuf
  );

/** Free up all the memory used by a Formset hierarchy.

  Free up all the memory that had been allocated for the Formset and it's
  child nodes, including Varstores, Forms, individual Statements etc.

  @param [in]  Formset          The formset hierarchy that needs to be
                                freed up.

**/
VOID
EFIAPI
DynHiiFreeFormSet (
  IN DYN_HII_FORMSET  *Formset
  );

/** Free up all the memory used by a Form package.

  Free up all the memory that had been allocated for the Form package which
  keeps the serialized IFR byte-stream for the formset.

  @param [in]  IfrBuf           Pointer to the IFR buffer that needs to be freed.

**/
VOID
EFIAPI
DynHiiFreeFormPackage (
  IN DYN_HII_IFR_BUFFER  *IfrBuf
  );

/** Get a Form from a given formset

  Get a form from the given formset.

  @param [in]  Formset           Pointer to the formset from which the form is
                                 to be fetched.
  @param [in]  FormId            The form ID of the required form.

  @retval  A pointer to the form object or NULL if form not found.

**/
DYN_HII_FORM *
EFIAPI
DynHiiGetForm (
  IN  DYN_HII_FORMSET  *Formset,
  IN  EFI_FORM_ID      FormId
  );
