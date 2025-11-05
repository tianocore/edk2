/** @file
  This file is cloned from DMTF libredfish library tag v1.0.0 and maintained
  by EDKII.

//----------------------------------------------------------------------------
// Copyright Notice:
// Copyright 2017 Distributed Management Task Force, Inc. All rights reserved.
// License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/libredfish/LICENSE.md
//----------------------------------------------------------------------------

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LIBREDFISH_REDPATH_H_
#define LIBREDFISH_REDPATH_H_

#include <Include/Library/RedfishCrtLib.h>

#include <jansson.h>

typedef struct _redPathNode {
  bool                   isRoot;
  bool                   isIndex;

  char                   *version;
  char                   *nodeName;
  size_t                 index;
  char                   *op;
  char                   *propName;
  char                   *value;

  struct _redPathNode    *next;
} redPathNode;

redPathNode *
parseRedPath (
  const char  *path
  );

void
cleanupRedPath (
  redPathNode  *node
  );

#endif
