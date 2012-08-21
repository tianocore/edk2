/** @file
  Xen PV test Dxe.

  Copyright (c) 2011-2012, Bei Guan <gbtju85@gmail.com>

  This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "HypercallTestDxe.h"

/**
  Helper functions: print a string. (For Hypercall test)
**/
UINTN
EFIAPI
PrintAsciiStr (
  IN      CONST CHAR8               *FirstString,
  IN      UINTN                     Length
  )
{
  if (!FirstString || !AsciiStrSize (FirstString)){
    DEBUG ((EFI_D_INFO, "\n"));
    return EFI_ABORTED;
  }

  while ((*FirstString != '\0') && (Length > 0)) {
    DEBUG ((EFI_D_INFO, "%c", *FirstString));
    FirstString++;
    Length--;
  }

  DEBUG ((EFI_D_INFO, "\n"));
  return EFI_SUCCESS;
}


////////////////////////////////////////////////////
/// Front block driver for OVMF (move to blkfront.c)
////////////////////////////////////////////////////
/**
  Initial the block device
**/
BLK_FRONT_DEV*
EFIAPI
InitBlkFront (
  CHAR8                     *ParaNodeName,
  BLK_FRONT_INFO            *Info
  )
{
  EFI_STATUS                Status;
  CHAR8                     *Message;
  BLKIF_SRING               *Str;
  UINTN                     Retry;
  CHAR16                    *Msg;
  //CHAR16                    *Chr;
  CHAR8                     *NodeName = ParaNodeName ? ParaNodeName : "device/vbd/768";
  BLK_FRONT_DEV             *Dev;
  CHAR8                     Path[AsciiStrLen (NodeName) + 1 + 10 + 1];

  DEBUG ((EFI_D_INFO, "[test]OVMF_BlkFrontDriver_Initializing....\n"));

  Message       = NULL;
  Retry         = 0;
  Msg           = NULL;
  Dev           = (BLK_FRONT_DEV *) AllocateZeroPool (sizeof (*Dev));
  Dev->NodeName = NodeName;

  AsciiSPrint (Path, sizeof (Path), "%a/backend-id", NodeName);
  Dev->DomId = (DOMID) XenbusReadInteger (Path);
  EventChannelAllocateUnbound (Dev->DomId, /*blkfront_handler,*/Dev, &Dev->EvtChn);

  Str = (BLKIF_SRING *) AllocatePages (EFI_PAGE_SIZE);
  ZeroMem (Str, EFI_PAGE_SIZE);

  SHARED_RING_INIT (Str);
  FRONT_RING_INIT(&Dev->Ring, Str, EFI_PAGE_SIZE);

  Dev->RingRef = GrantTableGrantAccess(Dev->DomId, (UINTN) Str, 0);
  Dev->Events  = NULL;

  Status = XenbusPrintf (NodeName, "ring-ref", "%d", Dev->RingRef);
  if (EFI_ERROR(Status)) {
    Message = "Writing ring-ref";
    goto Error;
  }

  Status = XenbusPrintf (NodeName, "event-channel", "%d", Dev->EvtChn);
  if (EFI_ERROR(Status)) {
    Message = "Writing event-channel";
    goto Error;
  }

  Status = XenbusPrintf (NodeName, "protocol", "%a", XEN_IO_PROTO_ABI_NATIVE);
  if (EFI_ERROR(Status)) {
    Message = "Writing protocol";
    goto Error;
  }

  AsciiSPrint (Path, sizeof (Path), "%a/state", NodeName);
  Status = XenbusSwitchState (Path, XenbusStateConnected);
  if (EFI_ERROR(Status)) {
    Message = "Switching state";
    goto Error;
  }

  //
  // TODO More
  //


  DEBUG ((EFI_D_INFO, "[test]OVMF_BlkFrontDriver_Completing Transaction....\n"));
  goto Done;

Error:
  DEBUG ((EFI_D_ERROR, "Blk front driver initialing faild. (%a)\n", Message));
Done:
  FreePool (Dev);
  return NULL;
}


/**
  Read something from the disk using the Xen PV drivers.
**/
EFI_STATUS
EFIAPI
BlockFrontDriverTestFunc (
  VOID
  )
{
  BLK_FRONT_INFO                 BlkInfo;


  InitBlkFront (NULL, &BlkInfo);


  return EFI_SUCCESS;
}

///////////////////////////////////////////////////
/// End of front block driver (move to blkfront.c)
///////////////////////////////////////////////////
/**
  Test Xen Hypercall. You can use hypercall in this function.

**/
EFI_STATUS
EFIAPI
HypercallTestFunc (
  VOID
  )
{
  EFI_STATUS         Status;
  UINT32             Major, Minor;
  XenExtraversion    ExtraVersion;

  Major = 0; Minor = 0;
  Minor = HypervisorXenVersion(XENVER_VERSION, 0);
  Major = Minor >> 16;
  Minor &= 0xFFFF;
  DEBUG ((EFI_D_INFO, "[HypercallTest_CallHypercallFromDXE]OVMF is running on Xen version: %d.%d\n", Major, Minor));

  Status = HypervisorXenVersion(XENVER_EXTRAVERSION, ExtraVersion);
  ASSERT_EFI_ERROR (Status);
  DEBUG ((EFI_D_INFO, "[HypercallTest_CallHypercallFromDXE]Detected Xen"));
  PrintAsciiStr(ExtraVersion, 16);

  return EFI_SUCCESS;
}

/**
  Test XenBus: Read and write a key-value pair in Xenstore.

**/
EFI_STATUS
EFIAPI
XenbusTestFunc (
  VOID
  )
{
  //
  // Pointer to the value of a key in Xenstore.
  //
  CHAR8            *XenstoreValue;
  //
  // Array of pointers to keys of a directory.
  //
  CHAR8            **XenstoreValuePtrs;
  UINTN            Index;
  DOMID            DomId;

  //
  // Get current domain ID
  //
  DomId = XenbusGetDomId ();
  if (DomId < 0) {
    DEBUG ((EFI_D_ERROR, "Can't get domain ID!\n"));
  }
  else {
    DEBUG ((EFI_D_INFO, "[XenbusTest_GetDomIdFromXenstore] Domain Id: %d\n", DomId));
  }

  //
  // Read a value from xenstore.
  //
  XenstoreValue = NULL;
  XenbusRead ("hvmloader/bios", &XenstoreValue);
  DEBUG ((EFI_D_INFO, "[XenbusTest_ReadXenstore] Old HVM BIOS (hvmloader/bios): "));
  PrintAsciiStr(XenstoreValue, 30);

  XenbusRead ("name", &XenstoreValue);
  DEBUG ((EFI_D_INFO, "[XenbusTest_ReadXenstore] Old HVM BIOS (name): "));
  PrintAsciiStr(XenstoreValue, 30);

  //
  // Write a value into xenstore.
  // TODO modify failed!!(Need authority)
  //
  DEBUG ((EFI_D_INFO, "[XenbusTest_WriteXenstore] Write to Xenstore\n"));
  XenbusWrite ("hvmloader/bios", "ovmf-pv-test");
  XenbusRead ("hvmloader/bios", &XenstoreValue);
  DEBUG ((EFI_D_INFO, "[XenbusTest_WriteXenstore] New HVM BIOS (hvmloader/bios): "));
  PrintAsciiStr(XenstoreValue, 30);

  //
  // Create a new key in Xenstore
  //
  //DEBUG ((EFI_D_INFO, "[XenbusTest_WriteXenstore_2] Write to Xenstore\n"));
  //XenbusWrite ("testkey", "ovmf-hvm");
  //XenbusRead ("testkey", &XenstoreValue);
  //DEBUG ((EFI_D_INFO, "[XenbusTest_ReadXenstore] Old HVM BIOS (testkey): "));
  //PrintAsciiStr(XenstoreValue, 30);

  //
  // List a directory in xenstore.
  //
  XenstoreValuePtrs = NULL;
  XenbusLs ("device", &XenstoreValuePtrs);
  if (XenstoreValuePtrs) {
    DEBUG ((EFI_D_INFO, "[XenbusTest_ReadDireXenstore] Diretory in Xenstore (device): \n"));
    for (Index = 0; Index < (XENSTORE_PAYLOAD_MAX + 1); Index++) {
      if (!XenstoreValuePtrs[Index]) {
        break;
      }
      PrintAsciiStr((CHAR8 *) (UINTN) XenstoreValuePtrs[Index] , 30);
    }
  }

  return EFI_SUCCESS;
}

/**
  Test Grant Table: .


**/
EFI_STATUS
EFIAPI
GrantTableTestFunc (
  VOID
  )
{
  //
  // We just use a simple block front driver to test the Xen PV drivers.
  //
  BlockFrontDriverTestFunc ();

  return EFI_SUCCESS;
}


/**
  Stub Dxe.

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios protocol installed
  @retval Other          No protocol installed, unload driver.

**/
EFI_STATUS
EFIAPI
XenParaVirtualizationTestEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID *HypercallPages;

  //
  // Test if we can use Xen Hypercalls.
  //
  HypercallPages = InitializeHypercallPage ();
  if (HypercallPages == NULL) {
    DEBUG ((EFI_D_ERROR, "Can't find HVM hypercall page!\n"));
    return 0;
  }
  //
  // Set Xenbus.
  //
  XenbusSetup ();
  DEBUG ((EFI_D_INFO, "XenBus Setup ...\n"));
  //
  // Initial grant tables
  //
  InitGrantTable ();
  DEBUG ((EFI_D_INFO, "InitGrantTable Finished ...\n"));

  //////////////////////////////////////////////
  //
  // Test Xen Hypercall.
  //
  HypercallTestFunc();

  //
  // Test Xenbus.
  // Read or Write a key-value in xenstore
  //
  XenbusTestFunc();

  //
  // Test Xen Grant table.
  //
  GrantTableTestFunc();

  //
  //Test finished.
  //////////////////////////////////////////////
  //
  // Reset grant tables
  //
  FinishGrantTable ();
  DEBUG ((EFI_D_INFO, "GrantTable Closed ...\n"));
  //
  // Clear the Xenbus.
  //
  XenbusShutdown ();
  DEBUG ((EFI_D_INFO, "XenBus Shutdown ...\n"));


  DEBUG ((EFI_D_INFO, "XenParaVirtualizationTestEntry finished!\n"));
  return EFI_SUCCESS;
}

