/** @file

  Copyright (c) 2025, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
   - Realm Management Monitor (RMM) Specification, version 1.0-rel0
     (https://developer.arm.com/documentation/den0137/)
   - TCG PC Client Platform Firmware Profile Specification, Level 00
     Version 1.06 Revision 52 Family “2.0” - December 4, 2023
     (https://trustedcomputinggroup.org/resource/pc-client-specific-platform-
      firmware-profile-specification/)
**/

#include <PiPei.h>

#include <Guid/TcgEventHob.h>

#include <IndustryStandard/UefiTcgPlatform.h>

#include <Library/ArmCcaLib.h>
#include <Library/ArmCcaRsiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseCryptLib.h>

#include <Protocol/CcMeasurement.h>

STATIC TPM_ALG_ID  mTpmAlgId = TPM_ALG_ERROR;

/**
  Computes the digest of a input data buffer.

  This function performs the digest of a given data buffer, and
  places the digest value into the specified memory.

  If this interface is not supported, then return FALSE.

  @param[in]   Data        Pointer to the buffer containing the data to be
                           hashed.
  @param[in]   DataSize    Size of Data buffer in bytes.
  @param[out]  HashValue   Pointer to a buffer that receives the digest value.

  @retval TRUE   Digest computation succeeded.
  @retval FALSE  Digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *DIGEST_FUNCTION)(
  IN   CONST VOID  *Data,
  IN   UINTN       DataSize,
  OUT  UINT8       *HashValue
  );

/**
 A structure mapping the Arm CCA hash information.
*/
typedef struct {
  /// The Realm hash algorithm.
  UINT8              RealmHashAlgorithm;
  /// The alogrithm ID for the hash function.
  TPMI_ALG_HASH      HashAlgo;
  /// The hash size.
  UINT16             HashSize;
  /// The hash mask.
  UINT32             HashMask;
  /// A function pointer to the hash function.
  DIGEST_FUNCTION    DigestFunction;
} ARMCCA_HASH_INFO;

/**
  A map of the hash algorithms supported by a Realm.
*/
STATIC ARMCCA_HASH_INFO  mHashInfo[] = {
  {
    ARM_CCA_RSI_HASH_SHA_256,
    TPM_ALG_SHA256,
    SHA256_DIGEST_SIZE,
    HASH_ALG_SHA256,
    Sha256HashAll
  },
  {
    ARM_CCA_RSI_HASH_SHA_512,
    TPM_ALG_SHA512,
    SHA512_DIGEST_SIZE,
    HASH_ALG_SHA512,
    Sha512HashAll
  }
};

/**
  Get hash algorithm ID corresponding to the Realm Hash algorithm.

  @param[in]    RealmHashAlgorithm   The Realm Hash Algorithm.

  @return Hash Algorithm ID if success else TPM_ALG_ERROR.
**/
STATIC
TPM_ALG_ID
RealmHashAlgoToTpmAlgoId (
  UINT8  RealmHashAlgorithm
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].RealmHashAlgorithm == RealmHashAlgorithm) {
      return mHashInfo[Index].HashAlgo;
    }
  }

  // No suitable algorithm found return.
  return TPM_ALG_ERROR;
}

/**
  Get the hash information based on the Hash Algo.

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Pointer to the Hash information on success or NULL.
**/
STATIC
ARMCCA_HASH_INFO *
GetHashInfoFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  UINTN  Index;

  for (Index = 0; Index < ARRAY_SIZE (mHashInfo); Index++) {
    if (mHashInfo[Index].HashAlgo == HashAlgo) {
      return &mHashInfo[Index];
    }
  }

  return NULL;
}

/**
  Get hash size based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Size of the hash.
**/
STATIC
UINT16
GetHashSizeFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  ARMCCA_HASH_INFO  *HashInfo;

  HashInfo = GetHashInfoFromAlgo (HashAlgo);
  if (HashInfo != NULL) {
    return HashInfo->HashSize;
  }

  return 0;
}

/**
  Get hash mask based on Algo

  @param[in]     HashAlgo           Hash Algorithm Id.

  @return Hash mask.
**/
STATIC
UINT32
GetHashMaskFromAlgo (
  IN TPMI_ALG_HASH  HashAlgo
  )
{
  ARMCCA_HASH_INFO  *HashInfo;

  HashInfo = GetHashInfoFromAlgo (HashAlgo);
  if (HashInfo != NULL) {
    return HashInfo->HashMask;
  }

  ASSERT (FALSE);
  return 0;
}

/**
  Read the Realm configuration to get the Realm Hash algorithm and return
  the Hash algorithm ID.

  @param[out] HashAlgId         The Hash algorithm ID corresponding to the
                                Realm Hash algorithm.

  @retval EFI_SUCCESS           Success, the Realm Hash algorithm is returned.
  @retval EFI_OUT_OF_RESOURCES  Out of resources.
  @retval EFI_UNSUPPORTED       Unsupported algorithm.
**/
STATIC
EFI_STATUS
EFIAPI
GetRealmHashAlgorithm (
  TPM_ALG_ID  *HashAlgId
  )
{
  EFI_STATUS            Status;
  ARM_CCA_REALM_CONFIG  *Config;
  UINT8                 HashAlgorithm;

  Config = AllocateAlignedPages (
             EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)),
             ARM_CCA_REALM_GRANULE_SIZE
             );
  if (Config == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (Config, sizeof (ARM_CCA_REALM_CONFIG));

  Status = ArmCcaRsiGetRealmConfig (Config);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    FreeAlignedPages (
      Config,
      EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG))
      );
    return Status;
  }

  HashAlgorithm = Config->HashAlgorithm;

  FreeAlignedPages (Config, EFI_SIZE_TO_PAGES (sizeof (ARM_CCA_REALM_CONFIG)));

  *HashAlgId = RealmHashAlgoToTpmAlgoId (HashAlgorithm);
  if (*HashAlgId == TPM_ALG_ERROR) {
    DEBUG ((DEBUG_ERROR, "Unsupported HashAlgId = 0x%x\n", *HashAlgId));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_INFO, "HashAlgId = 0x%x\n", *HashAlgId));
  return EFI_SUCCESS;
}

#ifdef DUMP_EVENT_LOGS

/**
  This function performs a raw data dump of the ACPI table.

  @param [in] Ptr     Pointer to the start of the table buffer.
  @param [in] Length  The length of the buffer.
**/
STATIC
VOID
EFIAPI
DumpDigest (
  IN UINT8   *Ptr,
  IN UINT32  Length
  )
{
  UINTN  ByteCount;
  UINTN  PartLineChars;
  UINTN  AsciiBufferIndex;
  CHAR8  AsciiBuffer[17];

  ByteCount        = 0;
  AsciiBufferIndex = 0;

  DEBUG ((DEBUG_INFO, "Digest Dump - Start\n"));
  DEBUG ((DEBUG_INFO, "Address  : 0x%p\n", Ptr));
  DEBUG ((DEBUG_INFO, "Length   : %d\n", Length));

  while (ByteCount < Length) {
    if ((ByteCount & 0x0F) == 0) {
      AsciiBuffer[AsciiBufferIndex] = '\0';
      DEBUG ((DEBUG_INFO, "  %a\n%08X : ", AsciiBuffer, ByteCount));
      AsciiBufferIndex = 0;
    } else if ((ByteCount & 0x07) == 0) {
      DEBUG ((DEBUG_INFO, "- "));
    }

    if ((*Ptr >= ' ') && (*Ptr < 0x7F)) {
      AsciiBuffer[AsciiBufferIndex++] = *Ptr;
    } else {
      AsciiBuffer[AsciiBufferIndex++] = '.';
    }

    DEBUG ((DEBUG_INFO, "%02X ", *Ptr++));

    ByteCount++;
  }

  // Justify the final line using spaces before printing
  // the ASCII data.
  PartLineChars = (Length & 0x0F);
  if (PartLineChars != 0) {
    PartLineChars = 48 - (PartLineChars * 3);
    if ((Length & 0x0F) <= 8) {
      PartLineChars += 2;
    }

    while (PartLineChars > 0) {
      DEBUG ((DEBUG_INFO, " "));
      PartLineChars--;
    }
  }

  // Print ASCII data for the final line.
  AsciiBuffer[AsciiBufferIndex] = '\0';
  DEBUG ((DEBUG_INFO, "  %a\n\n", AsciiBuffer));
  DEBUG ((DEBUG_INFO, "Digest Dump - End\n"));
}

#endif // DUMP_EVENT_LOGS

/**
  Get TPML_DIGEST_VALUES data size.

  @param[in]     DigestList    TPML_DIGEST_VALUES data.

  @return TPML_DIGEST_VALUES data size.
**/
UINT32
EFIAPI
GetDigestListSize (
  IN TPML_DIGEST_VALUES  *DigestList
  )
{
  UINTN   Index;
  UINT16  DigestSize;
  UINT32  TotalSize;

  TotalSize = sizeof (DigestList->count);
  for (Index = 0; Index < DigestList->count; Index++) {
    DigestSize = GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    TotalSize += sizeof (DigestList->digests[Index].hashAlg) + DigestSize;
  }

  return TotalSize;
}

/**
  Copy TPML_DIGEST_VALUES into a buffer

  @param[in,out] Buffer             Buffer to hold copied TPML_DIGEST_VALUES
                                    compact binary.
  @param[in]     DigestList         TPML_DIGEST_VALUES to be copied.
  @param[in]     HashAlgorithmMask  HASH bits corresponding to the desired
                                    digests to copy.

  @return The end of buffer to hold TPML_DIGEST_VALUES.
**/
STATIC
VOID *
CopyDigestListToBuffer (
  IN OUT VOID            *Buffer,
  IN TPML_DIGEST_VALUES  *DigestList,
  IN UINT32              HashAlgorithmMask
  )
{
  UINTN   Index;
  UINT16  DigestSize;
  UINT32  DigestListCount;
  UINT32  *DigestListCountPtr;

  DigestListCountPtr = (UINT32 *)Buffer;
  DigestListCount    = 0;
  Buffer             = (UINT8 *)Buffer + sizeof (DigestList->count);
  for (Index = 0; Index < DigestList->count; Index++) {
    if ((GetHashMaskFromAlgo (DigestList->digests[Index].hashAlg) &
         HashAlgorithmMask) == 0)
    {
      DEBUG ((
        DEBUG_WARN,
        "WARNING: ArmCCA Event log has HashAlg unsupported (0x%x)\n",
        DigestList->digests[Index].hashAlg
        ));
      continue;
    }

    CopyMem (
      Buffer,
      &DigestList->digests[Index].hashAlg,
      sizeof (DigestList->digests[Index].hashAlg)
      );

    Buffer = (UINT8 *)Buffer +
             sizeof (DigestList->digests[Index].hashAlg);
    DigestSize = GetHashSizeFromAlgo (DigestList->digests[Index].hashAlg);
    CopyMem (Buffer, &DigestList->digests[Index].digest, DigestSize);
    Buffer = (UINT8 *)Buffer + DigestSize;
    DigestListCount++;
  }

  WriteUnaligned32 (DigestListCountPtr, DigestListCount);

  return Buffer;
}

/**
  Calculate the digest of input Data and extend it to the REM register.

  @param TpmAlgId        The algorithm Id
  @param RemIndex        Index of the REM register
  @param DataToHash      Data to be hashed
  @param DataToHashLen   Length of the data
  @param Digest          Hash value of the input data
  @param DigestLen       Length of the hash value

  @retval EFI_SUCCESS    Successfully hash and extend to REM
  @retval Others         Other errors as indicated
**/
STATIC
EFI_STATUS
EFIAPI
ArmCcaHashAndExtendToRem (
  IN TPM_ALG_ID  TpmAlgId,
  IN UINT32      RemIndex,
  IN VOID        *DataToHash,
  IN UINTN       DataToHashLen,
  OUT UINT8      *Digest,
  IN  UINTN      DigestLen
  )
{
  EFI_STATUS        Status;
  ARMCCA_HASH_INFO  *HashInfo;

  if ((DataToHash == NULL) || (DataToHashLen == 0) || (Digest == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  HashInfo = GetHashInfoFromAlgo (TpmAlgId);
  if (HashInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  if ((DigestLen != HashInfo->HashSize)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the digest of the data
  //
  if (!HashInfo->DigestFunction (DataToHash, DataToHashLen, Digest)) {
    return EFI_ABORTED;
  }

 #ifdef DUMP_EVENT_LOGS
  DumpDigest (Digest, DigestLen);
 #endif

  Status = ArmCcaRsiExtendMeasurement (
             (UINTN)RemIndex,
             Digest,
             DigestLen
             );

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Add a new entry to the Event Log.

  @param[in]     TpmAlgId      The algorithm Id
  @param[in]     DigestList    A list of digest.
  @param[in,out] NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]     NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           The new event log entry was added.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
**/
STATIC
EFI_STATUS
EFIAPI
LogHashEvent (
  IN      TPM_ALG_ID         TpmAlgId,
  IN TPML_DIGEST_VALUES      *DigestList,
  IN OUT  TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  VOID            *HobData;
  TCG_PCR_EVENT2  *TcgPcrEvent2;
  UINT8           *DigestBuffer;

  //
  // Use GetDigestListSize (DigestList) in the GUID HOB DataLength calculation
  // to reserve enough buffer to hold TPML_DIGEST_VALUES compact binary.
  //
  HobData = BuildGuidHob (
              &gCcEventEntryHobGuid,
              sizeof (TcgPcrEvent2->PCRIndex) +
              sizeof (TcgPcrEvent2->EventType) +
              GetDigestListSize (DigestList) +
              sizeof (TcgPcrEvent2->EventSize) +
              NewEventHdr->EventSize
              );
  if (HobData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcgPcrEvent2            = HobData;
  TcgPcrEvent2->PCRIndex  = NewEventHdr->PCRIndex;
  TcgPcrEvent2->EventType = NewEventHdr->EventType;
  DigestBuffer            = (UINT8 *)&TcgPcrEvent2->Digest;

  //
  // Arm CCA support one digest hash among below hash algo list:
  //     - TPM_ALG_SHA256
  //     - TPM_ALG_SHA512
  //
  DigestBuffer = CopyDigestListToBuffer (
                   DigestBuffer,
                   DigestList,
                   GetHashMaskFromAlgo (TpmAlgId)
                   );

  CopyMem (
    DigestBuffer,
    &NewEventHdr->EventSize,
    sizeof (TcgPcrEvent2->EventSize)
    );
  DigestBuffer = DigestBuffer + sizeof (TcgPcrEvent2->EventSize);

  CopyMem (DigestBuffer, NewEventData, NewEventHdr->EventSize);
  return EFI_SUCCESS;
}

/**
  Do a hash operation on a data buffer, extend a specific Measurement Register
  with the hash result, and build a GUIDed HOB recording the event which will
  be passed to the DXE phase and added into the Event Log.

  @param[in]      HashData      If BIT0 of Flags is 0, it is physical address
                                of the start of the data buffer to be hashed,
                                extended, and logged.
                                If BIT0 of Flags is 1, it is physical address
                                of the start of the pre-hash data buffter to be
                                extended, and logged.
                                The pre-hash data format is TPML_DIGEST_VALUES.
  @param[in]      HashDataLen   The length, in bytes, of the buffer referenced
                                by HashData.
  @param[in]      NewEventHdr   Pointer to a TCG_PCR_EVENT_HDR data structure.
  @param[in]      NewEventData  Pointer to the new event data.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.
  @retval EFI_UNSUPPORTED       Execution context is not a Realm.

**/
STATIC
EFI_STATUS
EFIAPI
HashLogExtendEvent (
  IN      UINT8              *HashData,
  IN      UINTN              HashDataLen,
  IN      TCG_PCR_EVENT_HDR  *NewEventHdr,
  IN      UINT8              *NewEventData
  )
{
  EFI_STATUS          Status;
  TPML_DIGEST_VALUES  DigestList;
  UINTN               DigestLen;

  if (mTpmAlgId == TPM_ALG_ERROR) {
    Status = GetRealmHashAlgorithm (&mTpmAlgId);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  DigestLen = GetHashSizeFromAlgo (mTpmAlgId);
  if (DigestLen == 0) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  DigestList.count              = 1;
  DigestList.digests[0].hashAlg = mTpmAlgId;

  Status = ArmCcaHashAndExtendToRem (
             mTpmAlgId,
             NewEventHdr->PCRIndex,
             HashData,
             HashDataLen,
             (UINT8 *)&DigestList.digests[0].digest,
             DigestLen
             );

  if (!EFI_ERROR (Status)) {
    Status = LogHashEvent (
               mTpmAlgId,
               &DigestList,
               NewEventHdr,
               NewEventData
               );
  } else {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HashLogExtendEvent failed !!!, Error = %r.\n",
      Status
      ));
  }

  return Status;
}

/**
  Measure CRTM version.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureCRTMVersion (
  VOID
  )
{
  TCG_PCR_EVENT_HDR  TcgEventHdr;
  EFI_STATUS         Status;

  //
  // Use FirmwareVersion string to represent CRTM version.
  // OEMs should get real CRTM version string and measure it.
  //
  TcgEventHdr.PCRIndex  = ARMCCA_MR_INDEX_1_REM0;
  TcgEventHdr.EventType = EV_S_CRTM_VERSION;
  TcgEventHdr.EventSize =
    (UINT32)StrSize ((CHAR16 *)PcdGetPtr (PcdFirmwareVersionString));

  Status =  HashLogExtendEvent (
              (UINT8 *)PcdGetPtr (PcdFirmwareVersionString),
              TcgEventHdr.EventSize,
              &TcgEventHdr,
              (UINT8 *)PcdGetPtr (PcdFirmwareVersionString)
              );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Measure CRTM version.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
STATIC
EFI_STATUS
EFIAPI
MeasureFdt (
  VOID
  )
{
  EFI_STATUS         Status;
  VOID               *DeviceTreeBase;
  TCG_PCR_EVENT_HDR  TcgEventHdr;
  CHAR8              *EventDescription;

  DeviceTreeBase = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (DeviceTreeBase != NULL);
  ASSERT (FdtCheckHeader (DeviceTreeBase) == 0);

  //
  // Referring to "TCG PC Client Platform Firmware Profile Specification,
  // Level 00 Version 1.06 Revision 52 Family “2.0” - December 4, 2023",
  // The EV_NONHOST_CONFIG event MUST extend Configuration information
  // or data associated with a Non-Host Platform to the PCR[1] only.
  //
  // The digests field contains the tagged hash of the Non-Host
  // Platform configuration information for each PCR bank.
  // The event field is defined by the platform manufacturer.
  // See Section 2.3.4.2.
  //
  EventDescription      = "Kvmtool FDT.";
  TcgEventHdr.PCRIndex  = ARMCCA_MR_INDEX_1_REM0;
  TcgEventHdr.EventType = EV_NONHOST_CONFIG;
  TcgEventHdr.EventSize = AsciiStrLen (EventDescription);

  DEBUG ((
    DEBUG_INFO,
    "FdtBase = 0x%lx, size = 0x%x\n",
    DeviceTreeBase,
    FdtTotalSize (DeviceTreeBase)
    ));

  Status =  HashLogExtendEvent (
              (UINT8 *)DeviceTreeBase,
              FdtTotalSize (DeviceTreeBase),
              &TcgEventHdr,
              (UINT8 *)EventDescription
              );

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Measurement for PeilessSec.

  @retval EFI_SUCCESS           Operation completed successfully.
  @retval EFI_INVALID_PARAMETER Invalid firmware volume information.
  @retval EFI_OUT_OF_RESOURCES  No enough memory to log the new event.
  @retval EFI_DEVICE_ERROR      The command was unsuccessful.

**/
EFI_STATUS
MeasurePeilessSec (
  VOID
  )
{
  EFI_STATUS  Status;

  if (!ArmCcaIsRealm ()) {
    // Nothing to do as the execution context is not a Realm.
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_INFO, "MeasurePeilessSec\n"));

  Status = MeasureCRTMVersion ();
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = MeasureFdt ();
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  DEBUG ((DEBUG_INFO, "MeasurePeilessSec - Done\n"));

  return Status;
}
