/** @file
  Definitions related to IPSEC_CONFIG_PROTOCOL implementations.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IPSEC_CONFIG_IMPL_H_
#define _IPSEC_CONFIG_IMPL_H_

#include <Protocol/IpSec.h>
#include <Protocol/IpSecConfig.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PrintLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>

#include "IpSecImpl.h"

#define EFI_IPSEC_ANY_PROTOCOL    0xFFFF
#define EFI_IPSEC_ANY_PORT        0

#define IPSEC_VAR_ITEM_HEADER_LOGO_BIT     0x80
#define IPSEC_VAR_ITEM_HEADER_CONTENT_BIT  0x7F

#define IPSECCONFIG_VARIABLE_NAME       L"IpSecConfig"
#define IPSECCONFIG_STATUS_NAME         L"IpSecStatus"

#define SIZE_OF_SPD_SELECTOR(x) (UINTN) (sizeof (EFI_IPSEC_SPD_SELECTOR) \
       + sizeof (EFI_IP_ADDRESS_INFO) * ((x)->LocalAddressCount + (x)->RemoteAddressCount))

#define FIX_REF_BUF_ADDR(addr, base)    addr = (VOID *) ((UINTN) (addr) - (UINTN) (base))
#define UNFIX_REF_BUF_ADDR(addr, base)  addr = (VOID *) ((UINTN) (addr) + (UINTN) (base))

//
// The data structure used to store the genernall information of IPsec configuration.
//
typedef struct {
  UINT32 VariableCount;      // the total number of the IPsecConfig variables.
  UINT32 VariableSize;       // The total size of all IpsecConfig variables.
  UINT32 SingleVariableSize; // The max size of single variable
} IP_SEC_VARIABLE_INFO;

typedef struct {
  EFI_IPSEC_CONFIG_SELECTOR *Selector;
  VOID                      *Data;
  LIST_ENTRY                List;
} IPSEC_COMMON_POLICY_ENTRY;

typedef struct {
  UINT8 *Ptr;
  UINTN Size;
  UINTN Capacity;
} IPSEC_VARIABLE_BUFFER;

#pragma pack(1)
typedef struct {
  UINT8   Type;
  UINT16  Size;
} IPSEC_VAR_ITEM_HEADER;
#pragma pack()

/**
  The prototype of Copy Source Selector to the Destination Selector.

  @param[in, out] DstSel             Pointer of Destination Selector. It would be
                                     SPD Selector, or SAD Selector or PAD Selector.
  @param[in]      SrcSel             Pointer of Source  Selector. It would be
                                     SPD Selector, or SAD Selector or PAD Selector.
  @param[in, out] Size               The size of the Destination Selector. If it
                                     is not NULL and its value is less than the size of
                                     Source Selector, the value of Source Selector's
                                     size will be passed to the caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source Selector is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size is less than size of Source Selector.
  @retval EFI_SUCCESS            Copy Source Selector to the Destination
                                 Selector successfully.

**/
typedef
EFI_STATUS
(*IPSEC_DUPLICATE_SELECTOR) (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  );

/**
  It is prototype of compare two Selectors. The Selector would be SPD Selector,
  or SAD Selector, or PAD selector.

  @param[in]   Selector1           Pointer of the first  Selector.
  @param[in]   Selector2           Pointer of the second Selector.

  @retval  TRUE    These two Selectors have the same value in certain fields.
  @retval  FALSE   Not all fields have the same value in these two Selectors.

**/
typedef
BOOLEAN
(*IPSEC_COMPARE_SELECTOR) (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  );

/**
  The prototype of a function to check if the Selector is Zero by its certain fields.

  @param[in]  Selector      Pointer of the Selector.

  @retval     TRUE          If the Selector is Zero.
  @retval     FALSE         If the Selector is not Zero.

**/
typedef
BOOLEAN
(*IPSEC_IS_ZERO_SELECTOR) (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  );

/**
  The prototype of a function to fix the value of particular members of the Selector.

  @param[in]  Selector              Pointer of Selector.
  @param[in]  Data                  Pointer of Data.

**/
typedef
VOID
(*IPSEC_FIX_POLICY_ENTRY) (
  IN EFI_IPSEC_CONFIG_SELECTOR           *Selector,
  IN VOID                                *Data
  );

/**
  It is prototype function to define a routine function by the caller of IpSecVisitConfigData().

  @param[in]      Type              A specified IPSEC_CONFIG_DATA_TYPE.
  @param[in]      Selector          Points to EFI_IPSEC_CONFIG_SELECTOR to be copied
                                    to the buffer.
  @param[in]      Data              Points to data to be copied to the buffer. The
                                    Data type is related to the Type.
  @param[in]      SelectorSize      The size of the Selector.
  @param[in]      DataSize          The size of the Data.
  @param[in, out] Buffer            The buffer to store the Selector and Data.

  @retval EFI_SUCCESS            Copied the Selector and Data to a buffer successfully.
  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.

**/
typedef
EFI_STATUS
(*IPSEC_COPY_POLICY_ENTRY) (
  IN     EFI_IPSEC_CONFIG_DATA_TYPE          Type,
  IN     EFI_IPSEC_CONFIG_SELECTOR           *Selector,
  IN     VOID                                *Data,
  IN     UINTN                               SelectorSize,
  IN     UINTN                               DataSize,
  IN OUT VOID                                *Context
  );

/**
  Set the security policy information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated
                                 configuration data specified by DataType.
                                 A NULL Selector causes the entire specified-type
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set.
  @param[in]  Context            Pointer to one entry selector that describes
                                 the expected position the new data entry will
                                 be added. If Context is NULL, the new entry will
                                 be appended to the end of the database.

  @retval EFI_INVALID_PARAMETER Certain Parameters are not correct. The Parameter
                                requiring a check depends on the Selector type.
  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
typedef
EFI_STATUS
(*IPSEC_SET_POLICY_ENTRY) (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector,
  IN VOID                             *Data,
  IN VOID                             *Context OPTIONAL
  );

/**
  A prototype function definition to lookup the data entry from IPsec. Return the configuration
  value of the specified Entry.

  @param[in]      Selector      Pointer to an entry selector that is an identifier
                                of the  entry.
  @param[in, out] DataSize      On output, the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec
                                configuration data. The type of the data buffer
                                is associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER Data is NULL and *DataSize is not zero.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
typedef
EFI_STATUS
(*IPSEC_GET_POLICY_ENTRY) (
  IN     EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN OUT UINTN                        *DataSize,
  IN     VOID                         *Data
  );

/**
  Compare two SPD Selectors.

  Compare two SPD Selector by the fields of LocalAddressCount/RemoteAddressCount/
  NextLayerProtocol/LocalPort/LocalPortRange/RemotePort/RemotePortRange and the
  Local Addresses and remote Addresses.

  @param[in]   Selector1           Pointer of the first SPD Selector.
  @param[in]   Selector2           Pointer of the second SPD Selector.

  @retval  TRUE    These two Selectors have the same value in above fields.
  @retval  FALSE   Not all of the above fields have the same value in these two Selectors.

**/
BOOLEAN
CompareSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  );


/**
  Visit all IPsec Configurations of specified Type and call the caller defined
  interface.

  @param[in]  DataType          The specified IPsec Config Data Type.
  @param[in]  Routine           The function caller defined.
  @param[in]  Context           The data passed to the Routine.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.
  @retval EFI_SUCCESS            This function complete successfully.

**/
EFI_STATUS
IpSecVisitConfigData (
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN IPSEC_COPY_POLICY_ENTRY          Routine,
  IN VOID                             *Context
  );


/**
  This function is the subfunction of the EFIIpSecConfigSetData.

  This function call IpSecSetVaraible to set the IPsec Configuration into the firmware.

  @retval EFI_OUT_OF_RESOURCES   The required system resource could not be allocated.
  @retval EFI_SUCCESS            Saved the configration successfully.
  @retval Others                 Other errors were found while obtaining the variable.

**/
EFI_STATUS
IpSecConfigSave (
  VOID
  );

/**
  Initialize IPsecConfig protocol

  @param[in, out]  Private   Pointer to IPSEC_PRIVATE_DATA. After this function finish,
                             the pointer of IPsecConfig Protocol implementation will copy
                             into its IPsecConfig member.

  @retval     EFI_SUCCESS    Initialized the IPsecConfig Protocol successfully.
  @retval     Others         Initializing the IPsecConfig Protocol failed.

**/
EFI_STATUS
IpSecConfigInitialize (
  IN OUT IPSEC_PRIVATE_DATA               *Private
  );

/**
  Calculate the entire size of EFI_IPSEC_SPD_DATA, which includes the buffer size pointed
  by the pointer members.

  @param[in]  SpdData             Pointer to a specified EFI_IPSEC_SPD_DATA.

  @return The entire size of the specified EFI_IPSEC_SPD_DATA.

**/
UINTN
IpSecGetSizeOfEfiSpdData (
  IN EFI_IPSEC_SPD_DATA               *SpdData
  );

/**
  Calculate the a entire size of IPSEC_SPD_DATA, which includes the buffer size pointed
  by the pointer members and the buffer size used by Sa List.

  @param[in]  SpdData       Pointer to the specified IPSEC_SPD_DATA.

  @return The entire size of IPSEC_SPD_DATA.

**/
UINTN
IpSecGetSizeOfSpdData (
  IN IPSEC_SPD_DATA                   *SpdData
  );

/**
  Copy Source Process Policy to the Destination Process Policy.

  @param[in]  Dst                  Pointer to the Source Process Policy.
  @param[in]  Src                  Pointer to the Destination Process Policy.

**/
VOID
IpSecDuplicateProcessPolicy (
  IN EFI_IPSEC_PROCESS_POLICY            *Dst,
  IN EFI_IPSEC_PROCESS_POLICY            *Src
  );

/**
  Find if the two SPD Selectors has subordinative.

  Compare two SPD Selector by the fields of LocalAddressCount/RemoteAddressCount/
  NextLayerProtocol/LocalPort/LocalPortRange/RemotePort/RemotePortRange and the 
  Local Addresses and remote Addresses.

  @param[in]   Selector1           Pointer of first SPD Selector.
  @param[in]   Selector2           Pointer of second SPD Selector.

  @retval  TRUE    The first SPD Selector is subordinate Selector of second SPD Selector.
  @retval  FALSE   The first SPD Selector is not subordinate Selector of second 
                   SPD Selector.
  
**/
BOOLEAN
IsSubSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  );

/**
  Compare two SA IDs.

  @param[in]   Selector1           Pointer of the first SA ID.
  @param[in]   Selector2           Pointer of the second SA ID.

  @retval  TRUE    This two Selectors have the same SA ID.
  @retval  FALSE   This two Selecotrs don't have the same SA ID.

**/
BOOLEAN
CompareSaId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  );

/**
  Compare two PAD IDs.

  @param[in]   Selector1           Pointer of the first PAD ID.
  @param[in]   Selector2           Pointer of the second PAD ID.

  @retval  TRUE    This two Selectors have the same PAD ID.
  @retval  FALSE   This two Selecotrs don't have the same PAD ID.

**/
BOOLEAN
ComparePadId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector1,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector2
  );

/**
  Check if the SPD Selector is Zero by its LocalAddressCount and RemoteAddressCount
  fields.

  @param[in]  Selector      Pointer of the SPD Selector.

  @retval     TRUE          If the SPD Selector is Zero.
  @retval     FALSE         If the SPD Selector is not Zero.

**/
BOOLEAN
IsZeroSpdSelector (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  );

/**
  Check if the SA ID is Zero by its DestAddress.

  @param[in]  Selector      Pointer of the SA ID.

  @retval     TRUE          If the SA ID is Zero.
  @retval     FALSE         If the SA ID is not Zero.

**/
BOOLEAN
IsZeroSaId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  );

/**
  Check if the PAD ID is Zero.

  @param[in]  Selector      Pointer of the PAD ID.

  @retval     TRUE          If the PAD ID is Zero.
  @retval     FALSE         If the PAD ID is not Zero.

**/
BOOLEAN
IsZeroPadId (
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector
  );

/**
  Copy Source SPD Selector to the Destination SPD Selector.

  @param[in, out] DstSel             Pointer of Destination SPD Selector.
  @param[in]      SrcSel             Pointer of Source SPD Selector.
  @param[in, out] Size               The size of the Destination SPD Selector. If
                                     it is not NULL and its value is less than the
                                     size of Source SPD Selector, the value of
                                     Source SPD Selector's size will be passed to
                                     the caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source SPD Selector is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size is less than size of Source SPD Selector.
  @retval EFI_SUCCESS            Copy Source SPD Selector to the Destination SPD
                                 Selector successfully.

**/
EFI_STATUS
DuplicateSpdSelector (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  );

/**
  Copy Source SA ID to the Destination SA ID.

  @param[in, out] DstSel             Pointer of the Destination SA ID.
  @param[in]      SrcSel             Pointer of the Source SA ID.
  @param[in, out] Size               The size of the Destination SA ID. If it
                                     not NULL, and its value is less than the size of
                                     Source SA ID, the value of Source SA ID's size
                                     will be passed to the caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source SA ID is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size less than size of source SA ID.
  @retval EFI_SUCCESS            Copied Source SA ID to the Destination SA ID successfully.

**/
EFI_STATUS
DuplicateSaId (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  );

/**
  Copy Source PAD ID to the Destination PAD ID.

  @param[in, out] DstSel             Pointer of Destination PAD ID.
  @param[in]      SrcSel             Pointer of Source PAD ID.
  @param[in, out] Size               The size of the Destination PAD ID. If it
                                     not NULL, and its value less than the size of
                                     Source PAD ID, the value of Source PAD ID's size
                                     will be passed to the caller by this parameter.

  @retval EFI_INVALID_PARAMETER  If the Destination or Source PAD ID is NULL.
  @retval EFI_BUFFER_TOO_SMALL   If the input Size less than size of source PAD ID.
  @retval EFI_SUCCESS            Copied Source PAD ID to the Destination PAD ID successfully.

**/
EFI_STATUS
DuplicatePadId (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *DstSel,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *SrcSel,
  IN OUT UINTN                        *Size
  );

/**
  Fix the value of some members of the  SPD Selector.

  This function is called by IpSecCopyPolicyEntry(), which copies the Policy
  Entry into the Variable. Since some members in SPD Selector are pointers,
  a physical address to relative address conversion is required before copying
  this SPD entry into the variable.

  @param[in]       Selector              Pointer of SPD Selector.
  @param[in, out]  Data                  Pointer of SPD Data.

**/
VOID
FixSpdEntry (
  IN     EFI_IPSEC_SPD_SELECTOR            *Selector,
  IN OUT EFI_IPSEC_SPD_DATA                *Data
  );

/**
  Fix the value of some members of SA ID.

  This function is called by IpSecCopyPolicyEntry(), which copies the Policy
  Entry into the Variable. Since some members in SA ID are pointers,
  a physical address to relative address conversion is required before copying
  this SAD into the variable.

  @param[in]       SaId              Pointer of SA ID.
  @param[in, out]  Data              Pointer of SA Data.

**/
VOID
FixSadEntry (
  IN     EFI_IPSEC_SA_ID                  *SaId,
  IN OUT EFI_IPSEC_SA_DATA2                *Data
  );

/**
  Fix the value of some members of PAD ID.

  This function is called by IpSecCopyPolicyEntry(), which copy the Policy
  Entry into the Variable. Since some members in PAD ID are pointers,
  a physical address to relative address conversion is required before copying
  this PAD into the variable.

  @param[in]       PadId              Pointer of PAD ID.
  @param[in, out]  Data               Pointer of PAD Data.

**/
VOID
FixPadEntry (
  IN     EFI_IPSEC_PAD_ID                  *PadId,
  IN OUT EFI_IPSEC_PAD_DATA                *Data
  );

/**
  Recover the value of some members of SPD Selector.

  This function is corresponding to FixSpdEntry(). It recovers the value of members
  of SPD Selector which fix by the FixSpdEntry().

  @param[in, out]  Selector              Pointer of SPD Selector.
  @param[in, out]  Data                  Pointer of SPD Data.

**/
VOID
UnfixSpdEntry (
  IN OUT EFI_IPSEC_SPD_SELECTOR           *Selector,
  IN OUT EFI_IPSEC_SPD_DATA               *Data
  );


/**
  Recover the value of some members of SA ID.

  This function is corresponding to FixSadEntry(). It recovers the value of members
  of SAD ID which fix by the FixSadEntry().

  @param[in, out]       SaId              Pointer of SAD ID
  @param[in, out]  Data              Pointer of SAD Data.

**/
VOID
UnfixSadEntry (
  IN OUT EFI_IPSEC_SA_ID                     *SaId,
  IN OUT EFI_IPSEC_SA_DATA2                   *Data
  );

/**
  Recover the value of some members of PAD ID.

  This function is corresponding to FixPadEntry(). It recovers the value of members
  of PAD ID which fix by the FixPadEntry().

  @param[in]       PadId              Pointer of PAD ID
  @param[in, out]  Data               Pointer of PAD Data.

**/
VOID
UnfixPadEntry (
  IN     EFI_IPSEC_PAD_ID                 *PadId,
  IN OUT EFI_IPSEC_PAD_DATA               *Data
  );

/**
  Set the security policy information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated
                                 configuration data specified by DataType.
                                 A NULL Selector causes the entire specified-type
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure
                                 of the data buffer should be EFI_IPSEC_SPD_DATA.
  @param[in]  Context            Pointer to one entry selector that describes
                                 the expected position the new data entry will
                                 be added. If Context is NULL,the new entry will
                                 be appended the end of database.

  @retval EFI_INVALID_PARAMETER  One or more of the following are TRUE:
                                   - Selector is not NULL and its LocalAddress
                                     is NULL or its RemoteAddress is NULL.
                                   - Data is not NULL, its Action is Protected,
                                     and its policy is NULL.
                                   - Data is not NULL and its Action is not protected
                                     and its policy is not NULL.
                                   - The Action of Data is Protected, its policy
                                     mode is Tunnel, and its tunnel option is NULL.
                                   - The Action of Data is protected, its policy
                                     mode is not Tunnel, and it tunnel option is not NULL.
  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetSpdEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  );

/**
  Set the security association information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated
                                 configuration data specified by DataType.
                                 A NULL Selector causes the entire specified-type
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure
                                 of the data buffer should be EFI_IPSEC_SA_DATA.
  @param[in]  Context            Pointer to one entry selector which describes
                                 the expected position the new data entry will
                                 be added. If Context is NULL,the new entry will
                                 be appended to the end of database.

  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetSadEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  );

/**
  Set the peer authorization configuration information for the EFI IPsec driver.

  The IPsec configuration data has a unique selector/identifier separately to
  identify a data entry.

  @param[in]  Selector           Pointer to an entry selector on operated
                                 configuration data specified by DataType.
                                 A NULL Selector causes the entire specified-type
                                 configuration information to be flushed.
  @param[in]  Data               The data buffer to be set. The structure
                                 of the data buffer should be EFI_IPSEC_PAD_DATA.
  @param[in]  Context            Pointer to one entry selector that describes
                                 the expected position where the new data entry will
                                 be added. If Context is NULL, the new entry will
                                 be appended the end of database.

  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.
  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.

**/
EFI_STATUS
SetPadEntry (
  IN EFI_IPSEC_CONFIG_SELECTOR       *Selector,
  IN VOID                            *Data,
  IN VOID                            *Context OPTIONAL
  );

/**
  This function looks up the data entry from IPsec SPD, and returns the configuration
  value of the specified SPD Entry.

  @param[in]      Selector      Pointer to an entry selector which is an identifier
                                of the SPD entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec
                                configuration data. The type of the data buffer
                                is associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER Data is NULL and *DataSize is not zero.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetSpdEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN OUT UINTN                        *DataSize,
     OUT VOID                         *Data
  );

/**
  This function looks up the data entry from IPsec SAD and returns the configuration
  value of the specified SAD Entry.

  @param[in]      Selector      Pointer to an entry selector that is an identifier
                                of the SAD entry.
  @param[in, out] DataSize      On output, the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec
                                configuration data. This type of the data buffer
                                is associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetSadEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR   *Selector,
  IN OUT UINTN                       *DataSize,
     OUT VOID                        *Data
  );

/**
  This function looks up the data entry from IPsec PADand returns the configuration
  value of the specified PAD Entry.

  @param[in]      Selector      Pointer to an entry selector that  is an identifier
                                of the PAD entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec
                                configuration data. This type of the data buffer
                                is associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
GetPadEntry (
  IN     EFI_IPSEC_CONFIG_SELECTOR   *Selector,
  IN OUT UINTN                       *DataSize,
     OUT VOID                        *Data
  );

/**
  Return the configuration value for the EFI IPsec driver.

  This function lookup the data entry from IPsec database or IKEv2 configuration
  information. The expected data type and unique identification are described in
  DataType and Selector parameters.

  @param[in]      This          Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in]      DataType      The type of data to retrieve.
  @param[in]      Selector      Pointer to an entry selector that is an identifier of the IPsec
                                configuration data entry.
  @param[in, out] DataSize      On output the size of data returned in Data.
  @param[out]     Data          The buffer to return the contents of the IPsec configuration data.
                                The type of the data buffer is associated with the DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - This is NULL.
                                - Selector is NULL.
                                - DataSize is NULL.
                                - Data is NULL and *DataSize is not zero
  @retval EFI_NOT_FOUND         The configuration data specified by Selector is not found.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the result. DataSize has been
                                updated with the size needed to complete the request.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigGetData (
  IN     EFI_IPSEC_CONFIG_PROTOCOL    *This,
  IN     EFI_IPSEC_CONFIG_DATA_TYPE   DataType,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *Selector,
  IN OUT UINTN                        *DataSize,
     OUT VOID                         *Data
  );

/**
  Set the security association, security policy and peer authorization configuration
  information for the EFI IPsec driver.

  This function is used to set the IPsec configuration information of type DataType for
  the EFI IPsec driver.
  The IPsec configuration data has a unique selector/identifier separately to identify
  a data entry. The selector structure depends on DataType's definition.
  Using SetData() with a Data of NULL causes the IPsec configuration data entry identified
  by DataType and Selector to be deleted.

  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The type of data to be set.
  @param[in] Selector           Pointer to an entry selector on operated configuration data
                                specified by DataType. A NULL Selector causes the entire
                                specified-type configuration information to be flushed.
  @param[in] Data               The data buffer to be set. The structure of the data buffer is
                                associated with the DataType.
  @param[in] InsertBefore       Pointer to one entry selector which describes the expected
                                position the new data entry will be added. If InsertBefore is NULL,
                                the new entry will be appended the end of database.

  @retval EFI_SUCCESS           The specified configuration entry data was set successfully.
  @retval EFI_INVALID_PARAMETER One or more of the following are TRUE:
                                - This is NULL.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_OUT_OF_RESOURCED  The required system resource could not be allocated.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigSetData (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_IPSEC_CONFIG_SELECTOR        *Selector,
  IN VOID                             *Data,
  IN EFI_IPSEC_CONFIG_SELECTOR        *InsertBefore OPTIONAL
  );

/**
  Enumerates the current selector for IPsec configuration data entry.

  This function is called multiple times to retrieve the entry Selector in IPsec
  configuration database. On each call to GetNextSelector(), the next entry
  Selector are retrieved into the output interface.

  If the entire IPsec configuration database has been iterated, the error
  EFI_NOT_FOUND is returned.
  If the Selector buffer is too small for the next Selector copy, an
  EFI_BUFFER_TOO_SMALL error is returned, and SelectorSize is updated to reflect
  the size of buffer needed.

  On the initial call to GetNextSelector() to start the IPsec configuration database
  search, a pointer to the buffer with all zero value is passed in Selector. Calls
  to SetData() between calls to GetNextSelector may produce unpredictable results.

  @param[in]      This          Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in]      DataType      The type of IPsec configuration data to retrieve.
  @param[in, out] SelectorSize  The size of the Selector buffer.
  @param[in, out] Selector      On input, supplies the pointer to last Selector that was
                                returned by GetNextSelector().
                                On output, returns one copy of the current entry Selector
                                of a given DataType.

  @retval EFI_SUCCESS           The specified configuration data was obtained successfully.
  @retval EFI_INVALID_PARAMETER One or more of the followings are TRUE:
                                - This is NULL.
                                - SelectorSize is NULL.
                                - Selector is NULL.
  @retval EFI_NOT_FOUND         The next configuration data entry was not found.
  @retval EFI_UNSUPPORTED       The specified DataType is not supported.
  @retval EFI_BUFFER_TOO_SMALL  The SelectorSize is too small for the result. This parameter
                                has been updated with the size needed to complete the search
                                request.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigGetNextSelector (
  IN     EFI_IPSEC_CONFIG_PROTOCOL    *This,
  IN     EFI_IPSEC_CONFIG_DATA_TYPE   DataType,
  IN OUT UINTN                        *SelectorSize,
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *Selector
  );

/**
  Register an event that is to be signaled whenever a configuration process on the
  specified IPsec configuration information is done.

  The register function is not surpport now and always returns EFI_UNSUPPORTED.

  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The type of data to be registered the event for.
  @param[in] Event              The event to be registered.

  @retval EFI_SUCCESS           The event is registered successfully.
  @retval EFI_INVALID_PARAMETER This is NULL, or Event is NULL.
  @retval EFI_ACCESS_DENIED     The Event is already registered for the DataType.
  @retval EFI_UNSUPPORTED       The notify registration unsupported, or the specified
                                DataType is not supported.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigRegisterNotify (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_EVENT                        Event
  );


/**
  Remove the specified event that was previously registered on the specified IPsec
  configuration data.

  This function is not supported now and always returns EFI_UNSUPPORTED.

  @param[in] This               Pointer to the EFI_IPSEC_CONFIG_PROTOCOL instance.
  @param[in] DataType           The configuration data type to remove the registered event for.
  @param[in] Event              The event to be unregistered.

  @retval EFI_SUCCESS           The event was removed successfully.
  @retval EFI_NOT_FOUND         The Event specified by DataType could not be found in the
                                database.
  @retval EFI_INVALID_PARAMETER This is NULL or Event is NULL.
  @retval EFI_UNSUPPORTED       The notify registration unsupported or the specified
                                DataType is not supported.

**/
EFI_STATUS
EFIAPI
EfiIpSecConfigUnregisterNotify (
  IN EFI_IPSEC_CONFIG_PROTOCOL        *This,
  IN EFI_IPSEC_CONFIG_DATA_TYPE       DataType,
  IN EFI_EVENT                        Event
  );

extern LIST_ENTRY   mConfigData[IPsecConfigDataTypeMaximum];

#endif
