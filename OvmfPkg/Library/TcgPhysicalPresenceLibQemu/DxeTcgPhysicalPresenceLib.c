/** @file

  Execute pending TPM requests from OS or BIOS and Lock TPM.

  Caution: This module requires additional review when modified.
  This driver will have external input - variable.
  This external input must be validated carefully to avoid security issue.

  ExecutePendingTpmRequest() will receive untrusted input and do validation.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/TcgService.h>
#include <Protocol/VariableLock.h>
#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuPPI.h>
#include <Library/HiiLib.h>
#include <Library/TcgPpVendorLib.h>

#define CONFIRM_BUFFER_SIZE         4096

EFI_HII_HANDLE mPpStringPackHandle;

STATIC volatile QEMU_TPM_PPI *mPpi;

/**
  Reads QEMU PPI config from fw_cfg.

  @param[out]  The Config structure to read to.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    Invalid fw_cfg entry size.
**/
STATIC
EFI_STATUS
QemuTpmReadConfig (
  OUT QEMU_FWCFG_TPM_CONFIG *Config
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;

  Status = QemuFwCfgFindFile ("etc/tpm/config", &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (FwCfgSize != sizeof (*Config)) {
    return EFI_PROTOCOL_ERROR;
  }

  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (sizeof (*Config), Config);
  return EFI_SUCCESS;
}


/**
  Initilalize the QEMU PPI memory region's function array
**/
STATIC
VOID
QemuTpmInitPPIFunc(
  EFI_PHYSICAL_PRESENCE_FLAGS     PpiFlags
  )
{
  ZeroMem ((void *)mPpi->Func, sizeof(mPpi->Func));

  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_DISABLE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ACTIVATE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_DEACTIVATE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_CLEAR] = TPM_PPI_CLEAR_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE_ACTIVATE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_DEACTIVATE_DISABLE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_OPERATOR_AUTH] = TPM_PPI_PROVISION_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE] = TPM_PPI_CLEAR_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_FALSE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_TRUE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_FALSE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_NOT_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_TRUE] = QEMU_TPM_PPI_FUNC_ALLOWED_USR_REQ;
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR] = TPM_PPI_CLEAR_MAINT_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE] = TPM_PPI_CLEAR_MAINT_FLAGS(PpiFlags);
  mPpi->Func[TCG_PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR] = TPM_PPI_CLEAR_MAINT_FLAGS(PpiFlags);
}


/**
  Initializes QEMU PPI memory region.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_PROTOCOL_ERROR    PPI address is invalid.
**/
STATIC
EFI_STATUS
QemuTpmInitPPI (
  VOID
  )
{
  EFI_STATUS                      Status;
  QEMU_FWCFG_TPM_CONFIG           Config;
  EFI_PHYSICAL_ADDRESS            PpiAddress64;
  EFI_GCD_MEMORY_SPACE_DESCRIPTOR Descriptor;
  EFI_PHYSICAL_PRESENCE_FLAGS     PpiFlags;

  if (mPpi != NULL) {
    return EFI_SUCCESS;
  }

  Status = QemuTpmReadConfig (&Config);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Config.TpmVersion != QEMU_TPM_VERSION_1_2) {
    DEBUG ((DEBUG_ERROR, "[TPMPP] Not setting up PPI. This is not a TPM 1.2.\n"));
    return EFI_PROTOCOL_ERROR;
  }

  mPpi = (QEMU_TPM_PPI *)(UINTN)Config.PpiAddress;
  if (mPpi == NULL) {
    return EFI_PROTOCOL_ERROR;
  }

  DEBUG ((DEBUG_INFO, "[TPMPP] mPpi=%p version=%d\n", mPpi, Config.TpmVersion));

  PpiAddress64 = (UINTN)mPpi;
  if ((PpiAddress64 & ~(UINT64)EFI_PAGE_MASK) !=
      ((PpiAddress64 + sizeof *mPpi - 1) & ~(UINT64)EFI_PAGE_MASK)) {
    DEBUG ((DEBUG_ERROR, "[TPMPP] mPpi crosses a page boundary\n"));
    goto InvalidPpiAddress;
  }

  Status = gDS->GetMemorySpaceDescriptor (PpiAddress64, &Descriptor);
  if (EFI_ERROR (Status) && Status != EFI_NOT_FOUND) {
    ASSERT_EFI_ERROR (Status);
    goto InvalidPpiAddress;
  }
  if (!EFI_ERROR (Status) &&
      (Descriptor.GcdMemoryType != EfiGcdMemoryTypeMemoryMappedIo &&
       Descriptor.GcdMemoryType != EfiGcdMemoryTypeNonExistent)) {
    DEBUG ((DEBUG_ERROR, "[TPMPP] mPpi has an invalid memory type\n"));
    goto InvalidPpiAddress;
  }

  PpiFlags.PPFlags = 0;
  QemuTpmInitPPIFunc(PpiFlags);

  if (mPpi->In == 0) {
    mPpi->In = 1;
    mPpi->Request = PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->LastRequest = PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->NextStep = PHYSICAL_PRESENCE_NO_ACTION;
  }

  return EFI_SUCCESS;

InvalidPpiAddress:
  mPpi = NULL;
  return EFI_PROTOCOL_ERROR;
}

/**
  Get string by string id from HII Interface.

  @param[in] Id          String ID.

  @retval    CHAR16 *    String from ID.
  @retval    NULL        If error occurs.

**/
CHAR16 *
PhysicalPresenceGetStringById (
  IN  EFI_STRING_ID   Id
  )
{
  return HiiGetString (mPpStringPackHandle, Id, NULL);
}

/**
  Get TPM physical presence permanent flags.

  @param[in]  TcgProtocol   EFI TCG Protocol instance.
  @param[out] LifetimeLock  physicalPresenceLifetimeLock permanent flag.
  @param[out] CmdEnable     physicalPresenceCMDEnable permanent flag.

  @retval EFI_SUCCESS       Flags were returns successfully.
  @retval other             Failed to locate EFI TCG Protocol.

**/
EFI_STATUS
GetTpmCapability (
  IN   EFI_TCG_PROTOCOL             *TcgProtocol,
  OUT  BOOLEAN                      *LifetimeLock,
  OUT  BOOLEAN                      *CmdEnable
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_RSP_COMMAND_HDR               *TpmRsp;
  UINT32                            *SendBufPtr;
  UINT8                             SendBuffer[sizeof (*TpmRqu) + sizeof (UINT32) * 3];
  TPM_PERMANENT_FLAGS               *TpmPermanentFlags;
  UINT8                             RecvBuffer[40];

  //
  // Fill request header
  //
  TpmRsp = (TPM_RSP_COMMAND_HDR*)RecvBuffer;
  TpmRqu = (TPM_RQU_COMMAND_HDR*)SendBuffer;

  TpmRqu->tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = SwapBytes32 (sizeof (SendBuffer));
  TpmRqu->ordinal   = SwapBytes32 (TPM_ORD_GetCapability);

  //
  // Set request parameter
  //
  SendBufPtr      = (UINT32*)(TpmRqu + 1);
  WriteUnaligned32 (SendBufPtr++, SwapBytes32 (TPM_CAP_FLAG));
  WriteUnaligned32 (SendBufPtr++, SwapBytes32 (sizeof (TPM_CAP_FLAG_PERMANENT)));
  WriteUnaligned32 (SendBufPtr, SwapBytes32 (TPM_CAP_FLAG_PERMANENT));

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          sizeof (SendBuffer),
                          (UINT8*)TpmRqu,
                          sizeof (RecvBuffer),
                          (UINT8*)&RecvBuffer
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((TpmRsp->tag != SwapBytes16 (TPM_TAG_RSP_COMMAND)) || (TpmRsp->returnCode != 0)) {
    return EFI_DEVICE_ERROR;
  }

  TpmPermanentFlags = (TPM_PERMANENT_FLAGS *)&RecvBuffer[sizeof (TPM_RSP_COMMAND_HDR) + sizeof (UINT32)];

  if (LifetimeLock != NULL) {
    *LifetimeLock = TpmPermanentFlags->physicalPresenceLifetimeLock;
  }

  if (CmdEnable != NULL) {
    *CmdEnable = TpmPermanentFlags->physicalPresenceCMDEnable;
  }

  return Status;
}

/**
  Issue TSC_PhysicalPresence command to TPM.

  @param[in] TcgProtocol          EFI TCG Protocol instance.
  @param[in] PhysicalPresence     The state to set the TPM's Physical Presence flags.

  @retval EFI_SUCCESS             TPM executed the command successfully.
  @retval EFI_SECURITY_VIOLATION  TPM returned error when executing the command.
  @retval other                   Failed to locate EFI TCG Protocol.

**/
EFI_STATUS
TpmPhysicalPresence (
  IN      EFI_TCG_PROTOCOL          *TcgProtocol,
  IN      TPM_PHYSICAL_PRESENCE     PhysicalPresence
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_PHYSICAL_PRESENCE             *TpmPp;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT8                             Buffer[sizeof (*TpmRqu) + sizeof (*TpmPp)];

  TpmRqu = (TPM_RQU_COMMAND_HDR*)Buffer;
  TpmPp = (TPM_PHYSICAL_PRESENCE*)(TpmRqu + 1);

  TpmRqu->tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  TpmRqu->paramSize = SwapBytes32 (sizeof (Buffer));
  TpmRqu->ordinal   = SwapBytes32 (TSC_ORD_PhysicalPresence);
  WriteUnaligned16 (TpmPp, (TPM_PHYSICAL_PRESENCE) SwapBytes16 (PhysicalPresence));

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          sizeof (Buffer),
                          (UINT8*)TpmRqu,
                          sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (TpmRsp.tag != SwapBytes16 (TPM_TAG_RSP_COMMAND)) {
    return EFI_DEVICE_ERROR;
  }

  if (TpmRsp.returnCode != 0) {
    //
    // If it fails, some requirements may be needed for this command.
    //
    return EFI_SECURITY_VIOLATION;
  }

  return Status;
}

/**
  Issue a TPM command for which no additional output data will be returned.

  @param[in] TcgProtocol              EFI TCG Protocol instance.
  @param[in] Ordinal                  TPM command code.
  @param[in] AdditionalParameterSize  Additional parameter size.
  @param[in] AdditionalParameters     Pointer to the Additional parameters.

  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE  Error occurred during sending command to TPM or
                                                  receiving response from TPM.
  @retval Others                                  Return code from the TPM device after command execution.

**/
UINT32
TpmCommandNoReturnData (
  IN      EFI_TCG_PROTOCOL          *TcgProtocol,
  IN      TPM_COMMAND_CODE          Ordinal,
  IN      UINTN                     AdditionalParameterSize,
  IN      VOID                      *AdditionalParameters
  )
{
  EFI_STATUS                        Status;
  TPM_RQU_COMMAND_HDR               *TpmRqu;
  TPM_RSP_COMMAND_HDR               TpmRsp;
  UINT32                            Size;

  TpmRqu = (TPM_RQU_COMMAND_HDR*) AllocatePool (sizeof (*TpmRqu) + AdditionalParameterSize);
  if (TpmRqu == NULL) {
    return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
  }

  TpmRqu->tag       = SwapBytes16 (TPM_TAG_RQU_COMMAND);
  Size              = (UINT32)(sizeof (*TpmRqu) + AdditionalParameterSize);
  TpmRqu->paramSize = SwapBytes32 (Size);
  TpmRqu->ordinal   = SwapBytes32 (Ordinal);
  CopyMem (TpmRqu + 1, AdditionalParameters, AdditionalParameterSize);

  Status = TcgProtocol->PassThroughToTpm (
                          TcgProtocol,
                          Size,
                          (UINT8*)TpmRqu,
                          (UINT32)sizeof (TpmRsp),
                          (UINT8*)&TpmRsp
                          );
  FreePool (TpmRqu);
  if (EFI_ERROR (Status) || (TpmRsp.tag != SwapBytes16 (TPM_TAG_RSP_COMMAND))) {
    return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
  }
  return SwapBytes32 (TpmRsp.returnCode);
}

/**
  Execute physical presence operation requested by the OS.

  @param[in]      TcgProtocol         EFI TCG Protocol instance.
  @param[in]      CommandCode         Physical presence operation value.
  @param[in, out] PpiFlags            The physical presence interface flags.

  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE  Unknown physical presence operation.
  @retval TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE  Error occurred during sending command to TPM or
                                                  receiving response from TPM.
  @retval Others                                  Return code from the TPM device after command execution.

**/
UINT32
ExecutePhysicalPresence (
  IN      EFI_TCG_PROTOCOL            *TcgProtocol,
  IN      UINT32                      CommandCode,
  IN OUT  EFI_PHYSICAL_PRESENCE_FLAGS *PpiFlags
  )
{
  BOOLEAN                           BoolVal;
  UINT32                            TpmResponse;
  UINT32                            InData[5];

  switch (CommandCode) {
    case PHYSICAL_PRESENCE_ENABLE:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalEnable,
               0,
               NULL
               );

    case PHYSICAL_PRESENCE_DISABLE:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalDisable,
               0,
               NULL
               );

    case PHYSICAL_PRESENCE_ACTIVATE:
      BoolVal = FALSE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalSetDeactivated,
               sizeof (BoolVal),
               &BoolVal
               );

    case PHYSICAL_PRESENCE_DEACTIVATE:
      BoolVal = TRUE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_PhysicalSetDeactivated,
               sizeof (BoolVal),
               &BoolVal
               );

    case PHYSICAL_PRESENCE_CLEAR:
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_ForceClear,
               0,
               NULL
               );

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ENABLE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ACTIVATE, PpiFlags);
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_DEACTIVATE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_DISABLE, PpiFlags);
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE:
      BoolVal = TRUE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetOwnerInstall,
               sizeof (BoolVal),
               &BoolVal
               );

    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE:
      BoolVal = FALSE;
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetOwnerInstall,
               sizeof (BoolVal),
               &BoolVal
               );

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE:
      //
      // PHYSICAL_PRESENCE_ENABLE_ACTIVATE + PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE
      // PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE will be executed after reboot
      //
      if ((PpiFlags->PPFlags & TCG_VENDOR_LIB_FLAG_RESET_TRACK) == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ENABLE_ACTIVATE, PpiFlags);
        PpiFlags->PPFlags |= TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      } else {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE, PpiFlags);
        PpiFlags->PPFlags &= ~TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_DEACTIVATE_DISABLE, PpiFlags);
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      InData[0] = SwapBytes32 (TPM_SET_STCLEAR_DATA);            // CapabilityArea
      InData[1] = SwapBytes32 (sizeof(UINT32));                  // SubCapSize
      InData[2] = SwapBytes32 (TPM_SD_DEFERREDPHYSICALPRESENCE); // SubCap
      InData[3] = SwapBytes32 (sizeof(UINT32));                  // SetValueSize
      InData[4] = SwapBytes32 (1);                               // UnownedFieldUpgrade; bit0
      return TpmCommandNoReturnData (
               TcgProtocol,
               TPM_ORD_SetCapability,
               sizeof (UINT32) * 5,
               InData
               );

    case PHYSICAL_PRESENCE_SET_OPERATOR_AUTH:
      //
      // TPM_SetOperatorAuth
      // This command requires UI to prompt user for Auth data
      // Here it is NOT implemented
      //
      return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;

    case PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE:
      TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_CLEAR, PpiFlags);
      if (TpmResponse == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ENABLE_ACTIVATE, PpiFlags);
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_FALSE:
      PpiFlags->PPFlags &= ~TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION;
      return 0;

    case PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_TRUE:
      PpiFlags->PPFlags |= TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION;
      return 0;

    case PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE:
      PpiFlags->PPFlags &= ~TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR;
      return 0;

    case PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
      PpiFlags->PPFlags |= TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR;
      return 0;

    case PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_FALSE:
      PpiFlags->PPFlags &= ~TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_MAINTENANCE;
      return 0;

    case PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_TRUE:
      PpiFlags->PPFlags |= TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_MAINTENANCE;
      return 0;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR:
      //
      // PHYSICAL_PRESENCE_ENABLE_ACTIVATE + PHYSICAL_PRESENCE_CLEAR
      // PHYSICAL_PRESENCE_CLEAR will be executed after reboot.
      //
      if ((PpiFlags->PPFlags & TCG_VENDOR_LIB_FLAG_RESET_TRACK) == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ENABLE_ACTIVATE, PpiFlags);
        PpiFlags->PPFlags |= TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      } else {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_CLEAR, PpiFlags);
        PpiFlags->PPFlags &= ~TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      }
      return TpmResponse;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      //
      // PHYSICAL_PRESENCE_ENABLE_ACTIVATE + PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE
      // PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE will be executed after reboot.
      //
      if ((PpiFlags->PPFlags & TCG_VENDOR_LIB_FLAG_RESET_TRACK) == 0) {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_ENABLE_ACTIVATE, PpiFlags);
        PpiFlags->PPFlags |= TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      } else {
        TpmResponse = ExecutePhysicalPresence (TcgProtocol, PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE, PpiFlags);
        PpiFlags->PPFlags &= ~TCG_VENDOR_LIB_FLAG_RESET_TRACK;
      }
      return TpmResponse;

    default:
      ;
  }
  return TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
}


/**
  Read the specified key for user confirmation.

  @param[in]  CautionKey  If true,  F12 is used as confirm key;
                          If false, F10 is used as confirm key.

  @retval     TRUE        User confirmed the changes by input.
  @retval     FALSE       User discarded the changes or device error.

**/
BOOLEAN
ReadUserKey (
  IN     BOOLEAN                    CautionKey
  )
{
  EFI_STATUS                        Status;
  EFI_INPUT_KEY                     Key;
  UINT16                            InputKey;
  UINTN                             Index;

  InputKey = 0;
  do {
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    if (Status == EFI_NOT_READY) {
      gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &Index);
      continue;
    }

    if (Status == EFI_DEVICE_ERROR) {
      return FALSE;
    }

    if (Key.ScanCode == SCAN_ESC) {
      InputKey = Key.ScanCode;
    }
    if ((Key.ScanCode == SCAN_F10) && !CautionKey) {
      InputKey = Key.ScanCode;
    }
    if ((Key.ScanCode == SCAN_F12) && CautionKey) {
      InputKey = Key.ScanCode;
    }
  } while (InputKey == 0);

  if (InputKey != SCAN_ESC) {
    return TRUE;
  }

  return FALSE;
}

/**
  The constructor function register UNI strings into imageHandle.

  It will ASSERT() if that operation fails and it will always return EFI_SUCCESS.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully added string package.
  @retval Other value   The constructor can't add string package.

**/
EFI_STATUS
EFIAPI
TcgPhysicalPresenceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  mPpStringPackHandle = HiiAddPackages (&gEfiPhysicalPresenceGuid, ImageHandle, TcgPhysicalPresenceLibQemuStrings, NULL);
  ASSERT (mPpStringPackHandle != NULL);

  return EFI_SUCCESS;
}

/**
  Display the confirm text and get user confirmation.

  @param[in] TpmPpCommand  The requested TPM physical presence command.

  @retval    TRUE          The user has confirmed the changes.
  @retval    FALSE         The user doesn't confirm the changes.
**/
BOOLEAN
UserConfirm (
  IN      UINT32                    TpmPpCommand
  )
{
  CHAR16                            *ConfirmText;
  CHAR16                            *TmpStr1;
  CHAR16                            *TmpStr2;
  UINTN                             BufSize;
  BOOLEAN                           CautionKey;
  UINT16                            Index;
  CHAR16                            DstStr[81];

  TmpStr2     = NULL;
  CautionKey  = FALSE;
  BufSize     = CONFIRM_BUFFER_SIZE;
  ConfirmText = AllocateZeroPool (BufSize);
  ASSERT (ConfirmText != NULL);

  switch (TpmPpCommand) {
    case PHYSICAL_PRESENCE_ENABLE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ENABLE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_DISABLE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_DISABLE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_ACTIVATE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACTIVATE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_DEACTIVATE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_DEACTIVATE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_CLEAR:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ENABLE_ACTIVATE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_ON));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_DEACTIVATE_DISABLE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_OFF));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ALLOW_TAKE_OWNERSHIP));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_DISALLOW_TAKE_OWNERSHIP));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_TURN_ON));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_ON));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_TURN_OFF));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_OFF));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_UNOWNED_FIELD_UPGRADE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_UPGRADE_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_MAINTAIN));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_OPERATOR_AUTH:
      //
      // TPM_SetOperatorAuth
      // This command requires UI to prompt user for Auth data
      // Here it is NOT implemented
      //
      break;

    case PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR_TURN_ON));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_ON));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR_CONT));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_TRUE:
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_PROVISION));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_PPI_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ACCEPT_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CLEAR));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_PPI_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_TRUE:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_MAINTAIN));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_PPI_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_MAINTAIN));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NO_PPI_INFO));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ENABLE_ACTIVATE_CLEAR));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), L" \n\n", (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      CautionKey = TRUE;
      TmpStr2 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE));

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_HEAD_STR));
      UnicodeSPrint (ConfirmText, BufSize, TmpStr1, TmpStr2);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_NOTE_ON));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_WARNING_CLEAR_CONT));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);

      TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_CAUTION_KEY));
      StrnCatS (ConfirmText, BufSize / sizeof (CHAR16), TmpStr1, (BufSize / sizeof (CHAR16)) - StrLen (ConfirmText) - 1);
      FreePool (TmpStr1);
      break;

    default:
      ;
  }

  if (TmpStr2 == NULL) {
    FreePool (ConfirmText);
    return FALSE;
  }

  // Console for user interaction
  // We need to connect all trusted consoles for TCG PP. Here we treat all consoles in OVMF to be trusted consoles.
  EfiBootManagerConnectAllDefaultConsoles ();

  TmpStr1 = PhysicalPresenceGetStringById (STRING_TOKEN (TPM_REJECT_KEY));
  BufSize -= StrSize (ConfirmText);
  UnicodeSPrint (ConfirmText + StrLen (ConfirmText), BufSize, TmpStr1, TmpStr2);

  DstStr[80] = L'\0';
  for (Index = 0; Index < StrLen (ConfirmText); Index += 80) {
    StrnCpyS(DstStr, sizeof (DstStr) / sizeof (CHAR16), ConfirmText + Index, sizeof (DstStr) / sizeof (CHAR16) - 1);
    Print (DstStr);
  }

  FreePool (TmpStr1);
  FreePool (TmpStr2);
  FreePool (ConfirmText);

  if (ReadUserKey (CautionKey)) {
    return TRUE;
  }

  return FALSE;
}

/**
  Check if there is a valid physical presence command request. Also updates parameter value
  to whether the requested physical presence command already confirmed by user

   @param[in]  TcgPpData           EFI TCG Physical Presence request data.
   @param[in]  Flags               The physical presence interface flags.
   @param[out] RequestConfirmed    If the physical presence operation command required user confirm from UI.
                                   True, it indicates the command doesn't require user confirm, or already confirmed
                                   in last boot cycle by user.
                                   False, it indicates the command need user confirm from UI.

   @retval  TRUE        Physical Presence operation command is valid.
   @retval  FALSE       Physical Presence operation command is invalid.

**/
BOOLEAN
HaveValidTpmRequest  (
  IN      EFI_PHYSICAL_PRESENCE       *TcgPpData,
  IN      EFI_PHYSICAL_PRESENCE_FLAGS Flags,
  OUT     BOOLEAN                     *RequestConfirmed
  )
{
  BOOLEAN  IsRequestValid;

  *RequestConfirmed = FALSE;

  switch (TcgPpData->PPRequest) {
    case PHYSICAL_PRESENCE_NO_ACTION:
      *RequestConfirmed = TRUE;
      return TRUE;
    case PHYSICAL_PRESENCE_ENABLE:
    case PHYSICAL_PRESENCE_DISABLE:
    case PHYSICAL_PRESENCE_ACTIVATE:
    case PHYSICAL_PRESENCE_DEACTIVATE:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE:
    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE:
    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_TRUE:
    case PHYSICAL_PRESENCE_SET_OWNER_INSTALL_FALSE:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE:
    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE:
    case PHYSICAL_PRESENCE_SET_OPERATOR_AUTH:
      if ((Flags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION) != 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case PHYSICAL_PRESENCE_CLEAR:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR:
      if ((Flags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR) != 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case PHYSICAL_PRESENCE_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
      if ((Flags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_MAINTENANCE) != 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      if ((Flags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_CLEAR) != 0 && (Flags.PPFlags & TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION) != 0) {
        *RequestConfirmed = TRUE;
      }
      break;

    case PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_FALSE:
    case PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_FALSE:
    case PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_FALSE:
      *RequestConfirmed = TRUE;
      break;

    case PHYSICAL_PRESENCE_SET_NO_PPI_PROVISION_TRUE:
    case PHYSICAL_PRESENCE_SET_NO_PPI_CLEAR_TRUE:
    case PHYSICAL_PRESENCE_SET_NO_PPI_MAINTENANCE_TRUE:
      break;

    default:
      if (TcgPpData->PPRequest >= TCG_PHYSICAL_PRESENCE_VENDOR_SPECIFIC_OPERATION) {
        IsRequestValid = FALSE; // vendor-specifc commands are not supported
        if (!IsRequestValid) {
          return FALSE;
        } else {
          break;
        }
      } else {
        //
        // Wrong Physical Presence command
        //
        return FALSE;
      }
  }

  if ((Flags.PPFlags & TCG_VENDOR_LIB_FLAG_RESET_TRACK) != 0) {
    //
    // It had been confirmed in last boot, it doesn't need confirm again.
    //
    *RequestConfirmed = TRUE;
  }

  //
  // Physical Presence command is correct
  //
  return TRUE;
}


/**
  Check and execute the requested physical presence command.

  Caution: This function may receive untrusted input.
  TcgPpData variable is external input, so this function will validate
  its data structure to be valid value.

  @param[in] TcgProtocol          EFI TCG Protocol instance.
  @param[in] TcgPpData            Point to the physical presence NV variable.
  @param[in] Flags                The physical presence interface flags.

**/
STATIC
VOID
ExecutePendingTpmRequest (
  IN      EFI_TCG_PROTOCOL            *TcgProtocol,
  IN      EFI_PHYSICAL_PRESENCE_FLAGS Flags
  )
{
  EFI_STATUS                        Status;
  BOOLEAN                           RequestConfirmed;
  EFI_PHYSICAL_PRESENCE             TcgPpData;
  EFI_PHYSICAL_PRESENCE_FLAGS       NewFlags;

  DEBUG ((DEBUG_INFO, "[TPMPP] Flags=0x%x, PPRequest=0x%x\n", Flags.PPFlags, mPpi->Request));

  TcgPpData.PPRequest = (UINT8)mPpi->Request;

  if (!HaveValidTpmRequest(&TcgPpData, Flags, &RequestConfirmed)) {
    //
    // Invalid operation request.
    //
    mPpi->Response = TCG_PP_OPERATION_RESPONSE_BIOS_FAILURE;
    mPpi->LastRequest = mPpi->Request;
    mPpi->Request = TCG_PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->RequestParameter = 0;
    return;
  }

  if (!RequestConfirmed) {
    //
    // Print confirm text and wait for approval.
    //
    RequestConfirmed = UserConfirm (mPpi->Request);
  }

  //
  // Execute requested physical presence command
  //
  mPpi->Response = TCG_PP_OPERATION_RESPONSE_USER_ABORT;
  NewFlags = Flags;
  if (RequestConfirmed) {
    mPpi->Response = ExecutePhysicalPresence (TcgProtocol, mPpi->Request, &NewFlags);
  }

  //
  // Save the flags if it is updated.
  //
  if (CompareMem (&Flags, &NewFlags, sizeof(EFI_PHYSICAL_PRESENCE_FLAGS)) != 0) {
    Status   = gRT->SetVariable (
                      PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiPhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_PHYSICAL_PRESENCE_FLAGS),
                      &NewFlags
                      );
    if (EFI_ERROR (Status)) {
      return;
    }

    //
    // Update the flags for the commands following PPFlags changes
    //
    QemuTpmInitPPIFunc(NewFlags);
    DEBUG ((DEBUG_INFO, "[TPMPP] New PPFlags = 0x%x\n", NewFlags.PPFlags));
  }

  //
  // Clear request
  //
  if ((NewFlags.PPFlags & TCG_VENDOR_LIB_FLAG_RESET_TRACK) == 0) {
    mPpi->LastRequest = mPpi->Request;
    mPpi->Request = PHYSICAL_PRESENCE_NO_ACTION;
    mPpi->RequestParameter = 0;
  }

  if (mPpi->Response == TCG_PP_OPERATION_RESPONSE_USER_ABORT) {
    return;
  }

  //
  // Reset system to make new TPM settings in effect
  //
  switch (mPpi->LastRequest) {
    case PHYSICAL_PRESENCE_ACTIVATE:
    case PHYSICAL_PRESENCE_DEACTIVATE:
    case PHYSICAL_PRESENCE_CLEAR:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE:
    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_OWNER_TRUE:
    case PHYSICAL_PRESENCE_DEACTIVATE_DISABLE_OWNER_FALSE:
    case PHYSICAL_PRESENCE_DEFERRED_PP_UNOWNERED_FIELD_UPGRADE:
    case PHYSICAL_PRESENCE_CLEAR_ENABLE_ACTIVATE:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR:
    case PHYSICAL_PRESENCE_ENABLE_ACTIVATE_CLEAR_ENABLE_ACTIVATE:
      break;
    default:
    if (mPpi->Request != TCG_PHYSICAL_PRESENCE_NO_ACTION) {
      break;
    }
    return;
  }

  Print (L"Rebooting system to make TPM settings in effect\n");
  gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
  ASSERT (FALSE);
}

/**
  Check and execute the pending TPM request and Lock TPM.

  The TPM request may come from OS or BIOS. This API will display request information and wait
  for user confirmation if TPM request exists. The TPM request will be sent to TPM device after
  the TPM request is confirmed, and one or more reset may be required to make TPM request to
  take effect. At last, it will lock TPM to prevent TPM state change by malware.

  This API should be invoked after console in and console out are all ready as they are required
  to display request information and get user input to confirm the request. This API should also
  be invoked as early as possible as TPM is locked in this function.

**/
VOID
EFIAPI
TcgPhysicalPresenceLibProcessRequest (
  VOID
  )
{
  EFI_STATUS                        Status;
  BOOLEAN                           LifetimeLock;
  BOOLEAN                           CmdEnable;
  UINTN                             DataSize;
  EFI_TCG_PROTOCOL                  *TcgProtocol;
//  EDKII_VARIABLE_LOCK_PROTOCOL      *VariableLockProtocol;
  EFI_PHYSICAL_PRESENCE_FLAGS       PpiFlags;

  TcgPhysicalPresenceLibConstructor(gImageHandle, NULL);

  Status = QemuTpmInitPPI ();
  if (EFI_ERROR (Status)) {
    return ;
  }

  DEBUG ((DEBUG_INFO, "[TPMPP] Detected a TPM 1.2\n"));

  //
  // Check S4 resume
  //
  if (GetBootModeHob () == BOOT_ON_S4_RESUME) {
    DEBUG ((DEBUG_INFO, "S4 Resume, Skip TPM PP process!\n"));
    return ;
  }

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **)&TcgProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }

  //
  // Initialize physical presence flags.
  //
  DataSize = sizeof (EFI_PHYSICAL_PRESENCE_FLAGS);
  Status = gRT->GetVariable (
                  PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                  &gEfiPhysicalPresenceGuid,
                  NULL,
                  &DataSize,
                  &PpiFlags
                  );
  if (EFI_ERROR (Status)) {
    PpiFlags.PPFlags = TCG_BIOS_TPM_MANAGEMENT_FLAG_NO_PPI_PROVISION;
    Status   = gRT->SetVariable (
                      PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                      &gEfiPhysicalPresenceGuid,
                      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                      sizeof (EFI_PHYSICAL_PRESENCE_FLAGS),
                      &PpiFlags
                      );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[TPM] Set physical presence flag failed, Status = %r\n", Status));
      return ;
    }
  }
  DEBUG ((DEBUG_INFO, "[TPM] PpiFlags = %x\n", PpiFlags.PPFlags));

  //
  // This flags variable controls whether physical presence is required for TPM command.
  // It should be protected from malicious software. We set it as read-only variable here.
  //
#if 0
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **)&VariableLockProtocol);
  if (!EFI_ERROR (Status)) {
    Status = VariableLockProtocol->RequestToLock (
                                     VariableLockProtocol,
                                     PHYSICAL_PRESENCE_FLAGS_VARIABLE,
                                     &gEfiPhysicalPresenceGuid
                                     );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "[TPM] Error when lock variable %s, Status = %r\n", PHYSICAL_PRESENCE_FLAGS_VARIABLE, Status));
      ASSERT_EFI_ERROR (Status);
    }
  }
#endif

  QemuTpmInitPPIFunc(PpiFlags);

  DEBUG ((DEBUG_INFO, "[TPMPP] Flags=0x%x, PPRequest=0x%x\n", PpiFlags.PPFlags, mPpi->Request));

  if (mPpi->Request == PHYSICAL_PRESENCE_NO_ACTION) {
    //
    // No operation request
    //
    return;
  }

  Status = GetTpmCapability (TcgProtocol, &LifetimeLock, &CmdEnable);
  if (EFI_ERROR (Status)) {
    return ;
  }

  if (!CmdEnable) {
    if (LifetimeLock) {
      //
      // physicalPresenceCMDEnable is locked, can't execute physical presence command.
      //
      return ;
    }
    Status = TpmPhysicalPresence (TcgProtocol, TPM_PHYSICAL_PRESENCE_CMD_ENABLE);
    if (EFI_ERROR (Status)) {
      return ;
    }
  }

  //
  // Set operator physical presence flags
  //
  Status = TpmPhysicalPresence (TcgProtocol, TPM_PHYSICAL_PRESENCE_PRESENT);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Execute pending TPM request.
  //
  ExecutePendingTpmRequest (TcgProtocol, PpiFlags);
  DEBUG ((DEBUG_INFO, "[TPMPP] PPResponse = %x\n", mPpi->Response));

  //
  // Lock physical presence.
  //
  TpmPhysicalPresence (TcgProtocol, TPM_PHYSICAL_PRESENCE_NOTPRESENT | TPM_PHYSICAL_PRESENCE_LOCK);
}

/**
  The handler for TPM physical presence function:
  Submit TPM Operation Request to Pre-OS Environment and
  Submit TPM Operation Request to Pre-OS Environment 2.

  Caution: This function may receive untrusted input.

  @param[in]      OperationRequest TPM physical presence operation request.

  @return Return Code for Submit TPM Operation Request to Pre-OS Environment and
          Submit TPM Operation Request to Pre-OS Environment 2.
**/
UINT32
EFIAPI
TcgPhysicalPresenceLibSubmitRequestToPreOSFunction (
  IN UINT32                 OperationRequest
  )
{
  EFI_STATUS Status;

  DEBUG ((DEBUG_INFO, "[TPMPP] SubmitRequestToPreOSFunction, Request = %x\n", OperationRequest));

  Status = QemuTpmInitPPI ();
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "[TPMPP] no PPI\n"));
    return TCG_PP_SUBMIT_REQUEST_TO_PREOS_GENERAL_FAILURE;
  }

  mPpi->Request = OperationRequest;

  return TCG_PP_SUBMIT_REQUEST_TO_PREOS_SUCCESS;
}
