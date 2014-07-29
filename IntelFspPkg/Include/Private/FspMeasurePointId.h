/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FSP_MEASURE_POINT_ID_H_
#define _FSP_MEASURE_POINT_ID_H_

//
// 0xD0 - 0xEF are reserved for FSP common measure point
//
#define  FSP_PERF_ID_MRCINIT_ENTRY               0xD0
#define  FSP_PERF_ID_MRCINIT_EXIT                (FSP_PERF_ID_MRCINIT_ENTRY +  1)

#define  FSP_PERF_ID_SOCINIT_ENTRY               0xD8
#define  FSP_PERF_ID_SOCINIT_EXIT                (FSP_PERF_ID_SOCINIT_ENTRY +  1)

#define  FSP_PERF_ID_PCHINIT_ENTRY               0xDA
#define  FSP_PERF_ID_PCHINIT_EXIT                (FSP_PERF_ID_PCHINIT_ENTRY +  1)

#define  FSP_PERF_ID_CPUINIT_ENTRY               0xE0
#define  FSP_PERF_ID_CPUINIT_EXIT                (FSP_PERF_ID_CPUINIT_ENTRY +  1)


//
// 0xF0 - 0xFF are reserved for FSP API
//
#define  FSP_PERF_ID_API_TMPRAMINIT_ENTRY        0xF0
#define  FSP_PERF_ID_API_TMPRAMINIT_EXIT         (FSP_PERF_ID_API_TMPRAMINIT_ENTRY + 1)

#define  FSP_PERF_ID_API_FSPINIT_ENTRY           0xF2
#define  FSP_PERF_ID_API_FSPINIT_EXIT            (FSP_PERF_ID_API_FSPINIT_ENTRY + 1)

#define  FSP_PERF_ID_API_NOTIFY_POSTPCI_ENTRY    0xF4
#define  FSP_PERF_ID_API_NOTIFY_POSTPCI_EXIT     (FSP_PERF_ID_API_NOTIFY_POSTPCI_ENTRY + 1)

#define  FSP_PERF_ID_API_NOTIFY_RDYBOOT_ENTRY    0xF6
#define  FSP_PERF_ID_API_NOTIFY_RDYBOOT_EXIT     (FSP_PERF_ID_API_NOTIFY_RDYBOOT_ENTRY + 1)

#endif
