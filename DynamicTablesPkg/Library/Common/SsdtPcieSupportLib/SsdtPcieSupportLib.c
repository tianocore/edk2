/** @file
  SSDT PCIe Support Library.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.0
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
  - Arm Base Boot Requirements v1.0
  - Arm Base System Architecture v1.0
**/

#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/TableHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/SsdtPcieSupportLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtPcieSupportLibPrivate.h"

/** Generate Pci slots devices.

  PCI Firmware Specification - Revision 3.3,
  s4.8 "Generic ACPI PCI Slot Description" requests to describe the PCI slot
  used. It should be possible to enumerate them, but this is additional
  information.

  @param [in]       PciInfo       Pci device information.
  @param [in]       MappingTable  The mapping table structure.
  @param [in]       Uid           Unique Id of the Pci device.
  @param [in, out]  PciNode       Pci node to amend.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
EFI_STATUS
EFIAPI
GeneratePciSlots (
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  *PciInfo,
  IN      CONST MAPPING_TABLE                         *MappingTable,
  IN            UINT32                                Uid,
  IN  OUT       AML_OBJECT_NODE_HANDLE                PciNode
  )
{
  EFI_STATUS              Status;
  UINT32                  Index;
  UINT32                  LastIndex;
  UINT32                  DeviceId;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  ASSERT (MappingTable != NULL);
  ASSERT (PciNode != NULL);

  // Generic device name is "Dxx".
  CopyMem (AslName, "Dxx_", AML_NAME_SEG_SIZE + 1);

  LastIndex = MappingTable->LastIndex;

  // There are at most 32 devices on a Pci bus.
  if (LastIndex >= 32) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < LastIndex; Index++) {
    DeviceId                       = MappingTable->Table[Index];
    AslName[AML_NAME_SEG_SIZE - 3] = AsciiFromHex (DeviceId & 0xF);
    AslName[AML_NAME_SEG_SIZE - 2] = AsciiFromHex ((DeviceId >> 4) & 0xF);

    // ASL:
    // Device (Dxx) {
    //   Name (_ADR, <address value>)
    // }
    Status = AmlCodeGenDevice (AslName, PciNode, &DeviceNode);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    /* ACPI 6.4 specification, Table 6.2: "ADR Object Address Encodings"
       High word-Device #, Low word-Function #. (for example, device 3,
       function 2 is 0x00030002). To refer to all the functions on a device #,
       use a function number of FFFF).
    */
    Status = AmlCodeGenNameInteger (
               "_ADR",
               (DeviceId << 16) | 0xFFFF,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    // _SUN object is not generated as we don't know which slot will be used.
  }

  return Status;
}

/** Add an _OSC template method to the PciNode.

  The _OSC method is provided as an AML blob. The blob is
  parsed and attached at the end of the PciNode list of variable elements.

  @param [in]       PciInfo     Pci device information.
  @param [in, out]  PciNode     Pci node to amend.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
EFI_STATUS
EFIAPI
AddOscMethod (
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  *PciInfo,
  IN  OUT   AML_OBJECT_NODE_HANDLE                    PciNode
  )
{
  EFI_STATUS                   Status;
  EFI_STATUS                   Status1;
  EFI_ACPI_DESCRIPTION_HEADER  *SsdtPcieOscTemplate;
  AML_ROOT_NODE_HANDLE         OscTemplateRoot;
  AML_OBJECT_NODE_HANDLE       OscNode;

  ASSERT (PciNode != NULL);

  // Parse the Ssdt Pci Osc Template.
  SsdtPcieOscTemplate = (EFI_ACPI_DESCRIPTION_HEADER *)
                        ssdtpcieosctemplate_aml_code;

  OscNode         = NULL;
  OscTemplateRoot = NULL;
  Status          = AmlParseDefinitionBlock (
                      SsdtPcieOscTemplate,
                      &OscTemplateRoot
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI-OSC: Failed to parse SSDT PCI OSC Template."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlFindNode (OscTemplateRoot, "\\_OSC", &OscNode);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  Status = AmlDetachNode (OscNode);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  Status = AmlAttachNode (PciNode, OscNode);
  if (EFI_ERROR (Status)) {
    // Free the detached node.
    AmlDeleteTree (OscNode);
    goto error_handler;
  }

error_handler:
  // Cleanup
  Status1 = AmlDeleteTree (OscTemplateRoot);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI-OSC: Failed to cleanup AML tree."
      " Status = %r\n",
      Status1
      ));
    // If Status was success but we failed to delete the AML Tree
    // return Status1 else return the original error code, i.e. Status.
    if (!EFI_ERROR (Status)) {
      return Status1;
    }
  }

  return Status;
}
