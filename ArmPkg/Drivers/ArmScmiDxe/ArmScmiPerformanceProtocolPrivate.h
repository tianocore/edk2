/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#ifndef ARM_SCMI_PERFORMANCE_PROTOCOL_PRIVATE_H_
#define ARM_SCMI_PERFORMANCE_PROTOCOL_PRIVATE_H_

#include <Protocol/ArmScmiPerformanceProtocol.h>

// Number of performance levels returned by a call to the SCP, Lvls Bits[11:0]
#define NUM_PERF_LEVELS_MASK          0x0FFF
#define NUM_PERF_LEVELS(Lvls) (Lvls & NUM_PERF_LEVELS_MASK)

// Number of performance levels remaining after a call to the SCP, Lvls Bits[31:16]
#define NUM_REMAIN_PERF_LEVELS_SHIFT  16
#define NUM_REMAIN_PERF_LEVELS(Lvls) (Lvls >> NUM_REMAIN_PERF_LEVELS_SHIFT)

/** Return values for SCMI_MESSAGE_ID_PERFORMANCE_DESCRIBE_LEVELS command.
  SCMI Spec § 4.5.2.5
**/
typedef struct {
  UINT32 NumLevels;

  // NOTE: Since EDK2 does not allow flexible array member [] we declare
  // here array of 1 element length. However below is used as a variable
  // length array.
  SCMI_PERFORMANCE_LEVEL PerfLevel[1]; // Offset to array of performance levels
} PERF_DESCRIBE_LEVELS;

/** Initialize performance management protocol and install on a given Handle.

  @param[in] Handle              Handle to install performance management
                                 protocol.

  @retval EFI_SUCCESS            Performance protocol installed successfully.
**/
EFI_STATUS
ScmiPerformanceProtocolInit (
  IN EFI_HANDLE* Handle
  );

#endif /* ARM_SCMI_PERFORMANCE_PROTOCOL_PRIVATE_H_ */
