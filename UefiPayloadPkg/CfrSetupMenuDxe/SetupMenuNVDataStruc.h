/** @file
  A Setup Menu for configuring boot options defined by bootloader CFR.

  Copyright (c) 2023, 9elements GmbH.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SETUPMENUNVDATASTRUC_H_
#define _SETUPMENUNVDATASTRUC_H_

#define SETUP_MENU_FORMSET_GUID \
  { \
    0x93E6FCD9, 0x8E17, 0x43DF, { 0xB7, 0xF0, 0x91, 0x3E, 0x58, 0xB1, 0xA7, 0x89 } \
  }

#define SETUP_MENU_FORM_ID   0x0001
#define LABEL_RT_COMP_START  0x0001
#define CFR_COMPONENT_START  0x1000
#define LABEL_RT_COMP_END    0xefff

#endif
