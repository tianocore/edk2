/** @file
  Header file for PCCT parser

  Copyright (c) 2020, Arm Limited.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef PCCT_PARSER_H_
#define PCCT_PARSER_H_

/**
  Minimum value for the 'length' field in subspaces of types 0, 1 and 2.
*/
#define MIN_MEMORY_RANGE_LENGTH  8

/**
  Minimum value for the 'length' field in subspaces of types 3 and 4.
*/
#define MIN_EXT_PCC_SUBSPACE_MEM_RANGE_LEN  16

/**
  Maximum number of PCC subspaces.
*/
#define MAX_PCC_SUBSPACES  256

/**
  Parser for the header of any type of PCC subspace.
*/
#define PCC_SUBSPACE_HEADER()                                             \
  {L"Type", 1, 0, L"0x%x", NULL, (VOID**)&PccSubspaceType, NULL, NULL},   \
  {L"Length", 1, 1, L"%u", NULL, (VOID**)&PccSubspaceLength, NULL, NULL}

#endif // PCCT_PARSER_H_
