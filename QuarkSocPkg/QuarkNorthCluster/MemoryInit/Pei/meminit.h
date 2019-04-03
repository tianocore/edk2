/************************************************************************
 *
 * Copyright (c) 2013-2015 Intel Corporation.
 *
* SPDX-License-Identifier: BSD-2-Clause-Patent
 *
 ************************************************************************/
#ifndef _MEMINIT_H_
#define _MEMINIT_H_

// function prototypes
void MemInit(MRCParams_t *mrc_params);

typedef void (*MemInitFn_t)(MRCParams_t *mrc_params);

typedef struct MemInit_s {
  uint16_t    post_code;
  uint16_t    boot_path;
  MemInitFn_t init_fn;
} MemInit_t;

#endif // _MEMINIT_H_
