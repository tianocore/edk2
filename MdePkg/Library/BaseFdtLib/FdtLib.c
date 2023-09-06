/** @file
  Flattened Device Tree Library.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <libfdt/libfdt/libfdt.h>

/**
  Convert UINT16 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT16
EFIAPI
Fdt16ToCpu (
  IN UINT16  Value
  )
{
  return fdt16_to_cpu (Value);
}

/**
  Convert UINT16 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT16
EFIAPI
CpuToFdt16 (
  IN UINT16  Value
  )
{
  return cpu_to_fdt16 (Value);
}

/**
  Convert UINT32 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT32
EFIAPI
Fdt32ToCpu (
  IN UINT32  Value
  )
{
  return fdt32_to_cpu (Value);
}

/**
  Convert UINT32 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT32
EFIAPI
CpuToFdt32 (
  IN UINT32  Value
  )
{
  return cpu_to_fdt32 (Value);
}

/**
  Convert UINT64 data of the FDT blob to little-endian

  @param[in] Value            The value to the blob data.

  @return The value to be converted to little-endian.

**/
UINT64
EFIAPI
Fdt64ToCpu (
  IN UINT64  Value
  )
{
  return fdt64_to_cpu (Value);
}

/**
  Convert UINT64 data to big-endian for aligned with the FDT blob

  @param[in] Value            The value to align with the FDT blob.

  @return The value to be converted to big-endian.

**/
UINT64
EFIAPI
CpuToFdt64 (
  IN UINT64  Value
  )
{
  return cpu_to_fdt64 (Value);
}

/**
  Verify the header of the Flattened Device Tree

  @param[in] Fdt            The pointer to FDT blob.

  @return Zero for successfully, otherwise failed.

**/
INT32
EFIAPI
FdtCheckHeader (
  IN CONST VOID  *Fdt
  )
{
  return fdt_check_header (Fdt);
}

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
  )
{
  return fdt_create_empty_tree (Buffer, (int)BufferSize);
}

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
  )
{
  return fdt_next_node (Fdt, Offset, Depth);
}

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
  )
{
  return fdt_first_subnode (Fdt, Offset);
}

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
  )
{
  return fdt_next_subnode (Fdt, Offset);
}

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
  )
{
  return fdt_subnode_offset_namelen (Fdt, ParentOffset, Name, NameLength);
}

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
  )
{
  return fdt_node_offset_by_prop_value (Fdt, StartOffset, PropertyName, PropertyValue, PropertyLength);
}

/**
  Returns a property with the given name from the given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     The offset to the given node.
  @param[in] Name           The name to the property which need be searched
  @param[in] Length         The length to the size of the property found.

  @return The property to the structure of the found property. Since the data
          come from FDT blob, it's encoding with big-endian.

**/
CONST struct fdt_property *
EFIAPI
FdtGetProperty (
  IN CONST VOID   *Fdt,
  IN INT32        NodeOffset,
  IN CONST CHAR8  *Name,
  IN INT32        *Length
  )
{
  return fdt_get_property (Fdt, NodeOffset, Name, Length);
}

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
  )
{
  return fdt_first_property_offset (Fdt, NodeOffset);
}

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
  )
{
  return fdt_next_property_offset (Fdt, Offset);
}

/**
  Returns a property from the given offset of the property.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] Offset         The offset to the given offset of the property.
  @param[in] Length         The length to the size of the property found.

  @return The property to the structure of the given property offset.

**/
CONST struct fdt_property *
EFIAPI
FdtGetPropertyByOffset (
  IN CONST VOID  *Fdt,
  IN INT32       Offset,
  IN INT32       *Length
  )
{
  return fdt_get_property_by_offset (Fdt, Offset, Length);
}

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
  )
{
  return fdt_get_string (Fdt, StrOffset, Length);
}

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
  )
{
  return fdt_add_subnode (Fdt, ParentOffset, Name);
}

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
  )
{
  return fdt_setprop (Fdt, NodeOffset, Name, Value, (int)Length);
}

/**
  Returns the name of a given node.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     Offset of node to check.
  @param[in] Length         The pointer to an integer variable (will be overwritten) or NULL.

  @return The pointer to the node's name.

**/
CONST CHAR8 *
EFIAPI
FdtGetName (
  IN VOID   *Fdt,
  IN INT32  NodeOffset,
  IN INT32  *Length
  )
{
  return fdt_get_name (Fdt, NodeOffset, Length);
}

/**
  FdtNodeDepth() finds the depth of a given node.  The root node
  has depth 0, its immediate subnodes depth 1 and so forth.

  @param[in] Fdt            The pointer to FDT blob.
  @param[in] NodeOffset     Offset of node to check.

  @returns Depth of the node at NodeOffset.
**/
INT32
EFIAPI
FdtNodeDepth (
  IN CONST VOID  *Fdt,
  IN INT32       NodeOffset
  )
{
  return fdt_node_depth (Fdt, NodeOffset);
}
