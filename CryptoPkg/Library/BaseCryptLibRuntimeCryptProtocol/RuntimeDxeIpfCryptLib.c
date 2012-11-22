/** @file
  Implementation of The runtime cryptographic library instance (for IPF).

Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>

#include <Protocol/RuntimeCrypt.h>

#include <Guid/EventGroup.h>

EFI_RUNTIME_CRYPT_PROTOCOL  *mCryptProtocol = NULL;
EFI_EVENT                   mIpfCryptLibVirtualNotifyEvent;

/**
  Notification function of EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE, which converts
  pointer to new virtual address.

  @param  Event        Event whose notification function is being invoked.
  @param  Context      Pointer to the notification function's context

**/
VOID
EFIAPI
IpfCryptLibAddressChangeEvent (
  IN  EFI_EVENT        Event,
  IN  VOID             *Context
  )
{
  //
  // Convert Address of Runtime Crypto Protocol.
  //
  EfiConvertPointer (0x0, (VOID **) &mCryptProtocol);
}

/**
  Constructor of IPF Crypto Library Instance.
  This function locates the Runtime Crypt Protocol and register notification
  function for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
RuntimeDxeIpfCryptLibConstructor (
  IN      EFI_HANDLE                ImageHandle,
  IN      EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Locate Runtime Crypt Protocol Instance
  //
  Status = gBS->LocateProtocol (
                  &gEfiRuntimeCryptProtocolGuid,
                  NULL,
                  (VOID**) &mCryptProtocol
                  );
  ASSERT_EFI_ERROR (Status);
  ASSERT (mCryptProtocol != NULL);

  //
  // Register SetVirtualAddressMap () notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  IpfCryptLibAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mIpfCryptLibVirtualNotifyEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Destructor of IPF Crypto Library Instance.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
  @retval Other value   The destructor did not complete successfully.

**/
EFI_STATUS
EFIAPI
RuntimeDxeIpfCryptLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the Set Virtual Address Map event
  //
  Status = gBS->CloseEvent (mIpfCryptLibVirtualNotifyEvent);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Check whether crypto service provided by Runtime Crypt protocol is ready to use.

  Crypto service is available if the call is in physical mode prior to
  SetVirtualAddressMap() or virtual mode after SetVirtualAddressMap(). If either
  of these two conditions are met, this routine will return TRUE; if neither of
  these conditions are met, this routine will return FALSE.

  @retval TRUE   The Crypto service is ready to use.
  @retval FALSE  The Crypto service is not available.

**/
BOOLEAN
EFIAPI
InternalIsCryptServiveAvailable (
  VOID
  )
{
  INT64    CpuMode;
  BOOLEAN  GoneVirtual;

  CpuMode = AsmCpuVirtual();
  if (CpuMode < 0) {
    //
    // CPU is in mixed mode, return failing the operation gracefully.
    //
    return FALSE;
  }

  GoneVirtual = EfiGoneVirtual();

  if ((CpuMode > 0) && !GoneVirtual) {
    //
    // CPU is in virtual mode, but SetVirtualAddressMap() has not been called,
    // so return failing the operation gracefully.
    //
    return FALSE;
  }

  if ((CpuMode == 0) && GoneVirtual) {
    //
    // CPU is in physical mode, but SetVirtualAddressMap() has been called,
    // so return failing the operation gracefully.
    //
    return FALSE;
  }

  return TRUE;
}

/**
  Retrieves the size, in bytes, of the context buffer required for SHA-256 operations.

  @return  The size, in bytes, of the context buffer required for SHA-256 operations.

**/
UINTN
EFIAPI
Sha256GetContextSize (
  VOID
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return 0;
  }

  return mCryptProtocol->Sha256GetContextSize ();
}

/**
  Initializes user-supplied memory pointed by Sha256Context as SHA-256 hash context for
  subsequent use.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to SHA-256 Context being initialized.

  @retval TRUE   SHA-256 context initialization succeeded.
  @retval FALSE  SHA-256 context initialization failed.

**/
BOOLEAN
EFIAPI
Sha256Init (
  IN OUT  VOID  *Sha256Context
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->Sha256Init (Sha256Context);
}


/**
  Makes a copy of an existing SHA-256 context.

  Return FALSE to indicate this interface is not supported.

  @param[in]  Sha256Context     Pointer to SHA-256 context being copied.
  @param[out] NewSha256Context  Pointer to new SHA-256 context.

  @retval FALSE  This interface is not supported.

**/
BOOLEAN
EFIAPI
Sha256Duplicate (
  IN   CONST VOID  *Sha256Context,
  OUT  VOID        *NewSha256Context
  )
{
  ASSERT (FALSE);
  return FALSE;
}


/**
  Performs SHA-256 digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If Sha256Context is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to the SHA-256 context.
  @param[in]       Data           Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength     Length of Data buffer in bytes.

  @retval TRUE   SHA-256 data digest succeeded.
  @retval FALSE  Invalid SHA-256 context. After Sha256Final function has been called, the
                 SHA-256 context cannot be reused.

**/
BOOLEAN
EFIAPI
Sha256Update (
  IN OUT  VOID        *Sha256Context,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->Sha256Update (Sha256Context, Data, DataLength);
}

/**
  Completes SHA-256 hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the SHA-256 context cannot be used again.

  If Sha256Context is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.

  @param[in, out]  Sha256Context  Pointer to SHA-256 context
  @param[out]      HashValue      Pointer to a buffer that receives the SHA-256 digest
                                  value (32 bytes).

  @retval TRUE   SHA-256 digest computation succeeded.
  @retval FALSE  SHA-256 digest computation failed.

**/
BOOLEAN
EFIAPI
Sha256Final (
  IN OUT  VOID   *Sha256Context,
  OUT     UINT8  *HashValue
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->Sha256Final (Sha256Context, HashValue);
}

/**
  Allocates and initializes one RSA context for subsequent use.

  @return  Pointer to the RSA context that has been initialized.
           If the allocations fails, RsaNew() returns NULL.

**/
VOID *
EFIAPI
RsaNew (
  VOID
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->RsaNew ();
}

/**
  Release the specified RSA context.

  @param[in]  RsaContext  Pointer to the RSA context to be released.

**/
VOID
EFIAPI
RsaFree (
  IN  VOID  *RsaContext
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return;
  }

  mCryptProtocol->RsaFree (RsaContext);
}

/**
  Sets the tag-designated key component into the established RSA context.

  This function sets the tag-designated RSA key component into the established
  RSA context from the user-specified non-negative integer (octet string format
  represented in RSA PKCS#1).
  If BigNumber is NULL, then the specified key componenet in RSA context is cleared.

  If RsaContext is NULL, then return FALSE.

  @param[in, out]  RsaContext  Pointer to RSA context being set.
  @param[in]       KeyTag      Tag of RSA key component being set.
  @param[in]       BigNumber   Pointer to octet integer buffer.
                               If NULL, then the specified key componenet in RSA
                               context is cleared.
  @param[in]       BnSize      Size of big number buffer in bytes.
                               If BigNumber is NULL, then it is ignored.

  @retval  TRUE   RSA key component was set successfully.
  @retval  FALSE  Invalid RSA key component tag.

**/
BOOLEAN
EFIAPI
RsaSetKey (
  IN OUT  VOID         *RsaContext,
  IN      RSA_KEY_TAG  KeyTag,
  IN      CONST UINT8  *BigNumber,
  IN      UINTN        BnSize
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->RsaSetKey (RsaContext, KeyTag, BigNumber, BnSize);
}

/**
  Verifies the RSA-SSA signature with EMSA-PKCS1-v1_5 encoding scheme defined in
  RSA PKCS#1.

  If RsaContext is NULL, then return FALSE.
  If MessageHash is NULL, then return FALSE.
  If Signature is NULL, then return FALSE.
  If HashSize is not equal to the size of MD5, SHA-1 or SHA-256 digest, then return FALSE.

  @param[in]  RsaContext   Pointer to RSA context for signature verification.
  @param[in]  MessageHash  Pointer to octet message hash to be checked.
  @param[in]  HashSize     Size of the message hash in bytes.
  @param[in]  Signature    Pointer to RSA PKCS1-v1_5 signature to be verified.
  @param[in]  SigSize      Size of signature in bytes.

  @retval  TRUE   Valid signature encoded in PKCS1-v1_5.
  @retval  FALSE  Invalid signature or invalid RSA context.

**/
BOOLEAN
EFIAPI
RsaPkcs1Verify (
  IN  VOID         *RsaContext,
  IN  CONST UINT8  *MessageHash,
  IN  UINTN        HashSize,
  IN  CONST UINT8  *Signature,
  IN  UINTN        SigSize
  )
{
  if (!InternalIsCryptServiveAvailable ()) {
    return FALSE;
  }

  return mCryptProtocol->RsaPkcs1Verify (
                           RsaContext,
                           MessageHash,
                           HashSize,
                           Signature,
                           SigSize
                           );
}
