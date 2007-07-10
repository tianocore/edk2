/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  ExtendedSalGuid.h

Abstract:

                  
--*/

#ifndef _EXTENDED_SAL_GUID_H_
#define _EXTENDED_SAL_GUID_H_

//
// Extended SAL Services protocol GUIDs
//


#define GUID_STRUCTURE(Lo, Hi) \
{ (Lo & 0xffffffff),           \
  ((Lo >> 32) & 0xffff),         \
  ((Lo >> 48) & 0xffff),         \
  { (Hi & 0xff),               \
    ((Hi >> 8) & 0xff),          \
    ((Hi >> 16) & 0xff),         \
    ((Hi >> 24) & 0xff),         \
    ((Hi >> 32) & 0xff),         \
    ((Hi >> 40) & 0xff),         \
    ((Hi >> 48) & 0xff),         \
    ((Hi >> 56) & 0xff)          \
  }                            \
}         


#define EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID_LO 0x451531e15aea42b5
#define EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID_HI 0xa6657525d5b831bc
#define EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID \
  { 0x5aea42b5, 0x31e1, 0x4515, {0xbc, 0x31, 0xb8, 0xd5, 0x25, 0x75, 0x65, 0xa6 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_BASE_IO_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID_LO 0x4d8cac2753a58d06
#define EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID_HI 0x704165808af0e9b5
#define EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID \
  { 0x53a58d06, 0xac27, 0x4d8c, {0xb5, 0xe9, 0xf0, 0x8a, 0x80, 0x65, 0x41, 0x70 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_STALL_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID_LO 0x4e17fe4f76b75c23
#define EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID_HI 0x4a49bb3d651aada2
#define EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID  \
  { 0x76b75c23, 0xfe4f, 0x4e17, {0xa2, 0xad, 0x1a, 0x65, 0x3d, 0xbb, 0x49, 0x4a } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_LOCK_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO 0x4871260ec1a74056
#define EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI 0x116e5ba645e631a0
#define EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID  \
  { 0xc1a74056, 0x260e, 0x4871, {0xa0, 0x31, 0xe6, 0x45, 0xa6, 0x5b, 0x6e, 0x11 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO 0x4d02efdb7e97a470
#define EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI 0x96a27bd29061ce8f 
#define EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID  \
  { 0x7e97a470, 0xefdb, 0x4d02, {0x8f, 0xce, 0x61, 0x90, 0xd2, 0x7b, 0xa2, 0x96 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO 0x4370c6414ecb6c53 
#define EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI 0x78836e490e3bb28c
#define EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID  \
  { 0x4ecb6c53, 0xc641, 0x4370, {0x8c, 0xb2, 0x3b, 0x0e, 0x49, 0x6e, 0x83, 0x78 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_LO 0x408b75e8899afd18
#define EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_HI 0x54f4cd7e2e6e1aa4
#define EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID  \
  { 0x899afd18, 0x75e8, 0x408b, {0xa4, 0x1a, 0x6e, 0x2e, 0x7e, 0xcd, 0xf4, 0x54 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_LO  0x46f58ce17d019990
#define EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_HI  0xa06a6798513c76a7
#define EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID  \
  { 0x7d019990, 0x8ce1, 0x46f5, {0xa7, 0x76, 0x3c, 0x51, 0x98, 0x67, 0x6a, 0xa0 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID_LO 0x420f55e9dbd91d
#define EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID_HI 0x4fb437849f5e3996
#define EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID  \
  { 0xdbd91d, 0x55e9, 0x420f, {0x96, 0x39, 0x5e, 0x9f, 0x84, 0x37, 0xb4, 0x4f } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO 0x4f1dbcbba2271df1
#define EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI 0x1a072f17bc06a998
#define EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID \
  { 0xa2271df1, 0xbcbb, 0x4f1d, {0x98, 0xa9, 0x06, 0xbc, 0x17, 0x2f, 0x07, 0x1a } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID_LO 0x4dc0cf18697d81a2
#define EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID_HI 0x3f8a613b11060d9e
#define EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID \
  { 0x697d81a2, 0xcf18, 0x4dc0, {0x9e, 0x0d, 0x06, 0x11, 0x3b, 0x61, 0x8a, 0x3f } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_MP_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID_LO 0x438d0fc2e1cd9d21
#define EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID_HI 0x571e966de6040397
#define EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID \
  { 0xe1cd9d21, 0x0fc2, 0x438d, {0x97, 0x03, 0x04, 0xe6, 0x6d, 0x96, 0x1e, 0x57 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_PAL_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID_LO 0x41c30fe0d9e9fa06
#define EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID_HI 0xf894335a4283fb96
#define EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID \
  { 0xd9e9fa06, 0x0fe0, 0x41c3, {0x96, 0xfb, 0x83, 0x42, 0x5a, 0x33, 0x94, 0xf8 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_BASE_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID_LO 0x42b16cc72a591128
#define EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID_HI 0xbb2d683b9358f08a
#define EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID \
  { 0x2a591128, 0x6cc7, 0x42b1, {0x8a, 0xf0, 0x58, 0x93, 0x3b, 0x68, 0x2d, 0xbb } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_MCA_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID_LO 0x4905ad66a46b1a31
#define EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID_HI 0x6330dc59462bf692
#define EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID \
  { 0xa46b1a31, 0xad66, 0x4905, {0x92, 0xf6, 0x2b, 0x46, 0x59, 0xdc, 0x30, 0x63 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_PCI_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID_LO 0x4ba52743edc9494
#define EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID_HI 0x88f11352ef0a1888
#define EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID \
  { 0xedc9494, 0x2743, 0x4ba5, { 0x88, 0x18, 0x0a, 0xef, 0x52, 0x13, 0xf1, 0x88 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_CACHE_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID_LO 0x4c0338a3cb3fd86e
#define EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID_HI 0x7aaba2a3cf905c9a
#define EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID \
  { 0xcb3fd86e, 0x38a3, 0x4c03, {0x9a, 0x5c, 0x90, 0xcf, 0xa3, 0xa2, 0xab, 0x7a } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_MCA_LOG_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID_LO 0x453c3e0ad5e4ee5f
#define EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID_HI 0x5a3606bb92b625a7
#define EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID \
  { 0xd5e4ee5f, 0x3e0a, 0x453c, {0xa7, 0x25, 0xb6, 0x92, 0xbb, 0x06, 0x36, 0x5a } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_ELOG_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID_LO 0x498285a14a153b6e
#define EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID_HI 0xa1aba4fc8c6af498
#define EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID \
  { 0x4a153b6e, 0x85a1, 0x4982, {0x98, 0xf4, 0x6a, 0x8c, 0xfc, 0xa4, 0xab, 0xa1 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_SENSOR_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID_LO 0x4e0881b74356799
#define EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID_HI 0x42ba47fa78d98da3
#define EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID \
  { 0x4356799, 0x81b7, 0x4e08, { 0xa3, 0x8d, 0xd9, 0x78, 0xfa, 0x47, 0xba, 0x42 } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID_LO, EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_PROTOCOL_GUID_HI)

#define EFI_EXTENDED_SAL_SST_GUID_LO 0x4b4e868a38802700
#define EFI_EXTENDED_SAL_SST_GUID_HI 0x6fb4cfdc1b4fd481
#define EFI_EXTENDED_SAL_SST_GUID \
  { 0x38802700, 0x868a, 0x4b4e, {0x81, 0xd4, 0x4f, 0x1b, 0xdc, 0xcf, 0xb4, 0x6f } }
//  GUID_STRUCTURE(EFI_EXTENDED_SAL_SST_GUID_LO, EFI_EXTENDED_SAL_SST_GUID_HI)

#define EFI_ERROR_EVENT_INFORMATION_PROTOCOL_GUID_LO 0x45431114D0D7913F
#define EFI_ERROR_EVENT_INFORMATION_PROTOCOL_GUID_HI 0x96FF7A730CC368B5
#define EFI_ERROR_EVENT_INFORMATION_PROTOCOL_GUID \
  { 0xD0D7913F, 0x1114, 0x4543, {0xB5, 0x68, 0xC3, 0x0C, 0x73, 0x7A, 0xFF, 0x96 } }

//
// Extended Sal Proc Function IDs.
//

//
// BugBug: These enums are name colisions waiting to happen. They should all be
//         prefixed with Esal! It might be better to just make them #define, so
//         they would be all caps.
//

typedef enum {
  IoReadFunctionId,
  IoWriteFunctionId,
  MemReadFunctionId,
  MemWriteFunctionId
} EFI_EXTENDED_SAL_BASE_IO_SERVICES_FUNC_ID;

typedef enum {
  StallFunctionId
} EFI_EXTENDED_SAL_STALL_FUNC_ID;


typedef enum {
  InitializeLockServiceFunctionId,
  AcquireLockServiceFunctionId,
  ReleaseLockServiceFunctionId,
  MaxLockServiceFunctionId
} EFI_EXTENDED_SAL_LOCK_SERVICES_FUNC_ID;

//
// BugBug : Covert the first 3 functions into a lib functions
// and move SalRegisterPhysicalAddress to SAL BASE Class
//
typedef enum {
  SetVirtualAddressFunctionId,
  IsVirtualFunctionId,
  IsEfiRuntimeFunctionId,
  SalRegisterPhysicalAddressFunctionId
} EFI_EXTENDED_SAL_VIRTUAL_SERVICES_FUNC_ID;

typedef enum {
  GetTimeFunctionId,
  SetTimeFunctionId,
  GetWakeupTimeFunctionId,
  SetWakeupTimeFunctionId,
  GetRtcFreqFunctionId,
  InitializeThresholdFunctionId,
  BumpThresholdCountFunctionId,
  GetThresholdCountFunctionId
} EFI_EXTENDED_SAL_RTC_SERVICES_FUNC_ID;

typedef enum {
  EsalGetVariableFunctionId,
  EsalGetNextVariableNameFunctionId,
  EsalSetVariableFunctionId
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  ,
  EsalQueryVariableInfoFunctionId
#endif
} EFI_EXTENDED_SAL_VARIABLE_SERVICES_FUNC_ID;

typedef enum {
  GetNextHighMonotonicCountFunctionId
} EFI_EXTENDED_SAL_MTC_SERVICES_FUNC_ID;

typedef enum {
  ResetSystemFunctionId
} EFI_EXTENDED_SAL_RESET_SERVICES_FUNC_ID;

typedef enum {
  StatusCodeFunctionId
} EFI_EXTENDED_SAL_STATUS_CODE_FUNC_ID;

typedef enum {
  ReportStatusCodeServiceFunctionId
} EFI_EXTENDED_SAL_STATUS_CODE_SERVICES_FUNC_ID;

typedef enum {
  ReadFunctionId,
  WriteFunctionId,
  EraseBlockFunctionId,
  GetVolumeAttributesFunctionId,
  SetVolumeAttributesFunctionId,
  GetPhysicalAddressFunctionId,
  GetBlockSizeFunctionId,
  EraseCustomBlockRangeFunctionId
} EFI_EXTENDED_SAL_FV_BLOCK_SERVICES_FUNC_ID;

typedef enum {
  AddCpuDataFunctionId,
  RemoveCpuDataFunctionId,
  ModifyCpuDataFunctionId,
  GetCpuDataByIDFunctionId,
  GetCpuDataByIndexFunctionId,
  SendIpiFunctionId,
  CurrentProcInfoFunctionId,
  NumProcessorsFunctionId,
  SetMinStateFunctionId,
  GetMinStateFunctionId,
  EsalPhysicalIdInfoFunctionId
} EFI_EXTENDED_SAL_MP_SERVICES_FUNC_ID;

typedef enum {
  PalProcFunctionId,
  SetNewPalEntryFunctionId,
  GetNewPalEntryFunctionId,
  EsalUpdatePalFunctionId
} EFI_EXTENDED_SAL_PAL_SERVICES_FUNC_ID;

typedef enum {
  McaVector,
  BspInitVector,
  BootRendezVector,
  ApInitVector
} ESAL_GET_VECTOR_TYPE;

typedef enum {
  SalSetVectorsFunctionId,
  SalMcRendezFunctionId,
  SalMcSetParamsFunctionId,
  EsalGetVectorsFunctionId,
  EsalMcGetParamsFunctionId,
  EsalMcGetMcParamsFunctionId,
  EsalGetMcCheckinFlagsFunctionId,
  EsalGetPlatformBaseFreqFunctionId,
  EsalRegisterPhysicalAddrFunctionId
} EFI_EXTENDED_SAL_BASE_SERVICES_FUNC_ID;

typedef enum {
  McaGetStateInfoFunctionId,
  McaRegisterCpuFunctionId
} EFI_EXTENDED_SAL_MCA_SERVICES_FUNC_ID;

typedef enum {
  SalPciConfigReadFunctionId,
  SalPciConfigWriteFunctionId
} EFI_EXTENDED_SAL_PCI_SERVICES_FUNC_ID;

typedef enum {
  SalCacheInitFunctionId,
  SalCacheFlushFunctionId
} EFI_EXTENDED_SAL_CACHE_SERVICES_FUNC_ID;

typedef enum {
  SalGetStateInfoFunctionId,
  SalGetStateInfoSizeFunctionId,
  SalClearStateInfoFunctionId,
  EsalGetStateBufferFunctionId,
  EsalSaveStateBufferFunctionId
} EFI_EXTENDED_SAL_MCA_LOG_SERVICES_FUNC_ID;

typedef enum {
  SalSetEventLogDataFunctionId,
  SalGetEventLogDataFunctionId,
  SalEraseEventLogDataFunctionId,
  SalActivateEventLogDataFunctionId
} EFI_EXTENDED_SAL_ELOG_SERVICES_FUNC_ID;

typedef enum {
  EsalGetComControllerInfoFunctionId,
  EsalSendComDataFunctionId,
  EsalReceiveComDataFunctionId 
} EFI_EXTENDED_SAL_SM_COM_LAYER_SERVICES_FUNC_ID;

typedef enum {
  SalUpdatePalFunctionId
} EFI_EXTENDED_SAL_UPDATE_PAL_SERVICES_FUNC_ID;

typedef enum {
  EsalReadSensorInfoFunctionId,   
  EsalReadSensorStatusFunctionId,
  EsalRearmSensorFunctionId,
  EsalReadSensorDataFunctionId
} EFI_EXTENDED_SAL_SENSOR_SERVICES_FUNC_ID;

typedef enum {
  EsalGetProcessorErrorEventInfoFunctionId,
  EsalGetPlatformErrorEventInfoFunctionId,
  EsalClearProcessorErrorEventInfoFunctionId,
  EsalClearPlatformErrorEventInfoFunctionId
} EFI_EXTENDED_SAL_ERROR_EVENT_INFO_FUNC_ID;

typedef struct {
  UINT64      ProtoData;       
} ESAL_GUID_DUMMY_PROTOCOL;

extern EFI_GUID gEfiExtendedSalBaseIoServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalStallServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalLockServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalVirtualServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalRtcServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalVariableServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalMtcServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalResetServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalStatusCodeServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalFvBlockServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalMpServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalPalServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalBaseServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalMcaServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalPciServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalCacheServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalMcaLogServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalElogServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalSensorServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalSmComLayerServicesProtocolGuid;
extern EFI_GUID gEfiExtendedSalSstGuid;
extern EFI_GUID gEfiExtendedSalErrorEventInfoProtocolGuid;


#endif
