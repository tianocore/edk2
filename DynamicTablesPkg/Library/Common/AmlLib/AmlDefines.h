/** @file
  AML Defines.

  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_DEFINES_H_
#define AML_DEFINES_H_

/**
  @defgroup TreeStructures Tree structures
  @ingroup AMLLib
  @{
    The AML tree created by the AMLLib relies on enum/define values and
    structures defined here.
  @}
*/

/** AML tree node types.

  Data nodes are tagged with the data type they contain.
  Some data types cannot be used for data nodes (None, Object).
  EAmlUIntX types are converted to the EAML_NODE_DATA_TYPE enum type.
  These types are accessible externally.

  @ingroup TreeStructures
*/
typedef enum EAmlNodeDataType {
  EAmlNodeDataTypeNone = 0,         ///< EAmlNone,   not accessible.
  EAmlNodeDataTypeReserved1,        ///< EAmlUInt8,  converted to the UInt type.
  EAmlNodeDataTypeReserved2,        ///< EAmlUInt16, converted to the UInt type.
  EAmlNodeDataTypeReserved3,        ///< EAmlUInt32, converted to the UInt type.
  EAmlNodeDataTypeReserved4,        ///< EAmlUInt64, converted to the UInt type.
  EAmlNodeDataTypeReserved5,        ///< EAmlObject, not accessible.
  EAmlNodeDataTypeNameString,       ///< EAmlName, name corresponding to the
                                    ///  NameString keyword in the ACPI
                                    ///  specification. E.g.: "\_SB_.DEV0"
  EAmlNodeDataTypeString,           ///< EAmlString, NULL terminated string.
  EAmlNodeDataTypeUInt,             ///< Integer data of any length, EAmlUIntX
                                    ///  are converted to this type.
  EAmlNodeDataTypeRaw,              ///< Raw bytes contained in a buffer.
  EAmlNodeDataTypeResourceData,     ///< Resource data element.
  EAmlNodeDataTypeFieldPkgLen,      ///< FieldPkgLen data element.
                                    ///  PkgLen are usually stored as
                                    ///  part of object node structures.
                                    ///  However, they can be found
                                    ///  standalone in a FieldList.
  EAmlNodeDataTypeMax               ///< Max enum.
} EAML_NODE_DATA_TYPE;

/** Indexes of fixed arguments.

  AML objects defined the ACPI 6.3 specification,
  s20.3 "AML Byte Stream Byte Values" can have at most 6 fixed arguments.

  Method and functions can have at most 7 arguments, cf
  s19.6.83 "Method (Declare Control Method)". The enum goes to 8 to store the
  name of the method invocation.

  @ingroup TreeStructures
*/
typedef enum EAmlParseIndex {
  EAmlParseIndexTerm0  = 0,     ///< First fixed argument index.
  EAmlParseIndexTerm1,          ///< Second fixed argument index.
  EAmlParseIndexTerm2,          ///< Third fixed argument index.
  EAmlParseIndexTerm3,          ///< Fourth fixed argument index.
  EAmlParseIndexTerm4,          ///< Fifth fixed argument index.
  EAmlParseIndexTerm5,          ///< Sixth fixed argument index.
  EAmlParseIndexMax             ///< Maximum fixed argument index (=6).
} EAML_PARSE_INDEX;

/** Maximum size of an AML NameString.

 An AML NameString can be at most (255 * 4) + 255 + 2 = 1277 bytes long.
 Indeed, according to ACPI 6.3 specification, s20.2.2,
 an AML NameString can be resolved as a MultiNamePath.

 The encoding of this MultiNamePath can be made of at most:
  - 255 carets ('^'), one for each level in the namespace;
  - 255 NameSeg of 4 bytes;
  - 2 bytes for the MultiNamePrefix and SegCount.

  @ingroup TreeStructures
*/
#define MAX_AML_NAMESTRING_SIZE       1277U

/** Maximum size of an ASL NameString.

 An ASL NameString can be at most (255 * 4) + 255 + 254 = 1529 bytes long.
 Cf the ASL grammar available in ACPI 6.3 specification, 19.2.2.

 The encoding of an ASL NameString can be made of at most:
  - 255 carets ('^'), one for each level in the namespace;
  - 255 NameSeg of 4 bytes;
  - 254 NameSeg separators ('.').

  @ingroup TreeStructures
*/
#define MAX_ASL_NAMESTRING_SIZE       1529U

/** Pseudo OpCode for method invocations.

  The AML grammar does not attribute an OpCode/SubOpCode couple for
  method invocations. This library is representing method invocations
  as if they had one.

  The AML encoding for method invocations in the ACPI specification 6.3 is:
    MethodInvocation := NameString TermArgList
  In this library, it is:
    MethodInvocation := MethodInvocationOp NameString ArgumentCount TermArgList
    ArgumentCount    := ByteData

  When computing the size of a tree or serializing it, the additional data is
  not taken into account (i.e. the MethodInvocationOp and the ArgumentCount).

  @ingroup TreeStructures
*/
#define AML_METHOD_INVOC_OP           0xD0

/** Pseudo OpCode for NamedField field elements.

  The AML grammar does not attribute an OpCode/SubOpCode couple for
  the NamedField field element. This library is representing NamedField field
  elements as if they had one.

  The AML encoding for NamedField field elements in the ACPI specification 6.3
  is:
    NamedField      := NameSeg PkgLength
  In this library, it is:
    NamedField      := NamedFieldOp NameSeg PkgLength

  When computing the size of a tree or serializing it, the additional data is
  not taken into account (i.e. the NamedFieldOp).

  @ingroup TreeStructures
*/
#define AML_FIELD_NAMED_OP            0x04

/** AML object types.

  The ACPI specification defines several object types. They are listed
  with the definition of ObjectTypeKeyword.

  @ingroup TreeStructures
*/
typedef enum EAmlObjType {
  EAmlObjTypeUnknown        = 0x0,
  EAmlObjTypeInt,
  EAmlObjTypeStrObj,
  EAmlObjTypeBuffObj,
  EAmlObjTypePkgObj,
  EAmlObjTypeFieldUnitObj,
  EAmlObjTypeDeviceObj,
  EAmlObjTypeEventObj,
  EAmlObjTypeMethodObj,
  EAmlObjTypeMutexObj,
  EAmlObjTypeOpRegionObj,
  EAmlObjTypePowerResObj,
  EAmlObjTypeProcessorObj,
  EAmlObjTypeThermalZoneObj,
  EAmlObjTypeBuffFieldObj,
  EAmlObjTypeDDBHandleObj,
} EAML_OBJ_TYPE;

/** Node types.

  @ingroup TreeStructures
*/
typedef enum EAmlNodeType {
  EAmlNodeUnknown,  ///< Unknown/Invalid AML Node Type.
  EAmlNodeRoot,     ///< AML Root Node, typically represents a DefinitionBlock.
  EAmlNodeObject,   ///< AML Object Node, typically represents an ASL statement
                    ///  or its arguments.
  EAmlNodeData,     ///< AML Data Node, typically represents arguments for an
                    ///  ASL statement.
  EAmlNodeMax       ///< Max enum.
} EAML_NODE_TYPE;

#endif // AML_DEFINES_H_
