/** @file
  Header file for FF-A ABI's that will be used for
  communication between S-EL0 and the Secure Partition
  Manager(SPM)

  Copyright (c) 2020, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
    - FF-A Version 1.0


**/

#ifndef ARM_FFA_SVC_H_
#define ARM_FFA_SVC_H_

#define ARM_SVC_ID_FFA_VERSION_AARCH32                  0x84000063
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_REQ_AARCH64      0xC400006F
#define ARM_SVC_ID_FFA_MSG_SEND_DIRECT_RESP_AARCH64     0xC4000070

#endif // ARM_FFA_SVC_H_
