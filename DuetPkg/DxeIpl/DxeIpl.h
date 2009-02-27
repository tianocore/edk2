
#ifndef _DUET_DXEIPL_H_
#define _DUET_DXEIPL_H_

#include "FrameworkPei.h"
#include "FrameworkModulePei.h"

#include "EfiLdrHandoff.h"
#include "EfiFlashMap.h"

#include <Guid/MemoryTypeInformation.h>
#include <Guid/PciExpressBaseAddress.h>
#include <Guid/AcpiDescription.h>
#include <Guid/PeiPeCoffLoader.h>
#include <Guid/MemoryAllocationHob.h>
#include <Guid/Acpi.h>
#include <Guid/SmBios.h>
#include <Guid/Mps.h>
#include <Guid/FlashMapHob.h>
#include <Guid/SystemNvDataGuid.h>
#include <Guid/VariableFormat.h>

#include <Protocol/Decompress.h>
#include <Protocol/StatusCode.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PrintLib.h>
#include <Library/IoLib.h>

#include <CpuIA32.h>

#endif // _DUET_DXEIPL_H_

