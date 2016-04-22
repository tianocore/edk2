/** @file
  Implementation of Opal password support library.

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "OpalPasswordSupportNotify.h"

#define OPAL_PASSWORD_MAX_LENGTH         32

LIST_ENTRY           mDeviceList = INITIALIZE_LIST_HEAD_VARIABLE (mDeviceList);
BOOLEAN              gInSmm = FALSE;
EFI_GUID             gOpalPasswordNotifyProtocolGuid = OPAL_PASSWORD_NOTIFY_PROTOCOL_GUID;

/**

  The function performs determines the available actions for the OPAL_DISK provided.

  @param[in]   SupportedAttributes   The support attribute for the device.
  @param[in]   LockingFeature        The locking status for the device.
  @param[in]   OwnerShip             The ownership for the device.
  @param[out]  AvalDiskActions       Pointer to fill-out with appropriate disk actions.

**/
TCG_RESULT
EFIAPI
OpalSupportGetAvailableActions(
  IN  OPAL_DISK_SUPPORT_ATTRIBUTE      *SupportedAttributes,
  IN  TCG_LOCKING_FEATURE_DESCRIPTOR   *LockingFeature,
  IN  UINT16                           OwnerShip,
  OUT OPAL_DISK_ACTIONS                *AvalDiskActions
  )
{
  BOOLEAN ExistingPassword;

  NULL_CHECK(AvalDiskActions);

  AvalDiskActions->AdminPass = 1;
  AvalDiskActions->UserPass = 0;
  AvalDiskActions->DisableUser = 0;
  AvalDiskActions->Unlock = 0;

  //
  // Revert is performed on locking sp, so only allow if locking sp is enabled
  //
  if (LockingFeature->LockingEnabled) {
    AvalDiskActions->Revert = 1;
  }

  //
  // Psid revert is available for any device with media encryption support
  // Revert is allowed for any device with media encryption support, however it requires
  //
  if (SupportedAttributes->MediaEncryption) {

    //
    // Only allow psid revert if media encryption is enabled.
    // Otherwise, someone who steals a disk can psid revert the disk and the user Data is still
    // intact and accessible
    //
    AvalDiskActions->PsidRevert = 1;
    AvalDiskActions->RevertKeepDataForced = 0;

    //
    // Secure erase is performed by generating a new encryption key
    // this is only available is encryption is supported
    //
    AvalDiskActions->SecureErase = 1;
  } else {
    AvalDiskActions->PsidRevert = 0;
    AvalDiskActions->SecureErase = 0;

    //
    // If no media encryption is supported, then a revert (using password) will not
    // erase the Data (since you can't generate a new encryption key)
    //
    AvalDiskActions->RevertKeepDataForced = 1;
  }

  if (LockingFeature->Locked) {
    AvalDiskActions->Unlock = 1;
  } else {
    AvalDiskActions->Unlock = 0;
  }

  //
  // Only allow user to set password if an admin password exists
  //
  ExistingPassword = OpalUtilAdminPasswordExists(OwnerShip, LockingFeature);
  AvalDiskActions->UserPass = ExistingPassword;

  //
  // This will still show up even if there isn't a user, which is fine
  //
  AvalDiskActions->DisableUser = ExistingPassword;

  return TcgResultSuccess;
}

/**
  Creates a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts device using Admin SP Revert method.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Psid               PSID of device to revert.
  @param[in]      PsidLength         Length of PSID in bytes.
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportPsidRevert(
  IN OPAL_SESSION              *Session,
  IN VOID                      *Psid,
  IN UINT32                    PsidLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Psid);

  Ret = OpalUtilPsidRevert (Session, Psid, PsidLength);
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, 0, NULL);
  }

  return Ret;
}

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_SID_AUTHORITY,
  sets OPAL_UID_ADMIN_SP_C_PIN_SID with the new password,
  and sets OPAL_LOCKING_SP_C_PIN_ADMIN1 with the new password.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      OldPassword        Current admin password
  @param[in]      OldPasswordLength  Length of current admin password in bytes
  @param[in]      NewPassword        New admin password to set
  @param[in]      NewPasswordLength  Length of new password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.
  @param[in]      SetAdmin           Whether set admin password or user password.
                                     TRUE for admin, FALSE for user.

**/
TCG_RESULT
EFIAPI
OpalSupportSetPassword(
  IN OPAL_SESSION              *Session,
  IN VOID                      *OldPassword,
  IN UINT32                    OldPasswordLength,
  IN VOID                      *NewPassword,
  IN UINT32                    NewPasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath,
  IN BOOLEAN                   SetAdmin
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(OldPassword);
  NULL_CHECK(NewPassword);

  if (SetAdmin) {
    Ret = OpalUtilSetAdminPassword(Session, OldPassword, OldPasswordLength, NewPassword, NewPasswordLength);
  } else {
    Ret = OpalUtilSetUserPassword(Session, OldPassword, OldPasswordLength, NewPassword, NewPasswordLength);
  }
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, NewPasswordLength, NewPassword);
  }

  return Ret;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_ADMIN1_AUTHORITY and disables the User1 authority.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[out]     PasswordFailed     Indicates if password failed (start session didn't work)
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportDisableUser(
  IN  OPAL_SESSION              *Session,
  IN  VOID                      *Password,
  IN  UINT32                    PasswordLength,
  OUT BOOLEAN                   *PasswordFailed,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Password);
  NULL_CHECK(PasswordFailed);

  Ret = OpalUtilDisableUser(Session, Password, PasswordLength, PasswordFailed);
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, PasswordLength, Password);
  }

  return Ret;
}

/**
  Enable Opal Feature for the input device.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[in]      Password           Admin password
  @param[in]      PassLength         Length of password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportEnableOpalFeature (
  IN OPAL_SESSION              *Session,
  IN VOID                      *Msid,
  IN UINT32                    MsidLength,
  IN VOID                      *Password,
  IN UINT32                    PassLength,
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Msid);
  NULL_CHECK(Password);

  Ret = OpalUtilSetAdminPasswordAsSid(
                          Session,
                          Msid,
                          MsidLength,
                          Password,
                          PassLength
                          );
  if (Ret == TcgResultSuccess) {
    //
    // Enable global locking range
    //
    Ret = OpalUtilSetOpalLockingRange(
                              Session,
                              Password,
                              PassLength,
                              OPAL_LOCKING_SP_LOCKING_GLOBALRANGE,
                              0,
                              0,
                              TRUE,
                              TRUE,
                              FALSE,
                              FALSE
                              );
  }

  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, PassLength, Password);
  }

  return Ret;
}

/**
  Opens a session with OPAL_UID_ADMIN_SP as OPAL_ADMIN_SP_PSID_AUTHORITY, then reverts the device using the RevertSP method.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      KeepUserData       TRUE to keep existing Data on the disk, or FALSE to erase it
  @param[in]      Password           Admin password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in]      Msid               Msid
  @param[in]      MsidLength         Msid Length
  @param[out]     PasswordFailed     indicates if password failed (start session didn't work)
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportRevert(
  IN  OPAL_SESSION              *Session,
  IN  BOOLEAN                   KeepUserData,
  IN  VOID                      *Password,
  IN  UINT32                    PasswordLength,
  IN  VOID                      *Msid,
  IN  UINT32                    MsidLength,
  OUT BOOLEAN                   *PasswordFailed,
  IN  EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Password);
  NULL_CHECK(Msid);
  NULL_CHECK(PasswordFailed);

  Ret = OpalUtilRevert(Session, KeepUserData, Password, PasswordLength, PasswordFailed, Msid, MsidLength);
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, 0, NULL);
  }

  return Ret;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and updates the global locking range ReadLocked and WriteLocked columns to FALSE.

  @param[in]      Session            The opal session for the opal device.
  @param[in]      Password           Admin or user password
  @param[in]      PasswordLength     Length of password in bytes
  @param[in]      DevicePath         The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportUnlock(
  IN OPAL_SESSION               *Session,
  IN VOID                       *Password,
  IN UINT32                     PasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  TCG_RESULT   Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Password);

  Ret = OpalUtilUpdateGlobalLockingRange(Session, Password, PasswordLength, FALSE, FALSE);
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, PasswordLength, Password);
  }

  return Ret;
}

/**
  Starts a session with OPAL_UID_LOCKING_SP as OPAL_LOCKING_SP_USER1_AUTHORITY or OPAL_LOCKING_SP_ADMIN1_AUTHORITY
  and updates the global locking range ReadLocked and WriteLocked columns to TRUE.

  @param[in]      Session             The opal session for the opal device.
  @param[in]      Password            Admin or user password
  @param[in]      PasswordLength      Length of password in bytes
  @param[in]      DevicePath          The device path for the opal devcie.

**/
TCG_RESULT
EFIAPI
OpalSupportLock(
  IN OPAL_SESSION               *Session,
  IN VOID                       *Password,
  IN UINT32                     PasswordLength,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
{
  TCG_RESULT  Ret;

  NULL_CHECK(Session);
  NULL_CHECK(Password);

  Ret = OpalUtilUpdateGlobalLockingRange(Session, Password, PasswordLength, TRUE, TRUE);
  if (Ret == TcgResultSuccess && !gInSmm) {
    OpalSupportSendPasword (DevicePath, PasswordLength, Password);
  }

  return Ret;
}

/**
  Initialize the communicate Buffer using DataSize and Function.

  @param[out]      DataPtr          Points to the Data in the communicate Buffer.
  @param[in]       DataSize         The Data Size to send to SMM.
  @param[in]       Function         The function number to initialize the communicate Header.

  @retval EFI_INVALID_PARAMETER     The Data Size is too big.
  @retval EFI_SUCCESS               Find the specified variable.

**/
VOID*
OpalInitCommunicateBuffer (
  OUT     VOID                              **DataPtr OPTIONAL,
  IN      UINTN                             DataSize,
  IN      UINTN                             Function
  )
{
  EFI_SMM_COMMUNICATE_HEADER                *SmmCommunicateHeader;
  OPAL_SMM_COMMUNICATE_HEADER               *SmmFunctionHeader;
  VOID                                      *Buffer;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE   *SmmCommRegionTable;
  EFI_MEMORY_DESCRIPTOR                     *SmmCommMemRegion;
  UINTN                                     Index;
  UINTN                                     Size;
  EFI_STATUS                                Status;

  Buffer = NULL;
  Status = EfiGetSystemConfigurationTable (
             &gEdkiiPiSmmCommunicationRegionTableGuid,
             (VOID **) &SmmCommRegionTable
             );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  ASSERT (SmmCommRegionTable != NULL);
  SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *) (SmmCommRegionTable + 1);
  Size = 0;
  for (Index = 0; Index < SmmCommRegionTable->NumberOfEntries; Index++) {
    if (SmmCommMemRegion->Type == EfiConventionalMemory) {
      Size = EFI_PAGES_TO_SIZE ((UINTN) SmmCommMemRegion->NumberOfPages);
      if (Size >= (DataSize + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + OFFSET_OF (OPAL_SMM_COMMUNICATE_HEADER, Data))) {
        break;
      }
    }
    SmmCommMemRegion = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) SmmCommMemRegion + SmmCommRegionTable->DescriptorSize);
  }
  ASSERT (Index < SmmCommRegionTable->NumberOfEntries);

  Buffer = (VOID*)(UINTN)SmmCommMemRegion->PhysicalStart;
  ASSERT (Buffer != NULL);

  SmmCommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *) Buffer;
  CopyGuid (&SmmCommunicateHeader->HeaderGuid, &gOpalPasswordNotifyProtocolGuid);
  SmmCommunicateHeader->MessageLength = DataSize + OFFSET_OF (OPAL_SMM_COMMUNICATE_HEADER, Data);

  SmmFunctionHeader = (OPAL_SMM_COMMUNICATE_HEADER *) SmmCommunicateHeader->Data;
  SmmFunctionHeader->Function = Function;
  if (DataPtr != NULL) {
    *DataPtr = SmmFunctionHeader->Data;
  }

  return Buffer;
}

/**
  Send the Data in communicate Buffer to SMM.

  @param[in]   Buffer                 Points to the Data in the communicate Buffer.
  @param[in]   DataSize               This Size of the function Header and the Data.

  @retval      EFI_SUCCESS            Success is returned from the functin in SMM.
  @retval      Others                 Failure is returned from the function in SMM.

**/
EFI_STATUS
OpalSendCommunicateBuffer (
  IN      VOID                              *Buffer,
  IN      UINTN                             DataSize
  )
{
  EFI_STATUS                                Status;
  UINTN                                     CommSize;
  EFI_SMM_COMMUNICATE_HEADER                *SmmCommunicateHeader;
  OPAL_SMM_COMMUNICATE_HEADER               *SmmFunctionHeader;
  EFI_SMM_COMMUNICATION_PROTOCOL            *SmmCommunication;

  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &SmmCommunication);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CommSize = DataSize + OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data) + OFFSET_OF (OPAL_SMM_COMMUNICATE_HEADER, Data);
  Status = SmmCommunication->Communicate (SmmCommunication, Buffer, &CommSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  SmmCommunicateHeader = (EFI_SMM_COMMUNICATE_HEADER *) Buffer;
  SmmFunctionHeader    = (OPAL_SMM_COMMUNICATE_HEADER *)SmmCommunicateHeader->Data;

  return  SmmFunctionHeader->ReturnStatus;
}

/**
  Transfer the password to the smm driver.

  @param[in]  DevicePath     The device path for the opal devcie.
  @param      PasswordLen    The input password length.
  @param      Password       Input password buffer.

  @retval  EFI_SUCCESS       Do the required action success.
  @retval  Others            Error occured.

**/
EFI_STATUS
EFIAPI
OpalSupportSendPasword(
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  UINTN                       PasswordLen,
  VOID                        *Password
  )
{
  OPAL_COMM_DEVICE_LIST   *Parameter;
  VOID                    *Buffer;
  UINTN                   Length;
  EFI_STATUS              Status;
  UINTN                   DevicePathLen;

  Parameter = NULL;
  Buffer = NULL;

  if (DevicePath == NULL) {
    //
    // Assume DevicePath == NULL only when library used by SMM driver
    // and should not run to here, just return success.
    //
    return EFI_SUCCESS;
  }

  DevicePathLen = GetDevicePathSize (DevicePath);
  Length = OFFSET_OF (OPAL_COMM_DEVICE_LIST, OpalDevicePath) + DevicePathLen;
  Buffer = OpalInitCommunicateBuffer((VOID**)&Parameter, Length, SMM_FUNCTION_SET_OPAL_PASSWORD);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (Password != NULL) {
    CopyMem((VOID*)Parameter->Password, Password, PasswordLen);
    Parameter->PasswordLength = (UINT8)PasswordLen;
  }
  CopyMem (&Parameter->OpalDevicePath, DevicePath, DevicePathLen);

  Status = OpalSendCommunicateBuffer(Buffer, Length);
  if (EFI_ERROR(Status)) {
    goto EXIT;
  }

EXIT:
  ZeroMem(Parameter, Length);
  return Status;
}

/**
  Get saved Opal device list.

  @retval      return opal device list.

**/
LIST_ENTRY*
EFIAPI
OpalSupportGetOpalDeviceList (
  VOID
  )
{
  return &mDeviceList;
}

/**
  Check if the password is full zero.

  @param[in]   Password       Points to the Data Buffer

  @retval      TRUE           This password string is full zero.
  @retval      FALSE          This password string is not full zero.

**/
BOOLEAN
OpalPasswordIsFullZero (
  IN UINT8                    *Password
  )
{
  UINTN                       Index;

  for (Index = 0; Index < OPAL_PASSWORD_MAX_LENGTH; Index++) {
    if (Password[Index] != 0) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Save hdd password to SMM.

  @param[in] DevicePath                Input device path info for the device.
  @param[in] Password                  The hdd password of attached ATA device.
  @param[in] PasswordLength            The hdd password length.

  @retval EFI_OUT_OF_RESOURCES    Insufficient resources to create database record
  @retval EFI_SUCCESS             The function has been successfully executed.

**/
EFI_STATUS
OpalSavePasswordToSmm (
  IN  EFI_DEVICE_PATH_PROTOCOL      *DevicePath,
  IN  UINT8                         *Password,
  IN  UINT8                         PasswordLength
  )
{
  OPAL_DISK_AND_PASSWORD_INFO       *List;
  OPAL_DISK_AND_PASSWORD_INFO       *Dev;
  LIST_ENTRY                        *Entry;
  UINTN                             DevicePathLen;

  DevicePathLen = GetDevicePathSize (DevicePath);

  for (Entry = mDeviceList.ForwardLink; Entry != &mDeviceList; Entry = Entry->ForwardLink) {
    List = BASE_CR (Entry, OPAL_DISK_AND_PASSWORD_INFO, Link);
    if (CompareMem (&List->OpalDevicePath, DevicePath, DevicePathLen) == 0) {
      CopyMem(List->Password, Password, OPAL_PASSWORD_MAX_LENGTH);
      return EFI_SUCCESS;
    }
  }

  Dev = AllocateZeroPool (OFFSET_OF (OPAL_DISK_AND_PASSWORD_INFO, OpalDevicePath) + DevicePathLen);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->PasswordLength = PasswordLength;
  CopyMem(&(Dev->Password), Password, OPAL_PASSWORD_MAX_LENGTH);
  CopyMem(&(Dev->OpalDevicePath), DevicePath, DevicePathLen);

  InsertHeadList (&mDeviceList, &Dev->Link);

  return EFI_SUCCESS;
}

/**
  Communication service SMI Handler entry.

  This SMI handler provides services for saving HDD password and saving S3 boot script when ready to boot.

  @param[in]     DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param[in]     RegisterContext Points to an optional handler context which was specified when the
                                 handler was registered.
  @param[in, out] CommBuffer     A pointer to a collection of Data in memory that will
                                 be conveyed from a non-SMM environment into an SMM environment.
  @param[in, out] CommBufferSize The Size of the CommBuffer.

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
SmmOpalPasswordHandler (
  IN     EFI_HANDLE                 DispatchHandle,
  IN     CONST VOID                 *RegisterContext,
  IN OUT VOID                       *CommBuffer,
  IN OUT UINTN                      *CommBufferSize
  )
{
  EFI_STATUS                        Status;
  OPAL_SMM_COMMUNICATE_HEADER       *SmmFunctionHeader;
  UINTN                             TempCommBufferSize;
  UINT8                             *NewPassword;
  UINT8                             PasswordLength;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  if (CommBuffer == NULL || CommBufferSize == NULL) {
    return EFI_SUCCESS;
  }

  TempCommBufferSize = *CommBufferSize;
  if (TempCommBufferSize < OFFSET_OF (OPAL_SMM_COMMUNICATE_HEADER, Data)) {
    return EFI_SUCCESS;
  }

  Status   = EFI_SUCCESS;
  SmmFunctionHeader     = (OPAL_SMM_COMMUNICATE_HEADER *)CommBuffer;

  DevicePath = &((OPAL_COMM_DEVICE_LIST*)(SmmFunctionHeader->Data))->OpalDevicePath;
  PasswordLength = ((OPAL_COMM_DEVICE_LIST*)(SmmFunctionHeader->Data))->PasswordLength;
  NewPassword = ((OPAL_COMM_DEVICE_LIST*)(SmmFunctionHeader->Data))->Password;

  switch (SmmFunctionHeader->Function) {
    case SMM_FUNCTION_SET_OPAL_PASSWORD:
        if (OpalPasswordIsFullZero (NewPassword) || PasswordLength == 0) {
          Status = EFI_INVALID_PARAMETER;
          goto EXIT;
        }

        Status = OpalSavePasswordToSmm (DevicePath, NewPassword, PasswordLength);
      break;

    default:
      Status = EFI_UNSUPPORTED;
      break;
  }

EXIT:
  SmmFunctionHeader->ReturnStatus = Status;

  //
  // Return EFI_SUCCESS cause only one handler can be trigged.
  // so return EFI_WARN_INTERRUPT_SOURCE_PENDING to make all handler can be trigged.
  //
  return EFI_WARN_INTERRUPT_SOURCE_PENDING;
}

/**
  The constructor function.

  Register SMI handler when link to SMM driver.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OpalPasswordSupportLibConstructor (
  VOID
  )
{
  EFI_SMM_BASE2_PROTOCOL         *SmmBase2;
  EFI_SMM_SYSTEM_TABLE2          *Smst;
  EFI_HANDLE                     SmmHandle;
  EFI_STATUS                     Status;

  Status = gBS->LocateProtocol (&gEfiSmmBase2ProtocolGuid, NULL, (VOID**) &SmmBase2);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }
  Status = SmmBase2->InSmm (SmmBase2, &gInSmm);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }
  if (!gInSmm) {
    return RETURN_SUCCESS;
  }

  //
  // Good, we are in SMM
  //
  Status = SmmBase2->GetSmstLocation (SmmBase2, &Smst);
  if (EFI_ERROR (Status)) {
    return RETURN_SUCCESS;
  }

  SmmHandle = NULL;
  Status    = Smst->SmiHandlerRegister (SmmOpalPasswordHandler, &gOpalPasswordNotifyProtocolGuid, &SmmHandle);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The Destructor function.

  Clean the saved opal device list.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
OpalPasswordSupportLibDestructor (
  VOID
  )
{
  OPAL_DISK_AND_PASSWORD_INFO  *Device;

  while (!IsListEmpty (&mDeviceList)) {
    Device = BASE_CR (mDeviceList.ForwardLink, OPAL_DISK_AND_PASSWORD_INFO, Link);

    RemoveEntryList (&Device->Link);
    FreePool (Device);
  }

  return EFI_SUCCESS;
}
