/** @file
  The definition for VTD information PPI.

  This is a lightweight VTd information report in PEI phase.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __VTD_INFO_PPI_H__
#define __VTD_INFO_PPI_H__

#define EDKII_VTD_INFO_PPI_GUID \
    { \
      0x8a59fcb3, 0xf191, 0x400c, { 0x97, 0x67, 0x67, 0xaf, 0x2b, 0x25, 0x68, 0x4a } \
    }

typedef struct _EDKII_VTD_INFO_PPI  EDKII_VTD_INFO_PPI;

#define EDKII_VTD_INFO_PPI_REVISION 0x00010000

struct _EDKII_VTD_INFO_PPI {
  UINT64                                  Revision;
  UINT8                                   HostAddressWidth;
  UINT8                                   Reserved[3];
  UINT32                                  VTdEngineCount;
  UINT64                                  VTdEngineAddress[1];
};

extern EFI_GUID gEdkiiVTdInfoPpiGuid;

#endif

