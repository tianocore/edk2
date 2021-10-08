/** @file
  AML grammar definitions.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef AML_H_
#define AML_H_

#include <AmlDefines.h>
#include <AmlInclude.h>
#include <IndustryStandard/AcpiAml.h>

#if !defined (MDEPKG_NDEBUG)
#define AML_OPCODE_DEF(str, OpCode) str, OpCode
#else
#define AML_OPCODE_DEF(str, OpCode) OpCode
#endif // MDEPKG_NDEBUG

/** AML types.

  In the AML bytestream, data is represented using one of the following types.
  These types are used in the parsing logic to know what kind of data is
  expected next in the bytestream. This allows to parse data according
  to the AML_PARSE_FORMAT type.
  E.g.: A string will not be parsed in the same way as a UINT8.

  These are internal types.
*/
typedef enum EAmlParseFormat {
  EAmlNone   = 0,     ///< No data expected.
  EAmlUInt8,          ///< One byte value evaluated as a UINT8.
  EAmlUInt16,         ///< Two byte value evaluated as a UINT16.
  EAmlUInt32,         ///< Four byte value evaluated as a UINT32.
  EAmlUInt64,         ///< Eight byte value evaluated as a UINT64.
  EAmlObject,         ///< AML object, starting with an OpCode/SubOpCode
                      ///  couple, potentially followed by package length.
                      ///  EAmlName is a subtype of an EAmlObject.
                      ///  Indeed, an EAmlName can also be evaluated as
                      ///  an EAmlObject in the parsing.
  EAmlName,           ///< Name corresponding to the NameString keyword
                      ///  in the ACPI specification. E.g.: "\_SB_.DEV0"
  EAmlString,         ///< NULL terminated string.
  EAmlFieldPkgLen,    ///< A field package length (PkgLen). A data node of this
                      ///  type can only be found in a field list, in a
                      ///  NamedField statement. The PkgLen is otherwise
                      /// part of the object node structure.
  EAmlParseFormatMax  ///< Max enum.
} AML_PARSE_FORMAT;

/** AML attributes

  To add some more information to the byte encoding, it is possible to add
  these attributes.
*/
typedef UINT32 AML_OP_ATTRIBUTE;

/** A PkgLength is expected between the OpCode/SubOpCode couple and the first
    fixed argument of the object.
*/
#define AML_HAS_PKG_LENGTH      0x00001U

/** The object's OpCode is actually a character. Encodings with this attribute
    don't describe objects. The dual/multi name prefix have this attribute,
    indicating the start of a longer NameString.
*/
#define AML_IS_NAME_CHAR        0x00002U

/** A variable list of arguments is following the last fixed argument. Each
    argument is evaluated as an EAmlObject.
*/
#define AML_HAS_CHILD_OBJ       0x00004U

/** This is a sub-type of a variable list of arguments. It can only be
    found in buffer objects. A ByteList is either a list of
    bytes or a list of resource data elements. Resource data elements
    have specific opcodes.
*/
#define AML_HAS_BYTE_LIST       0x00008U

/** This is a sub-type of a variable list of arguments. It can only be
    found in Fields, IndexFields and BankFields.
    A FieldList is made of FieldElements. FieldElements have specific opcodes.
*/
#define AML_HAS_FIELD_LIST      0x00010U

/** This object node is a field element. Its opcode is to be fetched from
    the field encoding table.
*/
#define AML_IS_FIELD_ELEMENT    0x00020U

/** The object has a name and which is part of the AML namespace. The name
    can be found in the fixed argument list at the NameIndex.
*/
#define AML_IN_NAMESPACE        0x10000U

/** Some OpCodes have been created in this library. They are called
    pseudo opcodes and must stay internal to this library.
*/
#define AML_IS_PSEUDO_OPCODE    0x20000U

/** Encoding of an AML object.

  Every AML object has a specific encoding. This encoding information
  is used to parse AML objects. A table of AML_BYTE_ENCODING entries
  allows to parse an AML bytestream.
  This structure is also used to describe field objects.

  Cf. ACPI 6.3 specification, s20.2.
*/
typedef struct _AML_BYTE_ENCODING {
// Enable this field for debug.
#if !defined (MDEPKG_NDEBUG)
  /// String field allowing to print the AML object.
  CONST CHAR8         * Str;
#endif // MDEPKG_NDEBUG

  /// OpCode of the AML object.
  UINT8                 OpCode;

  /// SubOpCode of the AML object.
  /// The SubOpcode field has a valid value when the OpCode is 0x5B,
  /// otherwise this field must be zero.
  /// For field objects, the SubOpCode is not used.
  UINT8                 SubOpCode;

  /// Number of fixed arguments for the AML statement represented
  /// by the OpCode & SubOpcode.
  /// Maximum is 6 for AML objects.
  /// Maximum is 3 for field objects.
  EAML_PARSE_INDEX      MaxIndex;

  /// If the encoding has the AML_IN_NAMESPACE attribute (cf Attribute
  /// field below), indicate where to find the name in the fixed list
  /// of arguments.
  EAML_PARSE_INDEX      NameIndex;

  /// Type of each fixed argument.
  AML_PARSE_FORMAT      Format[EAmlParseIndexMax];

  /// Additional information on the AML object.
  AML_OP_ATTRIBUTE      Attribute;
} AML_BYTE_ENCODING;

/** Get the AML_BYTE_ENCODING entry in the AML encoding table.

  Note: For Pseudo OpCodes this function returns NULL.

  @param  [in]  Buffer    Pointer to an OpCode/SubOpCode couple.
                          If *Buffer = 0x5b (extended OpCode),
                          Buffer must be at least two bytes long.

  @return The corresponding AML_BYTE_ENCODING entry.
          NULL if not found.
**/
CONST
AML_BYTE_ENCODING *
EFIAPI
AmlGetByteEncoding (
  IN  CONST UINT8   * Buffer
  );

/** Get the AML_BYTE_ENCODING entry in the AML encoding table
    by providing an OpCode/SubOpCode couple.

  @param  [in]  OpCode     OpCode.
  @param  [in]  SubOpCode  SubOpCode.

  @return The corresponding AML_BYTE_ENCODING entry.
          NULL if not found.
**/
CONST
AML_BYTE_ENCODING *
EFIAPI
AmlGetByteEncodingByOpCode (
  IN  UINT8   OpCode,
  IN  UINT8   SubOpCode
  );

/** Get the AML_BYTE_ENCODING entry in the field encoding table.

  Note: For Pseudo OpCodes this function returns NULL.

  @param  [in]  Buffer  Pointer to a field OpCode.
                        No SubOpCode is expected.

  @return The corresponding AML_BYTE_ENCODING entry
          in the field encoding table.
          NULL if not found.
**/
CONST
AML_BYTE_ENCODING *
EFIAPI
AmlGetFieldEncoding (
  IN  CONST UINT8   * Buffer
  );

/** Get the AML_BYTE_ENCODING entry in the field encoding table
    by providing an OpCode/SubOpCode couple.

  @param  [in]  OpCode     OpCode.
  @param  [in]  SubOpCode  SubOpCode.

  @return The corresponding AML_BYTE_ENCODING entry
          in the field encoding table.
          NULL if not found.
**/
CONST
AML_BYTE_ENCODING *
EFIAPI
AmlGetFieldEncodingByOpCode (
  IN  UINT8   OpCode,
  IN  UINT8   SubOpCode
  );

// Enable this function for debug.
#if !defined (MDEPKG_NDEBUG)
/** Look for an OpCode/SubOpCode couple in the AML grammar,
    and return a corresponding string.

  @param  [in]  OpCode      The OpCode.
  @param  [in]  SubOpCode   The SubOpCode.

  @return A string describing the OpCode/SubOpCode couple.
          NULL if not found.
**/
CONST
CHAR8 *
AmlGetOpCodeStr (
  IN  UINT8   OpCode,
  IN  UINT8   SubOpCode
  );

/** Look for an OpCode/SubOpCode couple in the AML field element grammar,
    and return a corresponding string.

  @param  [in]  OpCode      The OpCode.
  @param  [in]  SubOpCode   The SubOpCode. Must be zero.

  @return A string describing the OpCode/SubOpCode couple.
          NULL if not found.
**/
CONST
CHAR8 *
AmlGetFieldOpCodeStr (
  IN  UINT8   OpCode,
  IN  UINT8   SubOpCode
  );
#endif // MDEPKG_NDEBUG

/** Check whether the OpCode/SubOpcode couple is a valid entry
    in the AML grammar encoding table.

  @param  [in]  OpCode     OpCode to check.
  @param  [in]  SubOpCode  SubOpCode to check.

  @retval TRUE    The OpCode/SubOpCode couple is valid.
  @retval FALSE   Otherwise.
**/
BOOLEAN
EFIAPI
AmlIsOpCodeValid (
  IN  UINT8   OpCode,
  IN  UINT8   SubOpCode
  );

/** Convert an AML_PARSE_FORMAT to its corresponding EAML_NODE_DATA_TYPE.

  @param  [in]  AmlType   Input AML Type.

  @return The corresponding EAML_NODE_DATA_TYPE.
          EAmlNodeDataTypeNone if not found.
**/
EAML_NODE_DATA_TYPE
EFIAPI
AmlTypeToNodeDataType (
  IN  AML_PARSE_FORMAT  AmlType
  );

/** Get the package length from the buffer.

  @param  [in]  Buffer      AML buffer.
  @param  [out] PkgLength   The interpreted PkgLen value.
                            Length cannot exceed 2^28.

  @return The number of bytes to represent the package length.
          0 if an issue occurred.
**/
UINT32
EFIAPI
AmlGetPkgLength (
  IN  CONST UINT8   * Buffer,
  OUT       UINT32  * PkgLength
  );

/** Convert the Length to the AML PkgLen encoding,
    then and write it in the Buffer.

  @param  [in]    Length  Length to convert.
                          Length cannot exceed 2^28.
  @param  [out]   Buffer  Write the result in this Buffer.

  @return The number of bytes used to write the Length.
**/
UINT8
EFIAPI
AmlSetPkgLength (
  IN  UINT32    Length,
  OUT UINT8   * Buffer
  );

/** Compute the number of bytes required to write a package length.

  @param  [in]  Length  The length to convert in the AML package length
                        encoding style.
                        Length cannot exceed 2^28.

  @return The number of bytes required to write the Length.
**/
UINT8
EFIAPI
AmlComputePkgLengthWidth (
  IN  UINT32  Length
  );

/** Given a length, compute the value of a PkgLen.

  In AML, some object have a PkgLen, telling the size of the AML object.
  It can be encoded in 1 to 4 bytes. The bytes used to encode the PkgLen is
  itself counted in the PkgLen value.
  This means that if an AML object sees its size increment/decrement,
  the number of bytes used to encode the PkgLen value can itself
  increment/decrement.

  For instance, the AML encoding of a DeviceOp is:
    DefDevice := DeviceOp PkgLength NameString TermList
  If:
   - sizeof (NameString) = 4 (the name is "DEV0" for instance);
   - sizeof (TermList) = (2^6-6)
  then the PkgLen is encoded on 1 byte. Indeed, its value is:
    sizeof (PkgLen) + sizeof (NameString) + sizeof (TermList) =
    sizeof (PkgLen) + 4 + (2^6-6)
  So:
    PkgLen = sizeof (PkgLen) + (2^6-2)

  The input arguments Length and PkgLen represent, for the DefDevice:
    DefDevice := DeviceOp PkgLength NameString TermList
                                    |------Length-----|
                          |--------*PgkLength---------|

  @param  [in]  Length  The length to encode as a PkgLen.
                        Length cannot exceed 2^28 - 4 (4 bytes for the
                        PkgLen encoding).
                        The size of the PkgLen encoding bytes should not be
                        counted in this length value.
  @param  [out] PkgLen  If success, contains the value of the PkgLen,
                        ready to encode in the PkgLen format.
                        This value takes into account the size of PkgLen
                        encoding.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
AmlComputePkgLength (
  IN  UINT32    Length,
  OUT UINT32  * PkgLen
  );

#endif // AML_H_

