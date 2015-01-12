/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

**/

#ifndef __COMMON_HEADER_H_
#define __COMMON_HEADER_H_



#include <FrameworkPei.h>

#include <IndustryStandard/SmBus.h>
#include <IndustryStandard/Pci22.h>
#include <Ppi/AtaController.h>
#include <Guid/Capsule.h>
#include <Ppi/Cache.h>
#include <Ppi/MasterBootMode.h>
#include <Guid/MemoryTypeInformation.h>
#include <Guid/RecoveryDevice.h>
#include <Ppi/ReadOnlyVariable2.h>
#include <Ppi/FvLoadFile.h>
#include <Ppi/DeviceRecoveryModule.h>
#include <Ppi/Capsule.h>
#include <Ppi/Reset.h>
#include <Ppi/Stall.h>
#include <Ppi/BootInRecoveryMode.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/RecoveryModule.h>
#include <Ppi/Smbus2.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/PciCf8Lib.h>
#include <Library/IoLib.h>
#include <Library/PciLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PcdLib.h>
#include <Library/SmbusLib.h>
#include <Library/TimerLib.h>
#include <Library/PrintLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PerformanceLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MtrrLib.h>


#endif
