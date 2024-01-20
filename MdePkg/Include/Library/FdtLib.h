/** @file
  Flattened Device Tree Library.

  All structure data are in big-endian format.
  Functions are provided for converting data between
  little-endian and big-endian.
  For example:
  Pushing data to FDT blob needs to convert data to
  big-endian by CpuToFdt*().
  Retrieving data from FDT blob needs to convert data to
  little-endian by Fdt*ToCpu().
  Refer to FDT specification: https://www.devicetree.org/specifications/

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef FDT_LIB_H_
#define FDT_LIB_H_

/**
  Flattened Device Tree definition

  The Devicetree Blob (DTB) format is a binary encoding with big-endian.
  When producing or consuming the blob data, CpuToFdt*() or Fdt*ToCpu()
  provided by this library may be called to convert data between
  big-endian and little-endian.
**/
typedef struct {
  UINT32    Magic;               /* magic word FDT_MAGIC */
  UINT32    TotalSize;           /* total size of DT block */
  UINT32    OffsetDtStruct;      /* offset to structure */
  UINT32    OffsetDtStrings;     /* offset to strings */
  UINT32    OffsetMemRsvmap;     /* offset to memory reserve map */
  UINT32    Version;             /* format version */
  UINT32    LastCompVersion;     /* last compatible version */

  /* version 2 fields below */
  UINT32    BootCpuidPhys;       /* Which physical CPU id we're
                                    booting on */
  /* version 3 fields below */
  UINT32    SizeDtStrings;       /* size of the strings block */

  /* version 17 fields below */
  UINT32    SizeDtStruct;        /* size of the structure block */
} FDT_HEADER;

typedef struct {
  UINT64    Address;
  UINT64    Size;
} FDT_RESERVE_ENTRY;

typedef struct {
  UINT32    Tag;
  CHAR8     Name[];
} FDT_NODE_HEADER;

typedef struct {
  UINT32    Tag;
  UINT32    Length;
  UINT32    NameOffset;
  CHAR8     Data[];
} FDT_PROPERTY;

/**
  Convert UINT16 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT16
EFIAPI
Fdt16ToCpu (
  IN UINT16  Value
  );

/**
  Convert UINT16 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT16
EFIAPI
CpuToFdt16 (
  IN UINT16  Value
  );

/**
  Convert UINT32 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT32
EFIAPI
Fdt32ToCpu (
  IN UINT32  Value
  );

/**
  Convert UINT32 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT32
EFIAPI
CpuToFdt32 (
  IN UINT32  Value
  );

/**
  Convert UINT64 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT64
EFIAPI
Fdt64ToCpu (
  IN UINT64  Value
  );

/**
  Convert UINT64 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT64
EFIAPI
CpuToFdt64 (
  IN UINT64  Value
  );

/**
  Verify the header of the Flattened Device Tree

  @param[in] Fdt            The pointer to FDT blob.

  @return Zero for successfully, otherwise failed.

**/
INT32
EFIAPI
FdtCheckHeader (
  IN CONST VOID  *Fdt
  );

/**
  Create a empty Flattened Device Tree.

  @param[in] Buffer         The pointer to allocate a pool for FDT blob.
  @param[in] BufferSize     The BufferSize to the pool size.

  @return Zero for successfully, otherwise failed.

**/
INT32
EFIAPI
FdtCreateEmptyTree (
  IN VOID    *Buffer,
  IN UINT32  BufferSize
  );

/**
  Returns a offset of next node from the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to previous node.
  @param[in] Depth          The depth to the level of tree hierarchy.

  @return The offset to next node offset.

**/
INT32
EFIAPI
FdtNextNode (
  IN CONST VOID  *Fdt,
  IN INT32       Offset,
  IN INT32       *Depth
  );

/**
  Returns a offset of first node under the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to previous node.

  @return The offset to next node offset.

**/
INT32
EFIAPI
FdtFirstSubnode (
  IN CONST VOID  *Fdt,
  IN INT32       Offset
  );

/**
  Returns a offset of next node from the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to previous node.

  @return The offset to next node offset.

**/
INT32
EFIAPI
FdtNextSubnode (
  IN CONST VOID  *Fdt,
  IN INT32       Offset
  );

/**
  Returns a offset of first node which includes the given name.

  @param[in] Fdt             The pointer to FDT blob.
  @param[in] ParentOffset    The offset to the node which start find under.
  @param[in] Name            The name to search the node with the name.
  @param[in] NameLength      The length of the name to check only.

  @return The offset to node offset with given node name.

**/
INT32
EFIAPI
FdtSubnodeOffsetNameLen (
  IN CONST VOID   *Fdt,
  IN INT32        ParentOffset,
  IN CONST CHAR8  *Name,
  IN INT32        NameLength
  );

/**
  Returns a offset of first node which includes the given property name and value.

  @param[in] Fdt             The pointer to FDT blob.
  @param[in] StartOffset     The offset to the starting node to find.
  @param[in] PropertyName    The property name to search the node including the named property.
  @param[in] PropertyValue   The property value (big-endian) to check the same property value.
  @param[in] PropertyLength  The length of the value in PropertValue.

  @return The offset to node offset with given property.

**/
INT32
EFIAPI
FdtNodeOffsetByPropValue (
  IN CONST VOID   *Fdt,
  IN INT32        StartOffset,
  IN CONST CHAR8  *PropertyName,
  IN CONST VOID   *PropertyValue,
  IN INT32        PropertyLength
  );

/**
  Returns a property with the given name from the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     The offset to the given node.
  @param[in] Name           The name to the property which need be searched
  @param[in] Length         The length to the size of the property found.

  @return The property to the structure of the found property. Since the data
          come from FDT blob, it's encoding with big-endian.

**/
CONST FDT_PROPERTY *
EFIAPI
FdtGetProperty (
  IN CONST VOID   *Fdt,
  IN INT32        NodeOffset,
  IN CONST CHAR8  *Name,
  IN INT32        *Length
  );

/**
  Returns a offset of first property in the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     The offset to the node which need be searched.

  @return The offset to first property offset in the given node.

**/
INT32
EFIAPI
FdtFirstPropertyOffset (
  IN CONST VOID  *Fdt,
  IN INT32       NodeOffset
  );

/**
  Returns a offset of next property from the given property.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to previous property.

  @return The offset to next property offset.

**/
INT32
EFIAPI
FdtNextPropertyOffset (
  IN CONST VOID  *Fdt,
  IN INT32       Offset
  );

/**
  Returns a property from the given offset of the property.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to the given offset of the property.
  @param[in] Length         The length to the size of the property found.

  @return The property to the structure of the given property offset.

**/
CONST FDT_PROPERTY *
EFIAPI
FdtGetPropertyByOffset (
  IN CONST VOID  *Fdt,
  IN INT32       Offset,
  IN INT32       *Length
  );

/**
  Returns a string by the given string offset.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] StrOffset      The offset to the location in the strings block of FDT.
  @param[in] Length         The length to the size of string which need be retrieved.

  @return The string to the given string offset.

**/
CONST CHAR8 *
EFIAPI
FdtGetString (
  IN CONST VOID  *Fdt,
  IN INT32       StrOffset,
  IN INT32       *Length        OPTIONAL
  );

/**
  Add a new node to the FDT.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] ParentOffset   The offset to the node offset which want to add in.
  @param[in] Name           The name to name the node.

  @return  The offset to the new node.

**/
INT32
EFIAPI
FdtAddSubnode (
  IN VOID         *Fdt,
  IN INT32        ParentOffset,
  IN CONST CHAR8  *Name
  );

/**
  Add or modify a property in the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     The offset to the node offset which want to add in.
  @param[in] Name           The name to name the property.
  @param[in] Value          The value (big-endian) to the property value.
  @param[in] Length         The length to the size of the property.

  @return  Zero for successfully, otherwise failed.

**/
INT32
EFIAPI
FdtSetProp (
  IN VOID         *Fdt,
  IN INT32        NodeOffset,
  IN CONST CHAR8  *Name,
  IN CONST VOID   *Value,
  IN UINT32       Length
  );

/**
  Returns the name of a given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffse      Offset of node to check.
  @param[in] Length         The pointer to an integer variable (will be overwritten) or NULL.

  @return The pointer to the node's name.

**/
CONST CHAR8 *
EFIAPI
FdtGetName (
  IN VOID   *Fdt,
  IN INT32  NodeOffset,
  IN INT32  *Length
  );

/**
  FdtNodeDepth() finds the depth of a given node.  The root node
  has depth 0, its immediate subnodes depth 1 and so forth.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     Offset of node to check.

  @return Depth of the node at NodeOffset.
**/
INT32
EFIAPI
FdtNodeDepth (
  IN CONST VOID  *Fdt,
  IN INT32       NodeOffset
  );

#endif /* FDT_LIB_H_ */
