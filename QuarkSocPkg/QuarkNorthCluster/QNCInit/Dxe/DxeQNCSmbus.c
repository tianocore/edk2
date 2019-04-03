/** @file
Implementation for SMBus DXE driver entry point and SMBus Host
Controller protocol.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "CommonHeader.h"

#include "DxeQNCSmbus.h"

//
// Interface defintion of SMBUS Host Controller Protocol.
//
EFI_SMBUS_HC_PROTOCOL mSmbusHc = {
  SmbusExecute,
  SmbusArpDevice,
  SmbusGetArpMap,
  SmbusNotify
};

//
// Handle to install SMBus Host Controller protocol.
//
EFI_HANDLE                   mSmbusHcHandle = NULL;
UINT8                        mDeviceMapEntries = 0;
EFI_SMBUS_DEVICE_MAP         mDeviceMap[MAX_SMBUS_DEVICES];
UINT8                        mPlatformNumRsvd = 0;
UINT8                        *mPlatformAddrRsvd = NULL;

//
// These addresses are reserved by the SMBus 2.0 specification
//
UINT8    mReservedAddress[SMBUS_NUM_RESERVED] = {
  0x00, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C, 0x0E, 0x10, 0x18, 0x50, 0x6E, 0xC2,
  0xF0, 0xF2, 0xF4, 0xF6, 0xF8, 0xFA, 0xFC, 0xFE
};


/**
  Gets Io port base address of Smbus Host Controller.

  This internal function depends on a feature flag named PcdIchSmbusFixedIoPortBaseAddress
  to retrieve Smbus Io port base. If that feature flag is true, it will get Smbus Io port base
  address from a preset Pcd entry named PcdIchSmbusIoPortBaseAddress; otherwise, it will always
  read Pci configuration space to get that value in each Smbus bus transaction.

  @return The Io port base address of Smbus host controller.

**/
UINTN
GetSmbusIoPortBaseAddress (
  VOID
  )
{
  UINTN     IoPortBaseAddress;

  if (FeaturePcdGet (PcdSmbaIoBaseAddressFixed)) {
    IoPortBaseAddress = (UINTN) PcdGet16 (PcdSmbaIoBaseAddress);
  } else {
    IoPortBaseAddress = (UINTN) LpcPciCfg32 (R_QNC_LPC_SMBUS_BASE) & B_QNC_LPC_SMBUS_BASE_MASK;
  }

  //
  // Make sure that the IO port base address has been properly set.
  //
  ASSERT (IoPortBaseAddress != 0);

  return IoPortBaseAddress;
}


VOID
InitializeInternal (
  )
{
  UINTN     IoPortBaseAddress;

  IoPortBaseAddress = GetSmbusIoPortBaseAddress ();

  //
  // Step1: Enable QNC SMBUS I/O space.
  //
  LpcPciCfg32Or(R_QNC_LPC_SMBUS_BASE, B_QNC_LPC_SMBUS_BASE_EN);

  //
  // Step2: Clear Status Register before anyone uses the interfaces.
  //
  IoWrite8 (IoPortBaseAddress + R_QNC_SMBUS_HSTS, B_QNC_SMBUS_HSTS_ALL);

  //
  // Step3: Program the correct smbus clock
  //
  IoWrite8 (IoPortBaseAddress + R_QNC_SMBUS_HCLK, V_QNC_SMBUS_HCLK_100KHZ);
}




BOOLEAN
IsAddressAvailable (
  IN      EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress
  )
{
  UINT8         Index;

  //
  // See if we have already assigned this address to a device
  //
  for (Index = 0; Index < mDeviceMapEntries; Index++) {
    if (SlaveAddress.SmbusDeviceAddress ==
         mDeviceMap[Index].SmbusDeviceAddress.SmbusDeviceAddress) {
      return FALSE;
    }
  }

  //
  // See if this address is claimed by a platform non-ARP-capable device
  //
  for (Index = 0; Index < mPlatformNumRsvd; Index++) {
    if ((SlaveAddress.SmbusDeviceAddress << 1) == mPlatformAddrRsvd[Index]) {
      return FALSE;
    }
  }

  //
  // See if this is a reserved address
  //
  for (Index = 0; Index < SMBUS_NUM_RESERVED; Index++) {
    if (SlaveAddress.SmbusDeviceAddress == (UINTN) mReservedAddress[Index]) {
      return FALSE;
    }
  }

  return TRUE;
}


EFI_STATUS
GetNextAvailableAddress (
  IN EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress
  )
{
  for (SlaveAddress->SmbusDeviceAddress = 0x03;
    SlaveAddress->SmbusDeviceAddress < 0x7F;
    SlaveAddress->SmbusDeviceAddress++
    ) {
    if (IsAddressAvailable (*SlaveAddress)) {
      return EFI_SUCCESS;
    }
  }

  return EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
SmbusPrepareToArp (
  )
{
  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress;
  EFI_STATUS                Status;
  UINTN                     Length;
  UINT8                     Buffer;

  SlaveAddress.SmbusDeviceAddress = SMBUS_ADDRESS_ARP;
  Length = 1;
  Buffer = SMBUS_DATA_PREPARE_TO_ARP;

  Status = Execute (
             SlaveAddress,
             0,
             EfiSmbusSendByte,
             TRUE,
             &Length,
             &Buffer
             );
  return Status;
}

EFI_STATUS
SmbusGetUdidGeneral (
  IN OUT  EFI_SMBUS_DEVICE_MAP  *DeviceMap
  )
{
  EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress;
  EFI_STATUS                    Status;
  UINTN                         Length;
  UINT8                         Buffer[SMBUS_GET_UDID_LENGTH];

  SlaveAddress.SmbusDeviceAddress = SMBUS_ADDRESS_ARP;
  Length = SMBUS_GET_UDID_LENGTH;

  Status = Execute (
             SlaveAddress,
             SMBUS_DATA_GET_UDID_GENERAL,
             EfiSmbusReadBlock,
             TRUE,
             &Length,
             Buffer
             );

  if (!EFI_ERROR(Status)) {
    if (Length == SMBUS_GET_UDID_LENGTH) {
      DeviceMap->SmbusDeviceUdid.DeviceCapabilities = Buffer[0];
      DeviceMap->SmbusDeviceUdid.VendorRevision = Buffer[1];
      DeviceMap->SmbusDeviceUdid.VendorId = (UINT16)((Buffer[2] << 8) + Buffer[3]);
      DeviceMap->SmbusDeviceUdid.DeviceId = (UINT16)((Buffer[4] << 8) + Buffer[5]);
      DeviceMap->SmbusDeviceUdid.Interface = (UINT16)((Buffer[6] << 8) + Buffer[7]);
      DeviceMap->SmbusDeviceUdid.SubsystemVendorId = (UINT16)((Buffer[8] << 8) + Buffer[9]);
      DeviceMap->SmbusDeviceUdid.SubsystemDeviceId = (UINT16)((Buffer[10] << 8) + Buffer[11]);
      DeviceMap->SmbusDeviceUdid.VendorSpecificId = (UINT32)((Buffer[12] << 24) + (Buffer[13] << 16) + (Buffer[14] << 8) + Buffer[15]);
      DeviceMap->SmbusDeviceAddress.SmbusDeviceAddress = (UINT8)(Buffer[16] >> 1);
    } else {
      Status = EFI_DEVICE_ERROR;
    }
  }

  return Status;
}

EFI_STATUS
SmbusAssignAddress (
  IN OUT  EFI_SMBUS_DEVICE_MAP  *DeviceMap
  )
{
  EFI_SMBUS_DEVICE_ADDRESS      SlaveAddress;
  EFI_STATUS                    Status;
  UINTN                         Length;
  UINT8                         Buffer[SMBUS_GET_UDID_LENGTH];

  Buffer[0] = DeviceMap->SmbusDeviceUdid.DeviceCapabilities;
  Buffer[1] = DeviceMap->SmbusDeviceUdid.VendorRevision;
  Buffer[2] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorId >> 8);
  Buffer[3] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorId);
  Buffer[4] = (UINT8)(DeviceMap->SmbusDeviceUdid.DeviceId >> 8);
  Buffer[5] = (UINT8)(DeviceMap->SmbusDeviceUdid.DeviceId);
  Buffer[6] = (UINT8)(DeviceMap->SmbusDeviceUdid.Interface >> 8);
  Buffer[7] = (UINT8)(DeviceMap->SmbusDeviceUdid.Interface);
  Buffer[8] = (UINT8)(DeviceMap->SmbusDeviceUdid.SubsystemVendorId >> 8);
  Buffer[9] = (UINT8)(DeviceMap->SmbusDeviceUdid.SubsystemVendorId);
  Buffer[10] = (UINT8)(DeviceMap->SmbusDeviceUdid.SubsystemDeviceId >> 8);
  Buffer[11] = (UINT8)(DeviceMap->SmbusDeviceUdid.SubsystemDeviceId);
  Buffer[12] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorSpecificId >> 24);
  Buffer[13] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorSpecificId >> 16);
  Buffer[14] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorSpecificId >> 8);
  Buffer[15] = (UINT8)(DeviceMap->SmbusDeviceUdid.VendorSpecificId);
  Buffer[16] = (UINT8)(DeviceMap->SmbusDeviceAddress.SmbusDeviceAddress << 1);

  SlaveAddress.SmbusDeviceAddress = SMBUS_ADDRESS_ARP;
  Length = SMBUS_GET_UDID_LENGTH;

  Status = Execute (
             SlaveAddress,
             SMBUS_DATA_ASSIGN_ADDRESS,
             EfiSmbusWriteBlock,
             TRUE,
             &Length,
             Buffer
             );
  return Status;
}


EFI_STATUS
SmbusFullArp (
  )
{
  EFI_STATUS              Status;
  EFI_SMBUS_DEVICE_MAP    *CurrentDeviceMap;

  Status = SmbusPrepareToArp ();
  if (EFI_ERROR(Status)) {
    if (Status == EFI_DEVICE_ERROR) {
      //
      //  ARP is complete
      //
      return EFI_SUCCESS;
    } else {
      return Status;
    }
  }

  //
  //  Main loop to ARP all ARP-capable devices
  //
  do {
    CurrentDeviceMap = &mDeviceMap[mDeviceMapEntries];
    Status = SmbusGetUdidGeneral (CurrentDeviceMap);
    if (EFI_ERROR(Status)) {
      break;
    }

    if (CurrentDeviceMap->SmbusDeviceAddress.SmbusDeviceAddress == (0xFF >> 1)) {
      //
      // If address is unassigned, assign it
      //
      Status = GetNextAvailableAddress (
                 &CurrentDeviceMap->SmbusDeviceAddress
                 );
      if (EFI_ERROR(Status)) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else if (((CurrentDeviceMap->SmbusDeviceUdid.DeviceCapabilities) & 0xC0) != 0) {
      //
      // if address is not fixed, check if the current address is available
      //
      if (!IsAddressAvailable (
             CurrentDeviceMap->SmbusDeviceAddress
             )) {
        //
        // if currently assigned address is already used, get a new one
        //
        Status = GetNextAvailableAddress (
                   &CurrentDeviceMap->SmbusDeviceAddress
                   );
        if (EFI_ERROR(Status)) {
          return EFI_OUT_OF_RESOURCES;
        }
      }
    }

    Status = SmbusAssignAddress (CurrentDeviceMap);
    if (EFI_ERROR(Status)) {
      //
      // If there was a device error, just continue on and try again.
      // Other errors should be reported.
      //
      if (Status != EFI_DEVICE_ERROR) {
        return Status;
      }
    } else {
      //
      // If there was no error, the address was assigned and we must update our
      // records.
      //
      mDeviceMapEntries++;
    }

  } while (mDeviceMapEntries < MAX_SMBUS_DEVICES);

  return EFI_SUCCESS;
}


EFI_STATUS
SmbusDirectedArp (
  IN      EFI_SMBUS_UDID            *SmbusUdid,
  IN OUT  EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress
  )
{
  EFI_STATUS                        Status;
  EFI_SMBUS_DEVICE_MAP              *CurrentDeviceMap;

  if (mDeviceMapEntries >= MAX_SMBUS_DEVICES) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentDeviceMap = &mDeviceMap[mDeviceMapEntries];

  //
  // Find an available address to assign
  //
  Status = GetNextAvailableAddress (
             &CurrentDeviceMap->SmbusDeviceAddress
             );
  if (EFI_ERROR(Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  CurrentDeviceMap->SmbusDeviceUdid.DeviceCapabilities  = SmbusUdid->DeviceCapabilities;
  CurrentDeviceMap->SmbusDeviceUdid.DeviceId            = SmbusUdid->DeviceId;
  CurrentDeviceMap->SmbusDeviceUdid.Interface           = SmbusUdid->Interface;
  CurrentDeviceMap->SmbusDeviceUdid.SubsystemDeviceId   = SmbusUdid->SubsystemDeviceId;
  CurrentDeviceMap->SmbusDeviceUdid.SubsystemVendorId   = SmbusUdid->SubsystemVendorId;
  CurrentDeviceMap->SmbusDeviceUdid.VendorId            = SmbusUdid->VendorId;
  CurrentDeviceMap->SmbusDeviceUdid.VendorRevision      = SmbusUdid->VendorRevision;
  CurrentDeviceMap->SmbusDeviceUdid.VendorSpecificId    = SmbusUdid->VendorSpecificId;

  Status = SmbusAssignAddress (CurrentDeviceMap);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  mDeviceMapEntries++;
  SlaveAddress->SmbusDeviceAddress = CurrentDeviceMap->SmbusDeviceAddress.SmbusDeviceAddress;

  return EFI_SUCCESS;
}



/**
  Executes an SMBus operation to an SMBus controller. Returns when either the command has been
  executed or an error is encountered in doing the operation.

  The Execute() function provides a standard way to execute an operation as defined in the System
  Management Bus (SMBus) Specification. The resulting transaction will be either that the SMBus
  slave devices accept this transaction or that this function returns with error.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  SlaveAddress            The SMBus slave address of the device with which to communicate.
  @param  Command                 This command is transmitted by the SMBus host controller to the
                                  SMBus slave device and the interpretation is SMBus slave device
                                  specific. It can mean the offset to a list of functions inside an
                                  SMBus slave device. Not all operations or slave devices support
                                  this command's registers.
  @param  Operation               Signifies which particular SMBus hardware protocol instance that
                                  it will use to execute the SMBus transactions. This SMBus
                                  hardware protocol is defined by the SMBus Specification and is
                                  not related to EFI.
  @param  PecCheck                Defines if Packet Error Code (PEC) checking is required for this
                                  operation.
  @param  Length                  Signifies the number of bytes that this operation will do. The
                                  maximum number of bytes can be revision specific and operation
                                  specific. This field will contain the actual number of bytes that
                                  are executed for this operation. Not all operations require this
                                  argument.
  @param  Buffer                  Contains the value of data to execute to the SMBus slave device.
                                  Not all operations require this argument. The length of this
                                  buffer is identified by Length.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_INVALID_PARAMETER   Operation is not defined in EFI_SMBUS_OPERATION.
  @retval EFI_INVALID_PARAMETER   Length/Buffer is NULL for operations except for EfiSmbusQuickRead
                                  and EfiSmbusQuickWrite. Length is outside the range of valid
                                  values.
  @retval EFI_UNSUPPORTED         The SMBus operation or PEC is not supported.
  @retval EFI_BUFFER_TOO_SMALL    Buffer is not sufficient for this operation.

**/
EFI_STATUS
EFIAPI
SmbusExecute (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN CONST  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN CONST  EFI_SMBUS_DEVICE_COMMAND  Command,
  IN CONST  EFI_SMBUS_OPERATION       Operation,
  IN CONST  BOOLEAN                   PecCheck,
  IN OUT    UINTN                     *Length,
  IN OUT    VOID                      *Buffer
  )
{
  InitializeInternal ();
  return Execute (
           SlaveAddress,
           Command,
           Operation,
           PecCheck,
           Length,
           Buffer
           );
}

/**
  Sets the SMBus slave device addresses for the device with a given unique ID or enumerates the
  entire bus.

  The ArpDevice() function provides a standard way for a device driver to enumerate the entire
  SMBus or specific devices on the bus.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  ArpAll                  A Boolean expression that indicates if the host drivers need to
                                  enumerate all the devices or enumerate only the device that is
                                  identified by SmbusUdid. If ArpAll is TRUE, SmbusUdid and
                                  SlaveAddress are optional. If ArpAll is FALSE, ArpDevice will
                                  enumerate SmbusUdid and the address will be at SlaveAddress.
  @param  SmbusUdid               The Unique Device Identifier (UDID) that is associated with this
                                  device.
  @param  SlaveAddress            The SMBus slave address that is associated with an SMBus UDID.

  @retval EFI_SUCCESS             The last data that was returned from the access matched the poll
                                  exit criteria.
  @retval EFI_CRC_ERROR           Checksum is not correct (PEC is incorrect).
  @retval EFI_TIMEOUT             Timeout expired before the operation was completed. Timeout is
                                  determined by the SMBus host controller device.
  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR        The request was not completed because a failure that was
                                  reflected in the Host Status Register bit. Device errors are a
                                  result of a transaction collision, illegal command field,
                                  unclaimed cycle (host initiated), or bus errors (collisions).
  @retval EFI_UNSUPPORTED         The corresponding SMBus operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusArpDevice (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN        BOOLEAN                   ArpAll,
  IN        EFI_SMBUS_UDID            *SmbusUdid,   OPTIONAL
  IN OUT    EFI_SMBUS_DEVICE_ADDRESS  *SlaveAddress OPTIONAL
  )
{
    InitializeInternal ();

    if (ArpAll) {
    return SmbusFullArp ();
  } else {
    if ((SmbusUdid == NULL) || (SlaveAddress == NULL)) {
      return EFI_INVALID_PARAMETER;
    }
    return SmbusDirectedArp ((EFI_SMBUS_UDID *)SmbusUdid, SlaveAddress);
  }
}

/**
  Returns a pointer to the Address Resolution Protocol (ARP) map that contains the ID/address pair
  of the slave devices that were enumerated by the SMBus host controller driver.

  The GetArpMap() function returns the mapping of all the SMBus devices that were enumerated by the
  SMBus host driver.

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  Length                  Size of the buffer that contains the SMBus device map.
  @param  SmbusDeviceMap          The pointer to the device map as enumerated by the SMBus
                                  controller driver.

  @retval EFI_SUCCESS             The SMBus returned the current device map.
  @retval EFI_UNSUPPORTED         The corresponding operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusGetArpMap (
  IN CONST  EFI_SMBUS_HC_PROTOCOL   *This,
  IN OUT    UINTN                   *Length,
  IN OUT    EFI_SMBUS_DEVICE_MAP    **SmbusDeviceMap
  )
{
  *Length = mDeviceMapEntries;
  *SmbusDeviceMap = mDeviceMap;
  return EFI_SUCCESS;
}


/**
  Allows a device driver to register for a callback when the bus driver detects a state that it
  needs to propagate to other drivers that are registered for a callback.

  The Notify() function registers all the callback functions to allow the bus driver to call these
  functions when the SlaveAddress/Data pair happens.
  If NotifyFunction is NULL, then ASSERT ().

  @param  This                    A pointer to the EFI_SMBUS_HC_PROTOCOL instance.
  @param  SlaveAddress            The SMBUS hardware address to which the SMBUS device is
                                  preassigned or allocated.
  @param  Data                    Data of the SMBus host notify command that the caller wants to be
                                  called.
  @param  NotifyFunction          The function to call when the bus driver detects the SlaveAddress
                                  and Data pair.

  @retval EFI_SUCCESS             NotifyFunction was registered.
  @retval EFI_UNSUPPORTED         The corresponding operation is not supported.

**/
EFI_STATUS
EFIAPI
SmbusNotify (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN CONST  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN CONST  UINTN                     Data,
  IN CONST  EFI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Entry point to the DXE Driver that produces the SMBus Host Controller Protocol.

  @param  ImageHandle      ImageHandle of the loaded driver.
  @param  SystemTable      Pointer to the EFI System Table.

  @retval EFI_SUCCESS      The entry point of SMBus DXE driver is executed successfully.
  @retval !EFI_SUCESS      Some error occurs in the entry point of SMBus DXE driver.

**/
EFI_STATUS
EFIAPI
InitializeQNCSmbus (
  IN EFI_HANDLE            ImageHandle,
  IN EFI_SYSTEM_TABLE      *SystemTable
  )
{
  EFI_STATUS    Status;

  mPlatformNumRsvd = (UINT8)PcdGet32 (PcdPlatformSmbusAddrNum);
  mPlatformAddrRsvd = (UINT8 *)(UINTN) PcdGet64 (PcdPlatformSmbusAddrTable);

  //
  // Install SMBus Host Controller protocol interface.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mSmbusHcHandle,
                  &gEfiSmbusHcProtocolGuid,
                  &mSmbusHc,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
