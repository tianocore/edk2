/** @file
  AML grammar definitions.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AmlEncoding/Aml.h>

/** AML grammar encoding table.

  The ASL language is a description language, used to define abstract
  objects, like devices, thermal zones, etc. and their place in a hierarchical
  tree. The following table stores the AML grammar definition. It can be used
  to parse an AML bytestream. Each line corresponds to the definition of an
  opcode and what is expected to be found with this opcode.
  See table 20-440 in the ACPI 6.3 specification s20.3, and the AML
  grammar definitions in s20.2.

  - OpCode/SubOpCode:
  An OpCode/SubOpCode couple allows to identify an object type.
  The OpCode and SubOpCode are one byte each. The SubOpCode is
  used when the Opcode value is 0x5B (extended OpCode). Otherwise
  the SubOpcode is set to 0. If the SubOpCode is 0 in the table
  below, there is no SubOpCode in the AML bytestream, only the
  OpCode is used to identify the object.

  - Fixed arguments:
  The fixed arguments follow the OpCode and SubOpCode. Their number
  and type can be found in the table below. There can be at the most
  6 fixed arguments for an object.
  Fixed arguments's type allow to know what is expected in the AML bytestream.
  Knowing the size of the incoming element, AML bytes can be packed and parsed
  accordingly. These types can be found in the same table 20-440 in the
  ACPI 6.3, s20.3 specification.
  E.g.: An AML object, a UINT8, a NULL terminated string, etc.

  -Attributes:
  The attribute field gives additional information on each object. This can
  be the presence of a variable list of arguments, the presence of a PkgLen,
  etc.

  In summary, an AML object is described as:
    OpCode [SubOpcode] [PkgLen] [FixedArgs] [VarArgs]

  OpCode                        {1 byte}
  [SubOpCode]                   {1 byte.
                                 Only relevant if the OpCode value is
                                 0x5B (extended OpCode prefix).
                                 Otherwise 0. Most objects don't have one.}
  [PkgLen]                      {Size of the object.
                                 It has a special encoding, cf. ACPI 6.3
                                 specification, s20.2.4 "Package Length
                                 Encoding".
                                 Most objects don't have one.}
  [FixedArgs[0..X]]             {Fixed list of arguments.
    (where X <= 5)               Can be other objects or data (a byte,
                                 a string, etc.). They belong to the
                                 current AML object.
                                 The number of fixed arguments varies according
                                 to the object, but it is fixed for each kind of
                                 object.}
  [VarArgs]                     {Variable list of arguments.
                                 They also belong to the current object and can
                                 be objects or data.
                                 Most objects don't have one.}
    [ByteList]                  {This is a sub-type of a variable list of
                                 arguments. It can only be found in buffer
                                 objects.
                                 A ByteList is either a list of bytes or
                                 a list of resource data elements. Resource
                                 data elements have specific opcodes.}
    [FieldList]                 {This is a sub-type of a variable list of
                                 arguments. It can only be found in Fields,
                                 IndexFields and BankFields.
                                 A FieldList is made of FieldElements.
                                 FieldElements have specific opcodes.}
*/
GLOBAL_REMOVE_IF_UNREFERENCED
STATIC
CONST
AML_BYTE_ENCODING mAmlByteEncoding[] = {
  // Comment                       Str                    OpCode                     SubOpCode               MaxIndex  NameIndex   0                 1                 2                 3                 4                 5                 Attribute
  /* 0x00 */      {AML_OPCODE_DEF ("ZeroOp",              AML_ZERO_OP),              0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x01 */      {AML_OPCODE_DEF ("OneOp",               AML_ONE_OP),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x06 */      {AML_OPCODE_DEF ("AliasOp",             AML_ALIAS_OP),             0,                      2,        1,         {EAmlName,         EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x08 */      {AML_OPCODE_DEF ("NameOp",              AML_NAME_OP),              0,                      2,        0,         {EAmlName,         EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x0A */      {AML_OPCODE_DEF ("BytePrefix",          AML_BYTE_PREFIX),          0,                      1,        0,         {EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0B */      {AML_OPCODE_DEF ("WordPrefix",          AML_WORD_PREFIX),          0,                      1,        0,         {EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0C */      {AML_OPCODE_DEF ("DWordPrefix",         AML_DWORD_PREFIX),         0,                      1,        0,         {EAmlUInt32,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0D */      {AML_OPCODE_DEF ("StringPrefix",        AML_STRING_PREFIX),        0,                      1,        0,         {EAmlString,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0E */      {AML_OPCODE_DEF ("QWordPrefix",         AML_QWORD_PREFIX),         0,                      1,        0,         {EAmlUInt64,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x10 */      {AML_OPCODE_DEF ("ScopeOp",             AML_SCOPE_OP),             0,                      1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x11 */      {AML_OPCODE_DEF ("BufferOp",            AML_BUFFER_OP),            0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_BYTE_LIST},
  /* 0x12 */      {AML_OPCODE_DEF ("PackageOp",           AML_PACKAGE_OP),           0,                      1,        0,         {EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0x13 */      {AML_OPCODE_DEF ("VarPackageOp",        AML_VAR_PACKAGE_OP),       0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0x14 */      {AML_OPCODE_DEF ("MethodOp",            AML_METHOD_OP),            0,                      2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x15 */      {AML_OPCODE_DEF ("ExternalOp",          AML_EXTERNAL_OP),          0,                      3,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x2E */      {AML_OPCODE_DEF ("DualNamePrefix",      AML_DUAL_NAME_PREFIX),     0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x2F */      {AML_OPCODE_DEF ("MultiNamePrefix",     AML_MULTI_NAME_PREFIX),    0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x41 */      {AML_OPCODE_DEF ("NameChar_A",          'A'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x42 */      {AML_OPCODE_DEF ("NameChar_B",          'B'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x43 */      {AML_OPCODE_DEF ("NameChar_C",          'C'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x44 */      {AML_OPCODE_DEF ("NameChar_D",          'D'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x45 */      {AML_OPCODE_DEF ("NameChar_E",          'E'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x46 */      {AML_OPCODE_DEF ("NameChar_F",          'F'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x47 */      {AML_OPCODE_DEF ("NameChar_G",          'G'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x48 */      {AML_OPCODE_DEF ("NameChar_H",          'H'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x49 */      {AML_OPCODE_DEF ("NameChar_I",          'I'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4A */      {AML_OPCODE_DEF ("NameChar_J",          'J'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4B */      {AML_OPCODE_DEF ("NameChar_K",          'K'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4C */      {AML_OPCODE_DEF ("NameChar_L",          'L'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4D */      {AML_OPCODE_DEF ("NameChar_M",          'M'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4E */      {AML_OPCODE_DEF ("NameChar_N",          'N'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4F */      {AML_OPCODE_DEF ("NameChar_O",          'O'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x50 */      {AML_OPCODE_DEF ("NameChar_P",          'P'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x51 */      {AML_OPCODE_DEF ("NameChar_Q",          'Q'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x52 */      {AML_OPCODE_DEF ("NameChar_R",          'R'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x53 */      {AML_OPCODE_DEF ("NameChar_S",          'S'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x54 */      {AML_OPCODE_DEF ("NameChar_T",          'T'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x55 */      {AML_OPCODE_DEF ("NameChar_U",          'U'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x56 */      {AML_OPCODE_DEF ("NameChar_V",          'V'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x57 */      {AML_OPCODE_DEF ("NameChar_W",          'W'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x58 */      {AML_OPCODE_DEF ("NameChar_X",          'X'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x59 */      {AML_OPCODE_DEF ("NameChar_Y",          'Y'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5A */      {AML_OPCODE_DEF ("NameChar_Z",          'Z'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5B 0x01 */ {AML_OPCODE_DEF ("MutexOp",             AML_EXT_OP),               AML_EXT_MUTEX_OP,       2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x02 */ {AML_OPCODE_DEF ("EventOp",             AML_EXT_OP),               AML_EXT_EVENT_OP,       1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x12 */ {AML_OPCODE_DEF ("CondRefOfOp",         AML_EXT_OP),               AML_EXT_COND_REF_OF_OP, 2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x13 */ {AML_OPCODE_DEF ("CreateFieldOp",       AML_EXT_OP),               AML_EXT_CREATE_FIELD_OP,4,        3,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x1F */ {AML_OPCODE_DEF ("LoadTableOp",         AML_EXT_OP),               AML_EXT_LOAD_TABLE_OP,  6,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject},      0},
  /* 0x5B 0x20 */ {AML_OPCODE_DEF ("LoadOp",              AML_EXT_OP),               AML_EXT_LOAD_OP,        2,        0,         {EAmlName,         EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x21 */ {AML_OPCODE_DEF ("StallOp",             AML_EXT_OP),               AML_EXT_STALL_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x22 */ {AML_OPCODE_DEF ("SleepOp",             AML_EXT_OP),               AML_EXT_SLEEP_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x23 */ {AML_OPCODE_DEF ("AcquireOp",           AML_EXT_OP),               AML_EXT_ACQUIRE_OP,     2,        0,         {EAmlObject,       EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x24 */ {AML_OPCODE_DEF ("SignalOp",            AML_EXT_OP),               AML_EXT_SIGNAL_OP,      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x25 */ {AML_OPCODE_DEF ("WaitOp",              AML_EXT_OP),               AML_EXT_WAIT_OP,        2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x26 */ {AML_OPCODE_DEF ("ResetOp",             AML_EXT_OP),               AML_EXT_RESET_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x27 */ {AML_OPCODE_DEF ("ReleaseOp",           AML_EXT_OP),               AML_EXT_RELEASE_OP,     1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x28 */ {AML_OPCODE_DEF ("FromBCDOp",           AML_EXT_OP),               AML_EXT_FROM_BCD_OP,    2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x29 */ {AML_OPCODE_DEF ("ToBCDOp",             AML_EXT_OP),               AML_EXT_TO_BCD_OP,      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x2A */ {AML_OPCODE_DEF ("UnloadOp",            AML_EXT_OP),               AML_EXT_UNLOAD_OP,      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x30 */ {AML_OPCODE_DEF ("RevisionOp",          AML_EXT_OP),               AML_EXT_REVISION_OP,    0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x31 */ {AML_OPCODE_DEF ("DebugOp",             AML_EXT_OP),               AML_EXT_DEBUG_OP,       0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x32 */ {AML_OPCODE_DEF ("FatalOp",             AML_EXT_OP),               AML_EXT_FATAL_OP,       3,        0,         {EAmlUInt8,        EAmlUInt32,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x33 */ {AML_OPCODE_DEF ("TimerOp",             AML_EXT_OP),               AML_EXT_TIMER_OP,       0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x80 */ {AML_OPCODE_DEF ("OpRegionOp",          AML_EXT_OP),               AML_EXT_REGION_OP,      4,        0,         {EAmlName,         EAmlUInt8,        EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x81 */ {AML_OPCODE_DEF ("FieldOp",             AML_EXT_OP),               AML_EXT_FIELD_OP,       2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x82 */ {AML_OPCODE_DEF ("DeviceOp",            AML_EXT_OP),               AML_EXT_DEVICE_OP,      1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x83 */ {AML_OPCODE_DEF ("ProcessorOp",         AML_EXT_OP),               AML_EXT_PROCESSOR_OP,   4,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt32,       EAmlUInt8,        EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x84 */ {AML_OPCODE_DEF ("PowerResOp",          AML_EXT_OP),               AML_EXT_POWER_RES_OP,   3,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x85 */ {AML_OPCODE_DEF ("ThermalZoneOp",       AML_EXT_OP),               AML_EXT_THERMAL_ZONE_OP,1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x86 */ {AML_OPCODE_DEF ("IndexFieldOp",        AML_EXT_OP),               AML_EXT_INDEX_FIELD_OP, 3,        0,         {EAmlName,         EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x87 */ {AML_OPCODE_DEF ("BankFieldOp",         AML_EXT_OP),               AML_EXT_BANK_FIELD_OP,  4,        0,         {EAmlName,         EAmlName,         EAmlObject,       EAmlUInt8,        EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x88 */ {AML_OPCODE_DEF ("DataRegionOp",        AML_EXT_OP),               AML_EXT_DATA_REGION_OP, 4,        0,         {EAmlName,         EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5C */      {AML_OPCODE_DEF ("RootChar",            AML_ROOT_CHAR),            0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5E */      {AML_OPCODE_DEF ("ParentPrefixChar",    AML_PARENT_PREFIX_CHAR),   0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5F */      {AML_OPCODE_DEF ("NameChar",            '_'),                      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x60 */      {AML_OPCODE_DEF ("Local0Op",            AML_LOCAL0),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x61 */      {AML_OPCODE_DEF ("Local1Op",            AML_LOCAL1),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x62 */      {AML_OPCODE_DEF ("Local2Op",            AML_LOCAL2),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x63 */      {AML_OPCODE_DEF ("Local3Op",            AML_LOCAL3),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x64 */      {AML_OPCODE_DEF ("Local4Op",            AML_LOCAL4),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x65 */      {AML_OPCODE_DEF ("Local5Op",            AML_LOCAL5),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x66 */      {AML_OPCODE_DEF ("Local6Op",            AML_LOCAL6),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x67 */      {AML_OPCODE_DEF ("Local7Op",            AML_LOCAL7),               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x68 */      {AML_OPCODE_DEF ("Arg0Op",              AML_ARG0),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x69 */      {AML_OPCODE_DEF ("Arg1Op",              AML_ARG1),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6A */      {AML_OPCODE_DEF ("Arg2Op",              AML_ARG2),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6B */      {AML_OPCODE_DEF ("Arg3Op",              AML_ARG3),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6C */      {AML_OPCODE_DEF ("Arg4Op",              AML_ARG4),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6D */      {AML_OPCODE_DEF ("Arg5Op",              AML_ARG5),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6E */      {AML_OPCODE_DEF ("Arg6Op",              AML_ARG6),                 0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x70 */      {AML_OPCODE_DEF ("StoreOp",             AML_STORE_OP),             0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x71 */      {AML_OPCODE_DEF ("RefOfOp",             AML_REF_OF_OP),            0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x72 */      {AML_OPCODE_DEF ("AddOp",               AML_ADD_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x73 */      {AML_OPCODE_DEF ("ConcatOp",            AML_CONCAT_OP),            0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x74 */      {AML_OPCODE_DEF ("SubtractOp",          AML_SUBTRACT_OP),          0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x75 */      {AML_OPCODE_DEF ("IncrementOp",         AML_INCREMENT_OP),         0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x76 */      {AML_OPCODE_DEF ("DecrementOp",         AML_DECREMENT_OP),         0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x77 */      {AML_OPCODE_DEF ("MultiplyOp",          AML_MULTIPLY_OP),          0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x78 */      {AML_OPCODE_DEF ("DivideOp",            AML_DIVIDE_OP),            0,                      4,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        0},
  /* 0x79 */      {AML_OPCODE_DEF ("ShiftLeftOp",         AML_SHIFT_LEFT_OP),        0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7A */      {AML_OPCODE_DEF ("ShiftRightOp",        AML_SHIFT_RIGHT_OP),       0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7B */      {AML_OPCODE_DEF ("AndOp",               AML_AND_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7C */      {AML_OPCODE_DEF ("NAndOp",              AML_NAND_OP),              0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7D */      {AML_OPCODE_DEF ("OrOp",                AML_OR_OP),                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7E */      {AML_OPCODE_DEF ("NorOp",               AML_NOR_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7F */      {AML_OPCODE_DEF ("XOrOp",               AML_XOR_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x80 */      {AML_OPCODE_DEF ("NotOp",               AML_NOT_OP),               0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x81 */      {AML_OPCODE_DEF ("FindSetLeftBitOp",    AML_FIND_SET_LEFT_BIT_OP), 0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x82 */      {AML_OPCODE_DEF ("FindSetRightBitOp",   AML_FIND_SET_RIGHT_BIT_OP),0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x83 */      {AML_OPCODE_DEF ("DerefOfOp",           AML_DEREF_OF_OP),          0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x84 */      {AML_OPCODE_DEF ("ConcatResOp",         AML_CONCAT_RES_OP),        0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x85 */      {AML_OPCODE_DEF ("ModOp",               AML_MOD_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x86 */      {AML_OPCODE_DEF ("NotifyOp",            AML_NOTIFY_OP),            0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x87 */      {AML_OPCODE_DEF ("SizeOfOp",            AML_SIZE_OF_OP),           0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x88 */      {AML_OPCODE_DEF ("IndexOp",             AML_INDEX_OP),             0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x89 */      {AML_OPCODE_DEF ("MatchOp",             AML_MATCH_OP),             0,                      6,        0,         {EAmlObject,       EAmlUInt8,        EAmlObject,       EAmlUInt8,        EAmlObject,       EAmlObject},      0},
  /* 0x8A */      {AML_OPCODE_DEF ("CreateDWordFieldOp",  AML_CREATE_DWORD_FIELD_OP),0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8B */      {AML_OPCODE_DEF ("CreateWordFieldOp",   AML_CREATE_WORD_FIELD_OP), 0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8C */      {AML_OPCODE_DEF ("CreateByteFieldOp",   AML_CREATE_BYTE_FIELD_OP), 0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8D */      {AML_OPCODE_DEF ("CreateBitFieldOp",    AML_CREATE_BIT_FIELD_OP),  0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8E */      {AML_OPCODE_DEF ("ObjectTypeOp",        AML_OBJECT_TYPE_OP),       0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x8F */      {AML_OPCODE_DEF ("CreateQWordFieldOp",  AML_CREATE_QWORD_FIELD_OP),0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x90 */      {AML_OPCODE_DEF ("LAndOp",              AML_LAND_OP),              0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x91 */      {AML_OPCODE_DEF ("LOrOp",               AML_LOR_OP),               0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x92 */      {AML_OPCODE_DEF ("LNotOp",              AML_LNOT_OP),              0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x93 */      {AML_OPCODE_DEF ("LEqualOp",            AML_LEQUAL_OP),            0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x94 */      {AML_OPCODE_DEF ("LGreaterOp",          AML_LGREATER_OP),          0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x95 */      {AML_OPCODE_DEF ("LLessOp",             AML_LLESS_OP),             0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x96 */      {AML_OPCODE_DEF ("ToBufferOp",          AML_TO_BUFFER_OP),         0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x97 */      {AML_OPCODE_DEF ("ToDecimalStringOp",   AML_TO_DEC_STRING_OP),     0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x98 */      {AML_OPCODE_DEF ("ToHexStringOp",       AML_TO_HEX_STRING_OP),     0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x99 */      {AML_OPCODE_DEF ("ToIntegerOp",         AML_TO_INTEGER_OP),        0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9C */      {AML_OPCODE_DEF ("ToStringOp",          AML_TO_STRING_OP),         0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9D */      {AML_OPCODE_DEF ("CopyObjectOp",        AML_COPY_OBJECT_OP),       0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9E */      {AML_OPCODE_DEF ("MidOp",               AML_MID_OP),               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9F */      {AML_OPCODE_DEF ("ContinueOp",          AML_CONTINUE_OP),          0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA0 */      {AML_OPCODE_DEF ("IfOp",                AML_IF_OP),                0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA1 */      {AML_OPCODE_DEF ("ElseOp",              AML_ELSE_OP),              0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA2 */      {AML_OPCODE_DEF ("WhileOp",             AML_WHILE_OP),             0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA3 */      {AML_OPCODE_DEF ("NoopOp",              AML_NOOP_OP),              0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA4 */      {AML_OPCODE_DEF ("ReturnOp",            AML_RETURN_OP),            0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA5 */      {AML_OPCODE_DEF ("BreakOp",             AML_BREAK_OP),             0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xCC */      {AML_OPCODE_DEF ("BreakPointOp",        AML_BREAK_POINT_OP),       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xD0 */      {AML_OPCODE_DEF ("MethodInvocOp",       AML_METHOD_INVOC_OP),      0,                      2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_PSEUDO_OPCODE | AML_HAS_CHILD_OBJ},
  /* 0xFF */      {AML_OPCODE_DEF ("OnesOp",              AML_ONES_OP),              0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
};

/** AML grammar encoding for field elements.

  Some AML objects are expecting a FieldList. They are referred in this library
  as field nodes. These objects have the following opcodes:
   - FieldOp;
   - IndexFieldOp;
   - BankFieldOp.
  In the AML grammar encoding table, they have the AML_HAS_FIELD_LIST
  attribute.

  A field list is made of field elements.
  According to the ACPI 6.3 specification, s20.2.5.2 "Named Objects Encoding",
  field elements can be:
   - NamedField           := NameSeg PkgLength;
   - ReservedField        := 0x00 PkgLength;
   - AccessField          := 0x01 AccessType AccessAttrib;
   - ConnectField         := <0x02 NameString> | <0x02 BufferData>;
   - ExtendedAccessField  := 0x03 AccessType ExtendedAccessAttrib AccessLength.

  A small set of opcodes describes field elements. They are referred in this
  library as field opcodes.
  The NamedField field element doesn't have a field opcode. A pseudo
  OpCode/SubOpCode couple has been created for it.

  Field elements:
   - don't have a SubOpCode;
   - have at most 3 fixed arguments (6 for object opcodes,
     8 for method invocations);
   - don't have variable list of arguments;
   - are not part of the AML namespace, except NamedField field elements.
*/
GLOBAL_REMOVE_IF_UNREFERENCED
STATIC
CONST
AML_BYTE_ENCODING mAmlFieldEncoding[] = {
  // Comment                       Str                    OpCode                     SubOpCode               MaxIndex  NameIndex   0                 1                 2                 3                 4                 5                 Attribute
  /* 0x00 */      {AML_OPCODE_DEF ("FieldReservedOp",     AML_FIELD_RESERVED_OP),    0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT | AML_HAS_PKG_LENGTH},
  /* 0x01 */      {AML_OPCODE_DEF ("FieldAccessOp",       AML_FIELD_ACCESS_OP),      0,                      2,        0,         {EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x02 */      {AML_OPCODE_DEF ("FieldConnectionOp",   AML_FIELD_CONNECTION_OP),  0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x03 */      {AML_OPCODE_DEF ("FieldExtAccessOp",    AML_FIELD_EXT_ACCESS_OP),  0,                      3,        0,         {EAmlUInt8,        EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x04 */      {AML_OPCODE_DEF ("FieldNamed",          AML_FIELD_NAMED_OP),       0,                      2,        0,         {EAmlName,         EAmlFieldPkgLen,  EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT | AML_IS_PSEUDO_OPCODE | AML_IN_NAMESPACE}
};

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
  )
{
  UINT8     OpCode;
  UINT8     SubOpCode;
  UINT32    Index;

  if (Buffer == NULL) {
    ASSERT (0);
    return NULL;
  }

  // Get OpCode and SubOpCode.
  OpCode = Buffer[0];
  if (OpCode == AML_EXT_OP) {
    SubOpCode = Buffer[1];
  } else {
    SubOpCode = 0;
  }

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlByteEncoding) / sizeof (mAmlByteEncoding[0]));
       Index++) {
    if ((mAmlByteEncoding[Index].OpCode == OpCode) &&
        (mAmlByteEncoding[Index].SubOpCode == SubOpCode)) {
       if ((mAmlByteEncoding[Index].Attribute & AML_IS_PSEUDO_OPCODE) ==
            AML_IS_PSEUDO_OPCODE) {
        // A pseudo OpCode cannot be parsed as it is internal to this library.
        // The MethodInvocation encoding can be detected by NameSpace lookup.
        ASSERT (0);
        return NULL;
      }
      return &mAmlByteEncoding[Index];
    }
  }

  return NULL;
}

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
  )
{
  UINT32    Index;

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlByteEncoding) / sizeof (mAmlByteEncoding[0]));
       Index++) {
    if ((mAmlByteEncoding[Index].OpCode == OpCode) &&
        (mAmlByteEncoding[Index].SubOpCode == SubOpCode)) {
      return &mAmlByteEncoding[Index];
    }
  }
  return NULL;
}

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
  )
{
  UINT8     OpCode;
  UINT32    Index;

  if (Buffer == NULL) {
    ASSERT (0);
    return NULL;
  }

  // Get OpCode.
  OpCode = *Buffer;

  // Search in the table.
  for (Index = 0;
       Index < (sizeof (mAmlFieldEncoding) / sizeof (mAmlFieldEncoding[0]));
       Index++) {
    if (mAmlFieldEncoding[Index].OpCode == OpCode) {
      if ((mAmlFieldEncoding[Index].Attribute & AML_IS_PSEUDO_OPCODE) ==
             AML_IS_PSEUDO_OPCODE) {
        // A pseudo OpCode cannot be parsed as it is internal to this library.
        // The NamedField encoding can be detected because it begins with a
        // char.
        ASSERT (0);
        return NULL;
      }
      return &mAmlFieldEncoding[Index];
    }
  }

  return NULL;
}

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
  )
{
  UINT32    Index;

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlFieldEncoding) / sizeof (mAmlFieldEncoding[0]));
       Index++) {
    if ((mAmlFieldEncoding[Index].OpCode == OpCode) &&
        (mAmlFieldEncoding[Index].SubOpCode == SubOpCode)) {
      return &mAmlFieldEncoding[Index];
    }
  }
  return NULL;
}

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
  )
{
  EAML_PARSE_INDEX  Index;

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlByteEncoding) / sizeof (mAmlByteEncoding[0]));
       Index++) {
    if ((mAmlByteEncoding[Index].OpCode == OpCode) &&
        (mAmlByteEncoding[Index].SubOpCode == SubOpCode)) {
      return mAmlByteEncoding[Index].Str;
    }
  }

  ASSERT (0);
  return NULL;
}

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
  )
{
  EAML_PARSE_INDEX  Index;

  if (SubOpCode != 0) {
    ASSERT (0);
    return NULL;
  }

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlFieldEncoding) / sizeof (mAmlFieldEncoding[0]));
       Index++) {
    if ((mAmlFieldEncoding[Index].OpCode == OpCode)) {
      return mAmlFieldEncoding[Index].Str;
    }
  }

  ASSERT (0);
  return NULL;
}
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
  )
{
  EAML_PARSE_INDEX  Index;

  // Search the table.
  for (Index = 0;
       Index < (sizeof (mAmlByteEncoding) / sizeof (mAmlByteEncoding[0]));
       Index++) {
    if ((mAmlByteEncoding[Index].OpCode == OpCode) &&
        (mAmlByteEncoding[Index].SubOpCode == SubOpCode)) {
      return TRUE;
    }
  }
  return FALSE;
}

/** AML_PARSE_FORMAT to EAML_NODE_DATA_TYPE translation table.

  AML_PARSE_FORMAT describes an internal set of values identifying the types
  that can be found while parsing an AML bytestream.
  EAML_NODE_DATA_TYPE describes an external set of values allowing to identify
  what type of data can be found in data nodes.
*/
GLOBAL_REMOVE_IF_UNREFERENCED
STATIC
CONST
EAML_NODE_DATA_TYPE mAmlTypeToNodeDataType[] = {
  EAmlNodeDataTypeNone,         // EAmlNone
  EAmlNodeDataTypeUInt,         // EAmlUInt8
  EAmlNodeDataTypeUInt,         // EAmlUInt16
  EAmlNodeDataTypeUInt,         // EAmlUInt32
  EAmlNodeDataTypeUInt,         // EAmlUInt64
  EAmlNodeDataTypeReserved5,    // EAmlObject
  EAmlNodeDataTypeNameString,   // EAmlName
  EAmlNodeDataTypeString,       // EAmlString
  EAmlNodeDataTypeFieldPkgLen   // EAmlFieldPkgLen
};

/** Convert an AML_PARSE_FORMAT to its corresponding EAML_NODE_DATA_TYPE.

  @param  [in]  AmlType   Input AML Type.

  @return The corresponding EAML_NODE_DATA_TYPE.
          EAmlNodeDataTypeNone if not found.
**/
EAML_NODE_DATA_TYPE
EFIAPI
AmlTypeToNodeDataType (
  IN  AML_PARSE_FORMAT  AmlType
  )
{
  if (AmlType >=
      (sizeof (mAmlTypeToNodeDataType) / sizeof (mAmlTypeToNodeDataType[0]))) {
    ASSERT (0);
    return EAmlNodeDataTypeNone;
  }

  return mAmlTypeToNodeDataType[AmlType];
}

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
  )
{
  UINT8     LeadByte;
  UINT8     ByteCount;
  UINT32    RealLength;
  UINT32    Offset;

  if ((Buffer == NULL)  ||
      (PkgLength == NULL)) {
    ASSERT (0);
    return 0;
  }

  /* From ACPI 6.3 specification, s20.2.4 "Package Length Encoding":

  PkgLength := PkgLeadByte |
               <PkgLeadByte ByteData> |
               <PkgLeadByte ByteData ByteData> |
               <PkgLeadByte ByteData ByteData ByteData>

  PkgLeadByte := <bit 7-6: ByteData count that follows (0-3)>
                 <bit 5-4: Only used if PkgLength < 63>
                 <bit 3-0: Least significant package length nibble>

  Note:
    The high 2 bits of the first byte reveal how many
    follow bytes are in the PkgLength. If the
    PkgLength has only one byte, bit 0 through 5 are
    used to encode the package length (in other
    words, values 0-63). If the package length value
    is more than 63, more than one byte must be
    used for the encoding in which case bit 4 and 5 of
    the PkgLeadByte are reserved and must be zero.
    If the multiple bytes encoding is used, bits 0-3 of
    the PkgLeadByte become the least significant 4
    bits of the resulting package length value. The next
    ByteData will become the next least
    significant 8 bits of the resulting value and so on,
    up to 3 ByteData bytes. Thus, the maximum
    package length is 2**28.
  */

  LeadByte = *Buffer;
  ByteCount = (LeadByte >> 6) & 0x03U;
  Offset = ByteCount + 1U;
  RealLength = 0;

  // Switch on the number of bytes used to store the PkgLen.
  switch (ByteCount) {
    case 0:
    {
      RealLength = LeadByte;
      break;
    }
    case 1:
    {
      RealLength = *(Buffer + 1);
      RealLength = (RealLength << 4) | (LeadByte & 0xF);
      break;
    }
    case 2:
    {
      RealLength = *(Buffer + 1);
      RealLength |= ((UINT32)(*(Buffer + 2))) << 8;
      RealLength = (RealLength << 4) | (LeadByte & 0xF);
      break;
    }
    case 3:
    {
      RealLength = *(Buffer + 1);
      RealLength |= ((UINT32)(*(Buffer + 2))) << 8;
      RealLength |= ((UINT32)(*(Buffer + 3))) << 16;
      RealLength = (RealLength << 4) | (LeadByte & 0xF);
      break;
    }
    default:
    {
      ASSERT (0);
      Offset = 0;
      break;
    }
  } // switch

  *PkgLength = RealLength;

  return Offset;
}

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
  )
{
  UINT8   LeadByte;
  UINT8   Offset;
  UINT8   CurrentOffset;
  UINT8   CurrentShift;
  UINT32  ComputedLength;

  if (Buffer == NULL) {
    ASSERT (0);
    return 0;
  }

  LeadByte = 0;
  Offset = 0;

  if ((Length < (1 << 6))) {
    // Length < 2^6, only need one byte to encode it.
    LeadByte = (UINT8)Length;

  } else {
    // Need more than one byte to encode it.
    // Test Length to find how many bytes are needed.

    if (Length >= (1 << 28)) {
      // Length >= 2^28, should not be possible.
      ASSERT (0);
      return 0;

    } else if (Length >= (1 << 20)) {
      // Length >= 2^20
      Offset = 3;

    } else if (Length >= (1 << 12)) {
      // Length >= 2^12
      Offset = 2;

    } else if (Length >= (1 << 6)) {
      // Length >= 2^6
      Offset = 1;

    } else {
      // Should not be possible.
      ASSERT (0);
      return 0;
    }

    // Set the LeadByte.
    LeadByte = (UINT8)(Offset << 6);
    LeadByte = (UINT8)(LeadByte | (Length & 0xF));
  }

  // Write to the Buffer.
  *Buffer = LeadByte;
  CurrentOffset = 1;
  while (CurrentOffset < (Offset + 1)) {
    CurrentShift = (UINT8)((CurrentOffset - 1) * 8);
    ComputedLength = Length & (UINT32)(0x00000FF0 << CurrentShift);
    ComputedLength = (ComputedLength) >> (4 + CurrentShift);
    LeadByte = (UINT8)(ComputedLength & 0xFF);
    *(Buffer + CurrentOffset) = LeadByte;
    CurrentOffset++;
  }

  return ++Offset;
}

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
  )
{
  // Length >= 2^28, should not be possible.
  if (Length >= (1 << 28)) {
    ASSERT (0);
    return 0;

  } else if (Length >= (1 << 20)) {
    // Length >= 2^20
    return 4;

  } else if (Length >= (1 << 12)) {
    // Length >= 2^12
    return 3;

  } else if (Length >= (1 << 6)) {
    // Length >= 2^6
    return 2;
  }

  // Length < 2^6
  return 1;
}

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
  )
{
  UINT32  PkgLenWidth;
  UINT32  ReComputedPkgLenWidth;

  if (PkgLen == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Compute the PkgLenWidth.
  PkgLenWidth = AmlComputePkgLengthWidth (Length);
  if (PkgLenWidth == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Add it to the Length.
  Length += PkgLenWidth;

  // Check that adding the PkgLenWidth didn't trigger a domino effect,
  // increasing the encoding width of the PkgLen again.
  // The PkgLen is encoded in at most 4 bytes. It is possible to increase
  // the PkgLen width if its encoding is less than 3 bytes.
  ReComputedPkgLenWidth = AmlComputePkgLengthWidth (Length);
  if (ReComputedPkgLenWidth != PkgLenWidth) {
    if ((ReComputedPkgLenWidth != 0)   &&
        (ReComputedPkgLenWidth < 4)) {
      // No need to recompute the PkgLen since a new threshold cannot
      // be reached by incrementing the value by one.
      Length += 1;
    } else {
      ASSERT (0);
      return EFI_INVALID_PARAMETER;
    }
  }

  *PkgLen = Length;

  return EFI_SUCCESS;
}
