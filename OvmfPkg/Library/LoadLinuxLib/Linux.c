/** @file

  Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "LoadLinuxLib.h"

/**
  A simple check of the kernel setup image

  An assumption is made that the size of the data is at least the
  size of struct boot_params.

  @param[in]    KernelSetup - The kernel setup image

  @retval    EFI_SUCCESS - The kernel setup looks valid and supported
  @retval    EFI_INVALID_PARAMETER - KernelSetup was NULL
  @retval    EFI_UNSUPPORTED - The kernel setup is not valid or supported

**/
STATIC
EFI_STATUS
EFIAPI
BasicKernelSetupCheck (
  IN VOID  *KernelSetup
  )
{
  return LoadLinuxCheckKernelSetup (KernelSetup, sizeof (struct boot_params));
}

EFI_STATUS
EFIAPI
LoadLinuxCheckKernelSetup (
  IN VOID   *KernelSetup,
  IN UINTN  KernelSetupSize
  )
{
  struct boot_params  *Bp;

  if (KernelSetup == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (KernelSetupSize < sizeof (*Bp)) {
    return EFI_UNSUPPORTED;
  }

  Bp = (struct boot_params *)KernelSetup;

  if ((Bp->hdr.signature != 0xAA55) || // Check boot sector signature
      (Bp->hdr.header != SETUP_HDR) ||
      (Bp->hdr.version < 0x205) || // We only support relocatable kernels
      (!Bp->hdr.relocatable_kernel)
      )
  {
    return EFI_UNSUPPORTED;
  } else {
    return EFI_SUCCESS;
  }
}

UINTN
EFIAPI
LoadLinuxGetKernelSize (
  IN VOID   *KernelSetup,
  IN UINTN  KernelSize
  )
{
  struct boot_params  *Bp;

  if (EFI_ERROR (BasicKernelSetupCheck (KernelSetup))) {
    return 0;
  }

  Bp = (struct boot_params *)KernelSetup;

  if (Bp->hdr.version > 0x20a) {
    return Bp->hdr.init_size;
  } else {
    //
    // Add extra size for kernel decompression
    //
    return 3 * KernelSize;
  }
}

VOID *
EFIAPI
LoadLinuxAllocateKernelSetupPages (
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  Address = BASE_1GB;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiLoaderData,
                   Pages,
                   &Address
                   );
  if (!EFI_ERROR (Status)) {
    return (VOID *)(UINTN)Address;
  } else {
    return NULL;
  }
}

EFI_STATUS
EFIAPI
LoadLinuxInitializeKernelSetup (
  IN VOID  *KernelSetup
  )
{
  EFI_STATUS          Status;
  UINTN               SetupEnd;
  struct boot_params  *Bp;

  Status = BasicKernelSetupCheck (KernelSetup);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bp = (struct boot_params *)KernelSetup;

  SetupEnd = 0x202 + (Bp->hdr.jump & 0xff);

  //
  // Clear all but the setup_header
  //
  ZeroMem (KernelSetup, 0x1f1);
  ZeroMem (((UINT8 *)KernelSetup) + SetupEnd, 4096 - SetupEnd);
  DEBUG ((
    DEBUG_INFO,
    "Cleared kernel setup 0-0x1f1, 0x%Lx-0x1000\n",
    (UINT64)SetupEnd
    ));

  return EFI_SUCCESS;
}

VOID *
EFIAPI
LoadLinuxAllocateKernelPages (
  IN VOID   *KernelSetup,
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  KernelAddress;
  UINT32                Loop;
  struct boot_params    *Bp;

  if (EFI_ERROR (BasicKernelSetupCheck (KernelSetup))) {
    return NULL;
  }

  Bp = (struct boot_params *)KernelSetup;

  for (Loop = 1; Loop < 512; Loop++) {
    KernelAddress = MultU64x32 (
                      2 * Bp->hdr.kernel_alignment,
                      Loop
                      );
    Status = gBS->AllocatePages (
                    AllocateAddress,
                    EfiLoaderCode,
                    Pages,
                    &KernelAddress
                    );
    if (!EFI_ERROR (Status)) {
      return (VOID *)(UINTN)KernelAddress;
    }
  }

  return NULL;
}

VOID *
EFIAPI
LoadLinuxAllocateCommandLinePages (
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  Address = 0xa0000;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiLoaderData,
                   Pages,
                   &Address
                   );
  if (!EFI_ERROR (Status)) {
    return (VOID *)(UINTN)Address;
  } else {
    return NULL;
  }
}

VOID *
EFIAPI
LoadLinuxAllocateInitrdPages (
  IN VOID   *KernelSetup,
  IN UINTN  Pages
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  Address;

  struct boot_params  *Bp;

  if (EFI_ERROR (BasicKernelSetupCheck (KernelSetup))) {
    return NULL;
  }

  Bp = (struct boot_params *)KernelSetup;

  Address = (EFI_PHYSICAL_ADDRESS)(UINTN)Bp->hdr.ramdisk_max;
  Status  = gBS->AllocatePages (
                   AllocateMaxAddress,
                   EfiLoaderData,
                   Pages,
                   &Address
                   );
  if (!EFI_ERROR (Status)) {
    return (VOID *)(UINTN)Address;
  } else {
    return NULL;
  }
}

STATIC
VOID
SetupLinuxMemmap (
  IN OUT struct boot_params  *Bp
  )
{
  EFI_STATUS             Status;
  UINT8                  TmpMemoryMap[1];
  UINTN                  MapKey;
  UINTN                  DescriptorSize;
  UINT32                 DescriptorVersion;
  UINTN                  MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR  *MemoryMap;
  EFI_MEMORY_DESCRIPTOR  *MemoryMapPtr;
  UINTN                  Index;
  struct efi_info        *Efi;
  struct e820_entry      *LastE820;
  struct e820_entry      *E820;
  UINTN                  E820EntryCount;
  EFI_PHYSICAL_ADDRESS   LastEndAddr;

  //
  // Get System MemoryMapSize
  //
  MemoryMapSize = sizeof (TmpMemoryMap);
  Status        = gBS->GetMemoryMap (
                         &MemoryMapSize,
                         (EFI_MEMORY_DESCRIPTOR *)TmpMemoryMap,
                         &MapKey,
                         &DescriptorSize,
                         &DescriptorVersion
                         );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  //
  // Enlarge space here, because we will allocate pool now.
  //
  MemoryMapSize += EFI_PAGE_SIZE;
  Status         = gBS->AllocatePool (
                          EfiLoaderData,
                          MemoryMapSize,
                          (VOID **)&MemoryMap
                          );
  ASSERT_EFI_ERROR (Status);

  //
  // Get System MemoryMap
  //
  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT_EFI_ERROR (Status);

  LastE820       = NULL;
  E820           = &Bp->e820_map[0];
  E820EntryCount = 0;
  LastEndAddr    = 0;
  MemoryMapPtr   = MemoryMap;
  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); Index++) {
    UINTN  E820Type = 0;

    if (MemoryMap->NumberOfPages == 0) {
      continue;
    }

    switch (MemoryMap->Type) {
      case EfiReservedMemoryType:
      case EfiRuntimeServicesCode:
      case EfiRuntimeServicesData:
      case EfiMemoryMappedIO:
      case EfiMemoryMappedIOPortSpace:
      case EfiPalCode:
        E820Type = E820_RESERVED;
        break;

      case EfiUnusableMemory:
        E820Type = E820_UNUSABLE;
        break;

      case EfiACPIReclaimMemory:
        E820Type = E820_ACPI;
        break;

      case EfiLoaderCode:
      case EfiLoaderData:
      case EfiBootServicesCode:
      case EfiBootServicesData:
      case EfiConventionalMemory:
        E820Type = E820_RAM;
        break;

      case EfiACPIMemoryNVS:
        E820Type = E820_NVS;
        break;

      default:
        DEBUG ((
          DEBUG_ERROR,
          "Invalid EFI memory descriptor type (0x%x)!\n",
          MemoryMap->Type
          ));
        continue;
    }

    if ((LastE820 != NULL) &&
        (LastE820->type == (UINT32)E820Type) &&
        (MemoryMap->PhysicalStart == LastEndAddr))
    {
      LastE820->size += EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages);
      LastEndAddr    += EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages);
    } else {
      if (E820EntryCount >= ARRAY_SIZE (Bp->e820_map)) {
        break;
      }

      E820->type  = (UINT32)E820Type;
      E820->addr  = MemoryMap->PhysicalStart;
      E820->size  = EFI_PAGES_TO_SIZE ((UINTN)MemoryMap->NumberOfPages);
      LastE820    = E820;
      LastEndAddr = E820->addr + E820->size;
      E820++;
      E820EntryCount++;
    }

    //
    // Get next item
    //
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *)((UINTN)MemoryMap + DescriptorSize);
  }

  Bp->e820_entries = (UINT8)E820EntryCount;

  Efi                       = &Bp->efi_info;
  Efi->efi_systab           = (UINT32)(UINTN)gST;
  Efi->efi_memdesc_size     = (UINT32)DescriptorSize;
  Efi->efi_memdesc_version  = DescriptorVersion;
  Efi->efi_memmap           = (UINT32)(UINTN)MemoryMapPtr;
  Efi->efi_memmap_size      = (UINT32)MemoryMapSize;
  Efi->efi_systab_hi        = (UINT32)(((UINT64)(UINTN)gST) >> 32);
  Efi->efi_memmap_hi        = (UINT32)(((UINT64)(UINTN)MemoryMapPtr) >> 32);
  Efi->efi_loader_signature = SIGNATURE_32 ('E', 'L', '6', '4');

  gBS->ExitBootServices (gImageHandle, MapKey);
}

EFI_STATUS
EFIAPI
LoadLinuxSetCommandLine (
  IN OUT VOID  *KernelSetup,
  IN CHAR8     *CommandLine
  )
{
  EFI_STATUS          Status;
  struct boot_params  *Bp;

  Status = BasicKernelSetupCheck (KernelSetup);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bp = (struct boot_params *)KernelSetup;

  Bp->hdr.cmd_line_ptr = (UINT32)(UINTN)CommandLine;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadLinuxSetInitrd (
  IN OUT VOID  *KernelSetup,
  IN VOID      *Initrd,
  IN UINTN     InitrdSize
  )
{
  EFI_STATUS          Status;
  struct boot_params  *Bp;

  Status = BasicKernelSetupCheck (KernelSetup);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bp = (struct boot_params *)KernelSetup;

  Bp->hdr.ramdisk_start = (UINT32)(UINTN)Initrd;
  Bp->hdr.ramdisk_len   = (UINT32)InitrdSize;

  return EFI_SUCCESS;
}

STATIC VOID
FindBits (
  unsigned long  Mask,
  UINT8          *Pos,
  UINT8          *Size
  )
{
  UINT8  First, Len;

  First = 0;
  Len   = 0;

  if (Mask) {
    while (!(Mask & 0x1)) {
      Mask = Mask >> 1;
      First++;
    }

    while (Mask & 0x1) {
      Mask = Mask >> 1;
      Len++;
    }
  }

  *Pos  = First;
  *Size = Len;
}

STATIC
EFI_STATUS
SetupGraphicsFromGop (
  struct screen_info            *Si,
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info;
  EFI_STATUS                            Status;
  UINTN                                 Size;

  Status = Gop->QueryMode (Gop, Gop->Mode->Mode, &Size, &Info);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  /* We found a GOP */

  /* EFI framebuffer */
  Si->orig_video_isVGA = 0x70;

  Si->orig_x            = 0;
  Si->orig_y            = 0;
  Si->orig_video_page   = 0;
  Si->orig_video_mode   = 0;
  Si->orig_video_cols   = 0;
  Si->orig_video_lines  = 0;
  Si->orig_video_ega_bx = 0;
  Si->orig_video_points = 0;

  Si->lfb_base   = (UINT32)Gop->Mode->FrameBufferBase;
  Si->lfb_size   = (UINT32)Gop->Mode->FrameBufferSize;
  Si->lfb_width  = (UINT16)Info->HorizontalResolution;
  Si->lfb_height = (UINT16)Info->VerticalResolution;
  Si->pages      = 1;
  Si->vesapm_seg = 0;
  Si->vesapm_off = 0;

  if (Info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor) {
    Si->lfb_depth      = 32;
    Si->red_size       = 8;
    Si->red_pos        = 0;
    Si->green_size     = 8;
    Si->green_pos      = 8;
    Si->blue_size      = 8;
    Si->blue_pos       = 16;
    Si->rsvd_size      = 8;
    Si->rsvd_pos       = 24;
    Si->lfb_linelength = (UINT16)(Info->PixelsPerScanLine * 4);
  } else if (Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
    Si->lfb_depth      = 32;
    Si->red_size       = 8;
    Si->red_pos        = 16;
    Si->green_size     = 8;
    Si->green_pos      = 8;
    Si->blue_size      = 8;
    Si->blue_pos       = 0;
    Si->rsvd_size      = 8;
    Si->rsvd_pos       = 24;
    Si->lfb_linelength = (UINT16)(Info->PixelsPerScanLine * 4);
  } else if (Info->PixelFormat == PixelBitMask) {
    FindBits (
      Info->PixelInformation.RedMask,
      &Si->red_pos,
      &Si->red_size
      );
    FindBits (
      Info->PixelInformation.GreenMask,
      &Si->green_pos,
      &Si->green_size
      );
    FindBits (
      Info->PixelInformation.BlueMask,
      &Si->blue_pos,
      &Si->blue_size
      );
    FindBits (
      Info->PixelInformation.ReservedMask,
      &Si->rsvd_pos,
      &Si->rsvd_size
      );
    Si->lfb_depth = Si->red_size + Si->green_size +
                    Si->blue_size + Si->rsvd_size;
    Si->lfb_linelength = (UINT16)((Info->PixelsPerScanLine * Si->lfb_depth) / 8);
  } else {
    Si->lfb_depth      = 4;
    Si->red_size       = 0;
    Si->red_pos        = 0;
    Si->green_size     = 0;
    Si->green_pos      = 0;
    Si->blue_size      = 0;
    Si->blue_pos       = 0;
    Si->rsvd_size      = 0;
    Si->rsvd_pos       = 0;
    Si->lfb_linelength = Si->lfb_width / 2;
  }

  return Status;
}

STATIC
EFI_STATUS
SetupGraphics (
  IN OUT struct boot_params  *Bp
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         HandleCount;
  UINTN                         Index;
  EFI_GRAPHICS_OUTPUT_PROTOCOL  *Gop;

  ZeroMem ((VOID *)&Bp->screen_info, sizeof (Bp->screen_info));

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiGraphicsOutputProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      HandleBuffer[Index],
                      &gEfiGraphicsOutputProtocolGuid,
                      (VOID *)&Gop
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      Status = SetupGraphicsFromGop (&Bp->screen_info, Gop);
      if (!EFI_ERROR (Status)) {
        FreePool (HandleBuffer);
        return EFI_SUCCESS;
      }
    }

    FreePool (HandleBuffer);
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
SetupLinuxBootParams (
  IN OUT struct boot_params  *Bp
  )
{
  SetupGraphics (Bp);

  SetupLinuxMemmap (Bp);

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LoadLinux (
  IN VOID      *Kernel,
  IN OUT VOID  *KernelSetup
  )
{
  EFI_STATUS          Status;
  struct boot_params  *Bp;

  Status = BasicKernelSetupCheck (KernelSetup);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Bp = (struct boot_params *)KernelSetup;

  if ((Bp->hdr.version < 0x205) || !Bp->hdr.relocatable_kernel) {
    //
    // We only support relocatable kernels
    //
    return EFI_UNSUPPORTED;
  }

  InitLinuxDescriptorTables ();

  Bp->hdr.code32_start = (UINT32)(UINTN)Kernel;
  if ((Bp->hdr.version >= 0x20c) && Bp->hdr.handover_offset &&
      (Bp->hdr.xloadflags & ((sizeof (UINTN) == 4) ? BIT2 : BIT3)))
  {
    DEBUG ((DEBUG_INFO, "Jumping to kernel EFI handover point at ofs %x\n", Bp->hdr.handover_offset));

    DisableInterrupts ();
    JumpToUefiKernel ((VOID *)gImageHandle, (VOID *)gST, KernelSetup, Kernel);
  }

  //
  // Old kernels without EFI handover protocol
  //
  SetupLinuxBootParams (KernelSetup);

  DEBUG ((DEBUG_INFO, "Jumping to kernel\n"));
  DisableInterrupts ();
  SetLinuxDescriptorTables ();
  JumpToKernel (Kernel, (VOID *)KernelSetup);

  return EFI_SUCCESS;
}
