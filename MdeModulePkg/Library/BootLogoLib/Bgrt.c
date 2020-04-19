#include <IndustryStandard/Acpi.h>
#include <IndustryStandard/Bmp.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootLogoLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/SimpleFileSystem.h>

/** RSDP (Root System Description Pointer) */
typedef struct {
  CHAR8  signature[8];
  UINT8  checksum;
  CHAR8  oem_id[6];
  UINT8  revision;
  UINT32 rsdt_address;
  UINT32 length;
  UINT64 xsdt_address;
  UINT8  extended_checksum;
  UINT8  reserved[3];
} ACPI_20_RSDP;

/** SDT (System Description Table) entry header */
typedef struct {
    CHAR8  signature[4];
    UINT32 length;
    UINT8  revision;
    UINT8  checksum;
    CHAR8  oem_id[6];
    CHAR8  oem_table_id[8];
    UINT32 oem_revision;
    UINT32 asl_compiler_id;
    UINT32 asl_compiler_revision;
} ACPI_SDT_HEADER;

/** BGRT structure */
typedef struct {
    ACPI_SDT_HEADER header;
    UINT16 version;
    UINT8  status;
    UINT8  image_type;
    UINT64 image_address;
    UINT32 image_offset_x;
    UINT32 image_offset_y;
} ACPI_BGRT;

EFI_STATUS
EFIAPI
LoadBmp(
    OUT EFI_PHYSICAL_ADDRESS *BmpAddress,
    OUT UINT32 *BmpSize
)
{
  EFI_STATUS                    Status;
  UINTN                         FvProtocolCount;
  EFI_HANDLE                    *FvHandles;
  EFI_FIRMWARE_VOLUME2_PROTOCOL  *Fv;
  UINTN                         Index;
  UINT32                        AuthenticationStatus;

  UINT8                         *Buffer;
  UINTN                         BmpBufferSize;

  Buffer = 0;
  FvHandles       = NULL;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiFirmwareVolume2ProtocolGuid,
    NULL,
    &FvProtocolCount,
    &FvHandles
    );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < FvProtocolCount; Index++) {
      Status = gBS->HandleProtocol (
                      FvHandles[Index],
                      &gEfiFirmwareVolume2ProtocolGuid,
                      (VOID **) &Fv
                      );
      BmpBufferSize = 0;
      Status = Fv->ReadSection (
                     Fv,
                     (EFI_GUID *)PcdGetPtr(PcdLogoFile),
                     EFI_SECTION_RAW,
                     0,
                    (void **)&Buffer,
                     &BmpBufferSize,
                     &AuthenticationStatus
                     );

      if (!EFI_ERROR (Status)) {
        *BmpAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)Buffer;
        *BmpSize = (UINT32)BmpBufferSize;
        Status = EFI_SUCCESS;
        break;
      }
    }
  } else {
    Status = EFI_NOT_FOUND;
  }

  if (FvHandles != NULL) {
    gBS->FreePool (FvHandles);
    FvHandles = NULL;
  }

  return Status;
}

UINT8 SumBytes(const UINT8* arr, UINTN size) {
  UINT8 sum = 0;
  UINTN i;
  for (i = 0; i < size; ++i) {
    sum += arr[i];
  }
  return sum;
}

int VerifyAcpiRsdp2Checksums(const void* data) {
  const UINT8* arr = data;
  UINTN size = *(const UINT32*)&arr[20];
  return SumBytes(arr, 20) == 0 && SumBytes(arr, size) == 0;
}

void SetAcpiRsdp2Checksums(void* data) {
  UINT8* arr = data;
  UINTN size = *(const UINT32*)&arr[20];
  arr[9] = 0;
  arr[32] = 0;
  arr[9] = -SumBytes(arr, 20);
  arr[32] = -SumBytes(arr, size);
}

int VerifyAcpiSdtChecksum(const void* data) {
  const UINT8* arr = data;
  UINTN size = *(const UINT32*)&arr[4];
  return SumBytes(arr, size) == 0;
}

void SetAcpiSdtChecksum(void* data) {
  UINT8* arr = data;
  UINTN size = *(const UINT32*)&arr[4];
  arr[9] = 0;
  arr[9] = -SumBytes(arr, size);
}

const CHAR16* TmpStr(CHAR8 *src, int length) {
  static CHAR16 arr[4][16];
  static int j;
  CHAR16* dest = arr[j = (j+1) % 4];
  int i;
  for (i = 0; i < length && i < 16-1 && src[i]; ++i) {
    dest[i] = src[i];
  }
  dest[i] = 0;
  return dest;
}

static UINT32 min(UINT32 first, UINT32 second){
  if (first < second)
    return first;
  return second;
}

ACPI_SDT_HEADER* CreateXsdt(ACPI_SDT_HEADER* xsdt0, UINTN entries) {
  ACPI_SDT_HEADER* xsdt = 0;
  UINT32 xsdt_len = (UINT32)(sizeof(ACPI_SDT_HEADER) + entries * sizeof(UINT64));
  gBS->AllocatePool(EfiACPIReclaimMemory, xsdt_len, (void**)&xsdt);
  if (!xsdt) {
    DEBUG ((EFI_D_INFO, "HackBGRT: Failed to allocate memory for XSDT.\n"));
    return 0;
  }
  ZeroMem(xsdt, xsdt_len);
  CopyMem(xsdt, xsdt0, min(xsdt0->length, xsdt_len));
  xsdt->length = xsdt_len;
  SetAcpiSdtChecksum(xsdt);
  return xsdt;
}

static ACPI_BGRT* HandleAcpiTables(ACPI_BGRT* bgrt) {
  int i;

  for (i = 0; i < gST->NumberOfTableEntries; i++) {
    EFI_GUID* vendor_guid = &gST->ConfigurationTable[i].VendorGuid;
    ACPI_20_RSDP *rsdp;
    ACPI_SDT_HEADER *xsdt;
    UINT64 *entry_arr;
    UINT32 entry_arr_length;

    if (!CompareGuid(vendor_guid, &gEfiAcpiTableGuid) && !CompareGuid(vendor_guid, &gEfiAcpi20TableGuid)) {
      continue;
    }
    rsdp = (ACPI_20_RSDP *) gST->ConfigurationTable[i].VendorTable;
    if (CompareMem(rsdp->signature, "RSD PTR ", 8) != 0 || rsdp->revision < 2 || !VerifyAcpiRsdp2Checksums(rsdp)) {
      continue;
    }
    DEBUG ((EFI_D_INFO, "RSDP: revision = %d, OEM ID = %s\n", rsdp->revision, TmpStr(rsdp->oem_id, 6)));

    xsdt = (ACPI_SDT_HEADER *) (UINTN) rsdp->xsdt_address;
    if (!xsdt || CompareMem(xsdt->signature, "XSDT", 4) != 0 || !VerifyAcpiSdtChecksum(xsdt)) {
      DEBUG ((EFI_D_INFO, "* XSDT: missing or invalid\n"));
      continue;
    }
    entry_arr = (UINT64*)&xsdt[1];
    entry_arr_length = (xsdt->length - sizeof(*xsdt)) / sizeof(UINT64);

    DEBUG ((EFI_D_INFO, "* XSDT: OEM ID = %s, entry count = %d\n", TmpStr(xsdt->oem_id, 6), entry_arr_length));

    if (bgrt) {
      DEBUG ((EFI_D_INFO, " - Adding missing BGRT.\n"));
      xsdt = CreateXsdt(xsdt, entry_arr_length + 1);
      entry_arr = (UINT64*)&xsdt[1];
      entry_arr[entry_arr_length++] = (UINTN) bgrt;
      rsdp->xsdt_address = (UINTN) xsdt;
      SetAcpiRsdp2Checksums(rsdp);
    }
    SetAcpiSdtChecksum(xsdt);
  }
  return bgrt;
}

VOID
AddBGRT (
  VOID
  )
{
  EFI_STATUS                         Status;
  ACPI_BGRT                          *bgrt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL       *GraphicsOutput;
  EFI_PHYSICAL_ADDRESS               BmpAddress;
  UINT32                             BmpSize;
  BMP_IMAGE_HEADER                   *BmpHeader;
  const char data[0x38] =
    "BGRT" "\x38\x00\x00\x00" "\x00" "\xd6" "INTEL " "    EDK2"
    "\x20\x17\x00\x00" "PTL " "\x02\x00\x00\x00"
    "\x01\x00" "\x00" "\x00";

  BmpAddress = 0;

  DEBUG ((EFI_D_INFO, "HackBGRT Start\n"));

  Status = gBS->HandleProtocol (
    gST->ConsoleOutHandle,
    &gEfiGraphicsOutputProtocolGuid,
    (VOID**)&GraphicsOutput
    );

  // Replace missing = allocate new.
  gBS->AllocatePool(EfiACPIReclaimMemory, sizeof(*bgrt), (void**)&bgrt);
  if (!bgrt) {
    DEBUG ((EFI_D_INFO, "HackBGRT MEM ERR\n"));
    return;
  }

  DEBUG ((EFI_D_INFO, "HackBGRT Load Bmp\n"));
  Status = LoadBmp(&BmpAddress, &BmpSize);
  if (EFI_ERROR(Status)){
    DEBUG ((EFI_D_INFO, "HackBGRT BMP Load ERR\n"));
    return;
  }

  DEBUG ((EFI_D_INFO, "HackBGRT Set Table; BMP Size: %d\n", BmpSize));
  // Clear the BGRT.
  CopyMem(bgrt, data, sizeof(data));

  if (GraphicsOutput != NULL && GraphicsOutput->Mode != NULL && GraphicsOutput->Mode->Info != NULL) 
  {
      BmpHeader = (BMP_IMAGE_HEADER *)BmpAddress;
      bgrt->image_address = (UINTN)BmpAddress;
      bgrt->image_offset_x = (GraphicsOutput->Mode->Info->HorizontalResolution - BmpHeader->PixelWidth) / 2;
      bgrt->image_offset_y = ((GraphicsOutput->Mode->Info->VerticalResolution * 382) / 1000) -
                             (BmpHeader->PixelHeight / 2);
      DEBUG ((EFI_D_INFO, "HackBGRT Set checksum\n"));
      SetAcpiSdtChecksum(bgrt);
      DEBUG ((EFI_D_INFO, "HackBGRT Add Table\n"));
      HandleAcpiTables(bgrt);
  } else {
      DEBUG ((EFI_D_INFO, "HackBGRT no display connected, skip adding table\n"));
  }
}
