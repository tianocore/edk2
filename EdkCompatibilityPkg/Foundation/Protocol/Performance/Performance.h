/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Performance.h

Abstract:

  
--*/

#ifndef _EFI_PERFORMANCE_H_
#define _EFI_PERFORMANCE_H_

#define EFI_PERFORMANCE_PROTOCOL_GUID \
  { 0xFFECFFFF, 0x923C, 0x14d2, {0x9E, 0x3F, 0x22, 0xA0, 0xC9, 0x69, 0x56, 0x3B} }

#define EFI_NULL_GUID \
  { 0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} }


EFI_FORWARD_DECLARATION (EFI_PERFORMANCE_PROTOCOL);

#define DXE_TOK L"DXE"
#define SHELL_TOK L"SHELL"
#define PEI_TOK L"PEI"
#define BDS_TOK L"BDS"
#define DRIVERBINDING_START_TOK L"DriverBinding:Start"
#define DRIVERBINDING_SUPPORT_TOK L"DriverBinding:Support"
#define START_IMAGE_TOK L"StartImage"
#define LOAD_IMAGE_TOK L"LoadImage"

#define DXE_PHASE 0
#define SHELL_PHASE 1
#define PEI_PHASE 2

#define EFI_PERF_TOKEN_LENGTH        32
#define EFI_PERF_HOST_LENGTH         32
#define EFI_PERF_PDBFILENAME_LENGTH  28

typedef struct {
  EFI_HANDLE        Handle; 
  UINT16            Token[EFI_PERF_TOKEN_LENGTH];
  UINT16            Host[EFI_PERF_HOST_LENGTH];
  UINT64            StartTick;
  UINT64            EndTick;
  EFI_GUID          GuidName;
  CHAR8             PdbFileName[EFI_PERF_PDBFILENAME_LENGTH];
  UINT8             Phase;
} EFI_GAUGE_DATA ;


typedef 
EFI_STATUS
(EFIAPI * EFI_PERF_START_GAUGE) (
  IN EFI_PERFORMANCE_PROTOCOL *This,
  IN EFI_HANDLE        Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker
  );

typedef 
EFI_STATUS
(EFIAPI * EFI_PERF_END_GAUGE) (
  IN EFI_PERFORMANCE_PROTOCOL *This,
  IN EFI_HANDLE        Handle,
  IN UINT16           *Token,
  IN UINT16           *Host,
  IN UINT64           Ticker
  );


typedef 
EFI_GAUGE_DATA *
(EFIAPI * EFI_PERF_GET_GAUGE) (
  IN EFI_PERFORMANCE_PROTOCOL *This,
  IN EFI_HANDLE                       Handle,     
  IN UINT16                           *Token,     
  IN UINT16                           *Host,      
  IN EFI_GAUGE_DATA                   *PrevGauge      
  );


struct _EFI_PERFORMANCE_PROTOCOL {
  EFI_PERF_START_GAUGE  StartGauge;
  EFI_PERF_END_GAUGE    EndGauge;
  EFI_PERF_GET_GAUGE    GetGauge;
};

extern EFI_GUID gEfiPerformanceProtocolGuid;

#endif

