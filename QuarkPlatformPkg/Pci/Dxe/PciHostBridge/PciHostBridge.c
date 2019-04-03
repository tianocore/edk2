/** @file
Pci Host Bridge driver for a simple IIO. There is only one PCI Root Bridge in the system.
Provides the basic interfaces to abstract a PCI Host Bridge Resource Allocation.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include "PciHostBridge.h"
#include <IntelQNCRegs.h>

//
// We can hardcode the following for a Simple IIO -
// Root Bridge Count within the host bridge
// Root Bridge's device path
// Root Bridge's resource appeture
//
EFI_PCI_ROOT_BRIDGE_DEVICE_PATH    mEfiPciRootBridgeDevicePath[ROOT_BRIDGE_COUNT] = {
  {
    {
      {
        ACPI_DEVICE_PATH,
        ACPI_DP,
        {
          (UINT8) (sizeof (ACPI_HID_DEVICE_PATH)),
          (UINT8) ((sizeof (ACPI_HID_DEVICE_PATH)) >> 8)
        }
      },
      EISA_PNP_ID (0x0A03),
      0
    },
    {
      END_DEVICE_PATH_TYPE,
      END_ENTIRE_DEVICE_PATH_SUBTYPE,
      {
        END_DEVICE_PATH_LENGTH,
        0
      }
    }
  }
};

EFI_HANDLE                         mDriverImageHandle;
PCI_ROOT_BRIDGE_RESOURCE_APERTURE  *mResAperture;

//
// Implementation
//
EFI_STATUS
EFIAPI
InitializePciHostBridge (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++

Routine Description:

  Entry point of this driver.

Arguments:

  ImageHandle  -  Image handle of this driver.
  SystemTable  -  Pointer to standard EFI system table.

Returns:

  EFI_SUCCESS       -  Succeed.
  EFI_DEVICE_ERROR  -  Fail to install PCI_ROOT_BRIDGE_IO protocol.

--*/
{
  EFI_STATUS                  Status;
  UINTN                       TotalRootBridgeFound;
  PCI_HOST_BRIDGE_INSTANCE    *HostBridge;
  PCI_ROOT_BRIDGE_INSTANCE    *PrivateData;
  UINT64                      AllocAttributes;
  EFI_PHYSICAL_ADDRESS        BaseAddress;

  PrivateData = NULL;

  mDriverImageHandle = ImageHandle;

  //
  // Most systems in the world including complex servers
  // have only one Host Bridge. Create Host Bridge Device Handle
  //
  Status = gBS->AllocatePool(EfiBootServicesData, sizeof(PCI_HOST_BRIDGE_INSTANCE), (VOID **) &HostBridge);
  ASSERT_EFI_ERROR (Status);
  ZeroMem (HostBridge, sizeof (PCI_HOST_BRIDGE_INSTANCE));

  HostBridge->Signature         = PCI_HOST_BRIDGE_SIGNATURE;
  HostBridge->RootBridgeCount   = 1;
  HostBridge->ResourceSubmited  = FALSE;
  HostBridge->CanRestarted      = TRUE;
  //
  // InitializeListHead (&HostBridge->Head);
  //
  HostBridge->ResAlloc.NotifyPhase          = NotifyPhase;
  HostBridge->ResAlloc.GetNextRootBridge    = GetNextRootBridge;
  HostBridge->ResAlloc.GetAllocAttributes   = GetAttributes;
  HostBridge->ResAlloc.StartBusEnumeration  = StartBusEnumeration;
  HostBridge->ResAlloc.SetBusNumbers        = SetBusNumbers;
  HostBridge->ResAlloc.SubmitResources      = SubmitResources;
  HostBridge->ResAlloc.GetProposedResources = GetProposedResources;
  HostBridge->ResAlloc.PreprocessController = PreprocessController;

  Status = gBS->InstallProtocolInterface (
                    &HostBridge->HostBridgeHandle,
                    &gEfiPciHostBridgeResourceAllocationProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &HostBridge->ResAlloc
                    );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (HostBridge);
    return EFI_DEVICE_ERROR;
  }

  Status = gBS->AllocatePool (EfiBootServicesData,
                HostBridge->RootBridgeCount * sizeof(PCI_ROOT_BRIDGE_RESOURCE_APERTURE),
                (VOID **) &mResAperture);
  ASSERT_EFI_ERROR (Status);
  ZeroMem (mResAperture, HostBridge->RootBridgeCount * sizeof(PCI_ROOT_BRIDGE_RESOURCE_APERTURE));

  DEBUG ((EFI_D_INFO, "Address of resource Aperture:  %x\n", mResAperture));

  //
  // Create Root Bridge Device Handle in this Host Bridge
  //
  InitializeListHead (&HostBridge->Head);

  TotalRootBridgeFound = 0;

  Status = gBS->AllocatePool ( EfiBootServicesData,sizeof (PCI_ROOT_BRIDGE_INSTANCE), (VOID **) &PrivateData);
  ASSERT_EFI_ERROR (Status);
  ZeroMem (PrivateData, sizeof (PCI_ROOT_BRIDGE_INSTANCE));

  PrivateData->Signature  = PCI_ROOT_BRIDGE_SIGNATURE;
  PrivateData->DevicePath = (EFI_DEVICE_PATH_PROTOCOL *) &mEfiPciRootBridgeDevicePath[TotalRootBridgeFound];
  AllocAttributes         = GetAllocAttributes (TotalRootBridgeFound);

  SimpleIioRootBridgeConstructor (
      &PrivateData->Io,
      HostBridge->HostBridgeHandle,
      &(mResAperture[TotalRootBridgeFound]),
      AllocAttributes
      );
  //
  // Update Root Bridge with UDS resource information
  //
  PrivateData->Aperture.BusBase    = QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSBASE;
  PrivateData->Aperture.BusLimit   = QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSLIMIT;
  PrivateData->Aperture.Mem32Base  = PcdGet32 (PcdPciHostBridgeMemory32Base);
  PrivateData->Aperture.Mem32Limit = PcdGet32 (PcdPciHostBridgeMemory32Base) + (PcdGet32 (PcdPciHostBridgeMemory32Size) - 1);
  PrivateData->Aperture.IoBase  = PcdGet16 (PcdPciHostBridgeIoBase);
  PrivateData->Aperture.IoLimit = PcdGet16 (PcdPciHostBridgeIoBase) + (PcdGet16 (PcdPciHostBridgeIoSize) - 1);

  DEBUG ((EFI_D_INFO, "PCI Host Bridge BusBase:               %x\n",  QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSBASE));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge BusLimit:              %x\n",  QNC_PCI_HOST_BRIDGE_RESOURCE_APPETURE_BUSLIMIT));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceMem32Base:  %x\n",  PcdGet32 (PcdPciHostBridgeMemory32Base)));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceMem32Limit: %x\n",  PcdGet32 (PcdPciHostBridgeMemory32Base) + (PcdGet32 (PcdPciHostBridgeMemory32Size) - 1)));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceMem64Base:  %lX\n", PcdGet64 (PcdPciHostBridgeMemory64Base)));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceMem64Limit: %lX\n", PcdGet64 (PcdPciHostBridgeMemory64Base) + (PcdGet64 (PcdPciHostBridgeMemory64Size) - 1)));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceIoBase:     %x\n",  PcdGet16 (PcdPciHostBridgeIoBase)));
  DEBUG ((EFI_D_INFO, "PCI Host Bridge PciResourceIoLimit:    %x\n",  PcdGet16 (PcdPciHostBridgeIoBase) + (PcdGet16 (PcdPciHostBridgeIoSize) - 1)));

  PrivateData->Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &PrivateData->Handle,
                  &gEfiDevicePathProtocolGuid,
                  PrivateData->DevicePath,
                  &gEfiPciRootBridgeIoProtocolGuid,
                  &PrivateData->Io,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  InsertTailList (&HostBridge->Head, &PrivateData->Link);
  TotalRootBridgeFound++;           // This is a valid rootbridge so imcrement total root bridges found

  //
  // Add PCIE base into Runtime memory so that it can be reported in E820 table
  //
  Status = gDS->AddMemorySpace (
                  EfiGcdMemoryTypeMemoryMappedIo,
                  PcdGet64 (PcdPciExpressBaseAddress),
                  PcdGet64 (PcdPciExpressSize),
                  EFI_MEMORY_RUNTIME | EFI_MEMORY_UC
                  );
  ASSERT_EFI_ERROR(Status);

  BaseAddress = PcdGet64 (PcdPciExpressBaseAddress);

  Status = gDS->AllocateMemorySpace (
                  EfiGcdAllocateAddress,
                  EfiGcdMemoryTypeMemoryMappedIo,
                  0,
                  PcdGet64 (PcdPciExpressSize),
                  &BaseAddress,
                  ImageHandle,
                  NULL
                  );
  ASSERT_EFI_ERROR(Status);

  Status = gDS->SetMemorySpaceAttributes (
                  PcdGet64 (PcdPciExpressBaseAddress),
                  PcdGet64 (PcdPciExpressSize),
                  EFI_MEMORY_RUNTIME
                  );
  ASSERT_EFI_ERROR (Status);

  if (PcdGet16 (PcdPciHostBridgeIoSize) > 0) {
    //
    // At present, we use up the first 4k for fixed ranges like
    // ICH GPIO, ACPI and ISA devices. The first 4k is not
    // tracked through GCD. It should be.
    //
    Status = gDS->AddIoSpace (
                    EfiGcdIoTypeIo,
                    PcdGet16(PcdPciHostBridgeIoBase),
                    PcdGet16(PcdPciHostBridgeIoSize)
                    );
    ASSERT_EFI_ERROR (Status);
  }

  if (PcdGet32(PcdPciHostBridgeMemory32Size) > 0) {
    //
    // Shouldn't the capabilities be UC?
    //
    Status = gDS->AddMemorySpace (
                    EfiGcdMemoryTypeMemoryMappedIo,
                    PcdGet32(PcdPciHostBridgeMemory32Base),
                    PcdGet32(PcdPciHostBridgeMemory32Size),
                    0
                    );
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

EFI_STATUS
EFIAPI
NotifyPhase (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PHASE    Phase
  )
/*++

Routine Description:

  Enter a certain phase of the PCI enumeration process.

Arguments:

  This   -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL instance.
  Phase  -  The phase during enumeration.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong phase parameter passed in.
  EFI_NOT_READY          -  Resources have not been submitted yet.

--*/
{
  PCI_HOST_BRIDGE_INSTANCE              *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE              *RootBridgeInstance;
  PCI_RESOURCE_TYPE                     Index;
  EFI_LIST_ENTRY                        *List;
  EFI_PHYSICAL_ADDRESS                  BaseAddress;
  UINT64                                AddrLen;
  UINTN                                 BitsOfAlignment;
  UINT64                                Alignment;
  EFI_STATUS                            Status;
  EFI_STATUS                            ReturnStatus;
  PCI_RESOURCE_TYPE                     Index1;
  PCI_RESOURCE_TYPE                     Index2;
  BOOLEAN                               ResNodeHandled[TypeMax];
  UINT64                                MaxAlignment;

  HostBridgeInstance = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);

  switch (Phase) {
    case EfiPciHostBridgeBeginEnumeration:
        if (HostBridgeInstance->CanRestarted) {
          //
          // Reset Root Bridge
          //
          List = HostBridgeInstance->Head.ForwardLink;

          while (List != &HostBridgeInstance->Head) {
            RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
            for (Index = TypeIo; Index < TypeMax; Index++) {
              RootBridgeInstance->ResAllocNode[Index].Type    = Index;
              RootBridgeInstance->ResAllocNode[Index].Base    = 0;
              RootBridgeInstance->ResAllocNode[Index].Length  = 0;
              RootBridgeInstance->ResAllocNode[Index].Status  = ResNone;
            } // for

            List = List->ForwardLink;
          } // while

          HostBridgeInstance->ResourceSubmited  = FALSE;
          HostBridgeInstance->CanRestarted      = TRUE;
        } else {
          //
          // Can not restart
          //
          return EFI_NOT_READY;
        } // if
        break;

    case EfiPciHostBridgeEndEnumeration:
        return EFI_SUCCESS;
        break;

    case EfiPciHostBridgeBeginBusAllocation:
        //
        // No specific action is required here, can perform any chipset specific programing
        //
        HostBridgeInstance->CanRestarted = FALSE;
        return EFI_SUCCESS;
        break;

    case EfiPciHostBridgeEndBusAllocation:
        //
        // No specific action is required here, can perform any chipset specific programing
        //
        // HostBridgeInstance->CanRestarted = FALSE;
        //
        return EFI_SUCCESS;
        break;

    case EfiPciHostBridgeBeginResourceAllocation:
        //
        // No specific action is required here, can perform any chipset specific programing
        //
        // HostBridgeInstance->CanRestarted = FALSE;
        //
        return EFI_SUCCESS;
        break;

    case EfiPciHostBridgeAllocateResources:
        ReturnStatus = EFI_SUCCESS;
        if (HostBridgeInstance->ResourceSubmited) {
          List = HostBridgeInstance->Head.ForwardLink;
          while (List != &HostBridgeInstance->Head) {
            for (Index1 = TypeIo; Index1 < TypeBus; Index1++) {
              ResNodeHandled[Index1] = FALSE;
            }

            RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
            DEBUG ((EFI_D_INFO, "Address of RootBridgeInstance:   %x)\n", RootBridgeInstance));
            DEBUG ((EFI_D_INFO, "  Signature:              %x\n", RootBridgeInstance->Signature));
            DEBUG ((EFI_D_INFO, "  Bus Number Assigned:    %x\n", RootBridgeInstance->BusNumberAssigned));
            DEBUG ((EFI_D_INFO, "  Bus Scan Count:         %x\n", RootBridgeInstance->BusScanCount));

            for (Index1 = TypeIo; Index1 < TypeBus; Index1++) {
              if (RootBridgeInstance->ResAllocNode[Index1].Status == ResNone) {
                ResNodeHandled[Index1] = TRUE;
              } else {
                //
                // Allocate the resource node with max alignment at first
                //
                MaxAlignment = 0;
                Index        = TypeMax;
                for (Index2 = TypeIo; Index2 < TypeBus; Index2++) {
                  if (ResNodeHandled[Index2]) {
                    continue;
                  }
                  if (MaxAlignment <= RootBridgeInstance->ResAllocNode[Index2].Alignment) {
                    MaxAlignment = RootBridgeInstance->ResAllocNode[Index2].Alignment;
                    Index = Index2;
                  }
                } // for

                if (Index < TypeMax) {
                  ResNodeHandled[Index] = TRUE;
                } else {
                  ASSERT (FALSE);
                }

                Alignment = RootBridgeInstance->ResAllocNode[Index].Alignment;

                //
                // Get the number of '1' in Alignment.
                //
                for (BitsOfAlignment = 0; Alignment != 0; BitsOfAlignment++) {
                  Alignment = RShiftU64 (Alignment, 1);
                }

                AddrLen   = RootBridgeInstance->ResAllocNode[Index].Length;
                Alignment = RootBridgeInstance->ResAllocNode[Index].Alignment;

                DEBUG ((EFI_D_INFO, "\n\nResource Type to assign :   %x\n", Index));
                DEBUG ((EFI_D_INFO, "  Length to allocate:       %x\n", RootBridgeInstance->ResAllocNode[Index].Length));
                DEBUG ((EFI_D_INFO, "  Aligment:                 %x\n", Alignment));

                switch (Index) {
                  case TypeIo:
                      if (RootBridgeInstance->Aperture.IoBase < RootBridgeInstance->Aperture.IoLimit) {
                        //
                        // It is impossible for 0xFFFF Alignment for IO16
                        //
                        if (BitsOfAlignment >= 16)
                          Alignment = 0;

                        BaseAddress = RootBridgeInstance->Aperture.IoBase;

                        //
                        // Have to make sure Aligment is handled seeing we are doing direct address allocation
                        //
                        if ((BaseAddress & ~(Alignment)) != BaseAddress)
                          BaseAddress = ((BaseAddress + Alignment) & ~(Alignment));

                        while((BaseAddress + AddrLen) <= RootBridgeInstance->Aperture.IoLimit + 1) {

                          Status = gDS->AllocateIoSpace ( EfiGcdAllocateAddress, EfiGcdIoTypeIo, BitsOfAlignment,
                                                          AddrLen, &BaseAddress, mDriverImageHandle, NULL );

                          if (!EFI_ERROR (Status)) {
                            RootBridgeInstance->ResAllocNode[Index].Base    = (UINT64) BaseAddress;
                            RootBridgeInstance->ResAllocNode[Index].Status  = ResAllocated;
                            goto TypeIoFound;
                          }

                          BaseAddress += (Alignment + 1);
                        } // while

                      } // if

                      TypeIoFound:
                      if (RootBridgeInstance->ResAllocNode[Index].Status  != ResAllocated) {
                        //
                        // No Room at the Inn for this resources request
                        //
                        ReturnStatus = EFI_OUT_OF_RESOURCES;
                      } // if

                      break;

                  case TypeMem32:
                      if (RootBridgeInstance->Aperture.Mem32Base < RootBridgeInstance->Aperture.Mem32Limit) {

                        BaseAddress = RootBridgeInstance->Aperture.Mem32Base;
                        //
                        // Have to make sure Aligment is handled seeing we are doing direct address allocation
                        //
                        if ((BaseAddress & ~(Alignment)) != BaseAddress)
                          BaseAddress = ((BaseAddress + Alignment) & ~(Alignment));

                        while((BaseAddress + AddrLen) <= RootBridgeInstance->Aperture.Mem32Limit + 1) {

                          Status = gDS->AllocateMemorySpace ( EfiGcdAllocateAddress, EfiGcdMemoryTypeMemoryMappedIo,
                                                            BitsOfAlignment, AddrLen, &BaseAddress, mDriverImageHandle, NULL);

                          if (!EFI_ERROR (Status)) {
                            RootBridgeInstance->ResAllocNode[Index].Base    = (UINT64) BaseAddress;
                            RootBridgeInstance->ResAllocNode[Index].Status  = ResAllocated;
                            goto TypeMem32Found;
                          } // if

                          BaseAddress += (Alignment + 1);
                        } // while
                      } // if

                      TypeMem32Found:
                      if (RootBridgeInstance->ResAllocNode[Index].Status  != ResAllocated) {
                        //
                        // No Room at the Inn for this resources request
                        //
                        ReturnStatus = EFI_OUT_OF_RESOURCES;
                      }

                      break;

                  case TypePMem32:
                      StartTypePMem32:
                      if (RootBridgeInstance->Aperture.Mem32Base < RootBridgeInstance->Aperture.Mem32Limit) {

                        BaseAddress = RootBridgeInstance->Aperture.Mem32Limit + 1;
                        BaseAddress -= AddrLen;

                        //
                        // Have to make sure Aligment is handled seeing we are doing direct address allocation
                        //
                        if ((BaseAddress & ~(Alignment)) != BaseAddress)
                          BaseAddress = ((BaseAddress) & ~(Alignment));

                        while(RootBridgeInstance->Aperture.Mem32Base <= BaseAddress) {

                          DEBUG ((EFI_D_INFO, "      Attempting %x allocation at 0x%lx .....", Index, BaseAddress));
                          Status = gDS->AllocateMemorySpace ( EfiGcdAllocateAddress, EfiGcdMemoryTypeMemoryMappedIo,
                                                  BitsOfAlignment, AddrLen, &BaseAddress, mDriverImageHandle, NULL);

                          if (!EFI_ERROR (Status)) {
                            RootBridgeInstance->ResAllocNode[Index].Base    = (UINT64) BaseAddress;
                            RootBridgeInstance->ResAllocNode[Index].Status  = ResAllocated;
                            DEBUG ((EFI_D_INFO, "... Passed!!\n"));
                            goto TypePMem32Found;
                          }
                          DEBUG ((EFI_D_INFO, "... Failed!!\n"));
                          BaseAddress -= (Alignment + 1);
                        } // while
                      } // if

                      TypePMem32Found:
                      if (RootBridgeInstance->ResAllocNode[Index].Status  != ResAllocated) {
                        //
                        // No Room at the Inn for this resources request
                        //
                        ReturnStatus = EFI_OUT_OF_RESOURCES;
                      }

                      break;

                  case TypeMem64:
                  case TypePMem64:
                      if (RootBridgeInstance->ResAllocNode[Index].Status  != ResAllocated) {
                        //
                        // If 64-bit resourcing is not available, then try as PMem32
                        //
                        goto StartTypePMem32;
                      }

                      break;

                  default:
                      break;
                } // End switch (Index)

                DEBUG ((EFI_D_INFO, "Resource Type Assigned:   %x\n", Index));
                if (RootBridgeInstance->ResAllocNode[Index].Status == ResAllocated) {
                  DEBUG ((EFI_D_INFO, "  Base Address Assigned: %x\n", RootBridgeInstance->ResAllocNode[Index].Base));
                  DEBUG ((EFI_D_INFO, "  Length Assigned:       %x\n", RootBridgeInstance->ResAllocNode[Index].Length));
                } else {
                  DEBUG ((DEBUG_ERROR, "  Resource Allocation failed!  There was no room at the inn\n"));
                }

              }
            }

            List = List->ForwardLink;
          }

          if (ReturnStatus == EFI_OUT_OF_RESOURCES) {
            DEBUG ((DEBUG_ERROR, "Resource allocation Failed. Continue booting the system.\n"));
          }

          //
          // Set resource to zero for nodes where allocation fails
          //
          List = HostBridgeInstance->Head.ForwardLink;
          while (List != &HostBridgeInstance->Head) {
            RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
            for (Index = TypeIo; Index < TypeBus; Index++) {
              if (RootBridgeInstance->ResAllocNode[Index].Status != ResAllocated) {
                RootBridgeInstance->ResAllocNode[Index].Length = 0;
              }
            }
            List = List->ForwardLink;
          }
          return ReturnStatus;
        } else {
          return EFI_NOT_READY;
        }
        //
        // HostBridgeInstance->CanRestarted = FALSE;
        //
        break;

    case EfiPciHostBridgeSetResources:
        //
        // HostBridgeInstance->CanRestarted = FALSE;
        //
        break;

    case EfiPciHostBridgeFreeResources:
        //
        // HostBridgeInstance->CanRestarted = FALSE;
        //
        ReturnStatus  = EFI_SUCCESS;
        List          = HostBridgeInstance->Head.ForwardLink;
        while (List != &HostBridgeInstance->Head) {
          RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
          for (Index = TypeIo; Index < TypeBus; Index++) {
            if (RootBridgeInstance->ResAllocNode[Index].Status == ResAllocated) {
              AddrLen     = RootBridgeInstance->ResAllocNode[Index].Length;
              BaseAddress = (EFI_PHYSICAL_ADDRESS) RootBridgeInstance->ResAllocNode[Index].Base;
              switch (Index) {
                case TypeIo:
                    Status = gDS->FreeIoSpace (BaseAddress, AddrLen);
                    if (EFI_ERROR (Status)) {
                      ReturnStatus = Status;
                    }
                    break;

                case TypeMem32:
                    Status = gDS->FreeMemorySpace (BaseAddress, AddrLen);
                    if (EFI_ERROR (Status)) {
                      ReturnStatus = Status;
                    }
                    break;

                case TypePMem32:
                    break;

                case TypeMem64:
                    break;

                case TypePMem64:
                    Status = gDS->FreeMemorySpace (BaseAddress, AddrLen);
                    if (EFI_ERROR (Status)) {
                      ReturnStatus = Status;
                    }
                    break;

                default:
                    break;
              } // end switch (Index)

              RootBridgeInstance->ResAllocNode[Index].Type    = Index;
              RootBridgeInstance->ResAllocNode[Index].Base    = 0;
              RootBridgeInstance->ResAllocNode[Index].Length  = 0;
              RootBridgeInstance->ResAllocNode[Index].Status  = ResNone;
            }
          }

          List = List->ForwardLink;
        }

        HostBridgeInstance->ResourceSubmited  = FALSE;
        HostBridgeInstance->CanRestarted      = TRUE;
        return ReturnStatus;
        break;

    case EfiPciHostBridgeEndResourceAllocation:
        //
        // Resource enumeration is done. Perform any activities that
        // must wait until that time.
        //
        break;

    default:
        return EFI_INVALID_PARAMETER;
  } // End switch (Phase)

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetNextRootBridge (
  IN     EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN OUT EFI_HANDLE                                       *RootBridgeHandle
  )
/*++

Routine Description:
  Return the device handle of the next PCI root bridge that is associated with
  this Host Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  Returns the device handle of the next PCI Root Bridge.
                       On input, it holds the RootBridgeHandle returned by the most
                       recent call to GetNextRootBridge().The handle for the first
                       PCI Root Bridge is returned if RootBridgeHandle is NULL on input.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_NOT_FOUND          -  Next PCI root bridge not found.
  EFI_INVALID_PARAMETER  -  Wrong parameter passed in.

--*/
{
  BOOLEAN                   NoRootBridge;
  EFI_LIST_ENTRY            *List;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridgeInstance;

  NoRootBridge        = TRUE;
  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  while (List != &HostBridgeInstance->Head) {
    NoRootBridge        = FALSE;
    RootBridgeInstance  = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (*RootBridgeHandle == NULL) {
      //
      // Return the first Root Bridge Handle of the Host Bridge
      //
      *RootBridgeHandle = RootBridgeInstance->Handle;
      return EFI_SUCCESS;
    } else {
      if (*RootBridgeHandle == RootBridgeInstance->Handle) {
        //
        // Get next if have
        //
        List = List->ForwardLink;
        if (List != &HostBridgeInstance->Head) {
          RootBridgeInstance  = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
          *RootBridgeHandle   = RootBridgeInstance->Handle;
          return EFI_SUCCESS;
        } else {
          return EFI_NOT_FOUND;
        }
      }
    }

    List = List->ForwardLink;
    //
    // end while
    //
  }

  if (NoRootBridge) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_INVALID_PARAMETER;
  }
}

EFI_STATUS
EFIAPI
GetAttributes (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT UINT64                                           *Attributes
  )
/*++

Routine Description:
  Returns the attributes of a PCI Root Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The device handle of the PCI Root Bridge
                       that the caller is interested in.
  Attributes        -  The pointer to attributes of the PCI Root Bridge.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Attributes parameter passed in is NULL or
                            RootBridgeHandle is not an EFI_HANDLE
                            that was returned on a previous call to
                            GetNextRootBridge().

--*/
{
  EFI_LIST_ENTRY            *List;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridgeInstance;

  if (Attributes == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      *Attributes = RootBridgeInstance->RootBridgeAllocAttrib;
      return EFI_SUCCESS;
    }

    List = List->ForwardLink;
  }
  //
  // RootBridgeHandle is not an EFI_HANDLE
  // that was returned on a previous call to GetNextRootBridge()
  //
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
StartBusEnumeration (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:
  This is the request from the PCI enumerator to set up
  the specified PCI Root Bridge for bus enumeration process.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge to be set up.
  Configuration     -  Pointer to the pointer to the PCI bus resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_OUT_OF_RESOURCES   -  Not enough pool to be allocated.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
{
  EFI_LIST_ENTRY            *List;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridgeInstance;
  VOID                      *Buffer;
  UINT8                     *Temp;
  EFI_STATUS                Status;
  UINTN                     BusStart;
  UINTN                     BusEnd;
  UINT64                    BusReserve;

  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      //
      // Set up the Root Bridge for Bus Enumeration
      //
      BusStart  = RootBridgeInstance->Aperture.BusBase;
      BusEnd    = RootBridgeInstance->Aperture.BusLimit;
      BusReserve = RootBridgeInstance->Aperture.BusReserve;
      //
      // Program the Hardware(if needed) if error return EFI_DEVICE_ERROR
      //
      Status = gBS->AllocatePool (
                      EfiBootServicesData,
                      sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR),
                      &Buffer
                      );
      if (EFI_ERROR (Status)) {
        return EFI_OUT_OF_RESOURCES;
      }

      Temp  = (UINT8 *) Buffer;

      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->Desc                  = ACPI_ADDRESS_SPACE_DESCRIPTOR;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->Len                   = 0x2B;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->ResType               = ACPI_ADDRESS_SPACE_TYPE_BUS;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->GenFlag               = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->SpecificFlag          = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->AddrSpaceGranularity  = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->AddrRangeMin          = BusStart;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->AddrRangeMax          = BusReserve;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->AddrTranslationOffset = 0;
      ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp)->AddrLen               = BusEnd - BusStart + 1;

      Temp = Temp + sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
      ((EFI_ACPI_END_TAG_DESCRIPTOR *) Temp)->Desc = ACPI_END_TAG_DESCRIPTOR;
      ((EFI_ACPI_END_TAG_DESCRIPTOR *) Temp)->Checksum = 0x0;

      *Configuration = Buffer;
      return EFI_SUCCESS;
    }

    List = List->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
SetBusNumbers (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:
  This function programs the PCI Root Bridge hardware so that
  it decodes the specified PCI bus range.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge whose bus range is to be programmed.
  Configuration     -  The pointer to the PCI bus resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong parameters passed in.

--*/
{
  EFI_LIST_ENTRY            *List;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridgeInstance;
  UINT8                     *Ptr;
  UINTN                     BusStart;
  UINTN                     BusEnd;
  UINTN                     BusLen;

  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr = Configuration;

  //
  // Check the Configuration is valid
  //
  if (*Ptr != ACPI_ADDRESS_SPACE_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }

  if (((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Ptr)->ResType != ACPI_ADDRESS_SPACE_TYPE_BUS) {
    return EFI_INVALID_PARAMETER;
  }

  Ptr += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
  if (*Ptr != ACPI_END_TAG_DESCRIPTOR) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  Ptr                 = Configuration;

  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      BusStart  = (UINTN) ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Ptr)->AddrRangeMin;
      BusLen    = (UINTN) ((EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Ptr)->AddrLen;
      BusEnd    = BusStart + BusLen - 1;

      if (BusStart > BusEnd) {
        return EFI_INVALID_PARAMETER;
      }

      if ((BusStart < RootBridgeInstance->Aperture.BusBase) || (BusEnd > RootBridgeInstance->Aperture.BusLimit)) {
        return EFI_INVALID_PARAMETER;
      }
      //
      // Update the Bus Range
      //
      RootBridgeInstance->ResAllocNode[TypeBus].Base    = BusStart;
      RootBridgeInstance->ResAllocNode[TypeBus].Length  = BusLen;
      RootBridgeInstance->ResAllocNode[TypeBus].Status  = ResAllocated;
      RootBridgeInstance->BusScanCount++;
      if (RootBridgeInstance->BusScanCount > 0) {
        //
        // Only care about the 2nd PCI bus scanning
        //
        RootBridgeInstance->BusNumberAssigned = TRUE;
      }

      return EFI_SUCCESS;
    }

    List = List->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
SubmitResources (
  IN EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN EFI_HANDLE                                       RootBridgeHandle,
  IN VOID                                             *Configuration
  )
/*++

Routine Description:

  Submits the I/O and memory resource requirements for the specified PCI Root Bridge.

Arguments:
  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge whose I/O and memory resource requirements.
                       are being submitted.
  Configuration     -  The pointer to the PCI I/O and PCI memory resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  Wrong parameters passed in.

--*/
{
  EFI_LIST_ENTRY                    *List;
  PCI_HOST_BRIDGE_INSTANCE          *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE          *RootBridgeInstance;
  UINT8                             *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *ptr;
  UINT64                            AddrLen;
  UINT64                            Alignment;
  UINT64                            Value;

  //
  // Check the input parameter: Configuration
  //
  if (Configuration == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  Temp = (UINT8 *) Configuration;
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      //
      // Check the resource descriptors.
      // If the Configuration includes one or more invalid resource descriptors, all the resource
      // descriptors are ignored and the function returns EFI_INVALID_PARAMETER.
      //
      while (*Temp == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
        ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;
        DEBUG ((EFI_D_INFO, " ptr->ResType:%x \n",ptr->ResType));
        DEBUG ((EFI_D_INFO, "  ptr->AddrLen:0x%lx AddrRangeMin:0x%lx AddrRangeMax:0x%lx\n\n",ptr->AddrLen,ptr->AddrRangeMin,ptr->AddrRangeMax));

        switch (ptr->ResType) {
          case ACPI_ADDRESS_SPACE_TYPE_MEM:
            if (ptr->AddrSpaceGranularity != 32 && ptr->AddrSpaceGranularity != 64) {
              return EFI_INVALID_PARAMETER;
            }
            if (ptr->AddrSpaceGranularity == 32 && ptr->AddrLen > 0xffffffff) {
              return EFI_INVALID_PARAMETER;
            }
            //
            // If the PCI root bridge does not support separate windows for nonprefetchable and
            // prefetchable memory, then the PCI bus driver needs to include requests for
            // prefetchable memory in the nonprefetchable memory pool.
            //
            if ((RootBridgeInstance->RootBridgeAllocAttrib & EFI_PCI_HOST_BRIDGE_COMBINE_MEM_PMEM) != 0 &&
                ((ptr->SpecificFlag & (BIT2 | BIT1)) != 0)) {
              return EFI_INVALID_PARAMETER;
            }
          case ACPI_ADDRESS_SPACE_TYPE_IO:
            //
            // Check aligment, it should be of the form 2^n-1
            //
            Value = Power2MaxMemory (ptr->AddrRangeMax + 1);
            if (Value != (ptr->AddrRangeMax + 1)) {
              CpuDeadLoop();
              return EFI_INVALID_PARAMETER;
            }
            break;
          case ACPI_ADDRESS_SPACE_TYPE_BUS:
          default:
            return EFI_INVALID_PARAMETER;
        }
        Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) ;
      }
      if (*Temp != ACPI_END_TAG_DESCRIPTOR) {
        return EFI_INVALID_PARAMETER;
      }

      Temp = (UINT8 *) Configuration;
      while (*Temp == ACPI_ADDRESS_SPACE_DESCRIPTOR) {
        ptr = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;

        switch (ptr->ResType) {
        case ACPI_ADDRESS_SPACE_TYPE_MEM:
          AddrLen   = (UINT64) ptr->AddrLen;
          Alignment = (UINT64) ptr->AddrRangeMax;
          if (ptr->AddrSpaceGranularity == 32) {
            if (ptr->SpecificFlag == 0x06) {
              //
              // Apply from GCD
              //
              RootBridgeInstance->ResAllocNode[TypePMem32].Status = ResSubmitted;
            } else {
              RootBridgeInstance->ResAllocNode[TypeMem32].Length    = AddrLen;
              RootBridgeInstance->ResAllocNode[TypeMem32].Alignment = Alignment;
              RootBridgeInstance->ResAllocNode[TypeMem32].Status    = ResRequested;
              HostBridgeInstance->ResourceSubmited                  = TRUE;
            }
          }

          if (ptr->AddrSpaceGranularity == 64) {
            if (ptr->SpecificFlag == 0x06) {
              RootBridgeInstance->ResAllocNode[TypePMem64].Status = ResSubmitted;
            } else {
              RootBridgeInstance->ResAllocNode[TypeMem64].Length    = AddrLen;
              RootBridgeInstance->ResAllocNode[TypeMem64].Alignment = Alignment;
              RootBridgeInstance->ResAllocNode[TypeMem64].Status    = ResSubmitted;
              HostBridgeInstance->ResourceSubmited                  = TRUE;
            }
          }
          break;

        case ACPI_ADDRESS_SPACE_TYPE_IO:
          AddrLen   = (UINT64) ptr->AddrLen;
          Alignment = (UINT64) ptr->AddrRangeMax;
          RootBridgeInstance->ResAllocNode[TypeIo].Length     = AddrLen;
          RootBridgeInstance->ResAllocNode[TypeIo].Alignment  = Alignment;
          RootBridgeInstance->ResAllocNode[TypeIo].Status     = ResRequested;
          HostBridgeInstance->ResourceSubmited                = TRUE;
          break;

        default:
          break;
        }

        Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
      }

      return EFI_SUCCESS;
    }

    List = List->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
GetProposedResources (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL *This,
  IN  EFI_HANDLE                                       RootBridgeHandle,
  OUT VOID                                             **Configuration
  )
/*++

Routine Description:
  This function returns the proposed resource settings for the specified
  PCI Root Bridge.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge handle.
  Configuration     -  The pointer to the pointer to the PCI I/O
                       and memory resource descriptor.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_OUT_OF_RESOURCES   -  Not enough pool to be allocated.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
{
  EFI_LIST_ENTRY                    *List;
  PCI_HOST_BRIDGE_INSTANCE          *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE          *RootBridgeInstance;
  UINTN                             Index;
  UINTN                             Number;
  VOID                              *Buffer;
  UINT8                             *Temp;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *ptr;
  EFI_STATUS                        Status;
  UINT64                            ResStatus;

  Buffer  = NULL;
  Number  = 0;
  //
  // Get the Host Bridge Instance from the resource allocation protocol
  //
  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  //
  // Enumerate the root bridges in this host bridge
  //
  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);
    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      for (Index = 0; Index < TypeBus; Index++) {
        if (RootBridgeInstance->ResAllocNode[Index].Status != ResNone) {
          Number++;
        }
      }

      if (Number > 0) {
        Status = gBS->AllocatePool (
                        EfiBootServicesData,
                        Number * sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR),
                        &Buffer
                        );

        if (EFI_ERROR (Status)) {
          return EFI_OUT_OF_RESOURCES;
        }

        ZeroMem (Buffer, sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR) * Number + sizeof (EFI_ACPI_END_TAG_DESCRIPTOR));
      }

      ASSERT (Buffer != NULL);
      Temp = Buffer;
      for (Index = 0; Index < TypeBus; Index++) {
        if (RootBridgeInstance->ResAllocNode[Index].Status != ResNone) {
          ptr       = (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR *) Temp;
          ResStatus = RootBridgeInstance->ResAllocNode[Index].Status;

          switch (Index) {

          case TypeIo:
            //
            // Io
            //
            ptr->Desc                   = 0x8A;
            ptr->Len                    = 0x2B;
            ptr->ResType                = 1;
            ptr->GenFlag                = 0;
            ptr->SpecificFlag           = 0;
            ptr->AddrRangeMin           = RootBridgeInstance->ResAllocNode[Index].Base;
            ptr->AddrRangeMax           = 0;
            ptr->AddrTranslationOffset  = (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : EFI_RESOURCE_LESS;
            ptr->AddrLen                = RootBridgeInstance->ResAllocNode[Index].Length;
            break;

          case TypeMem32:
            //
            // Memory 32
            //
            ptr->Desc                   = 0x8A;
            ptr->Len                    = 0x2B;
            ptr->ResType                = 0;
            ptr->GenFlag                = 0;
            ptr->SpecificFlag           = 0;
            ptr->AddrSpaceGranularity   = 32;
            ptr->AddrRangeMin           = RootBridgeInstance->ResAllocNode[Index].Base;
            ptr->AddrRangeMax           = 0;
            ptr->AddrTranslationOffset  = (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : EFI_RESOURCE_LESS;
            ptr->AddrLen                = RootBridgeInstance->ResAllocNode[Index].Length;
            break;

          case TypePMem32:
            //
            // Prefetch memory 32
            //
            ptr->Desc                   = 0x8A;
            ptr->Len                    = 0x2B;
            ptr->ResType                = 0;
            ptr->GenFlag                = 0;
            ptr->SpecificFlag           = 6;
            ptr->AddrSpaceGranularity   = 32;
            ptr->AddrRangeMin           = 0;
            ptr->AddrRangeMax           = 0;
            ptr->AddrTranslationOffset  = EFI_RESOURCE_NONEXISTENT;
            ptr->AddrLen                = 0;
            break;

          case TypeMem64:
            //
            // Memory 64
            //
            ptr->Desc                   = 0x8A;
            ptr->Len                    = 0x2B;
            ptr->ResType                = 0;
            ptr->GenFlag                = 0;
            ptr->SpecificFlag           = 0;
            ptr->AddrSpaceGranularity   = 64;
            ptr->AddrRangeMin           = RootBridgeInstance->ResAllocNode[Index].Base;
            ptr->AddrRangeMax           = 0;
            ptr->AddrTranslationOffset  = (ResStatus == ResAllocated) ? EFI_RESOURCE_SATISFIED : EFI_RESOURCE_LESS;
            ptr->AddrLen                = RootBridgeInstance->ResAllocNode[Index].Length;
            break;

          case TypePMem64:
            //
            // Prefetch memory 64
            //
            ptr->Desc                   = 0x8A;
            ptr->Len                    = 0x2B;
            ptr->ResType                = 0;
            ptr->GenFlag                = 0;
            ptr->SpecificFlag           = 6;
            ptr->AddrSpaceGranularity   = 64;
            ptr->AddrRangeMin           = 0;
            ptr->AddrRangeMax           = 0;
            ptr->AddrTranslationOffset  = EFI_RESOURCE_NONEXISTENT;
            ptr->AddrLen                = 0;
            break;
          }

          Temp += sizeof (EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR);
        }
      }

      ((EFI_ACPI_END_TAG_DESCRIPTOR *) Temp)->Desc      = 0x79;
      ((EFI_ACPI_END_TAG_DESCRIPTOR *) Temp)->Checksum  = 0x0;

      *Configuration = Buffer;

      return EFI_SUCCESS;
    }

    List = List->ForwardLink;
  }

  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
PreprocessController (
  IN  EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_PROTOCOL          *This,
  IN  EFI_HANDLE                                                RootBridgeHandle,
  IN  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_PCI_ADDRESS               PciAddress,
  IN  EFI_PCI_CONTROLLER_RESOURCE_ALLOCATION_PHASE              Phase
  )
/*++

Routine Description:
  This function is called for all the PCI controllers that the PCI
  bus driver finds. Can be used to Preprogram the controller.

Arguments:

  This              -  The EFI_PCI_HOST_BRIDGE_RESOURCE_ALLOCATION_ PROTOCOL instance.
  RootBridgeHandle  -  The PCI Root Bridge handle.
  PciAddress        -  Address of the controller on the PCI bus.
  Phase             -  The Phase during resource allocation.

Returns:

  EFI_SUCCESS            -  Succeed.
  EFI_INVALID_PARAMETER  -  RootBridgeHandle is not a valid handle.

--*/
{
  BOOLEAN                   RootBridgeFound;
  EFI_LIST_ENTRY            *List;
  PCI_HOST_BRIDGE_INSTANCE  *HostBridgeInstance;
  PCI_ROOT_BRIDGE_INSTANCE  *RootBridgeInstance;

  if (RootBridgeHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  RootBridgeFound     = FALSE;
  HostBridgeInstance  = INSTANCE_FROM_RESOURCE_ALLOCATION_THIS (This);
  List                = HostBridgeInstance->Head.ForwardLink;

  while (List != &HostBridgeInstance->Head) {
    RootBridgeInstance = DRIVER_INSTANCE_FROM_LIST_ENTRY (List);

    if (RootBridgeHandle == RootBridgeInstance->Handle) {
      RootBridgeFound = TRUE;
      break;
    }
    //
    // Get next if have
    //
    List = List->ForwardLink;
  }

  if (RootBridgeFound == FALSE) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

UINT64
Power2MaxMemory (
  IN UINT64                     MemoryLength
  )
/*++

Routine Description:

  Calculate maximum memory length that can be fit to a mtrr.

Arguments:

  MemoryLength  -  Input memory length.

Returns:

  Returned Maximum length.

--*/
{
  UINT64  Result;

  if (RShiftU64 (MemoryLength, 32)) {
    Result = LShiftU64 ((UINT64) GetPowerOfTwo64 ((UINT32) RShiftU64 (MemoryLength, 32)), 32);
  } else {
    Result = (UINT64) GetPowerOfTwo64 ((UINT32) MemoryLength);
  }

  return Result;
}
