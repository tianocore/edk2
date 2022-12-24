/** @file
  Options handling code.

  Copyright (c) 2022, Qualcomm Innovation Center, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef MPSERVICESTEST_OPTIONS_H_
#define MPSERVICESTEST_OPTIONS_H_

#define INFINITE_TIMEOUT  0

typedef struct {
  UINTN      Timeout;
  UINTN      ProcessorIndex;
  BOOLEAN    RunAllAPs;
  BOOLEAN    RunSingleAP;
  BOOLEAN    DisableProcessor;
  BOOLEAN    EnableProcessor;
  BOOLEAN    SetProcessorHealthy;
  BOOLEAN    SetProcessorUnhealthy;
  BOOLEAN    PrintProcessorInformation;
  BOOLEAN    PrintBspProcessorIndex;
  BOOLEAN    RunAPsSequentially;
} MP_SERVICES_TEST_OPTIONS;

/**
  Parses any arguments provided on the command line.

  @param Options  The arguments structure.

  @return EFI_SUCCESS on success, or an error code.
**/
EFI_STATUS
ParseArguments (
  MP_SERVICES_TEST_OPTIONS  *Options
  );

#endif /* MPSERVICESTEST_OPTIONS_H_ */
