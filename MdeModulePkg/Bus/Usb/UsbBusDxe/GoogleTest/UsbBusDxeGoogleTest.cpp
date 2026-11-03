/** @file
  Unit tests for UsbBusDxe descriptor parsing and root hub reset handling.

  Copyright (c) 2026, Intel Corporation. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/GoogleTestLib.h>

extern "C" {
  #include <Uefi.h>
  #include <Library/BaseMemoryLib.h>
  #include "../UsbBus.h"

  BOOLEAN
  UsbIsSSDevice (
    IN EFI_USB_BOS_DESCRIPTOR  *BosDesc,
    IN UINTN                   Len
    );

  EFI_STATUS
  UsbGetDevBosDesc (
    IN USB_DEVICE  *UsbDev
    );

  EFI_STATUS
  UsbRootHubResetPort (
    IN USB_INTERFACE  *RootIf,
    IN UINT8          Port
    );
}

static EFI_BOOT_SERVICES       mBootServices;
static UINTN                   mFreePoolCount;
static UINTN                   mControlTransferCount;
static EFI_STATUS              mControlTransferStatus;
static EFI_USB_PORT_STATUS     mRootHubPortStatus;
static EFI_USB_BOS_DESCRIPTOR  mBosDescriptor;

extern "C" {
  EFI_BOOT_SERVICES  *gBS = &mBootServices;

  /**
    Count and free a buffer through the test's mocked boot-services table.

    @param[in] Buffer  Buffer to free.

    @retval EFI_SUCCESS  The buffer was freed.
  **/
  EFI_STATUS
  EFIAPI
  TestFreePool (
    IN VOID  *Buffer
    )
  {
    mFreePoolCount++;
    FreePool (Buffer);
    return EFI_SUCCESS;
  }

  /**
    Mock the boot-services stall function without delaying the test.

    @param[in] Microseconds  Requested delay, ignored by this mock.

    @retval EFI_SUCCESS  The mock completed successfully.
  **/
  EFI_STATUS
  EFIAPI
  TestStall (
    IN UINTN  Microseconds
    )
  {
    return EFI_SUCCESS;
  }

  /**
    Record a USB control-transfer call and return the status selected by the test.

    @param[in]  UsbBus       USB bus, unused by this mock.
    @param[in]  DevAddr      Device address, unused by this mock.
    @param[in]  DevSpeed     Device speed, unused by this mock.
    @param[in]  MaxPacket    Maximum packet size, unused by this mock.
    @param[in]  Request      Transfer request, unused by this mock.
    @param[in]  Direction    Transfer direction, unused by this mock.
    @param[in,out] Data      Transfer buffer, unused by this mock.
    @param[in,out] DataLength Transfer length, unused by this mock.
    @param[in]  TimeOut      Transfer timeout, unused by this mock.
    @param[in]  Translator   Transaction translator, unused by this mock.
    @param[out] UsbResult    USB transfer result, unused by this mock.

    @retval mControlTransferStatus  Status configured by the test.
  **/
  EFI_STATUS
  UsbHcControlTransfer (
    IN  USB_BUS                             *UsbBus,
    IN  UINT8                               DevAddr,
    IN  UINT8                               DevSpeed,
    IN  UINTN                               MaxPacket,
    IN  EFI_USB_DEVICE_REQUEST              *Request,
    IN  EFI_USB_DATA_DIRECTION              Direction,
    IN  OUT VOID                            *Data,
    IN  OUT UINTN                           *DataLength,
    IN  UINTN                               TimeOut,
    IN  EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Translator,
    OUT UINT32                              *UsbResult
    )
  {
    mControlTransferCount++;
    if ((Request->Value >> 8) == USB_DESC_TYPE_BOS) {
      CopyMem (Data, &mBosDescriptor, MIN (*DataLength, sizeof (mBosDescriptor)));
    }

    return mControlTransferStatus;
  }

  /**
    Mock retrieval of USB host-controller capabilities.

    @param[in]  UsbBus          USB bus, unused by this mock.
    @param[out] MaxSpeed        Maximum speed output, unused by this mock.
    @param[out] NumOfPort       Port-count output, unused by this mock.
    @param[out] Is64BitCapable  64-bit capability output, unused by this mock.

    @retval EFI_UNSUPPORTED  Capability retrieval is not used by these tests.
  **/
  EFI_STATUS
  UsbHcGetCapability (
    IN  USB_BUS  *UsbBus,
    OUT UINT8    *MaxSpeed,
    OUT UINT8    *NumOfPort,
    OUT UINT8    *Is64BitCapable
    )
  {
    return EFI_UNSUPPORTED;
  }

  /**
    Mock setting a root-hub port feature.

    @param[in] UsbBus     USB bus, unused by this mock.
    @param[in] PortIndex  Root-hub port, unused by this mock.
    @param[in] Feature    Feature to set, unused by this mock.

    @retval EFI_SUCCESS  The mock completed successfully.
  **/
  EFI_STATUS
  UsbHcSetRootHubPortFeature (
    IN USB_BUS               *UsbBus,
    IN UINT8                 PortIndex,
    IN EFI_USB_PORT_FEATURE  Feature
    )
  {
    return EFI_SUCCESS;
  }

  /**
    Mock clearing a root-hub port feature.

    @param[in] UsbBus     USB bus, unused by this mock.
    @param[in] PortIndex  Root-hub port, unused by this mock.
    @param[in] Feature    Feature to clear, unused by this mock.

    @retval EFI_SUCCESS  The mock completed successfully.
  **/
  EFI_STATUS
  UsbHcClearRootHubPortFeature (
    IN USB_BUS               *UsbBus,
    IN UINT8                 PortIndex,
    IN EFI_USB_PORT_FEATURE  Feature
    )
  {
    return EFI_SUCCESS;
  }

  /**
    Return the root-hub port status configured by the test.

    @param[in]  UsbBus     USB bus, unused by this mock.
    @param[in]  PortIndex  Root-hub port, unused by this mock.
    @param[out] PortStatus Receives the configured port status.

    @retval EFI_SUCCESS  The configured status was returned.
  **/
  EFI_STATUS
  UsbHcGetRootHubPortStatus (
    IN  USB_BUS              *UsbBus,
    IN  UINT8                PortIndex,
    OUT EFI_USB_PORT_STATUS  *PortStatus
    )
  {
    *PortStatus = mRootHubPortStatus;
    return EFI_SUCCESS;
  }

  /**
    Provide the no-op hub-enumeration callback needed to link UsbHub.c.

    @param[in] Event    Event that triggered enumeration, unused by this stub.
    @param[in] Context  Callback context, unused by this stub.
  **/
  VOID
  EFIAPI
  UsbHubEnumeration (
    IN EFI_EVENT  Event,
    IN VOID       *Context
    )
  {
  }

  /**
    Provide the no-op root-hub enumeration callback needed to link UsbHub.c.

    @param[in] Event    Event that triggered enumeration, unused by this stub.
    @param[in] Context  Callback context, unused by this stub.
  **/
  VOID
  EFIAPI
  UsbRootHubEnumeration (
    IN EFI_EVENT  Event,
    IN VOID       *Context
    )
  {
  }
}

class UsbBusDxeTest : public ::testing::Test {
protected:

  /**
    Reset mocked boot services and per-test USB call state.
  **/
  void
  SetUp (
    ) override
  {
    ZeroMem (&mBootServices, sizeof (mBootServices));
    mBootServices.FreePool = TestFreePool;
    mBootServices.Stall    = TestStall;

    mFreePoolCount         = 0;
    mControlTransferCount  = 0;
    mControlTransferStatus = EFI_DEVICE_ERROR;
    ZeroMem (&mBosDescriptor, sizeof (mBosDescriptor));
    mRootHubPortStatus.PortStatus       = USB_PORT_STAT_ENABLE;
    mRootHubPortStatus.PortChangeStatus = 0;
  }
};

/**
  Verify that a well-formed BOS containing a SuperSpeed capability is accepted.
**/
TEST_F (UsbBusDxeTest, RecognizesValidSuperSpeedCapability) {
  UINT8                       Buffer[sizeof (EFI_USB_BOS_DESCRIPTOR) + sizeof (EFI_USB_DEV_CAP_DESCRIPTOR)] = { 0 };
  EFI_USB_BOS_DESCRIPTOR      *Bos;
  EFI_USB_DEV_CAP_DESCRIPTOR  *Capability;

  Bos                           = (EFI_USB_BOS_DESCRIPTOR *)Buffer;
  Bos->Length                   = sizeof (*Bos);
  Bos->DescriptorType           = USB_DESC_TYPE_BOS;
  Bos->TotalLength              = sizeof (Buffer);
  Bos->NumDeviceCaps            = 1;
  Capability                    = (EFI_USB_DEV_CAP_DESCRIPTOR *)(Buffer + sizeof (*Bos));
  Capability->Length            = sizeof (*Capability);
  Capability->DescriptorType    = USB_DESC_TYPE_DEV_CAP;
  Capability->DevCapabilityType = USB_DEV_CAP_SUPPERSPEED_USB;

  EXPECT_TRUE (UsbIsSSDevice (Bos, sizeof (Buffer)));
}

/**
  Verify that a three-byte PTM capability is accepted before a SuperSpeed capability.
**/
TEST_F (UsbBusDxeTest, AcceptsThreeBytePtmCapability) {
  UINT8                       Buffer[sizeof (EFI_USB_BOS_DESCRIPTOR) + 3 + sizeof (EFI_USB_DEV_CAP_DESCRIPTOR)] = { 0 };
  EFI_USB_BOS_DESCRIPTOR      *Bos;
  UINT8                       *PtmCapability;
  EFI_USB_DEV_CAP_DESCRIPTOR  *SsCapability;

  Bos                             = (EFI_USB_BOS_DESCRIPTOR *)Buffer;
  Bos->Length                     = sizeof (*Bos);
  Bos->DescriptorType             = USB_DESC_TYPE_BOS;
  Bos->TotalLength                = sizeof (Buffer);
  Bos->NumDeviceCaps              = 2;
  PtmCapability                   = Buffer + sizeof (*Bos);
  PtmCapability[0]                = OFFSET_OF (EFI_USB_DEV_CAP_DESCRIPTOR, CapData);
  PtmCapability[1]                = USB_DESC_TYPE_DEV_CAP;
  PtmCapability[2]                = 0x0B;
  SsCapability                    = (EFI_USB_DEV_CAP_DESCRIPTOR *)(PtmCapability + PtmCapability[0]);
  SsCapability->Length            = sizeof (*SsCapability);
  SsCapability->DescriptorType    = USB_DESC_TYPE_DEV_CAP;
  SsCapability->DevCapabilityType = USB_DEV_CAP_SUPPERSPEED_USB;

  EXPECT_TRUE (UsbIsSSDevice (Bos, sizeof (Buffer)));
}

/**
  Verify that a capability extending beyond the BOS total length is rejected.
**/
TEST_F (UsbBusDxeTest, RejectsCapabilityBeyondBosTotalLength) {
  UINT8                       Buffer[sizeof (EFI_USB_BOS_DESCRIPTOR) + sizeof (EFI_USB_DEV_CAP_DESCRIPTOR)] = { 0 };
  EFI_USB_BOS_DESCRIPTOR      *Bos;
  EFI_USB_DEV_CAP_DESCRIPTOR  *Capability;

  Bos                           = (EFI_USB_BOS_DESCRIPTOR *)Buffer;
  Bos->Length                   = sizeof (*Bos);
  Bos->DescriptorType           = USB_DESC_TYPE_BOS;
  Bos->TotalLength              = sizeof (*Bos);
  Bos->NumDeviceCaps            = 1;
  Capability                    = (EFI_USB_DEV_CAP_DESCRIPTOR *)(Buffer + sizeof (*Bos));
  Capability->Length            = sizeof (*Capability);
  Capability->DescriptorType    = USB_DESC_TYPE_DEV_CAP;
  Capability->DevCapabilityType = USB_DEV_CAP_SUPPERSPEED_USB;

  EXPECT_FALSE (UsbIsSSDevice (Bos, sizeof (Buffer)));
}

/**
  Verify that a capability extending beyond the supplied BOS buffer is rejected.
**/
TEST_F (UsbBusDxeTest, RejectsCapabilityLengthBeyondBosBuffer) {
  UINT8                       Buffer[sizeof (EFI_USB_BOS_DESCRIPTOR) + sizeof (EFI_USB_DEV_CAP_DESCRIPTOR)] = { 0 };
  EFI_USB_BOS_DESCRIPTOR      *Bos;
  EFI_USB_DEV_CAP_DESCRIPTOR  *Capability;

  Bos                           = (EFI_USB_BOS_DESCRIPTOR *)Buffer;
  Bos->Length                   = sizeof (*Bos);
  Bos->DescriptorType           = USB_DESC_TYPE_BOS;
  Bos->TotalLength              = sizeof (Buffer);
  Bos->NumDeviceCaps            = 1;
  Capability                    = (EFI_USB_DEV_CAP_DESCRIPTOR *)(Buffer + sizeof (*Bos));
  Capability->Length            = sizeof (*Capability) + 1;
  Capability->DescriptorType    = USB_DESC_TYPE_DEV_CAP;
  Capability->DevCapabilityType = USB_DEV_CAP_SUPPERSPEED_USB;

  EXPECT_FALSE (UsbIsSSDevice (Bos, sizeof (Buffer)));
}

/**
  Verify failed BOS reads free their buffer and short total lengths are rejected.
**/
TEST_F (UsbBusDxeTest, HandlesBosReadFailureAndShortTotalLength) {
  USB_BUS          Bus;
  USB_DEVICE       UsbDevice;
  USB_DEVICE_DESC  DeviceDesc;

  ZeroMem (&Bus, sizeof (Bus));
  ZeroMem (&UsbDevice, sizeof (UsbDevice));
  ZeroMem (&DeviceDesc, sizeof (DeviceDesc));
  UsbDevice.Bus     = &Bus;
  UsbDevice.DevDesc = &DeviceDesc;

  EXPECT_EQ (UsbGetDevBosDesc (&UsbDevice), EFI_DEVICE_ERROR);
  EXPECT_EQ (mControlTransferCount, (UINTN)1);
  EXPECT_EQ (mFreePoolCount, (UINTN)1);
  EXPECT_EQ (DeviceDesc.BosDesc, nullptr);

  mFreePoolCount                = 0;
  mControlTransferCount         = 0;
  mControlTransferStatus        = EFI_SUCCESS;
  mBosDescriptor.Length         = sizeof (mBosDescriptor);
  mBosDescriptor.DescriptorType = USB_DESC_TYPE_BOS;
  mBosDescriptor.TotalLength    = sizeof (mBosDescriptor) - 1;

  EXPECT_EQ (UsbGetDevBosDesc (&UsbDevice), EFI_DEVICE_ERROR);
  EXPECT_EQ (mControlTransferCount, (UINTN)1);
  EXPECT_EQ (mFreePoolCount, (UINTN)1);
  EXPECT_EQ (DeviceDesc.BosDesc, nullptr);
}

/**
  Verify root-hub reset does not dereference a null device descriptor.
**/
TEST_F (UsbBusDxeTest, AvoidsNullDereferenceDuringRootHubReset) {
  USB_BUS               Bus;
  USB_DEVICE            RootDevice;
  USB_DEVICE            Child;
  USB_INTERFACE         RootIf;
  EFI_USB2_HC_PROTOCOL  Usb2Hc;
  CONST UINT8           Port = 1;

  ZeroMem (&Bus, sizeof (Bus));
  ZeroMem (&RootDevice, sizeof (RootDevice));
  ZeroMem (&Child, sizeof (Child));
  ZeroMem (&RootIf, sizeof (RootIf));
  ZeroMem (&Usb2Hc, sizeof (Usb2Hc));

  Usb2Hc.MajorRevision = 0x3;
  Bus.MaxDevices       = 2;
  Bus.Usb2Hc           = &Usb2Hc;
  Bus.Devices[1]       = &Child;

  RootDevice.Bus = &Bus;
  RootIf.Device  = &RootDevice;

  Child.ParentIf   = &RootIf;
  Child.ParentPort = Port;
  Child.Speed      = EFI_USB_SPEED_HIGH;
  Child.IsSSDev    = TRUE;
  Child.DevDesc    = NULL;

  ASSERT_EQ (Child.DevDesc, nullptr);
  EXPECT_EQ (UsbRootHubResetPort (&RootIf, Port), EFI_SUCCESS);
}

/**
  Initialize GoogleTest and return the aggregate result of the test suite.

  @param[in,out] argc  Number of command-line arguments.
  @param[in,out] argv  Command-line arguments.

  @retval GoogleTest test-suite result.
**/
int
main (
  int   argc,
  char  *argv[]
  )
{
  testing::InitGoogleTest (&argc, argv);
  return RUN_ALL_TESTS ();
}
