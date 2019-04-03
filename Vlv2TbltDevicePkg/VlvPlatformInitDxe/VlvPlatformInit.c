
/*++

Copyright (c)  1999  - 2014, Intel Corporation. All rights reserved
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


Module Name:

  VlvPlatformInit.c

Abstract:

  This is the driver that initializes the Intel ValleyView.

--*/

#include "VlvPlatformInit.h"
#include <Protocol/VlvPlatformPolicy.h>

extern DXE_VLV_PLATFORM_POLICY_PROTOCOL  *DxePlatformSaPolicy;
UINT64            GTTMMADR;

DXE_VLV_PLATFORM_POLICY_PROTOCOL  *DxePlatformSaPolicy;

/**
  "Poll Status" for GT Readiness

 @param  Base             Base address of MMIO
 @param  Offset           MMIO Offset
 @param  Mask             Mask
 @param  Result           Value to wait for

 @retval None

**/
VOID
PollGtReady_hang (
  UINT64 Base,
  UINT32 Offset,
  UINT32 Mask,
  UINT32 Result
  )
{
  UINT32  GtStatus;

  //
  // Register read
  //
  GtStatus = MmioRead32 ((UINTN)Base+ Offset);

  while (((GtStatus & Mask) != Result)) {

    GtStatus = MmioRead32 ((UINTN)Base + Offset);
  }

}

/**
  Do Post GT PM Init Steps after VBIOS Initialization.

  @param Event             A pointer to the Event that triggered the callback.
  @param Context           A pointer to private data registered with the callback function.

  @retval EFI_SUCCESS        GC_TODO


**/
EFI_STATUS
EFIAPI    
PostPmInitCallBack (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  UINT64      OriginalGTTMMADR;
  UINT32      LoGTBaseAddress;
  UINT32      HiGTBaseAddress;

  //
  // Enable Bus Master, I/O and Memory access on 0:2:0
  //
  PciOr8 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_CMD), (BIT2 | BIT1));

  //
  // only 32bit read/write is legal for device 0:2:0
  //
  OriginalGTTMMADR  = (UINT64) PciRead32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR));
  OriginalGTTMMADR  = LShiftU64 ((UINT64) PciRead32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR + 4)), 32) | (OriginalGTTMMADR);

  //
  // 64bit GTTMADR does not work for S3 save script table since it is executed in PEIM phase
  // Program temporarily 32bits GTTMMADR for POST and S3 resume
  //
  LoGTBaseAddress                   = (UINT32) (GTTMMADR & 0xFFFFFFFF);
  HiGTBaseAddress                   = (UINT32) RShiftU64 ((GTTMMADR & 0xFFFFFFFF00000000), 32);
  S3PciWrite32(PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR), LoGTBaseAddress);
  S3PciWrite32(PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR+4), HiGTBaseAddress);



  //
  // Restore original GTTMMADR
  //
  LoGTBaseAddress                   = (UINT32) (OriginalGTTMMADR & 0xFFFFFFFF);
  HiGTBaseAddress                   = (UINT32) RShiftU64 ((OriginalGTTMMADR & 0xFFFFFFFF00000000), 32);

  S3PciWrite32(PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR), LoGTBaseAddress);
  S3PciWrite32(PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_GTTMMADR+4), HiGTBaseAddress);


  //
  // Lock the following registers, GGC, BDSM, BGSM
  //
  PciOr32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_MGGC_OFFSET), LockBit);
  PciOr32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_BSM_OFFSET), LockBit);
  PciOr32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0,IGD_R_BGSM), LockBit);

  gBS->CloseEvent (Event);

  //
  // Return final status
  //
  return EFI_SUCCESS;
}

/**

  Routine Description:

  Initialize GT Post Routines.

  @param ImageHandle              Handle for the image of this driver
  @param DxePlatformSaPolicy      SA DxePlatformPolicy protocol

  @retval EFI_SUCCESS             GT POST initialization complete

**/
EFI_STATUS
IgdPmHook (
  IN EFI_HANDLE                      ImageHandle,
  IN DXE_VLV_PLATFORM_POLICY_PROTOCOL *DxePlatformSaPolicyParam
  )
{

  EFI_EVENT             mConOutEvent;
  VOID                  *gConOutNotifyReg;

  EFI_STATUS            Status;

  EFI_PHYSICAL_ADDRESS  MemBaseAddress;
  UINT32                LoGTBaseAddress;
  UINT32                HiGTBaseAddress;

  GTTMMADR    = 0;
  Status      = EFI_SUCCESS;

  //
  // If device 0:2:0 (Internal Graphics Device, or GT) is enabled, then Program GTTMMADR,
  //
  if (PciRead16(PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_VID))  != 0xFFFF) {

    ASSERT (gDS!=NULL);

    //
    // Enable Bus Master, I/O and Memory access on 0:2:0
    //
    PciOr8(PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_CMD), (BIT2 | BIT1 | BIT0));

    //
    // Means Allocate 4MB for GTTMADDR
    //
    MemBaseAddress = 0x0ffffffff;

    Status = gDS->AllocateMemorySpace (
                    EfiGcdAllocateMaxAddressSearchBottomUp,
                    EfiGcdMemoryTypeMemoryMappedIo,
                    GTT_MEM_ALIGN,
                    GTTMMADR_SIZE_4MB,
                    &MemBaseAddress,
                    ImageHandle,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);

    //
    // Program GT PM Settings if GTTMMADR allocation is Successful
    //
    GTTMMADR                          = (UINTN) MemBaseAddress;

    LoGTBaseAddress                   = (UINT32) (MemBaseAddress & 0xFFFFFFFF);
    HiGTBaseAddress                   = (UINT32) RShiftU64 ((MemBaseAddress & 0xFFFFFFFF00000000), 32);

    PciWrite32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_GTTMMADR), LoGTBaseAddress);
    PciWrite32 (PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_GTTMMADR+4), HiGTBaseAddress);


    S3PciRead32(PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_GTTMMADR));


    S3MmioRead32(IGD_R_GTTMMADR + 4);


    S3PciRead8(PCI_LIB_ADDRESS(0, IGD_DEV, 0, IGD_R_CMD));

    //
    // Do POST GT PM Init Steps after VBIOS Initialization in DoPostPmInitCallBack
    //
    Status = gBS->CreateEvent (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    (EFI_EVENT_NOTIFY)PostPmInitCallBack,
                    NULL,
                    &mConOutEvent
                    );

    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }


    Status = gBS->RegisterProtocolNotify (
                    &gEfiGraphicsOutputProtocolGuid,
                    mConOutEvent,
                    &gConOutNotifyReg
                    );



    MmioWrite64 (IGD_R_GTTMMADR, 0);

    //
    // Free allocated resources
    //
    gDS->FreeMemorySpace (
           MemBaseAddress,
           GTTMMADR_SIZE_4MB
           );

  }

  return EFI_SUCCESS;
}

/**

  This is the standard EFI driver point that detects
  whether there is an ICH southbridge in the system
  and if so, initializes the chip.

  @param  ImageHandle             Handle for the image of this driver
  @param  SystemTable             Pointer to the EFI System Table

  @retval EFI_SUCCESS             The function completed successfully

**/
EFI_STATUS
EFIAPI
VlvPlatformInitEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  EFI_STATUS                        Status;

  Status = gBS->LocateProtocol (&gDxeVlvPlatformPolicyGuid, NULL, (void **)&DxePlatformSaPolicy);
  ASSERT_EFI_ERROR (Status);

  //
  // GtPostInit Initialization
  //
  DEBUG ((EFI_D_ERROR, "Initializing GT PowerManagement and other GT POST related\n"));
  IgdPmHook (ImageHandle, DxePlatformSaPolicy);

  //
  // IgdOpRegion Install Initialization
  //
  DEBUG ((EFI_D_ERROR, "Initializing IGD OpRegion\n"));
  IgdOpRegionInit ();

  return EFI_SUCCESS;
}

