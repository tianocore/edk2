/** @file

  Copyright (c) 2017, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - ACPI   - Advanced Configuration and Power Interface
    - SMBIOS - System Management BIOS
    - DT     - Device Tree
**/

#ifndef TABLE_GENERATOR_H_
#define TABLE_GENERATOR_H_

/** The TABLE_GENERATOR_ID type describes the Table Generator ID

  Table Generator ID

_______________________________________________________________________________
|  31 | 30 |29 | 28 | 27 | 26 | 25 | 24 | 23 | 22 | 21 | 20 | 19 | 18 | 17| 16|
-------------------------------------------------------------------------------
|TNSID|  0 |   TT   |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0 |  0|  0|
_______________________________________________________________________________
_______________________________________________________________________________
|15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0|
-------------------------------------------------------------------------------
|                                 Table ID                                    |
_______________________________________________________________________________

  Bit [31] - Table NameSpace ID (TNSID)
      0 - Standard
      1 - Custom/OEM

  Bit [30] - Reserved, Must be Zero

  Bit [29:28] - Table Type (TT)
       0 - ACPI Table
       1 - SMBIOS Table
       2 - DT (Device Tree) Table
       3 - Reserved (INVALID)

  Bit [27:16] - Reserved, Must Be Zero

  Bit [15:0] - Table ID

    Standard ACPI Table IDs:
       0 - Reserved
       1 - RAW
       2 - FADT
       3 - DSDT
       4 - SSDT
       5 - MADT
       6 - GTDT
       7 - DBG2
       8 - SPCR
       9 - MCFG

    Standard SMBIOS Table IDs:
       0 - Reserved
       1 - RAW
       2 - Table Type00
       3 - Table Type01
       4 - Table Type02
       5 - Table Type03
       6 - Table Type04
       7 - Table Type05
       8 - Table Type06
       9 - Table Type07
      10 - Table Type08
      11 - Table Type09
      12 - Table Type10
      13 - Table Type11
      14 - Table Type12
      15 - Table Type13
      16 - Table Type14
      17 - Table Type15
      18 - Table Type16
      19 - Table Type17
      20 - Table Type18
      21 - Table Type19
      22 - Table Type20
      23 - Table Type21
      24 - Table Type22
      25 - Table Type23
      26 - Table Type24
      27 - Table Type25
      28 - Table Type26
      29 - Table Type27
      30 - Table Type28
      31 - Table Type29
      32 - Table Type30
      33 - Table Type31
      34 - Table Type32
      35 - Table Type33
      36 - Table Type34
      37 - Table Type35
      38 - Table Type36
      39 - Table Type37
      40 - Table Type38
      41 - Table Type39
      42 - Table Type40
      43 - Table Type41
      44 - Table Type42
  45-127 - Reserved
     128 - Table Type126
     129 - Table Type127
**/
typedef UINT32  TABLE_GENERATOR_ID;

/** This enum lists the Table Generator Types.
*/
typedef enum TableGeneratorType {
  ETableGeneratorTypeAcpi = 0,  ///< ACPI Table Generator Type.
  ETableGeneratorTypeSmbios,    ///< SMBIOS Table Generator Type.
  ETableGeneratorTypeDt,        ///< Device Tree Table Generator Type.
  ETableGeneratorTypeReserved
} ETABLE_GENERATOR_TYPE;

/** This enum lists the namespaces for the Table Generators.
*/
typedef enum TableGeneratorNameSpace {
  ETableGeneratorNameSpaceStd = 0,  ///< Standard Namespace.
  ETableGeneratorNameSpaceOem       ///< OEM Namespace.
} ETABLE_GENERATOR_NAMESPACE;

/** A mask for the Table ID bits of TABLE_GENERATOR_ID.
*/
#define TABLE_ID_MASK                 0xFF

/** A mask for the Namespace ID bits of TABLE_GENERATOR_ID.
*/
#define TABLE_NAMESPACEID_MASK        (BIT31)

/** A mask for the Table Type bits of TABLE_GENERATOR_ID.
*/
#define TABLE_TYPE_MASK               (BIT29 | BIT28)

/** Starting bit position for the Table Type bits
*/
#define TABLE_TYPE_BIT_SHIFT          28

/** Starting bit position for the Table Namespace ID bit
*/
#define TABLE_NAMESPACE_ID_BIT_SHIFT  31

/** This macro returns the Table ID from the TableGeneratorId.

  @param [in]  TableGeneratorId  The table generator ID.

  @return the Table ID described by the TableGeneratorId.
**/
#define GET_TABLE_ID(TableGeneratorId)         \
          ((TableGeneratorId) & TABLE_ID_MASK)

/** This macro returns the Table type from the TableGeneratorId.

  @param [in]  TableGeneratorId  The table generator ID.

  @return the Table type described by the TableGeneratorId.
**/
#define GET_TABLE_TYPE(TableGeneratorId)                                   \
          (((TableGeneratorId) & TABLE_TYPE_MASK) >> TABLE_TYPE_BIT_SHIFT)

/** This macro returns the Namespace ID from the TableGeneratorId.

  @param [in]  TableGeneratorId  The table generator ID.

  @return the Namespace described by the TableGeneratorId.
**/
#define GET_TABLE_NAMESPACEID(TableGeneratorId)             \
          (((TableGeneratorId) & TABLE_NAMESPACEID_MASK) >> \
            TABLE_NAMESPACE_ID_BIT_SHIFT)

/** This macro checks if the TableGeneratorId is in the Standard Namespace.

  @param [in]  TableGeneratorId  The table generator ID.

  @return TRUE if the TableGeneratorId is in the Standard Namespace.
**/
#define IS_GENERATOR_NAMESPACE_STD(TableGeneratorId) \
          (                                          \
          GET_TABLE_NAMESPACEID(TableGeneratorId) == \
          ETableGeneratorNameSpaceStd                \
          )

/** This macro creates a TableGeneratorId

  @param [in]  TableType        The table type.
  @param [in]  TableNameSpaceId The namespace ID for the table.
  @param [in]  TableId          The table ID.

  @return a TableGeneratorId calculated from the inputs.
**/
#define CREATE_TABLE_GEN_ID(TableType, TableNameSpaceId, TableId)      \
          ((((TableType) << TABLE_TYPE_BIT_SHIFT) & TABLE_TYPE_MASK) | \
           (((TableNameSpaceId) << TABLE_NAMESPACE_ID_BIT_SHIFT) &     \
             TABLE_NAMESPACEID_MASK) | ((TableId) & TABLE_ID_MASK))

/** Starting bit position for MAJOR revision
*/
#define MAJOR_REVISION_BIT_SHIFT  16

/** A mask for Major revision.
*/
#define MAJOR_REVISION_MASK       0xFFFF

/** A mask for Minor revision.
*/
#define MINOR_REVISION_MASK       0xFFFF

/** This macro generates a Major.Minor version
    where the Major and Minor fields are 16 bit.

  @param [in]  Major  The Major revision.
  @param [in]  Minor  The Minor revision.

  @return a 32 bit representation of the type Major.Minor.
**/
#define CREATE_REVISION(Major, Minor)                                      \
          ((((Major) & MAJOR_REVISION_MASK) << MAJOR_REVISION_BIT_SHIFT) | \
            ((Minor) & MINOR_REVISION_MASK))

/** This macro returns the Major revision

  Extracts Major from the 32 bit representation of the type Major.Minor

  @param [in]  Revision  The Revision value which is 32 bit.

  @return the Major part of the revision.
**/
#define GET_MAJOR_REVISION(Revision)                                       \
          (((Revision) >> MAJOR_REVISION_BIT_SHIFT) & MAJOR_REVISION_MASK)

/** This macro returns the Minor revision

  Extracts Minor from the 32 bit representation of the type Major.Minor

  @param [in]  Revision  The Revision value which is 32 bit.

  @return the Minor part of the revision.
**/
#define GET_MINOR_REVISION(Revision)  ((Revision) & MINOR_REVISION_MASK)

#endif // TABLE_GENERATOR_H_

