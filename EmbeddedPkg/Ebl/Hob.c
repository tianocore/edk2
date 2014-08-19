/** @file
  Hob command for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  Hob.c

  Search/Replace Dir with the name of your new command

  Boot Mode:
  ==========
  BOOT_WITH_FULL_CONFIGURATION                  0x00
  BOOT_WITH_MINIMAL_CONFIGURATION               0x01
  BOOT_ASSUMING_NO_CONFIGURATION_CHANGES        0x02
  BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS 0x03
  BOOT_WITH_DEFAULT_SETTINGS                    0x04
  BOOT_ON_S4_RESUME                             0x05
  BOOT_ON_S5_RESUME                             0x06
  BOOT_ON_S2_RESUME                             0x10
  BOOT_ON_S3_RESUME                             0x11
  BOOT_ON_FLASH_UPDATE                          0x12
  BOOT_IN_RECOVERY_MODE                         0x20
  BOOT_IN_RECOVERY_MODE_MASK                    0x40
  BOOT_SPECIAL_MASK                             0x80

  Mem Alloc HOB Type:
  ===================
  typedef enum {
    EfiReservedMemoryType       = 0x00
    EfiLoaderCode               = 0x01
    EfiLoaderData               = 0x02
    EfiBootServicesCode         = 0x03
    EfiBootServicesData         = 0x04
    EfiRuntimeServicesCode      = 0x05
    EfiRuntimeServicesData      = 0x06
    EfiConventionalMemory       = 0x07
    EfiUnusableMemory           = 0x08
    EfiACPIReclaimMemory        = 0x09
    EfiACPIMemoryNVS            = 0x0a
    EfiMemoryMappedIO           = 0x0b
    EfiMemoryMappedIOPortSpace  = 0x0c
    EfiPalCode                  = 0x0d
    EfiMaxMemoryType            = 0x0e
  } EFI_MEMORY_TYPE;

  Resource Hob Tye:
  =================
  EFI_RESOURCE_SYSTEM_MEMORY          0
  EFI_RESOURCE_MEMORY_MAPPED_IO       1
  EFI_RESOURCE_IO                     2
  EFI_RESOURCE_FIRMWARE_DEVICE        3
  EFI_RESOURCE_MEMORY_MAPPED_IO_PORT  4
  EFI_RESOURCE_MEMORY_RESERVED        5
  EFI_RESOURCE_IO_RESERVED            6
  EFI_RESOURCE_MAX_MEMORY_TYPE        7

  Resource Hob Attribute (last thing printed):
  ============================================
  EFI_RESOURCE_ATTRIBUTE_PRESENT                  0x00000001
  EFI_RESOURCE_ATTRIBUTE_INITIALIZED              0x00000002
  EFI_RESOURCE_ATTRIBUTE_TESTED                   0x00000004
  EFI_RESOURCE_ATTRIBUTE_SINGLE_BIT_ECC           0x00000008
  EFI_RESOURCE_ATTRIBUTE_MULTIPLE_BIT_ECC         0x00000010
  EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_1           0x00000020
  EFI_RESOURCE_ATTRIBUTE_ECC_RESERVED_2           0x00000040
  EFI_RESOURCE_ATTRIBUTE_READ_PROTECTED           0x00000080
  EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED          0x00000100
  EFI_RESOURCE_ATTRIBUTE_EXECUTION_PROTECTED      0x00000200
  EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE              0x00000400
  EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE        0x00000800
  EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE  0x00001000
  EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE     0x00002000
  EFI_RESOURCE_ATTRIBUTE_16_BIT_IO                0x00004000
  EFI_RESOURCE_ATTRIBUTE_32_BIT_IO                0x00008000
  EFI_RESOURCE_ATTRIBUTE_64_BIT_IO                0x00010000
  EFI_RESOURCE_ATTRIBUTE_UNCACHED_EXPORTED        0x00020000

**/

#include "Ebl.h"
// BugBug: Autogen does not allow this to be included currently
//#include <EdkModulePkg/Include/EdkDxe.h>

GLOBAL_REMOVE_IF_UNREFERENCED char *mHobResourceType[] = {
  "Memory     ",
  "MMIO       ",
  "IO         ",
  "Firmware   ",
  "MMIO Port  ",
  "Reserved   ",
  "IO Reserved",
  "Illegal    "
};


/**
  Dump out the HOBs in the system. HOBs are defined in the PI specification
  and they are used to hand off information from PEI to DXE.

  Argv[0] - "hob"

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblHobCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  UINTN                         CurrentRow;
  EFI_PEI_HOB_POINTERS          Hob;
  EFI_MEMORY_TYPE_INFORMATION   *EfiMemoryTypeInformation;
  UINTN                         Index;

  CurrentRow = 0;
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB(Hob)) {
    if (Hob.Header->HobType == EFI_HOB_TYPE_HANDOFF) {
      AsciiPrint ("PHIT HOB Ver %x Boot Mode %02x Top %lx  Bottom %lx\n",
        Hob.HandoffInformationTable->Version,
        Hob.HandoffInformationTable->BootMode,
        Hob.HandoffInformationTable->EfiMemoryTop,
        Hob.HandoffInformationTable->EfiMemoryBottom
        );

      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
        return EFI_SUCCESS;
      }

      AsciiPrint ("    Free Top %lx Free Bottom %lx  End Of HOB %lx\n",
        Hob.HandoffInformationTable->EfiFreeMemoryTop,
        Hob.HandoffInformationTable->EfiFreeMemoryBottom,
        Hob.HandoffInformationTable->EfiEndOfHobList
        );

    } else if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_ALLOCATION) {
      // mod(%) on array index is just to prevent buffer overrun
      AsciiPrint ("Mem Alloc HOB %a %g %08lx:%lx\n",
        (Hob.MemoryAllocation->AllocDescriptor.MemoryType < EfiMaxMemoryType) ? gMemMapType[Hob.MemoryAllocation->AllocDescriptor.MemoryType] : "ILLEGAL TYPE",
        &Hob.MemoryAllocation->AllocDescriptor.Name,
        Hob.MemoryAllocation->AllocDescriptor.MemoryBaseAddress,
        Hob.MemoryAllocation->AllocDescriptor.MemoryLength
        );
      if (CompareGuid (&gEfiHobMemoryAllocModuleGuid, &Hob.MemoryAllocation->AllocDescriptor.Name)) {
        if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
          return EFI_SUCCESS;
        }
        AsciiPrint ("    Module Name %g EntryPoint %lx\n", &Hob.MemoryAllocationModule->ModuleName, Hob.MemoryAllocationModule->EntryPoint);
      }
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      AsciiPrint ("Resource HOB %a %g %08lx:%lx\n    Attributes: %08x\n",
        (Hob.ResourceDescriptor->ResourceType < EFI_RESOURCE_MAX_MEMORY_TYPE) ? mHobResourceType[Hob.ResourceDescriptor->ResourceType] : mHobResourceType[EFI_RESOURCE_MAX_MEMORY_TYPE],
        &Hob.ResourceDescriptor->Owner,
        Hob.ResourceDescriptor->PhysicalStart,
        Hob.ResourceDescriptor->ResourceLength,
        Hob.ResourceDescriptor->ResourceAttribute
        );
        if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
          return EFI_SUCCESS;
        }
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_GUID_EXTENSION) {
      AsciiPrint ("GUID HOB %g\n", &Hob.Guid->Name);
      if (CompareGuid (&gEfiMemoryTypeInformationGuid, &Hob.Guid->Name)) {
        EfiMemoryTypeInformation = GET_GUID_HOB_DATA (Hob.Guid);
        for (Index = 0; Index < (GET_GUID_HOB_DATA_SIZE (Hob.Guid)/sizeof (EFI_MEMORY_TYPE_INFORMATION)); Index++, EfiMemoryTypeInformation++) {
          if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
            return EFI_SUCCESS;
          }
          AsciiPrint ("    %a 0x%08x\n",
            (EfiMemoryTypeInformation->Type < EfiMaxMemoryType) ? gMemMapType[EfiMemoryTypeInformation->Type] : "END       ",
            EfiMemoryTypeInformation->NumberOfPages
            );
       }
      }
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_FV) {
      AsciiPrint ("FV HOB %08lx:%08lx\n", Hob.FirmwareVolume->BaseAddress, Hob.FirmwareVolume->Length);
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_CPU) {
      AsciiPrint ("CPU HOB: Mem %x IO %x\n", Hob.Cpu->SizeOfMemorySpace, Hob.Cpu->SizeOfIoSpace);
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_MEMORY_POOL) {
      AsciiPrint ("Mem Pool HOB:\n");
/* Not in PI
    } else if (Hob.Header->HobType == EFI_HOB_TYPE_CV) {
      AsciiPrint ("CV HOB: %08lx:%08lx\n", Hob.CapsuleVolume->BaseAddress, Hob.CapsuleVolume->Length);
 */
    }

    if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
      break;
    }
  }

  return EFI_SUCCESS;
}


GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdHobTemplate[] =
{
  {
    "hob",
    "; dump HOBs",
    NULL,
    EblHobCmd
  }
};


/**
  Initialize the commands in this in this file
**/
VOID
EblInitializeHobCmd (
  VOID
  )
{
  if (FeaturePcdGet (PcdEmbeddedHobCmd)) {
    EblAddCommands (mCmdHobTemplate, sizeof (mCmdHobTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
}

