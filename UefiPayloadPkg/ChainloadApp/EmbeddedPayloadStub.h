/** @file
  Stub embedded payload for standalone builds of ChainloadApp.

  BuildChainloadEmbedded.sh overrides these definitions by generating
  EmbeddedPayload.h in the build output directory; ChainloadApp.c
  selects it via __has_include() when present.  In a plain UefiPayloadPkg.dsc build the
  stub resolves to an empty payload that ChainloadEntry() rejects at
  run time with a clear diagnostic.

  Copyright (c) 2026, Amazon.com, Inc. or its affiliates. All Rights Reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

STATIC CONST UINT8  mPayloadData[] = { 0 };
STATIC CONST UINTN  mPayloadSize   = 0;
