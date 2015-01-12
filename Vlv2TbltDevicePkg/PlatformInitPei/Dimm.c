/** @file

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

Module Name:


  Dimm.c

Abstract:

  PPI for reading SPD modules on DIMMs.

--*/


//
// Header Files
//
#include "Platformearlyinit.h"

#define DIMM_SOCKETS     4  // Total number of DIMM sockets allowed on
                            //   the platform
#define DIMM_SEGMENTS    1  // Total number of Segments Per DIMM.
#define MEMORY_CHANNELS  2  // Total number of memory channels
                            //   populated on the system board
//
// Prototypes
//

EFI_STATUS
EFIAPI
GetDimmState (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI   *This,
  IN      UINT8                   Dimm,
  OUT     PEI_PLATFORM_DIMM_STATE *State
  );

EFI_STATUS
EFIAPI
SetDimmState (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI   *This,
  IN      UINT8                   Dimm,
  IN      PEI_PLATFORM_DIMM_STATE *State
  );

EFI_STATUS
EFIAPI
ReadSpd (
  IN      EFI_PEI_SERVICES      **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI *This,
  IN      UINT8                 Dimm,
  IN      UINT8                 Offset,
  IN      UINTN                 Count,
  IN OUT  UINT8                 *Buffer
  );

static PEI_PLATFORM_DIMM_PPI mGchDimmPpi = {
  DIMM_SOCKETS,
  DIMM_SEGMENTS,
  MEMORY_CHANNELS,
  GetDimmState,
  SetDimmState,
  ReadSpd
};

static EFI_PEI_PPI_DESCRIPTOR mPpiPlatformDimm = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gPeiPlatformDimmPpiGuid,
  &mGchDimmPpi
};


//
// Functions
//

/**
  This function returns the current state of a single DIMM.  Present indicates
  that the DIMM slot is physically populated.  Disabled indicates that the DIMM
  should not be used.

  @param PeiServices   PEI services table pointer
  @param This          PPI pointer
  @param Dimm          DIMM to read from
  @param State         Pointer to a return buffer to be updated with the current state
                       of the DIMM

  @retval EFI_SUCCESS         The function completed successfully.

**/
EFI_STATUS
EFIAPI
GetDimmState (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI   *This,
  IN      UINT8                   Dimm,
  OUT     PEI_PLATFORM_DIMM_STATE *State
  )
{
  EFI_STATUS                    Status;
  UINT8                         Buffer;

  PEI_ASSERT (PeiServices, (Dimm < This->DimmSockets));

  //
  // A failure here does not necessarily mean that no DIMM is present.
  // Read a single byte.  All we care about is the return status.
  //
  Status = ReadSpd (
             PeiServices,
             This,
             Dimm,
             0,
             1,
             &Buffer
             );

  if (EFI_ERROR (Status)) {
    State->Present = 0;
  } else {
    State->Present = 1;
  }

  //
  // BUGBUG: Update to check platform variable when it is available
  //
  State->Disabled = 0;
  State->Reserved = 0;

  return EFI_SUCCESS;
}

/**

  This function updates the state of a single DIMM.

  @param PeiServices          PEI services table pointer
  @param This                 PPI pointer
  @param Dimm                 DIMM to set state for
  @param State                Pointer to the state information to set.

  @retval EFI_SUCCESS         The function completed successfully.
  @retval EFI_UNSUPPORTED     The function is not supported.

**/
EFI_STATUS
EFIAPI
SetDimmState (
  IN      EFI_PEI_SERVICES        **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI   *This,
  IN      UINT8                   Dimm,
  IN      PEI_PLATFORM_DIMM_STATE *State
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function reads SPD information from a DIMM.

  PeiServices   PEI services table pointer
  This          PPI pointer
  Dimm          DIMM to read from
  Offset        Offset in DIMM
  Count         Number of bytes
  Buffer        Return buffer

  @param EFI_SUCCESS              The function completed successfully.
  @param EFI_DEVICE_ERROR         The DIMM being accessed reported a device error,
                                  does not have an SPD module, or is not installed in
                                  the system.
  @retval EFI_TIMEOUT             Time out trying to read the SPD module.
  @retval EFI_INVALID_PARAMETER   A parameter was outside the legal limits.

**/
EFI_STATUS
EFIAPI
ReadSpd (
  IN      EFI_PEI_SERVICES      **PeiServices,
  IN      PEI_PLATFORM_DIMM_PPI *This,
  IN      UINT8                 Dimm,
  IN      UINT8                 Offset,
  IN      UINTN                 Count,
  IN OUT  UINT8                 *Buffer
  )
{
  EFI_STATUS                Status;
  PEI_SMBUS_PPI             *Smbus;
  UINTN                     Index;
  UINTN                     Index1;
  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress;
  EFI_SMBUS_DEVICE_COMMAND  Command;
  UINTN                     Length;

  Status = (**PeiServices).LocatePpi (
                             PeiServices,
                             &gPeiSmbusPpiGuid,   // GUID
                             0,                   // INSTANCE
                             NULL,                // EFI_PEI_PPI_DESCRIPTOR
                             &Smbus               // PPI
                             );
  ASSERT_PEI_ERROR (PeiServices, Status);

  switch (Dimm) {
  case 0:
    SlaveAddress.SmbusDeviceAddress = SMBUS_ADDR_CH_A_1 >> 1;
    break;
  case 1:
    SlaveAddress.SmbusDeviceAddress = SMBUS_ADDR_CH_A_2 >> 1;
    break;
  case 2:
    SlaveAddress.SmbusDeviceAddress = SMBUS_ADDR_CH_B_1 >> 1;
    break;
  case 3:
    SlaveAddress.SmbusDeviceAddress = SMBUS_ADDR_CH_B_2 >> 1;
    break;
  default:
    return EFI_INVALID_PARAMETER;
  }

  Index = Count % 4;
  if (Index != 0) {
    //
    // read the first serveral bytes to speed up following reading
    //
    for (Index1 = 0; Index1 < Index; Index1++) {
      Length = 1;
      Command = Offset + Index1;
      Status = Smbus->Execute (
                        PeiServices,
                        Smbus,
                        SlaveAddress,
                        Command,
                        EfiSmbusReadByte,
                        FALSE,
                        &Length,
                        &Buffer[Index1]
                        );
      if (EFI_ERROR(Status)) {
        return Status;
      }
    }
  }

  //
  // Now collect all the remaining bytes on 4 bytes block
  //
  for (; Index < Count; Index += 2) {
    Command = Index + Offset;
    Length = 2;
    Status = Smbus->Execute (
                      PeiServices,
                      Smbus,
                      SlaveAddress,
                      Command,
                      EfiSmbusReadWord,
                      FALSE,
                      &Length,
                      &Buffer[Index]
                      );
    if (EFI_ERROR(Status)) {
      return Status;
    }

    Index += 2;
    Command = Index + Offset;
    Length = 2;
    Status = Smbus->Execute (
                      PeiServices,
                      Smbus,
                      SlaveAddress,
                      Command,
                      EfiSmbusReadWord,
                      FALSE,
                      &Length,
                      &Buffer[Index]
                      );
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }
  return EFI_SUCCESS;
}

/**
  This function initializes the PEIM.  It simply installs the DIMM PPI.

  @param FfsHeader       Not used by this function
  @param PeiServices     Pointer to PEI services table

  @retval EFI_SUCCESS    The function completed successfully.

**/
EFI_STATUS
EFIAPI
PeimInitializeDimm (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *SmbusPpi
  )
{
  EFI_STATUS                    Status;

  Status = (**PeiServices).InstallPpi (
                             PeiServices,
                             &mPpiPlatformDimm
                             );
  ASSERT_PEI_ERROR (PeiServices, Status);

  return EFI_SUCCESS;
}

