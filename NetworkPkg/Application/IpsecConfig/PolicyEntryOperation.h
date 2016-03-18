/** @file
  The function declaration of policy entry operation in IpSecConfig application.

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _POLICY_ENTRY_OPERATION_H_
#define _POLICY_ENTRY_OPERATION_H_

#define LOCAL              BIT(0)
#define REMOTE             BIT(1)
#define PROTO              BIT(2)
#define LOCAL_PORT         BIT(3)
#define REMOTE_PORT        BIT(4)
#define ICMP_TYPE          BIT(5)
#define ICMP_CODE          BIT(6)
#define NAME               BIT(7)
#define PACKET_FLAG        BIT(8)
#define ACTION             BIT(9)
#define EXT_SEQUENCE       BIT(10)
#define SEQUENCE_OVERFLOW  BIT(11)
#define FRAGMENT_CHECK     BIT(12)
#define LIFEBYTE           BIT(13)
#define LIFETIME_SOFT      BIT(14)
#define LIFETIME           BIT(15)
#define MODE               BIT(16)
#define TUNNEL_LOCAL       BIT(17)
#define TUNNEL_REMOTE      BIT(18)
#define DONT_FRAGMENT      BIT(19)
#define IPSEC_PROTO        BIT(20)
#define AUTH_ALGO          BIT(21)
#define ENCRYPT_ALGO       BIT(22)
#define SPI                BIT(23)
#define DEST               BIT(24)
#define SEQUENCE_NUMBER    BIT(25)
#define ANTIREPLAY_WINDOW  BIT(26)
#define AUTH_KEY           BIT(27)
#define ENCRYPT_KEY        BIT(28)
#define PATH_MTU           BIT(29)
#define SOURCE             BIT(30)

#define PEER_ID            BIT(0)
#define PEER_ADDRESS       BIT(1)
#define AUTH_PROTO         BIT(2)
#define AUTH_METHOD        BIT(3)
#define IKE_ID             BIT(4)
#define AUTH_DATA          BIT(5)
#define REVOCATION_DATA    BIT(6)

typedef struct {
  EFI_IPSEC_CONFIG_DATA_TYPE    DataType;
  EFI_IPSEC_CONFIG_SELECTOR     *Selector;    // Data to be inserted.
  VOID                          *Data;
  UINT32                        Mask;
  POLICY_ENTRY_INDEXER          Indexer;
  EFI_STATUS                    Status;       // Indicate whether insertion succeeds.
} EDIT_POLICY_ENTRY_CONTEXT;

typedef struct {
  EFI_IPSEC_CONFIG_DATA_TYPE    DataType;
  EFI_IPSEC_CONFIG_SELECTOR     *Selector;    // Data to be inserted.
  VOID                          *Data;
  POLICY_ENTRY_INDEXER          Indexer;
  EFI_STATUS                    Status;       // Indicate whether insertion succeeds.
} INSERT_POLICY_ENTRY_CONTEXT;

/**
  The prototype for the CreateSpdEntry()/CreateSadEntry()/CreatePadEntry().
  Fill in EFI_IPSEC_CONFIG_SELECTOR and corresponding data thru ParamPackage list.

  @param[out] Selector        The pointer to the EFI_IPSEC_CONFIG_SELECTOR union.
  @param[out] Data            The pointer to corresponding data.
  @param[in]  ParamPackage    The pointer to the ParamPackage list.
  @param[out] Mask            The pointer to the Mask.
  @param[in]  CreateNew       The switch to create new.

  @retval EFI_SUCCESS              Filled in EFI_IPSEC_CONFIG_SELECTOR and corresponding data successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
typedef
EFI_STATUS
(*CREATE_POLICY_ENTRY) (
  OUT EFI_IPSEC_CONFIG_SELECTOR    **Selector,
  OUT VOID                         **Data,
  IN  LIST_ENTRY                   *ParamPackage,
  OUT UINT32                       *Mask,
  IN  BOOLEAN                      CreateNew
  );

/**
  The prototype for the CombineSpdEntry()/CombineSadEntry()/CombinePadEntry().
  Combine old SPD/SAD/PAD entry with new SPD/SAD/PAD entry.

  @param[in, out] OldSelector    The pointer to the old EFI_IPSEC_CONFIG_SELECTOR union.
  @param[in, out] OldData        The pointer to the corresponding old data.
  @param[in]      NewSelector    The pointer to the new EFI_IPSEC_CONFIG_SELECTOR union.
  @param[in]      NewData        The pointer to the corresponding new data.
  @param[in]      Mask           The pointer to the Mask.
  @param[out]     CreateNew      The switch to create new.

  @retval EFI_SUCCESS              Combined successfully.
  @retval EFI_INVALID_PARAMETER    Invalid user input parameter.

**/
typedef
EFI_STATUS
(* COMBINE_POLICY_ENTRY) (
  IN OUT EFI_IPSEC_CONFIG_SELECTOR    *OldSelector,
  IN OUT VOID                         *OldData,
  IN     EFI_IPSEC_CONFIG_SELECTOR    *NewSelector,
  IN     VOID                         *NewData,
  IN     UINT32                       Mask,
     OUT BOOLEAN                      *CreateNew
  );

/**
  Insert or add entry information in database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS             Insert or add entry information successfully.
  @retval EFI_NOT_FOUND           Can't find the specified entry.
  @retval EFI_BUFFER_TOO_SMALL    The entry already existed.
  @retval EFI_UNSUPPORTED         The operation is not supported./
  @retval Others                  Some mistaken case.
**/
EFI_STATUS
AddOrInsertPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  );

/**
  Edit entry information in the database according to datatype.

  @param[in] DataType        The value of EFI_IPSEC_CONFIG_DATA_TYPE.
  @param[in] ParamPackage    The pointer to the ParamPackage list.

  @retval EFI_SUCCESS             Edit entry information successfully.
  @retval EFI_NOT_FOUND           Can't find the specified entry.
  @retval Others                  Some mistaken case.
**/
EFI_STATUS
EditPolicyEntry (
  IN EFI_IPSEC_CONFIG_DATA_TYPE    DataType,
  IN LIST_ENTRY                    *ParamPackage
  );
#endif
