/*++

Copyright (c) 2005, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  IpfPcRtc.c

Abstract:
  Register the extended SAL infrastructure.

  Make the EFI RT APIs call extended SAL calls via the RT lib wrappers.
  We can not do this on IA-32 as RT lib wrappers call via rRT.

--*/

#include "RealTimeClock.h"

//
// Don't use directly after virtual address have been registered.
//
PC_RTC_MODULE_GLOBALS  mModuleGlobal;

SAL_RETURN_REGS
PcRtcEsalServicesClassCommonEntry (
  IN  UINT64                                      FunctionId,
  IN  UINT64                                      Arg2,
  IN  UINT64                                      Arg3,
  IN  UINT64                                      Arg4,
  IN  UINT64                                      Arg5,
  IN  UINT64                                      Arg6,
  IN  UINT64                                      Arg7,
  IN  UINT64                                      Arg8,
  IN  SAL_EXTENDED_SAL_PROC                       ExtendedSalProc,
  IN  BOOLEAN                                     VirtualMode,
  IN  PC_RTC_MODULE_GLOBALS                       *Global
  )
/*++

Routine Description:

  Main entry for Extended SAL Reset Services

Arguments:

  FunctionId    Function Id which needed to be called.
  Arg2          EFI_RESET_TYPE, whether WARM of COLD reset
  Arg3          Last EFI_STATUS 
  Arg4          Data Size of UNICODE STRING passed in ARG5
  Arg5          Unicode String which CHAR16*

Returns:

  SAL_RETURN_REGS

--*/
// TODO:    Arg6 - add argument and description to function comment
// TODO:    Arg7 - add argument and description to function comment
// TODO:    Arg8 - add argument and description to function comment
// TODO:    ExtendedSalProc - add argument and description to function comment
// TODO:    VirtualMode - add argument and description to function comment
// TODO:    Global - add argument and description to function comment
{
  EFI_STATUS      EfiStatus;
  SAL_RETURN_REGS ReturnVal;

  switch (FunctionId) {
  case GetTime:
    EfiStatus = PcRtcGetTime ((EFI_TIME *) Arg2, (EFI_TIME_CAPABILITIES *) Arg3, Global);
    break;

  case SetTime:
    EfiStatus = PcRtcSetTime ((EFI_TIME *) Arg2, Global);
    break;

  case GetWakeupTime:
    EfiStatus = PcRtcGetWakeupTime ((BOOLEAN *) Arg2, (BOOLEAN *) Arg3, (EFI_TIME *) Arg4, Global);
    break;

  case SetWakeupTime:
    EfiStatus = PcRtcSetWakeupTime ((BOOLEAN) Arg2, (EFI_TIME *) Arg3, Global);
    break;

  case InitializeThreshold:
    EfiStatus = EFI_SAL_NOT_IMPLEMENTED;
    break;

  case BumpThresholdCount:
    EfiStatus = EFI_SAL_NOT_IMPLEMENTED;
    break;

  case GetThresholdCount:
    EfiStatus = EFI_SAL_NOT_IMPLEMENTED;
    break;

  case GetRtcFreq:
    EfiStatus = EFI_SAL_NOT_IMPLEMENTED;
    break;

  default:
    EfiStatus = EFI_SAL_INVALID_ARGUMENT;
    break;;
  }

  ReturnVal.Status = EfiStatus;
  return ReturnVal;
}

EFI_STATUS
EFIAPI
InitializePcRtc (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:

  Arguments:

  

Returns: 
--*/
// TODO:    ImageHandle - add argument and description to function comment
// TODO:    SystemTable - add argument and description to function comment
// TODO:    EFI_SUCCESS - add return value to function comment
{
  EFI_TIME              Time;
  EFI_TIME_CAPABILITIES Capabilities;
  EFI_STATUS            EfiStatus;

  EfiInitializeRuntimeDriverLib (ImageHandle, SystemTable, NULL);

  EfiInitializeLock (&mModuleGlobal.RtcLock, EFI_TPL_HIGH_LEVEL);

  EfiStatus = PcRtcInit (&mModuleGlobal);
  if (EFI_ERROR (EfiStatus)) {
    return EfiStatus;
  }

  RegisterEsalClass (
    &gEfiExtendedSalRtcServicesProtocolGuid,
    &mModuleGlobal,
    PcRtcEsalServicesClassCommonEntry,
    GetTime,
    PcRtcEsalServicesClassCommonEntry,
    SetTime,
    PcRtcEsalServicesClassCommonEntry,
    GetWakeupTime,
    PcRtcEsalServicesClassCommonEntry,
    SetWakeupTime,
    PcRtcEsalServicesClassCommonEntry,
    GetRtcFreq,
    PcRtcEsalServicesClassCommonEntry,
    InitializeThreshold,
    PcRtcEsalServicesClassCommonEntry,
    BumpThresholdCount,
    PcRtcEsalServicesClassCommonEntry,
    GetThresholdCount,
    NULL
    );
  //
  //  the following code is to initialize the RTC fields in case the values read
  //  back from CMOS are invalid at the first time.
  //
  EfiStatus = PcRtcGetTime (&Time, &Capabilities, &mModuleGlobal);
  if (EFI_ERROR (EfiStatus)) {
    Time.Second = RTC_INIT_SECOND;
    Time.Minute = RTC_INIT_MINUTE;
    Time.Hour   = RTC_INIT_HOUR;
    Time.Day    = RTC_INIT_DAY;
    Time.Month  = RTC_INIT_MONTH;
    Time.Year   = RTC_INIT_YEAR;
    PcRtcSetTime (&Time, &mModuleGlobal);
  }

  return EFI_SUCCESS;
}
