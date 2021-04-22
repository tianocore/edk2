#include "AcpiPlatform.h"
#include <Library/MemoryAllocationLib.h>
#include <IndustryStandard/Acpi63.h>

// There is no elegant way to get the rsdp address in cloud-hyervisor
// so we set it in hard code
#define CLH_RSDP_ADDR 0x40200000

#define ACPI_ENTRY_SIZE 8
#define XSDT_LEN 36

EFI_STATUS
EFIAPI
InstallClhAcpiTable (
 IN     EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol
 )
{
  UINTN InstalledKey, TableSize;
  UINT64 Rsdp, Xsdt, table_offset, PointerValue;
  EFI_STATUS Status = 0;
  int size;

  Rsdp = PcdGet64 (PcdRsdpBaseAddress);
  Xsdt = ((EFI_ACPI_6_3_ROOT_SYSTEM_DESCRIPTION_POINTER *)Rsdp)->XsdtAddress;
  PointerValue = Xsdt;
  table_offset = Xsdt;
  size = ((EFI_ACPI_COMMON_HEADER *)PointerValue)->Length - XSDT_LEN;
  table_offset += XSDT_LEN;

  while(!Status && size > 0) {
    PointerValue = *(UINT64 *)table_offset;
    TableSize = ((EFI_ACPI_COMMON_HEADER *)PointerValue)->Length;
    Status = AcpiProtocol->InstallAcpiTable (AcpiProtocol,
             (VOID *)(UINT64)PointerValue, TableSize,
             &InstalledKey);
    table_offset += ACPI_ENTRY_SIZE;
    size -= ACPI_ENTRY_SIZE;
  }

  return Status;
}
