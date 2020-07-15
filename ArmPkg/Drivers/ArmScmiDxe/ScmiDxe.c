/** @file

  Copyright (c) 2017-2018, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  System Control and Management Interface V1.0
    http://infocenter.arm.com/help/topic/com.arm.doc.den0056a/
    DEN0056A_System_Control_and_Management_Interface.pdf
**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/ArmScmiBaseProtocol.h>
#include <Protocol/ArmScmiClockProtocol.h>
#include <Protocol/ArmScmiPerformanceProtocol.h>

#include "ArmScmiBaseProtocolPrivate.h"
#include "ArmScmiClockProtocolPrivate.h"
#include "ArmScmiPerformanceProtocolPrivate.h"
#include "ScmiDxe.h"
#include "ScmiPrivate.h"

STATIC CONST SCMI_PROTOCOL_ENTRY Protocols[] = {
  { SCMI_PROTOCOL_ID_BASE, ScmiBaseProtocolInit },
  { SCMI_PROTOCOL_ID_PERFORMANCE, ScmiPerformanceProtocolInit },
  { SCMI_PROTOCOL_ID_CLOCK, ScmiClockProtocolInit }
};

/** ARM SCMI driver entry point function.

  This function installs the SCMI Base protocol and a list of other
  protocols is queried using the Base protocol. If protocol is supported,
  driver will call each protocol init function to install the protocol on
  the ImageHandle.

  @param[in] ImageHandle     Handle to this EFI Image which will be used to
                             install Base, Clock and Performance protocols.
  @param[in] SystemTable     A pointer to boot time system table.

  @retval EFI_SUCCESS       Driver initalized successfully.
  @retval EFI_UNSUPPORTED   If SCMI base protocol version is not supported.
  @retval !(EFI_SUCCESS)    Other errors.
**/
EFI_STATUS
EFIAPI
ArmScmiDxeEntryPoint (
  IN EFI_HANDLE             ImageHandle,
  IN EFI_SYSTEM_TABLE       *SystemTable
  )
{
  EFI_STATUS          Status;
  SCMI_BASE_PROTOCOL  *BaseProtocol;
  UINT32              Version;
  UINT32              Index;
  UINT32              NumProtocols;
  UINT32              ProtocolIndex;
  UINT8               *SupportedList;
  UINT32              SupportedListSize;

  // Every SCMI implementation must implement the base protocol.
  ASSERT (Protocols[0].Id == SCMI_PROTOCOL_ID_BASE);

  Status = ScmiBaseProtocolInit (&ImageHandle);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Status = gBS->LocateProtocol (
                  &gArmScmiBaseProtocolGuid,
                  NULL,
                  (VOID**)&BaseProtocol
                  );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Get SCMI Base protocol version.
  Status = BaseProtocol->GetVersion (BaseProtocol, &Version);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  if (Version != BASE_PROTOCOL_VERSION) {
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // Apart from Base protocol, SCMI may implement various other protocols,
  // query total protocols implemented by the SCP firmware.
  NumProtocols = 0;
  Status = BaseProtocol->GetTotalProtocols (BaseProtocol, &NumProtocols);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  ASSERT (NumProtocols != 0);

  SupportedListSize = (NumProtocols * sizeof (*SupportedList));

  Status = gBS->AllocatePool (
                  EfiBootServicesData,
                  SupportedListSize,
                  (VOID**)&SupportedList
                  );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Get the list of protocols supported by SCP firmware on the platform.
  Status = BaseProtocol->DiscoverListProtocols (
                           BaseProtocol,
                           &SupportedListSize,
                           SupportedList
                           );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (SupportedList);
    ASSERT (FALSE);
    return Status;
  }

  // Install supported protocol on ImageHandle.
  for (ProtocolIndex = 1; ProtocolIndex < ARRAY_SIZE (Protocols);
       ProtocolIndex++) {
    for (Index = 0; Index < NumProtocols; Index++) {
      if (Protocols[ProtocolIndex].Id == SupportedList[Index]) {
        Status = Protocols[ProtocolIndex].InitFn (&ImageHandle);
        if (EFI_ERROR (Status)) {
          ASSERT_EFI_ERROR (Status);
          return Status;
        }
        break;
      }
    }
  }

  gBS->FreePool (SupportedList);

  return EFI_SUCCESS;
}
