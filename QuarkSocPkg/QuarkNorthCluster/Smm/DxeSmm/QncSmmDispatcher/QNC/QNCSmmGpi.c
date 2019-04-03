/** @file
File to contain all the hardware specific stuff for the Smm Gpi dispatch protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

//
// Include common header file for this module.
//
#include "CommonHeader.h"

#include "QNCSmmHelpers.h"

CONST QNC_SMM_SOURCE_DESC GPI_SOURCE_DESC = {
  QNC_SMM_NO_FLAGS,
  {
    {
      {GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIE}}, S_QNC_GPE0BLK_SMIE, N_QNC_GPE0BLK_SMIE_GPIO
    },
    NULL_BIT_DESC_INITIALIZER
  },
  {
    {
      {GPE_ADDR_TYPE, {R_QNC_GPE0BLK_SMIS}}, S_QNC_GPE0BLK_SMIS, N_QNC_GPE0BLK_SMIS_GPIO
  }
  }
};

