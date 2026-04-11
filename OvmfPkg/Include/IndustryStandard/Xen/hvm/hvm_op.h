/*
 * SPDX-License-Identifier: MIT
 */

#ifndef __XEN_PUBLIC_HVM_HVM_OP_H__
#define __XEN_PUBLIC_HVM_HVM_OP_H__

#include "../xen.h"

/* Get/set subcommands: extra argument == pointer to xen_hvm_param struct. */
#define HVMOP_set_param  0
#define HVMOP_get_param  1
struct xen_hvm_param {
  domid_t    domid;  /* IN */
  UINT32     index;  /* IN */
  UINT64     value;  /* IN/OUT */
};

typedef struct xen_hvm_param xen_hvm_param_t;
DEFINE_XEN_GUEST_HANDLE (xen_hvm_param_t);

#endif /* __XEN_PUBLIC_HVM_HVM_OP_H__ */
