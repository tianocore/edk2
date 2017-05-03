/** @file
  Opal password smm driver which is used to support Opal security feature at s3 path.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalPasswordSmm.h"

#define SMM_SIZE_ALLOC_BYTES                      (512)
#define RESPONSE_SIZE                             (200)

#define PCI_CLASS_MASS_STORAGE_AHCI               (0x06)

#define OPAL_PCIE_ROOTPORT_SAVESIZE               (0x40)
#define STORE_INVALID_ROOTPORT_INDEX              ((UINT8) -1)
#define OPAL_DEVICE_TYPE_SATA                     0x1
#define OPAL_DEVICE_TYPE_NVME                     0x2
#define OPAL_DEVICE_TYPE_UNKNOWN                  0xFF

//
// To unlock the Intel SATA controller at S3 Resume, restored the following registers.
//
const OPAL_HC_PCI_REGISTER_SAVE mSataHcRegisterSaveTemplate[] = {
  {0x9,  S3BootScriptWidthUint8},
  {0x10, S3BootScriptWidthUint32},
  {0x14, S3BootScriptWidthUint32},
  {0x18, S3BootScriptWidthUint32},
  {0x1C, S3BootScriptWidthUint32},
  {0x20, S3BootScriptWidthUint32},
  {0x24, S3BootScriptWidthUint32},
  {0x3c, S3BootScriptWidthUint8},
  {0x3d, S3BootScriptWidthUint8},
  {0x40, S3BootScriptWidthUint16},
  {0x42, S3BootScriptWidthUint16},
  {0x92, S3BootScriptWidthUint16},
  {0x94, S3BootScriptWidthUint32},
  {0x9C, S3BootScriptWidthUint32},
  {0x4,  S3BootScriptWidthUint16},
};


UINT8                mSwSmiValue;
LIST_ENTRY           *mOpalDeviceList;
LIST_ENTRY           mSmmDeviceList  = INITIALIZE_LIST_HEAD_VARIABLE (mSmmDeviceList);

BOOLEAN              mSendBlockSID   = FALSE;

// AHCI
UINT32               mAhciBar = 0;
EFI_AHCI_REGISTERS   mAhciRegisters;
VOID                 *mBuffer = NULL; // DMA can not read/write Data to smram, so we pre-allocates Buffer from AcpiNVS.
//
// NVME
NVME_CONTEXT         mNvmeContext;

/**
  Add new bridge node or nvme device info to the device list.

  @param[in]       BusNum        The bus number.
  @param[in]       DevNum        The device number.
  @param[in]       FuncNum       The function number.
  @param[in]       Dev           The device which need to add device node info.

**/
VOID
AddPciDeviceNode (
  UINT32                  BusNum,
  UINT32                  DevNum,
  UINT32                  FuncNum,
  OPAL_SMM_DEVICE         *Dev
  )
{
  UINT8       *DevList;
  PCI_DEVICE  *DeviceNode;

  DevList = AllocateZeroPool (sizeof (PCI_DEVICE) + Dev->Length);
  ASSERT (DevList != NULL);

  if (Dev->Length != 0) {
    CopyMem (DevList, Dev->PciBridgeNode, Dev->Length);
    FreePool (Dev->PciBridgeNode);
  }

  DeviceNode = (PCI_DEVICE *) (DevList + Dev->Length);

  DeviceNode->BusNum  = BusNum;
  DeviceNode->DevNum  = DevNum;
  DeviceNode->FuncNum = FuncNum;

  Dev->Length += sizeof (PCI_DEVICE);
  Dev->PciBridgeNode = (PCI_DEVICE *)DevList;
}

/**
  Extract device info from the input device path.

  @param[in]       DevicePath        Device path info for the device.
  @param[in,out]   Dev               The device which new inputed.

**/
VOID
ExtractDeviceInfoFromDevicePath (
  IN     EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN OUT OPAL_SMM_DEVICE             *Dev
  )
{
  EFI_DEVICE_PATH_PROTOCOL      *TmpDevPath;
  EFI_DEVICE_PATH_PROTOCOL      *TmpDevPath2;
  PCI_DEVICE_PATH               *PciDevPath;
  SATA_DEVICE_PATH              *SataDevPath;
  NVME_NAMESPACE_DEVICE_PATH    *NvmeDevPath;
  UINTN                         BusNum;

  TmpDevPath = DevicePath;
  Dev->DeviceType = OPAL_DEVICE_TYPE_UNKNOWN;

  while (!IsDevicePathEnd(TmpDevPath)) {
    if (TmpDevPath->Type == MESSAGING_DEVICE_PATH && TmpDevPath->SubType == MSG_SATA_DP) {
      //
      // SATA
      //
      SataDevPath = ( SATA_DEVICE_PATH* )TmpDevPath;
      Dev->SataPort = SataDevPath->HBAPortNumber;
      Dev->SataPortMultiplierPort = SataDevPath->PortMultiplierPortNumber;
      Dev->DeviceType = OPAL_DEVICE_TYPE_SATA;
      break;
    } else if (TmpDevPath->Type == MESSAGING_DEVICE_PATH && TmpDevPath->SubType == MSG_NVME_NAMESPACE_DP) {
      //
      // NVMe
      //
      NvmeDevPath = ( NVME_NAMESPACE_DEVICE_PATH* )TmpDevPath;
      Dev->NvmeNamespaceId = NvmeDevPath->NamespaceId;
      Dev->DeviceType = OPAL_DEVICE_TYPE_NVME;
      break;
    }
    TmpDevPath = NextDevicePathNode (TmpDevPath);
  }

  //
  // Get bridge node info for the nvme device.
  //
  BusNum = 0;
  TmpDevPath = DevicePath;
  TmpDevPath2 = NextDevicePathNode (DevicePath);
  while (!IsDevicePathEnd(TmpDevPath2)) {
    if (TmpDevPath->Type == HARDWARE_DEVICE_PATH && TmpDevPath->SubType == HW_PCI_DP) {
      PciDevPath = (PCI_DEVICE_PATH *) TmpDevPath;
      if ((TmpDevPath2->Type == MESSAGING_DEVICE_PATH && TmpDevPath2->SubType == MSG_NVME_NAMESPACE_DP)||
          (TmpDevPath2->Type == MESSAGING_DEVICE_PATH && TmpDevPath2->SubType == MSG_SATA_DP)) {
        Dev->BusNum = (UINT32)BusNum;
        Dev->DevNum = PciDevPath->Device;
        Dev->FuncNum = PciDevPath->Function;
      } else {
        AddPciDeviceNode((UINT32)BusNum, PciDevPath->Device, PciDevPath->Function, Dev);
        if (TmpDevPath2->Type == HARDWARE_DEVICE_PATH && TmpDevPath2->SubType == HW_PCI_DP) {
          BusNum = PciRead8 (PCI_LIB_ADDRESS (BusNum, PciDevPath->Device, PciDevPath->Function, NVME_PCIE_SEC_BNUM));
        }
      }
    }

    TmpDevPath  = NextDevicePathNode (TmpDevPath);
    TmpDevPath2 = NextDevicePathNode (TmpDevPath2);
  }
}

/**

  The function returns whether or not the device is Opal Locked.
  TRUE means that the device is partially or fully locked.
  This will perform a Level 0 Discovery and parse the locking feature descriptor

  @param[in]      OpalDev             Opal object to determine if locked
  @param[out]     BlockSidSupported   Whether device support BlockSid feature.

**/
BOOLEAN
IsOpalDeviceLocked(
  OPAL_SMM_DEVICE    *OpalDev,
  BOOLEAN            *BlockSidSupported
  )
{
  OPAL_SESSION                   Session;
  OPAL_DISK_SUPPORT_ATTRIBUTE    SupportedAttributes;
  TCG_LOCKING_FEATURE_DESCRIPTOR LockingFeature;
  UINT16                         OpalBaseComId;
  TCG_RESULT                     Ret;

  Session.Sscp = &OpalDev->Sscp;
  Session.MediaId = 0;

  Ret = OpalGetSupportedAttributesInfo (&Session, &SupportedAttributes, &OpalBaseComId);
  if (Ret != TcgResultSuccess) {
    return FALSE;
  }

  OpalDev->OpalBaseComId = OpalBaseComId;
  Session.OpalBaseComId  = OpalBaseComId;
  *BlockSidSupported     = SupportedAttributes.BlockSid == 1 ? TRUE : FALSE;

  Ret = OpalGetLockingInfo(&Session, &LockingFeature);
  if (Ret != TcgResultSuccess) {
    return FALSE;
  }

  return OpalDeviceLocked (&SupportedAttributes, &LockingFeature);
}

/**
  Save/Restore RootPort configuration space.

  @param[in] DeviceNode             - The device node.
  @param[in] SaveAction             - TRUE: Save, FALSE: Restore
  @param[in,out] PcieConfBufferList     - Configuration space data buffer for save/restore

  @retval - PCIE base address of this RootPort
**/
UINTN
SaveRestoreRootportConfSpace (
  IN     OPAL_SMM_DEVICE                *DeviceNode,
  IN     BOOLEAN                        SaveAction,
  IN OUT UINT8                          **PcieConfBufferList
  )
{
  UINTN      RpBase;
  UINTN      Length;
  PCI_DEVICE *DevNode;
  UINT8      *StorePcieConfData;
  UINTN      Index;

  Length = 0;
  Index  = 0;
  RpBase = 0;

  while (Length < DeviceNode->Length) {
    DevNode = (PCI_DEVICE *)((UINT8*)DeviceNode->PciBridgeNode + Length);
    RpBase = PCI_LIB_ADDRESS (DevNode->BusNum, DevNode->DevNum, DevNode->FuncNum, 0x0);

    if (PcieConfBufferList != NULL) {
      if (SaveAction) {
        StorePcieConfData = (UINT8 *) AllocateZeroPool (OPAL_PCIE_ROOTPORT_SAVESIZE);
        ASSERT (StorePcieConfData != NULL);
        OpalPciRead (StorePcieConfData, RpBase, OPAL_PCIE_ROOTPORT_SAVESIZE);
        PcieConfBufferList[Index] = StorePcieConfData;
      } else {
        // Skip PCIe Command & Status registers
        StorePcieConfData = PcieConfBufferList[Index];
        OpalPciWrite (RpBase, StorePcieConfData, 4);
        OpalPciWrite (RpBase + 8, StorePcieConfData + 8, OPAL_PCIE_ROOTPORT_SAVESIZE - 8);

        FreePool (StorePcieConfData);
      }
    }

    Length += sizeof (PCI_DEVICE);
    Index ++;
  }

  return RpBase;
}

/**
  Configure RootPort for downstream PCIe NAND devices.

  @param[in] RpBase             - PCIe configuration space address of this RootPort
  @param[in] BusNumber          - Bus number
  @param[in] MemoryBase         - Memory base address
  @param[in] MemoryLength       - Memory size

**/
VOID
ConfigureRootPortForPcieNand (
  IN UINTN   RpBase,
  IN UINTN   BusNumber,
  IN UINT32  MemoryBase,
  IN UINT32  MemoryLength
  )
{
  UINT32  MemoryLimit;

  DEBUG ((DEBUG_INFO, "ConfigureRootPortForPcieNand, BusNumber: %x, MemoryBase: %x, MemoryLength: %x\n",
    BusNumber, MemoryBase, MemoryLength));

  if (MemoryLength == 0) {
    MemoryLimit = MemoryBase;
  } else {
    MemoryLimit = MemoryBase + MemoryLength + 0xFFFFF; // 1M
  }

  ///
  /// Configue PCIE configuration space for RootPort
  ///
  PciWrite8  (RpBase + NVME_PCIE_BNUM + 1,  (UINT8) BusNumber);           // Secondary Bus Number registers
  PciWrite8  (RpBase + NVME_PCIE_BNUM + 2,  (UINT8) BusNumber);           // Subordinate Bus Number registers
  PciWrite8  (RpBase + NVME_PCIE_IOBL,      0xFF);                        // I/O Base registers
  PciWrite8  (RpBase + NVME_PCIE_IOBL + 1,  0x00);                        // I/O Limit registers
  PciWrite16 (RpBase + NVME_PCIE_MBL,       (UINT16) RShiftU64 ((UINTN)MemoryBase, 16));  // Memory Base register
  PciWrite16 (RpBase + NVME_PCIE_MBL + 2,   (UINT16) RShiftU64 ((UINTN)MemoryLimit, 16)); // Memory Limit register
  PciWrite16 (RpBase + NVME_PCIE_PMBL,      0xFFFF);                      // Prefetchable Memory Base registers
  PciWrite16 (RpBase + NVME_PCIE_PMBL + 2,  0x0000);                      // Prefetchable Memory Limit registers
  PciWrite32 (RpBase + NVME_PCIE_PMBU32,    0xFFFFFFFF);                  // Prefetchable Memory Upper Base registers
  PciWrite32 (RpBase + NVME_PCIE_PMLU32,    0x00000000);                  // Prefetchable Memory Upper Limit registers
}


/**
  Dispatch function for a Software SMI handler.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of Data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The Size of the CommBuffer.

  @retval EFI_SUCCESS            The interrupt was handled and quiesced. No other handlers
                                 should still be called.
  @retval Others                 Other execution results.
**/
EFI_STATUS
EFIAPI
SmmUnlockOpalPassword (
  IN     EFI_HANDLE              DispatchHandle,
  IN     CONST VOID              *RegisterContext,
  IN OUT VOID                    *CommBuffer,
  IN OUT UINTN                   *CommBufferSize
  )
{
  EFI_STATUS                     Status;
  OPAL_SMM_DEVICE                *OpalDev;
  LIST_ENTRY                     *Entry;
  UINT8                          BaseClassCode;
  UINT8                          SubClassCode;
  UINT8                          ProgInt;
  TCG_RESULT                     Result;
  UINT8                          SataCmdSt;
  UINT8                          *StorePcieConfDataList[16];
  UINTN                          RpBase;
  UINTN                          MemoryBase;
  UINTN                          MemoryLength;
  OPAL_SESSION                   Session;
  BOOLEAN                        BlockSidSupport;

  ZeroMem (StorePcieConfDataList, sizeof (StorePcieConfDataList));
  Status = EFI_DEVICE_ERROR;

  //
  // try to unlock all locked hdd disks.
  //
  for (Entry = mSmmDeviceList.ForwardLink; Entry != &mSmmDeviceList; Entry = Entry->ForwardLink) {
    OpalDev = BASE_CR(Entry, OPAL_SMM_DEVICE, Link);

    RpBase    = 0;
    SataCmdSt = 0;

    ///
    /// Configure RootPort for PCIe AHCI/NVME devices.
    ///
    if (OpalDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
      ///
      /// Save original RootPort configuration space to heap
      ///
      RpBase = SaveRestoreRootportConfSpace (
                  OpalDev,
                  TRUE,
                  StorePcieConfDataList
                  );
      MemoryBase = mNvmeContext.Nbar;
      MemoryLength = 0;
      ConfigureRootPortForPcieNand (RpBase, OpalDev->BusNum, (UINT32) MemoryBase, (UINT32) MemoryLength);

      ///
      /// Enable PCIE decode for RootPort
      ///
      SataCmdSt = PciRead8 (RpBase + NVME_PCIE_PCICMD);
      PciWrite8  (RpBase + NVME_PCIE_PCICMD,  0x6);
    } else {
      SataCmdSt = PciRead8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, NVME_PCIE_PCICMD));
      PciWrite8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, NVME_PCIE_PCICMD), 0x6);
    }

    BaseClassCode = PciRead8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, 0x0B));
    SubClassCode  = PciRead8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, 0x0A));
    ProgInt       = PciRead8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, 0x09));
    if (BaseClassCode != PCI_CLASS_MASS_STORAGE) {
      Status = EFI_INVALID_PARAMETER;
      break;
    }

    Status = EFI_DEVICE_ERROR;
    if (OpalDev->DeviceType == OPAL_DEVICE_TYPE_SATA) {
      if ((SubClassCode == PCI_CLASS_MASS_STORAGE_AHCI) || (SubClassCode == PCI_CLASS_MASS_STORAGE_RAID)) {
        Status = GetAhciBaseAddress (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "GetAhciBaseAddress error, Status: %r\n", Status));
          goto done;
        }
        Status = AhciModeInitialize ((UINT8)OpalDev->SataPort);
        ASSERT_EFI_ERROR (Status);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "AhciModeInitialize error, Status: %r\n", Status));
          goto done;
        }
      } else {
        DEBUG ((DEBUG_ERROR, "SubClassCode not support for SATA device\n"));
      }
    } else if (OpalDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
      if (SubClassCode == PCI_CLASS_MASS_STORAGE_NVM) {
        if (ProgInt != PCI_IF_NVMHCI) {
          DEBUG ((DEBUG_ERROR, "PI not support, skipped\n"));
          Status = EFI_NOT_FOUND;
          goto done;
        }

        mNvmeContext.PciBase = PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, 0x0);
        mNvmeContext.NvmeInitWaitTime = 0;
        mNvmeContext.Nsid = OpalDev->NvmeNamespaceId;
        Status = NvmeControllerInit (&mNvmeContext);
      } else {
        DEBUG ((DEBUG_ERROR, "SubClassCode not support for NVME device\n"));
      }
    } else {
      DEBUG ((DEBUG_ERROR, "Invalid Devicetype\n"));
      goto done;
    }

    Status = EFI_DEVICE_ERROR;
    BlockSidSupport = FALSE;
    if (IsOpalDeviceLocked (OpalDev, &BlockSidSupport)) {
      ZeroMem(&Session, sizeof(Session));
      Session.Sscp = &OpalDev->Sscp;
      Session.MediaId = 0;
      Session.OpalBaseComId = OpalDev->OpalBaseComId;

      Result = OpalSupportUnlock (&Session, OpalDev->Password, OpalDev->PasswordLength, NULL);
      if (Result == TcgResultSuccess) {
        Status = EFI_SUCCESS;
      }
    }

    if (mSendBlockSID && BlockSidSupport) {
      Result = OpalBlockSid (&Session, TRUE);
      if (Result != TcgResultSuccess) {
        break;
      }
    }

    if (OpalDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
      if (SubClassCode == PCI_CLASS_MASS_STORAGE_NVM) {
        Status = NvmeControllerExit (&mNvmeContext);
      }
    }

done:
    if (OpalDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
      ASSERT (RpBase != 0);
      PciWrite8  (RpBase + NVME_PCIE_PCICMD, 0);
      RpBase = SaveRestoreRootportConfSpace (
                  OpalDev,
                  FALSE,  // restore
                  StorePcieConfDataList
                  );
      PciWrite8  (RpBase + NVME_PCIE_PCICMD, SataCmdSt);
    } else {
      PciWrite8 (PCI_LIB_ADDRESS (OpalDev->BusNum, OpalDev->DevNum, OpalDev->FuncNum, NVME_PCIE_PCICMD), SataCmdSt);
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  return Status;
}

/**
  The function extracts device information from OpalDeviceList and creat SmmDeviceList used for S3.

  @param[in]       OpalDeviceList   Opal device list created at POST which contains the information of OPAL_DISK_AND_PASSWORD_INFO
  @param[in,out]   SmmDeviceList    Opal Smm device list to be created and used for unlocking devices at S3 resume.

  @retval EFI_SUCCESS            Create SmmDeviceList successfully.
  @retval Others                 Other execution results.
**/
EFI_STATUS
CreateSmmDeviceList (
  IN     LIST_ENTRY                 *OpalDeviceList,
  IN OUT LIST_ENTRY                 *SmmDeviceList
  )
{
  LIST_ENTRY                        *Entry;
  OPAL_DISK_AND_PASSWORD_INFO       *PciDev;
  OPAL_SMM_DEVICE                   *SmmDev;

  for (Entry = OpalDeviceList->ForwardLink; Entry != OpalDeviceList; Entry = Entry->ForwardLink) {
    PciDev = BASE_CR (Entry, OPAL_DISK_AND_PASSWORD_INFO, Link);

    SmmDev = AllocateZeroPool (sizeof (OPAL_SMM_DEVICE));
    if (SmmDev == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    SmmDev->Signature = OPAL_SMM_DEVICE_SIGNATURE;

    ExtractDeviceInfoFromDevicePath(&PciDev->OpalDevicePath, SmmDev);

    SmmDev->PasswordLength = PciDev->PasswordLength;
    CopyMem(&(SmmDev->Password), PciDev->Password, OPAL_PASSWORD_MAX_LENGTH);

    SmmDev->Sscp.ReceiveData = SecurityReceiveData;
    SmmDev->Sscp.SendData = SecuritySendData;

    DEBUG ((DEBUG_INFO, "Opal SMM: Insert device node to SmmDeviceList:\n"));
    DEBUG ((DEBUG_INFO, "DeviceType:%x, Bus:%d, Dev:%d, Fun:%d\n", \
      SmmDev->DeviceType, SmmDev->BusNum, SmmDev->DevNum, SmmDev->FuncNum));
    DEBUG ((DEBUG_INFO, "SataPort:%x, MultiplierPort:%x, NvmeNamespaceId:%x\n", \
      SmmDev->SataPort, SmmDev->SataPortMultiplierPort, SmmDev->NvmeNamespaceId));

    InsertHeadList (SmmDeviceList, &SmmDev->Link);
  }

  return EFI_SUCCESS;
}

/**
  Main entry point for an SMM handler dispatch or communicate-based callback.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     Context         Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in,out] CommBuffer      A pointer to a collection of Data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in,out] CommBufferSize  The Size of the CommBuffer.

  @retval EFI_SUCCESS                         The interrupt was handled and quiesced. No other handlers
                                              should still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED  The interrupt has been quiesced but other handlers should
                                              still be called.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING   The interrupt is still pending and other handlers should still
                                              be called.
  @retval EFI_INTERRUPT_PENDING               The interrupt could not be quiesced.
**/
EFI_STATUS
EFIAPI
S3SleepEntryCallBack (
  IN           EFI_HANDLE           DispatchHandle,
  IN     CONST VOID                 *Context         OPTIONAL,
  IN OUT       VOID                 *CommBuffer      OPTIONAL,
  IN OUT       UINTN                *CommBufferSize  OPTIONAL
  )
{
  UINTN                             Bus;
  UINTN                             Device;
  UINTN                             Function;
  UINTN                             Index;
  EFI_STATUS                        Status;
  LIST_ENTRY                        *Entry;
  UINTN                             Offset;
  UINT64                            Address;
  S3_BOOT_SCRIPT_LIB_WIDTH          Width;
  UINT32                            Data;
  OPAL_HC_PCI_REGISTER_SAVE         *HcRegisterSaveListPtr;
  UINTN                             Count;
  OPAL_SMM_DEVICE                   *SmmDev;

  Data     = 0;
  Status   = EFI_SUCCESS;

  mOpalDeviceList = OpalSupportGetOpalDeviceList();
  if (IsListEmpty (mOpalDeviceList)) {
    //
    // No Opal enabled device. Do nothing.
    //
    return EFI_SUCCESS;
  }

  if (IsListEmpty (&mSmmDeviceList)) {
    //
    // mSmmDeviceList for S3 is empty, creat it by mOpalDeviceList.
    //
    Status = CreateSmmDeviceList (mOpalDeviceList, &mSmmDeviceList);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  //
  // Go through SmmDeviceList to save register data for S3
  //
  for (Entry = mSmmDeviceList.ForwardLink; Entry != &mSmmDeviceList; Entry = Entry->ForwardLink) {
    SmmDev = BASE_CR (Entry, OPAL_SMM_DEVICE, Link);

    if (SmmDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
      continue;
    }

    //
    // Save register Data for S3. Sata controller only.
    //
    Bus        = SmmDev->BusNum;
    Device     = SmmDev->DevNum;
    Function   = SmmDev->FuncNum;

    ASSERT (SmmDev->DeviceType == OPAL_DEVICE_TYPE_SATA);
    HcRegisterSaveListPtr = (OPAL_HC_PCI_REGISTER_SAVE *) mSataHcRegisterSaveTemplate;
    Count = sizeof (mSataHcRegisterSaveTemplate) / sizeof (OPAL_HC_PCI_REGISTER_SAVE);

    for (Index = 0; Index < Count; Index += 1) {
      Offset  = HcRegisterSaveListPtr[Index].Address;
      Width   = HcRegisterSaveListPtr[Index].Width;

      switch (Width) {
        case S3BootScriptWidthUint8:
          Data = (UINT32)PciRead8 (PCI_LIB_ADDRESS(Bus,Device,Function,Offset));
          break;
        case S3BootScriptWidthUint16:
          Data = (UINT32)PciRead16 (PCI_LIB_ADDRESS(Bus,Device,Function,Offset));
          break;
        case S3BootScriptWidthUint32:
          Data = PciRead32 (PCI_LIB_ADDRESS(Bus,Device,Function,Offset));
          break;
        default:
          ASSERT (FALSE);
          break;
      }

      Address = S3_BOOT_SCRIPT_LIB_PCI_ADDRESS (Bus, Device, Function, Offset);
      Status  = S3BootScriptSavePciCfgWrite (Width, Address, 1, &Data);
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
  }

  Status = S3BootScriptSaveIoWrite (S3BootScriptWidthUint8, 0xB2, 1, &mSwSmiValue);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Main entry for this driver.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to SystemTable.

  @retval EFI_SUCESS     This function always complete successfully.

**/
EFI_STATUS
EFIAPI
OpalPasswordSmmInit (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable
  )
{
  EFI_STATUS                            Status;
  EFI_SMM_SW_DISPATCH2_PROTOCOL         *SwDispatch;
  EFI_SMM_SX_DISPATCH2_PROTOCOL         *SxDispatch;
  EFI_HANDLE                            SwHandle;
  EFI_SMM_SW_REGISTER_CONTEXT           Context;
  EFI_HANDLE                            S3SleepEntryHandle;
  EFI_SMM_SX_REGISTER_CONTEXT           EntryRegisterContext;
  EFI_SMM_VARIABLE_PROTOCOL             *SmmVariable;
  OPAL_EXTRA_INFO_VAR                   OpalExtraInfo;
  UINTN                                 DataSize;
  EFI_PHYSICAL_ADDRESS                  Address;

  mBuffer            = NULL;
  SwHandle           = NULL;
  S3SleepEntryHandle = NULL;
  ZeroMem (&mNvmeContext, sizeof (NVME_CONTEXT));

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSwDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SwDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " SmmLocateProtocol gEfiSmmSwDispatch2ProtocolGuid fail, Status: %r\n", Status));
    return Status;
  }

  Status = gSmst->SmmLocateProtocol (
                    &gEfiSmmSxDispatch2ProtocolGuid,
                    NULL,
                    (VOID **)&SxDispatch
                    );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " SmmLocateProtocol gEfiSmmSxDispatch2ProtocolGuid fail, Status: %r\n", Status));
    return Status;
  }

  //
  // Preallocate a 512 bytes Buffer to perform trusted I/O.
  // Assume this is big enough for unlock commands
  // It's because DMA can not access smmram stack at the cmd execution.
  //
  Address = 0xFFFFFFFF;
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiACPIMemoryNVS,
                  EFI_SIZE_TO_PAGES (SMM_SIZE_ALLOC_BYTES),
                  &Address
                  );
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " AllocatePages for SATA DAM fail, Status: %r\n", Status));
    return EFI_OUT_OF_RESOURCES;
  }

  mBuffer = (VOID *)(UINTN)Address;
  ZeroMem ((VOID *)(UINTN)mBuffer, SMM_SIZE_ALLOC_BYTES);

  //
  // Preallocate resource for AHCI transfer descriptor.
  //
  Status = AhciAllocateResource ();
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " AhciAllocateResource fail, Status: %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Preallocate resource for NVMe configuration space.
  //
  Status = NvmeAllocateResource (ImageHandle, &mNvmeContext);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " NvmeAllocateResource fail, Status: %r\n", Status));
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  //
  // Register a S3 entry callback function to store ATA host controller context to boot script.
  // These boot scripts would be invoked at S3 path to recovery ATA host controller h/w context
  // for executing HDD unlock cmd.
  //
  EntryRegisterContext.Type  = SxS3;
  EntryRegisterContext.Phase = SxEntry;
  Status = SxDispatch->Register (
                         SxDispatch,
                         S3SleepEntryCallBack,
                         &EntryRegisterContext,
                         &S3SleepEntryHandle
                         );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    goto EXIT;
  }

  //
  // Register Opal password smm unlock handler
  //
  Context.SwSmiInputValue = (UINTN) -1;
  Status = SwDispatch->Register (
               SwDispatch,
               SmmUnlockOpalPassword,
               &Context,
               &SwHandle
               );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, " SwDispatch->Register fail, Status: %r\n", Status));
    goto EXIT;
  }

  //
  // trigger smi to unlock hdd if it's locked.
  //
  mSwSmiValue = (UINT8) Context.SwSmiInputValue;

  Status = gSmst->SmmLocateProtocol (&gEfiSmmVariableProtocolGuid, NULL, (VOID**)&SmmVariable);
  if (!EFI_ERROR (Status)) {
    DataSize = sizeof (OPAL_EXTRA_INFO_VAR);
    Status = SmmVariable->SmmGetVariable (
                    OPAL_EXTRA_INFO_VAR_NAME,
                    &gOpalExtraInfoVariableGuid,
                    NULL,
                    &DataSize,
                    &OpalExtraInfo
                    );
    if (!EFI_ERROR (Status)) {
      mSendBlockSID = OpalExtraInfo.EnableBlockSid;
    }
  }

  return EFI_SUCCESS;

EXIT:
  if (S3SleepEntryHandle != NULL) {
    SxDispatch->UnRegister (SxDispatch, S3SleepEntryHandle);
  }

  AhciFreeResource ();

  NvmeFreeResource (&mNvmeContext);

  if (mBuffer != NULL) {
    gBS->FreePages ((EFI_PHYSICAL_ADDRESS)(UINTN) mBuffer, EFI_SIZE_TO_PAGES (SMM_SIZE_ALLOC_BYTES));
  }

  return Status;
}

/**
  Provide Io action support.

  @param[in]     SmmDev             the opal device need to perform trust io.
  @param[in]     IoType             OPAL_IO_TYPE indicating whether to perform a Trusted Send or Trusted Receive.
  @param[in]     SecurityProtocol   Security Protocol
  @param[in]     SpSpecific         Security Protocol Specific
  @param[in]     TransferLength     Transfer Length of Buffer (in bytes) - always a multiple of 512
  @param[in]     Buffer             Address of Data to transfer

  @retval        TcgResultSuccess   Perform the io action success.
  @retval        TcgResultFailure   Perform the io action failed.

**/
EFI_STATUS
PerformTrustedIo (
  OPAL_SMM_DEVICE  *SmmDev,
  OPAL_IO_TYPE     IoType,
  UINT8            SecurityProtocol,
  UINT16           SpSpecific,
  UINTN            TransferLength,
  VOID             *Buffer
  )
{
  EFI_STATUS                   Status;
  UINTN                        BufferSizeBlocks;
  EFI_ATA_COMMAND_BLOCK        AtaCommandBlock;

  Status = EFI_DEVICE_ERROR;
  if (SmmDev->DeviceType == OPAL_DEVICE_TYPE_SATA) {
    BufferSizeBlocks = TransferLength / 512;

    ZeroMem( &AtaCommandBlock, sizeof( EFI_ATA_COMMAND_BLOCK ) );
    AtaCommandBlock.AtaCommand = ( IoType == OpalSend ) ? ATA_COMMAND_TRUSTED_SEND : ATA_COMMAND_TRUSTED_RECEIVE;
    AtaCommandBlock.AtaSectorCount = ( UINT8 )BufferSizeBlocks;
    AtaCommandBlock.AtaSectorNumber = ( UINT8 )( BufferSizeBlocks >> 8 );
    AtaCommandBlock.AtaFeatures = SecurityProtocol;
    AtaCommandBlock.AtaCylinderLow = ( UINT8 )( SpSpecific >> 8 );
    AtaCommandBlock.AtaCylinderHigh = ( UINT8 )( SpSpecific );
    AtaCommandBlock.AtaDeviceHead = ATA_DEVICE_LBA;


    ZeroMem( mBuffer, HDD_PAYLOAD );
    ASSERT( TransferLength <= HDD_PAYLOAD );

    if (IoType == OpalSend) {
      CopyMem( mBuffer, Buffer, TransferLength );
    }

    Status = AhciPioTransfer(
                &mAhciRegisters,
                (UINT8) SmmDev->SataPort,
                (UINT8) SmmDev->SataPortMultiplierPort,
                NULL,
                0,
                ( IoType == OpalSend ) ? FALSE : TRUE,   // i/o direction
                &AtaCommandBlock,
                NULL,
                mBuffer,
                (UINT32)TransferLength,
                ATA_TIMEOUT
                );

    if (IoType == OpalRecv) {
      CopyMem( Buffer, mBuffer, TransferLength );
    }
  } else if (SmmDev->DeviceType == OPAL_DEVICE_TYPE_NVME) {
    Status = NvmeSecuritySendReceive (
                &mNvmeContext,
                IoType == OpalSend,
                SecurityProtocol,
                SwapBytes16(SpSpecific),
                TransferLength,
                Buffer
              );
  } else {
    DEBUG((DEBUG_ERROR, "DeviceType(%x) not support.\n", SmmDev->DeviceType));
  }

  return Status;
}

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command. The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.
  @param  PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                       data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
SecurityReceiveData (
  IN  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN  UINT32                                   MediaId,
  IN  UINT64                                   Timeout,
  IN  UINT8                                    SecurityProtocolId,
  IN  UINT16                                   SecurityProtocolSpecificData,
  IN  UINTN                                    PayloadBufferSize,
  OUT VOID                                     *PayloadBuffer,
  OUT UINTN                                    *PayloadTransferSize
  )
{
  OPAL_SMM_DEVICE              *SmmDev;

  SmmDev = OPAL_SMM_DEVICE_FROM_THIS (This);
  if (SmmDev == NULL) {
    return EFI_DEVICE_ERROR;
  }

  return PerformTrustedIo (
                        SmmDev,
                        OpalRecv,
                        SecurityProtocolId,
                        SecurityProtocolSpecificData,
                        PayloadBufferSize,
                        PayloadBuffer
                        );
}

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given MediaId. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL OUT command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command is
  sent using the Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED. If there is no media in the device, the function
  returns EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the
  device, the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall return
  EFI_SUCCESS. If the security protocol command completes with an error, the function
  shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the send data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
SecuritySendData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  IN VOID                                     *PayloadBuffer
  )
{
  OPAL_SMM_DEVICE              *SmmDev;

  SmmDev = OPAL_SMM_DEVICE_FROM_THIS (This);
  if (SmmDev == NULL) {
    return EFI_DEVICE_ERROR;
  }

  return PerformTrustedIo (
                          SmmDev,
                          OpalSend,
                          SecurityProtocolId,
                          SecurityProtocolSpecificData,
                          PayloadBufferSize,
                          PayloadBuffer
                          );

}

