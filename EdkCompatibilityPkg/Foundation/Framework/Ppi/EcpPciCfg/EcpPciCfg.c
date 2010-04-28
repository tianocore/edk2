/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

 EcpPciCfg.c

Abstract:

  This PPI which is same with PciCfg PPI. But Modify API is removed.

--*/

#include "Tiano.h"
#include "Pei.h"
#include EFI_PPI_DEFINITION (EcpPciCfg)

EFI_GUID  gEcpPeiPciCfgPpiGuid = ECP_PEI_PCI_CFG_PPI_GUID;

EFI_GUID_STRING(&gEcpPeiPciCfgPpiGuid, "Ecp PciCfg", "Ecp PciCfg PPI");
