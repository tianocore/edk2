/** @file
  TpmLib to call Tcg TPM reference code to implement core fTPM function.

  Here is a sample infrastructure using TpmLib in AARCH64:

         Normal world            |         Secure World
    -----------------------------|------------------------------
                                 |
     +--------------+            | +-----------+      +----------+
     |    Tcg2Dxe   |            | |  FtpmSmm  |<---->|  TpmLib  |
     +--------------+            | +-----------+      +----------+
             |                   |       |                  |
             |                   |       |          +------------------+
             |                   |       |          |  PlatformTpmLib  |
             |                   |       |          +------------------+
             |                   |       |
             |                   |    +------------------+
             |                   |    | StandaloneMmCpu  |
             |                   |    +------------------+
             |                   |             |
             |                   |             |
             |                   |             |
     +----------------------+    |       +----------------------------+
     |  Tpm2InstanceFfaLib  |<---------->| StandaloneMmCoreEntryPoint |
     +----------------------+    .       |      (Misc Service)        |
                                 .       +----------------------------+
                                 .
                             Communicate via CRB over FF-A [0]

  - TpmLib is fTPM itself so it handles the TPM command.
  - PlatformTpmLib is used for platform specific functions
  - required by TpmLib (i.e) Nv storage operation, time operation and etc
  - FtpmSmm (or like) is a frontEnd driver which recevie TPM request
    and deliver this request to TpmLib to hanlde the request.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi/UefiBaseType.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PlatformTpmLib.h>

#include <TpmConfiguration/TpmBuildSwitches.h>
#include <CompilerDependencies.h>
#include <ExecCommand_fp.h>
#include <Manufacture_fp.h>
#include <_TPM_Init_fp.h>
#include <platform_interface/tpm_to_platform_interface.h>

/**
  Execute TPM command.

  This function calls ExecuteCommand() in Tcg TPM reference code.
  See the detail ExecuteCommand()'s comment.

  @param [in]       RequestSize     Request Buffer Size
  @param [in]       Request         Request Buffer
  @param [in, out]  ResponseSize    Response Buffer Size
  @param [in, out]  Response        Response Buffer

  @return  EFI_SUCCESS              Success to exectue requested command
                                    But need to check the result of
                                    command execution in Response.
  @return  EFI_INVALID_PARAMETER    Invalid parameters.

**/
EFI_STATUS
EFIAPI
TpmLibExecuteCommand (
  IN      UINT32  RequestSize,          // IN: command buffer size
  IN      UINT8   *Request,             // IN: command buffer
  IN OUT  UINT32  *ResponseSize,        // IN/OUT: response buffer size
  IN OUT  UINT8   **Response            // IN/OUT: response buffer
  )
{
  if ((Request == NULL) || (RequestSize == 0) ||
      (Response == NULL) || (ResponseSize == NULL) ||
      (*Response == NULL) || (*ResponseSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  ExecuteCommand (
    RequestSize,
    Request,
    ResponseSize,
    Response
    );

  return EFI_SUCCESS;
}

/**
  Initailize TPM.

  This function calls _TPM_Init() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmInit (
  VOID
  )
{
  _TPM_Init ();
}

/**
  This indication from the TPM interface indicates the start of
  an H-CRTM measurement sequence. On receipt of this indication,
  the TPM will initialize an H-CRTM Event Sequence context

  This function calls _TPM_Hash_Start() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmHashStart (
  VOID
  )
{
  _TPM_Hash_Start ();
}

/**
  This indication from the TPM interface indicates arrival of
  one or more octets of data that are to be included in the H-CRTM
  Event Sequence sequence context created by the _TPM_Hash_Start indication.
  The context holds data for each hash algorithm for
  each PCR bank implemented on the TPM.
  If no H-CRTM Event Sequence context exists, this indication is discarded
  and no other action is performed.

  This function calls _TPM_Hash_Data() in Tcg TPM reference code.

  @param [in] DataSize Size of data to be extend
  @param [in] Data     Data Buffer

**/
VOID
EFIAPI
TpmLibTpmHashData (
  IN UINT32  DataSize,
  IN UINT8   *Data
  )
{
  _TPM_Hash_Data (DataSize, Data);
}

/**
  This indication from the TPM interface indicates
  the end of the H-CRTM measurement. This indication is discarded and
  no other action performed if the TPM does not contain
  an H-CRTM Event Sequence context.

  This function calls _TPM_Hash_End() in Tcg TPM reference code.

**/
VOID
EFIAPI
TpmLibTpmHashEnd (
  VOID
  )
{
  _TPM_Hash_End ();
}

/**
  This is called when TPM's NV storage isn't initailized only
  (PlatformTpmLibNVNeedsManufacture() return FALSE) otherwise,
  TPM's NV storage is cleared all.

  Before calling this, PlatformTpmLibNVEnable() must be called.

  This function calls TPM_Manufacture() in Tcg TPM reference code.

  @return   -2          NV System not available
  @return   -1          FAILURE - System is incorrectly compiled.
  @return    0          success

**/
INT32
EFIAPI
TpmLibTpmManufacture (
  VOID
  )
{
  return TPM_Manufacture (1);
}

/**
  Get current locality.

  This function calls _plat__LocalityGet() in Tcg TPM reference code.

  @return    Locality   Current locality.

**/
UINT8
EFIAPI
TpmLibGetLocality (
  VOID
  )
{
  return _plat__LocalityGet ();
}

/**
  Set current locality.


  This function calls _plat__LocalitySet() in Tcg TPM reference code.

  @param [in]   Locality

**/
VOID
EFIAPI
TpmLibSetLocality (
  IN UINT8  Locality
  )
{
  _plat__LocalitySet (Locality);
}

/**
  Constructor for TpmLib.

  @return EFI_SUCCESS           Success
  @return Others                Error

**/
EFI_STATUS
EFIAPI
TpmLibConstructor (
  VOID
  )
{
  return PlatformTpmLibInit ();
}
