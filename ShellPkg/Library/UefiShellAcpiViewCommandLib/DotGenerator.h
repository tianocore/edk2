/** @file
  Header file for Dot File Generation

  Copyright (c) 2021, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef DOT_GENERATOR_H_
#define DOT_GENERATOR_H_

#include <Protocol/Shell.h>

#define DOT_COLOR_MASK            0b111
// Flags for color of arrow or node.
#define DOT_COLOR_BLACK           0b000 // default
#define DOT_COLOR_GRAY            0b001
#define DOT_COLOR_BLUE            0b010
#define DOT_COLOR_YELLOW          0b011
#define DOT_COLOR_RED             0b100

#define DOT_ARROW_TYPE_MASK       0b1000
// Flags for style of arrow.
#define DOT_ARROW_FULL            0b0000 // default
#define DOT_ARROW_DOTTED          0b1000

// Flag for reversing how the nodes will be ranked and displayed.
#define DOT_ARROW_RANK_REVERSE    0b10000

#define DOT_BOX_TYPE_MASK         0b1100000
// Flag for shape of box
#define DOT_BOX_SQUARE            0b0000000 // default
#define DOT_BOX_DIAMOND           0b0100000

// Flag for adding the node's ID to the end of the label.
#define DOT_BOX_ADD_ID_TO_LABEL   0b10000000

// Valid flags for DotAddNode.
#define DOT_BOX_FLAGS_MASK        (DOT_COLOR_MASK |\
                                   DOT_BOX_TYPE_MASK |\
                                   DOT_BOX_ADD_ID_TO_LABEL)
// Valid flags for DotAddLink.
#define DOT_ARROW_FLAGS_MASK      (DOT_COLOR_MASK |\
                                   DOT_ARROW_TYPE_MASK |\
                                   DOT_ARROW_RANK_REVERSE)


/**
  Opens a new dot file and writes a dot directional graph.

  @param [in] FileName    Null terminated unicode string.
**/
SHELL_FILE_HANDLE
DotOpenNewFile (
  IN CHAR16* FileName
  );

/**
  Writes a dot graph footer and closes the dot file.

  @param [in] DotFileHandle  The handle of the dot file.
**/
VOID
DotCloseFile (
  SHELL_FILE_HANDLE DotFileHandle
  );

/**
  Writes a line in the previously opened dot file describing a
  new node.

  @param [in] DotFileHandle  The handle of the dot file.
  @param [in] Id             A unique identifier for the node.
  @param [in] Flags          Flags describing the node's characteristics.
  @param [in] Label          Label to be shown on the graph node.
**/
VOID
DotAddNode (
  SHELL_FILE_HANDLE DotFileHandle,
  IN UINT32         Id,
  IN UINT16         Flags,
  IN CONST CHAR16*  Label
  );

/**
  Writes a line in the previously opened dot file describing a
  new link between two nodes.

  @param [in] DotFileHandle  The handle of the dot file.
  @param [in] IdSource       An identifier for the source node of the link.
  @param [in] IdTarget       An identifier for the target node of the link.
  @param [in] Flags          Flags describing the node's characteristics.
**/
VOID
DotAddLink (
  SHELL_FILE_HANDLE DotFileHandle,
  IN UINT32 IdSource,
  IN UINT32 IdTarget,
  IN UINT16 Flags
  );

#endif // DOT_GENERATOR_H_
