/** @file
  AML grammar definitions.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved. <BR>
  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.<BR>

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
  // Comment       Str                                   OpCode                     SubOpCode               MaxIndex  NameIndex   0                 1                 2                 3                 4                 5                 Attribute
  /* 0x00 */      {AML_DEBUG_STR ("ZeroOp")              AML_ZERO_OP,               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x01 */      {AML_DEBUG_STR ("OneOp")               AML_ONE_OP,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x06 */      {AML_DEBUG_STR ("AliasOp")             AML_ALIAS_OP,              0,                      2,        1,         {EAmlName,         EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x08 */      {AML_DEBUG_STR ("NameOp")              AML_NAME_OP,               0,                      2,        0,         {EAmlName,         EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x0A */      {AML_DEBUG_STR ("BytePrefix")          AML_BYTE_PREFIX,           0,                      1,        0,         {EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0B */      {AML_DEBUG_STR ("WordPrefix")          AML_WORD_PREFIX,           0,                      1,        0,         {EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0C */      {AML_DEBUG_STR ("DWordPrefix")         AML_DWORD_PREFIX,          0,                      1,        0,         {EAmlUInt32,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0D */      {AML_DEBUG_STR ("StringPrefix")        AML_STRING_PREFIX,         0,                      1,        0,         {EAmlString,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x0E */      {AML_DEBUG_STR ("QWordPrefix")         AML_QWORD_PREFIX,          0,                      1,        0,         {EAmlUInt64,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x10 */      {AML_DEBUG_STR ("ScopeOp")             AML_SCOPE_OP,              0,                      1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x11 */      {AML_DEBUG_STR ("BufferOp")            AML_BUFFER_OP,             0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_BYTE_LIST},
  /* 0x12 */      {AML_DEBUG_STR ("PackageOp")           AML_PACKAGE_OP,            0,                      1,        0,         {EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0x13 */      {AML_DEBUG_STR ("VarPackageOp")        AML_VAR_PACKAGE_OP,        0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0x14 */      {AML_DEBUG_STR ("MethodOp")            AML_METHOD_OP,             0,                      2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x15 */      {AML_DEBUG_STR ("ExternalOp")          AML_EXTERNAL_OP,           0,                      3,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x2E */      {AML_DEBUG_STR ("DualNamePrefix")      AML_DUAL_NAME_PREFIX,      0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x2F */      {AML_DEBUG_STR ("MultiNamePrefix")     AML_MULTI_NAME_PREFIX,     0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x41 */      {AML_DEBUG_STR ("NameChar_A")          'A',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x42 */      {AML_DEBUG_STR ("NameChar_B")          'B',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x43 */      {AML_DEBUG_STR ("NameChar_C")          'C',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x44 */      {AML_DEBUG_STR ("NameChar_D")          'D',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x45 */      {AML_DEBUG_STR ("NameChar_E")          'E',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x46 */      {AML_DEBUG_STR ("NameChar_F")          'F',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x47 */      {AML_DEBUG_STR ("NameChar_G")          'G',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x48 */      {AML_DEBUG_STR ("NameChar_H")          'H',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x49 */      {AML_DEBUG_STR ("NameChar_I")          'I',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4A */      {AML_DEBUG_STR ("NameChar_J")          'J',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4B */      {AML_DEBUG_STR ("NameChar_K")          'K',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4C */      {AML_DEBUG_STR ("NameChar_L")          'L',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4D */      {AML_DEBUG_STR ("NameChar_M")          'M',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4E */      {AML_DEBUG_STR ("NameChar_N")          'N',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x4F */      {AML_DEBUG_STR ("NameChar_O")          'O',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x50 */      {AML_DEBUG_STR ("NameChar_P")          'P',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x51 */      {AML_DEBUG_STR ("NameChar_Q")          'Q',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x52 */      {AML_DEBUG_STR ("NameChar_R")          'R',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x53 */      {AML_DEBUG_STR ("NameChar_S")          'S',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x54 */      {AML_DEBUG_STR ("NameChar_T")          'T',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x55 */      {AML_DEBUG_STR ("NameChar_U")          'U',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x56 */      {AML_DEBUG_STR ("NameChar_V")          'V',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x57 */      {AML_DEBUG_STR ("NameChar_W")          'W',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x58 */      {AML_DEBUG_STR ("NameChar_X")          'X',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x59 */      {AML_DEBUG_STR ("NameChar_Y")          'Y',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5A */      {AML_DEBUG_STR ("NameChar_Z")          'Z',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5B 0x01 */ {AML_DEBUG_STR ("MutexOp")             AML_EXT_OP,                AML_EXT_MUTEX_OP,       2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x02 */ {AML_DEBUG_STR ("EventOp")             AML_EXT_OP,                AML_EXT_EVENT_OP,       1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x12 */ {AML_DEBUG_STR ("CondRefOfOp")         AML_EXT_OP,                AML_EXT_COND_REF_OF_OP, 2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x13 */ {AML_DEBUG_STR ("CreateFieldOp")       AML_EXT_OP,                AML_EXT_CREATE_FIELD_OP,4,        3,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x1F */ {AML_DEBUG_STR ("LoadTableOp")         AML_EXT_OP,                AML_EXT_LOAD_TABLE_OP,  6,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject},      0},
  /* 0x5B 0x20 */ {AML_DEBUG_STR ("LoadOp")              AML_EXT_OP,                AML_EXT_LOAD_OP,        2,        0,         {EAmlName,         EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x21 */ {AML_DEBUG_STR ("StallOp")             AML_EXT_OP,                AML_EXT_STALL_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x22 */ {AML_DEBUG_STR ("SleepOp")             AML_EXT_OP,                AML_EXT_SLEEP_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x23 */ {AML_DEBUG_STR ("AcquireOp")           AML_EXT_OP,                AML_EXT_ACQUIRE_OP,     2,        0,         {EAmlObject,       EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x24 */ {AML_DEBUG_STR ("SignalOp")            AML_EXT_OP,                AML_EXT_SIGNAL_OP,      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x25 */ {AML_DEBUG_STR ("WaitOp")              AML_EXT_OP,                AML_EXT_WAIT_OP,        2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x26 */ {AML_DEBUG_STR ("ResetOp")             AML_EXT_OP,                AML_EXT_RESET_OP,       1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x27 */ {AML_DEBUG_STR ("ReleaseOp")           AML_EXT_OP,                AML_EXT_RELEASE_OP,     1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x28 */ {AML_DEBUG_STR ("FromBCDOp")           AML_EXT_OP,                AML_EXT_FROM_BCD_OP,    2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x29 */ {AML_DEBUG_STR ("ToBCDOp")             AML_EXT_OP,                AML_EXT_TO_BCD_OP,      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x2A */ {AML_DEBUG_STR ("UnloadOp")            AML_EXT_OP,                AML_EXT_UNLOAD_OP,      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x30 */ {AML_DEBUG_STR ("RevisionOp")          AML_EXT_OP,                AML_EXT_REVISION_OP,    0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x31 */ {AML_DEBUG_STR ("DebugOp")             AML_EXT_OP,                AML_EXT_DEBUG_OP,       0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x32 */ {AML_DEBUG_STR ("FatalOp")             AML_EXT_OP,                AML_EXT_FATAL_OP,       3,        0,         {EAmlUInt8,        EAmlUInt32,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x33 */ {AML_DEBUG_STR ("TimerOp")             AML_EXT_OP,                AML_EXT_TIMER_OP,       0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x5B 0x80 */ {AML_DEBUG_STR ("OpRegionOp")          AML_EXT_OP,                AML_EXT_REGION_OP,      4,        0,         {EAmlName,         EAmlUInt8,        EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5B 0x81 */ {AML_DEBUG_STR ("FieldOp")             AML_EXT_OP,                AML_EXT_FIELD_OP,       2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x82 */ {AML_DEBUG_STR ("DeviceOp")            AML_EXT_OP,                AML_EXT_DEVICE_OP,      1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x83 */ {AML_DEBUG_STR ("ProcessorOp")         AML_EXT_OP,                AML_EXT_PROCESSOR_OP,   4,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt32,       EAmlUInt8,        EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x84 */ {AML_DEBUG_STR ("PowerResOp")          AML_EXT_OP,                AML_EXT_POWER_RES_OP,   3,        0,         {EAmlName,         EAmlUInt8,        EAmlUInt16,       EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x85 */ {AML_DEBUG_STR ("ThermalZoneOp")       AML_EXT_OP,                AML_EXT_THERMAL_ZONE_OP,1,        0,         {EAmlName,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ | AML_IN_NAMESPACE},
  /* 0x5B 0x86 */ {AML_DEBUG_STR ("IndexFieldOp")        AML_EXT_OP,                AML_EXT_INDEX_FIELD_OP, 3,        0,         {EAmlName,         EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x87 */ {AML_DEBUG_STR ("BankFieldOp")         AML_EXT_OP,                AML_EXT_BANK_FIELD_OP,  4,        0,         {EAmlName,         EAmlName,         EAmlObject,       EAmlUInt8,        EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_FIELD_LIST},
  /* 0x5B 0x88 */ {AML_DEBUG_STR ("DataRegionOp")        AML_EXT_OP,                AML_EXT_DATA_REGION_OP, 4,        0,         {EAmlName,         EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x5C */      {AML_DEBUG_STR ("RootChar")            AML_ROOT_CHAR,             0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5E */      {AML_DEBUG_STR ("ParentPrefixChar")    AML_PARENT_PREFIX_CHAR,    0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x5F */      {AML_DEBUG_STR ("NameChar")            '_',                       0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_NAME_CHAR},
  /* 0x60 */      {AML_DEBUG_STR ("Local0Op")            AML_LOCAL0,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x61 */      {AML_DEBUG_STR ("Local1Op")            AML_LOCAL1,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x62 */      {AML_DEBUG_STR ("Local2Op")            AML_LOCAL2,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x63 */      {AML_DEBUG_STR ("Local3Op")            AML_LOCAL3,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x64 */      {AML_DEBUG_STR ("Local4Op")            AML_LOCAL4,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x65 */      {AML_DEBUG_STR ("Local5Op")            AML_LOCAL5,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x66 */      {AML_DEBUG_STR ("Local6Op")            AML_LOCAL6,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x67 */      {AML_DEBUG_STR ("Local7Op")            AML_LOCAL7,                0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x68 */      {AML_DEBUG_STR ("Arg0Op")              AML_ARG0,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x69 */      {AML_DEBUG_STR ("Arg1Op")              AML_ARG1,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6A */      {AML_DEBUG_STR ("Arg2Op")              AML_ARG2,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6B */      {AML_DEBUG_STR ("Arg3Op")              AML_ARG3,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6C */      {AML_DEBUG_STR ("Arg4Op")              AML_ARG4,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6D */      {AML_DEBUG_STR ("Arg5Op")              AML_ARG5,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x6E */      {AML_DEBUG_STR ("Arg6Op")              AML_ARG6,                  0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x70 */      {AML_DEBUG_STR ("StoreOp")             AML_STORE_OP,              0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x71 */      {AML_DEBUG_STR ("RefOfOp")             AML_REF_OF_OP,             0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x72 */      {AML_DEBUG_STR ("AddOp")               AML_ADD_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x73 */      {AML_DEBUG_STR ("ConcatOp")            AML_CONCAT_OP,             0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x74 */      {AML_DEBUG_STR ("SubtractOp")          AML_SUBTRACT_OP,           0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x75 */      {AML_DEBUG_STR ("IncrementOp")         AML_INCREMENT_OP,          0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x76 */      {AML_DEBUG_STR ("DecrementOp")         AML_DECREMENT_OP,          0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x77 */      {AML_DEBUG_STR ("MultiplyOp")          AML_MULTIPLY_OP,           0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x78 */      {AML_DEBUG_STR ("DivideOp")            AML_DIVIDE_OP,             0,                      4,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone},        0},
  /* 0x79 */      {AML_DEBUG_STR ("ShiftLeftOp")         AML_SHIFT_LEFT_OP,         0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7A */      {AML_DEBUG_STR ("ShiftRightOp")        AML_SHIFT_RIGHT_OP,        0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7B */      {AML_DEBUG_STR ("AndOp")               AML_AND_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7C */      {AML_DEBUG_STR ("NAndOp")              AML_NAND_OP,               0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7D */      {AML_DEBUG_STR ("OrOp")                AML_OR_OP,                 0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7E */      {AML_DEBUG_STR ("NorOp")               AML_NOR_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x7F */      {AML_DEBUG_STR ("XOrOp")               AML_XOR_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x80 */      {AML_DEBUG_STR ("NotOp")               AML_NOT_OP,                0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x81 */      {AML_DEBUG_STR ("FindSetLeftBitOp")    AML_FIND_SET_LEFT_BIT_OP,  0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x82 */      {AML_DEBUG_STR ("FindSetRightBitOp")   AML_FIND_SET_RIGHT_BIT_OP, 0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x83 */      {AML_DEBUG_STR ("DerefOfOp")           AML_DEREF_OF_OP,           0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x84 */      {AML_DEBUG_STR ("ConcatResOp")         AML_CONCAT_RES_OP,         0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x85 */      {AML_DEBUG_STR ("ModOp")               AML_MOD_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x86 */      {AML_DEBUG_STR ("NotifyOp")            AML_NOTIFY_OP,             0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x87 */      {AML_DEBUG_STR ("SizeOfOp")            AML_SIZE_OF_OP,            0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x88 */      {AML_DEBUG_STR ("IndexOp")             AML_INDEX_OP,              0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x89 */      {AML_DEBUG_STR ("MatchOp")             AML_MATCH_OP,              0,                      6,        0,         {EAmlObject,       EAmlUInt8,        EAmlObject,       EAmlUInt8,        EAmlObject,       EAmlObject},      0},
  /* 0x8A */      {AML_DEBUG_STR ("CreateDWordFieldOp")  AML_CREATE_DWORD_FIELD_OP, 0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8B */      {AML_DEBUG_STR ("CreateWordFieldOp")   AML_CREATE_WORD_FIELD_OP,  0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8C */      {AML_DEBUG_STR ("CreateByteFieldOp")   AML_CREATE_BYTE_FIELD_OP,  0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8D */      {AML_DEBUG_STR ("CreateBitFieldOp")    AML_CREATE_BIT_FIELD_OP,   0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x8E */      {AML_DEBUG_STR ("ObjectTypeOp")        AML_OBJECT_TYPE_OP,        0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x8F */      {AML_DEBUG_STR ("CreateQWordFieldOp")  AML_CREATE_QWORD_FIELD_OP, 0,                      3,        2,         {EAmlObject,       EAmlObject,       EAmlName,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IN_NAMESPACE},
  /* 0x90 */      {AML_DEBUG_STR ("LAndOp")              AML_LAND_OP,               0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x91 */      {AML_DEBUG_STR ("LOrOp")               AML_LOR_OP,                0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x92 */      {AML_DEBUG_STR ("LNotOp")              AML_LNOT_OP,               0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x93 */      {AML_DEBUG_STR ("LEqualOp")            AML_LEQUAL_OP,             0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x94 */      {AML_DEBUG_STR ("LGreaterOp")          AML_LGREATER_OP,           0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x95 */      {AML_DEBUG_STR ("LLessOp")             AML_LLESS_OP,              0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x96 */      {AML_DEBUG_STR ("ToBufferOp")          AML_TO_BUFFER_OP,          0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x97 */      {AML_DEBUG_STR ("ToDecimalStringOp")   AML_TO_DEC_STRING_OP,      0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x98 */      {AML_DEBUG_STR ("ToHexStringOp")       AML_TO_HEX_STRING_OP,      0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x99 */      {AML_DEBUG_STR ("ToIntegerOp")         AML_TO_INTEGER_OP,         0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9C */      {AML_DEBUG_STR ("ToStringOp")          AML_TO_STRING_OP,          0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9D */      {AML_DEBUG_STR ("CopyObjectOp")        AML_COPY_OBJECT_OP,        0,                      2,        0,         {EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9E */      {AML_DEBUG_STR ("MidOp")               AML_MID_OP,                0,                      3,        0,         {EAmlObject,       EAmlObject,       EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0x9F */      {AML_DEBUG_STR ("ContinueOp")          AML_CONTINUE_OP,           0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA0 */      {AML_DEBUG_STR ("IfOp")                AML_IF_OP,                 0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA1 */      {AML_DEBUG_STR ("ElseOp")              AML_ELSE_OP,               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA2 */      {AML_DEBUG_STR ("WhileOp")             AML_WHILE_OP,              0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_HAS_PKG_LENGTH | AML_HAS_CHILD_OBJ},
  /* 0xA3 */      {AML_DEBUG_STR ("NoopOp")              AML_NOOP_OP,               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA4 */      {AML_DEBUG_STR ("ReturnOp")            AML_RETURN_OP,             0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xA5 */      {AML_DEBUG_STR ("BreakOp")             AML_BREAK_OP,              0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xCC */      {AML_DEBUG_STR ("BreakPointOp")        AML_BREAK_POINT_OP,        0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
  /* 0xD0 */      {AML_DEBUG_STR ("MethodInvocOp")       AML_METHOD_INVOC_OP,       0,                      2,        0,         {EAmlName,         EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_PSEUDO_OPCODE | AML_HAS_CHILD_OBJ},
  /* 0xFF */      {AML_DEBUG_STR ("OnesOp")              AML_ONES_OP,               0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        0},
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
  // Comment       Str                                   OpCode                     SubOpCode               MaxIndex  NameIndex   0                 1                 2                 3                 4                 5                 Attribute
  /* 0x00 */      {AML_DEBUG_STR ("FieldReservedOp")     AML_FIELD_RESERVED_OP,     0,                      0,        0,         {EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT | AML_HAS_PKG_LENGTH},
  /* 0x01 */      {AML_DEBUG_STR ("FieldAccessOp")       AML_FIELD_ACCESS_OP,       0,                      2,        0,         {EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x02 */      {AML_DEBUG_STR ("FieldConnectionOp")   AML_FIELD_CONNECTION_OP,   0,                      1,        0,         {EAmlObject,       EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x03 */      {AML_DEBUG_STR ("FieldExtAccessOp")    AML_FIELD_EXT_ACCESS_OP,   0,                      3,        0,         {EAmlUInt8,        EAmlUInt8,        EAmlUInt8,        EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT},
  /* 0x04 */      {AML_DEBUG_STR ("FieldNamed")          AML_FIELD_NAMED_OP,        0,                      2,        0,         {EAmlName,         EAmlFieldPkgLen,  EAmlNone,         EAmlNone,         EAmlNone,         EAmlNone},        AML_IS_FIELD_ELEMENT | AML_IS_PSEUDO_OPCODE | AML_IN_NAMESPACE}
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
