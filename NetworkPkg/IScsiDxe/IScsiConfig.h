/** @file
  The header file of functions for configuring or getting the parameters
  relating to iSCSI.

Copyright (c) 2004 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ISCSI_CONFIG_H_
#define _ISCSI_CONFIG_H_

#include "IScsiConfigNVDataStruc.h"

typedef struct _ISCSI_FORM_CALLBACK_INFO ISCSI_FORM_CALLBACK_INFO;

extern UINT8                     IScsiConfigVfrBin[];
extern UINT8                     IScsiDxeStrings[];
extern ISCSI_FORM_CALLBACK_INFO  *mCallbackInfo;

#define VAR_OFFSET(Field)    \
  ((UINT16) ((UINTN) &(((ISCSI_CONFIG_IFR_NVDATA *) 0)->Field)))

#define QUESTION_ID(Field)   \
  ((UINT16) (VAR_OFFSET (Field) + CONFIG_OPTION_OFFSET))

#define DYNAMIC_ONE_OF_VAR_OFFSET         VAR_OFFSET  (Enabled)
#define DYNAMIC_ORDERED_LIST_QUESTION_ID  QUESTION_ID (DynamicOrderedList)
#define DYNAMIC_ORDERED_LIST_VAR_OFFSET   VAR_OFFSET  (DynamicOrderedList)
#define ATTEMPT_DEL_QUESTION_ID           QUESTION_ID (DeleteAttemptList)
#define ATTEMPT_DEL_VAR_OFFSET            VAR_OFFSET  (DeleteAttemptList)
#define ATTEMPT_ADD_QUESTION_ID           QUESTION_ID (AddAttemptList)
#define ATTEMPT_ADD_VAR_OFFSET            VAR_OFFSET  (AddAttemptList)

//
// Define QuestionId and OffSet for Keywords.
//
#define ATTEMPT_MAC_ADDR_VAR_OFFSET                 VAR_OFFSET  (ISCSIMacAddr)
#define ATTEMPT_ATTEMPT_NAME_QUESTION_ID            QUESTION_ID (ISCSIAttemptName)
#define ATTEMPT_ATTEMPT_NAME_VAR_OFFSET             VAR_OFFSET  (ISCSIAttemptName)
#define ATTEMPT_BOOTENABLE_QUESTION_ID              QUESTION_ID (ISCSIBootEnableList)
#define ATTEMPT_BOOTENABLE_VAR_OFFSET               VAR_OFFSET  (ISCSIBootEnableList)
#define ATTEMPT_ADDRESS_TYPE_QUESTION_ID            QUESTION_ID (ISCSIIpAddressTypeList)
#define ATTEMPT_ADDRESS_TYPE_VAR_OFFSET             VAR_OFFSET  (ISCSIIpAddressTypeList)
#define ATTEMPT_CONNECT_RETRY_QUESTION_ID           QUESTION_ID (ISCSIConnectRetry)
#define ATTEMPT_CONNECT_RETRY_VAR_OFFSET            VAR_OFFSET  (ISCSIConnectRetry)
#define ATTEMPT_CONNECT_TIMEOUT_QUESTION_ID         QUESTION_ID (ISCSIConnectTimeout)
#define ATTEMPT_CONNECT_TIMEOUT_VAR_OFFSET          VAR_OFFSET  (ISCSIConnectTimeout)
#define ATTEMPT_ISID_QUESTION_ID                    QUESTION_ID (Keyword->ISCSIIsId)
#define ATTEMPT_ISID_VAR_OFFSET                     VAR_OFFSET  (Keyword->ISCSIIsId)
#define ATTEMPT_INITIATOR_VIA_DHCP_QUESTION_ID      QUESTION_ID (ISCSIInitiatorInfoViaDHCP)
#define ATTEMPT_INITIATOR_VIA_DHCP_VAR_OFFSET       VAR_OFFSET  (ISCSIInitiatorInfoViaDHCP)
#define ATTEMPT_INITIATOR_IP_ADDRESS_QUESTION_ID    QUESTION_ID (Keyword->ISCSIInitiatorIpAddress)
#define ATTEMPT_INITIATOR_IP_ADDRESS_VAR_OFFSET     VAR_OFFSET  (Keyword->ISCSIInitiatorIpAddress)
#define ATTEMPT_INITIATOR_NET_MASK_QUESTION_ID      QUESTION_ID (Keyword->ISCSIInitiatorNetmask)
#define ATTEMPT_INITIATOR_NET_MASK_VAR_OFFSET       VAR_OFFSET  (Keyword->ISCSIInitiatorNetmask)
#define ATTEMPT_INITIATOR_GATE_WAY_QUESTION_ID      QUESTION_ID (Keyword->ISCSIInitiatorGateway)
#define ATTEMPT_INITIATOR_GATE_WAY_VAR_OFFSET       VAR_OFFSET  (Keyword->ISCSIInitiatorGateway)
#define ATTEMPT_TARGET_VIA_DHCP_QUESTION_ID         QUESTION_ID (ISCSITargetInfoViaDHCP)
#define ATTEMPT_TARGET_VIA_DHCP_VAR_OFFSET          VAR_OFFSET  (ISCSITargetInfoViaDHCP)
#define ATTEMPT_TARGET_NAME_QUESTION_ID             QUESTION_ID (Keyword->ISCSITargetName)
#define ATTEMPT_TARGET_NAME_VAR_OFFSET              VAR_OFFSET  (Keyword->ISCSITargetName)
#define ATTEMPT_TARGET_IP_ADDRESS_QUESTION_ID       QUESTION_ID (Keyword->ISCSITargetIpAddress)
#define ATTEMPT_TARGET_IP_ADDRESS_VAR_OFFSET        VAR_OFFSET  (Keyword->ISCSITargetIpAddress)
#define ATTEMPT_TARGET_TCP_PORT_QUESTION_ID         QUESTION_ID (ISCSITargetTcpPort)
#define ATTEMPT_TARGET_TCP_PORT_VAR_OFFSET          VAR_OFFSET  (ISCSITargetTcpPort)
#define ATTEMPT_LUN_QUESTION_ID                     QUESTION_ID (Keyword->ISCSILun)
#define ATTEMPT_LUN_VAR_OFFSET                      VAR_OFFSET  (Keyword->ISCSILun)
#define ATTEMPT_AUTHENTICATION_METHOD_QUESTION_ID   QUESTION_ID (ISCSIAuthenticationMethod)
#define ATTEMPT_AUTHENTICATION_METHOD_VAR_OFFSET    VAR_OFFSET  (ISCSIAuthenticationMethod)
#define ATTEMPT_CHARTYPE_QUESTION_ID                QUESTION_ID (ISCSIChapType)
#define ATTEMPT_CHARTYPE_VAR_OFFSET                 VAR_OFFSET  (ISCSIChapType)
#define ATTEMPT_CHAR_USER_NAME_QUESTION_ID          QUESTION_ID (Keyword->ISCSIChapUsername)
#define ATTEMPT_CHAR_USER_NAME_VAR_OFFSET           VAR_OFFSET  (Keyword->ISCSIChapUsername)
#define ATTEMPT_CHAR_SECRET_QUESTION_ID             QUESTION_ID (Keyword->ISCSIChapSecret)
#define ATTEMPT_CHAR_SECRET_VAR_OFFSET              VAR_OFFSET  (Keyword->ISCSIChapSecret)
#define ATTEMPT_CHAR_REVERSE_USER_NAME_QUESTION_ID  QUESTION_ID (Keyword->ISCSIReverseChapUsername)
#define ATTEMPT_CHAR_REVERSE_USER_NAME_VAR_OFFSET   VAR_OFFSET  (Keyword->ISCSIReverseChapUsername)
#define ATTEMPT_CHAR_REVERSE_SECRET_QUESTION_ID     QUESTION_ID (Keyword->ISCSIReverseChapSecret)
#define ATTEMPT_CHAR_REVERSE_SECRET_VAR_OFFSET      VAR_OFFSET  (Keyword->ISCSIReverseChapSecret)

#define ISCSI_INITATOR_NAME_VAR_NAME  L"I_NAME"

#define ISCSI_CONFIG_VAR_ATTR  (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_NON_VOLATILE)

#define ISCSI_FORM_CALLBACK_INFO_SIGNATURE  SIGNATURE_32 ('I', 'f', 'c', 'i')

#define ISCSI_FORM_CALLBACK_INFO_FROM_FORM_CALLBACK(Callback) \
  CR ( \
  Callback, \
  ISCSI_FORM_CALLBACK_INFO, \
  ConfigAccess, \
  ISCSI_FORM_CALLBACK_INFO_SIGNATURE \
  )

#pragma pack(1)
struct _ISCSI_ATTEMPT_CONFIG_NVDATA {
  LIST_ENTRY                     Link;
  UINT8                          NicIndex;
  UINT8                          AttemptConfigIndex;
  BOOLEAN                        DhcpSuccess;
  BOOLEAN                        ValidiBFTPath;
  BOOLEAN                        ValidPath;
  UINT8                          AutoConfigureMode;
  EFI_STRING_ID                  AttemptTitleToken;
  EFI_STRING_ID                  AttemptTitleHelpToken;
  CHAR8                          AttemptName[ATTEMPT_NAME_SIZE];
  CHAR8                          MacString[ISCSI_MAX_MAC_STRING_LEN];
  EFI_IP_ADDRESS                 PrimaryDns;
  EFI_IP_ADDRESS                 SecondaryDns;
  EFI_IP_ADDRESS                 DhcpServer;
  ISCSI_SESSION_CONFIG_NVDATA    SessionConfigData;
  UINT8                          AuthenticationType;
  union {
    ISCSI_CHAP_AUTH_CONFIG_NVDATA    CHAP;
  } AuthConfigData;
  BOOLEAN                        AutoConfigureSuccess;
  UINT8                          Actived;
};

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

#pragma pack()

struct _ISCSI_FORM_CALLBACK_INFO {
  UINT32                            Signature;
  EFI_HANDLE                        DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  UINT16                            *KeyList;
  VOID                              *FormBuffer;
  EFI_HII_HANDLE                    RegisteredHandle;
  ISCSI_ATTEMPT_CONFIG_NVDATA       *Current;
};

/**
  Create Hii Extend Label OpCode as the start opcode and end opcode. It is
  a help function.

  @param[in]  StartLabelNumber   The number of start label.
  @param[out] StartOpCodeHandle  Points to the start opcode handle.
  @param[out] StartLabel         Points to the created start opcode.
  @param[out] EndOpCodeHandle    Points to the end opcode handle.
  @param[out] EndLabel           Points to the created end opcode.

  @retval EFI_OUT_OF_RESOURCES   Do not have sufficient resource to finish this
                                 operation.
  @retval EFI_INVALID_PARAMETER  Any input parameter is invalid.
  @retval EFI_SUCCESS            The operation is completed successfully.

**/
EFI_STATUS
IScsiCreateOpCode (
  IN  UINT16              StartLabelNumber,
  OUT VOID                **StartOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL  **StartLabel,
  OUT VOID                **EndOpCodeHandle,
  OUT EFI_IFR_GUID_LABEL  **EndLabel
  );

/**
  Initialize the iSCSI configuration form.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is initialized.
  @retval EFI_OUT_OF_RESOURCES    Failed to allocate memory.

**/
EFI_STATUS
IScsiConfigFormInit (
  IN EFI_HANDLE  DriverBindingHandle
  );

/**
  Unload the iSCSI configuration form, this includes: delete all the iSCSI
  configuration entries, uninstall the form callback protocol, and
  free the resources used.

  @param[in]  DriverBindingHandle The iSCSI driverbinding handle.

  @retval EFI_SUCCESS             The iSCSI configuration form is unloaded.
  @retval Others                  Failed to unload the form.

**/
EFI_STATUS
IScsiConfigFormUnload (
  IN EFI_HANDLE  DriverBindingHandle
  );

/**
  Update the MAIN form to display the configured attempts.

**/
VOID
IScsiConfigUpdateAttempt (
  VOID
  );

/**
  Get the attempt config data from global structure by the ConfigIndex.

  @param[in]  AttemptConfigIndex     The unique index indicates the attempt.

  @return       Pointer to the attempt config data.
  @retval NULL  The attempt configuration data can not be found.

**/
ISCSI_ATTEMPT_CONFIG_NVDATA *
IScsiConfigGetAttemptByConfigIndex (
  IN UINT8  AttemptConfigIndex
  );

#endif
