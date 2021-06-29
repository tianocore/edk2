## @file
# Network DSC include file for [Defines] section of all Architectures.
#
# This file can be included to the [Defines] section of a platform DSC file by
# using "!include NetworkPkg/NetworkDefines.dsc.inc" to set default value of
# flags if they are not defined somewhere else, and also check the value to see
# if there is any conflict.
#
# These flags can be defined before the !include line, or changed on the command
# line to enable or disable related feature support.
#   -D FLAG=VALUE
# The default value of these flags are:
#   DEFINE NETWORK_ENABLE                 = TRUE
#   DEFINE NETWORK_SNP_ENABLE             = TRUE
#   DEFINE NETWORK_IP4_ENABLE             = TRUE
#   DEFINE NETWORK_IP6_ENABLE             = TRUE
#   DEFINE NETWORK_TLS_ENABLE             = TRUE
#   DEFINE NETWORK_HTTP_ENABLE            = FALSE
#   DEFINE NETWORK_HTTP_BOOT_ENABLE       = TRUE
#   DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = FALSE
#   DEFINE NETWORK_ISCSI_ENABLE           = FALSE
#   DEFINE NETWORK_ISCSI_MD5_ENABLE       = TRUE
#   DEFINE NETWORK_VLAN_ENABLE            = TRUE
#
# Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
# (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!ifndef NETWORK_ENABLE
  #
  # This flag is to enable or disable the whole network stack.
  #
  DEFINE NETWORK_ENABLE = TRUE
!endif

!ifndef NETWORK_SNP_ENABLE
  #
  # This flag is to include the common SNP driver or not.
  #
  DEFINE NETWORK_SNP_ENABLE = TRUE
!endif

!ifndef NETWORK_VLAN_ENABLE
  #
  # This flag is to enable or disable VLAN feature.
  #
  DEFINE NETWORK_VLAN_ENABLE = TRUE
!endif

!ifndef NETWORK_IP4_ENABLE
  #
  # This flag is to enable or disable IPv4 network stack.
  #
  DEFINE NETWORK_IP4_ENABLE = TRUE
!endif

!ifndef NETWORK_IP6_ENABLE
  #
  # This flag is to enable or disable IPv6 network stack.
  #
  DEFINE NETWORK_IP6_ENABLE = TRUE
!endif

!ifndef NETWORK_TLS_ENABLE
  #
  # This flag is to enable or disable TLS feature.
  #
  # Note: This feature depends on the OpenSSL building. To enable this feature, please
  #       follow the instructions found in the file "OpenSSL-HOWTO.txt" located in
  #       CryptoPkg\Library\OpensslLib to enable the OpenSSL building first.
  #       The OpensslLib.inf library instance should be used since libssl is required.
  #
  DEFINE NETWORK_TLS_ENABLE = TRUE
!endif

!ifndef NETWORK_HTTP_ENABLE
  #
  # This flag is to enable or disable HTTP(S) feature.
  # The default is set to FALSE to not affecting the existing
  # platforms.
  # NETWORK_HTTP_ENABLE set to FALSE is not affecting NETWORK_HTTP_BOOT_ENABLE
  # when NETWORK_HTTP_BOOT_ENABLE is TRUE.
  DEFINE NETWORK_HTTP_ENABLE = FALSE
!endif

!ifndef NETWORK_HTTP_BOOT_ENABLE
  #
  # This flag is to enable or disable HTTP(S) boot feature.
  #
  #
  DEFINE NETWORK_HTTP_BOOT_ENABLE = TRUE
!endif

!ifndef NETWORK_ALLOW_HTTP_CONNECTIONS
  #
  # Indicates whether HTTP connections (i.e., unsecured) are permitted or not.
  #
  # Note: If NETWORK_ALLOW_HTTP_CONNECTIONS is TRUE, HTTP connections are allowed.
  #       Both the "https://" and "http://" URI schemes are permitted. Otherwise, HTTP
  #       connections are denied. Only the "https://" URI scheme is permitted.
  #
  DEFINE NETWORK_ALLOW_HTTP_CONNECTIONS = FALSE
!endif

!ifndef NETWORK_ISCSI_ENABLE
  #
  # This flag is to enable or disable iSCSI feature.
  #
  # Note: This feature depends on the OpenSSL building. To enable this feature, please
  #       follow the instructions found in the file "OpenSSL-HOWTO.txt" located in
  #       CryptoPkg\Library\OpensslLib to enable the OpenSSL building first.
  #       Both OpensslLib.inf and OpensslLibCrypto.inf library instance can be used
  #       since libssl is not required for iSCSI.
  #
  DEFINE NETWORK_ISCSI_ENABLE = FALSE
!endif

!ifndef NETWORK_ISCSI_MD5_ENABLE
  #
  # This flag enables the deprecated MD5 hash algorithm in iSCSI CHAP
  # authentication.
  #
  # Note: The NETWORK_ISCSI_MD5_ENABLE flag only makes a difference if
  #       NETWORK_ISCSI_ENABLE is TRUE; otherwise, NETWORK_ISCSI_MD5_ENABLE is
  #       ignored.
  #
  #       With NETWORK_ISCSI_MD5_ENABLE set to TRUE, MD5 is enabled as the
  #       least preferred CHAP hash algorithm. With NETWORK_ISCSI_MD5_ENABLE
  #       set to FALSE, MD5 is disabled statically, at build time.
  #
  #       The default value is TRUE, because RFC 7143 mandates MD5, and because
  #       several vendors' iSCSI targets only support MD5, for CHAP.
  #
  DEFINE NETWORK_ISCSI_MD5_ENABLE = TRUE
!endif

!if $(NETWORK_ENABLE) == TRUE
  #
  # Check the flags to see if there is any conflict.
  #
  !if ($(NETWORK_IP4_ENABLE) == FALSE) AND ($(NETWORK_IP6_ENABLE) == FALSE)
    !error "Must enable at least IP4 or IP6 stack if NETWORK_ENABLE is set to TRUE!"
  !endif

  !if ($(NETWORK_HTTP_BOOT_ENABLE) == TRUE) OR ($(NETWORK_HTTP_ENABLE) == TRUE)
    !if ($(NETWORK_TLS_ENABLE) == FALSE) AND ($(NETWORK_ALLOW_HTTP_CONNECTIONS) == FALSE)
      !error "Must enable TLS to support HTTPS, or allow unsecured HTTP connection, if NETWORK_HTTP_BOOT_ENABLE or NETWORK_HTTP_ENABLE is set to TRUE!"
    !endif
  !endif
!endif
