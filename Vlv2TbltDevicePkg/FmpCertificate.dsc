#/** @file
# FMP Certificates shared by multiple FmpDxe drivers for firmware update.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
#
#**/

!if $(CAPSULE_PKCS7_CERT) == SAMPLE_DEVELOPMENT_SAMPLE_PRODUCTION
  !include Vlv2TbltDevicePkg/Feature/Capsule/GenerateCapsule/SAMPLE_DEVELOPMENT_SAMPLE_PRODUCTION.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
!endif
!if $(CAPSULE_PKCS7_CERT) == SAMPLE_DEVELOPMENT
  !include Vlv2TbltDevicePkg/Feature/Capsule/GenerateCapsule/SAMPLE_DEVELOPMENT.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
!endif
!if $(CAPSULE_PKCS7_CERT) == EDKII_TEST
  !include BaseTools/Source/Python/Pkcs7Sign/TestRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
!endif
!if $(CAPSULE_PKCS7_CERT) == NEW_ROOT
  !include Vlv2TbltDevicePkg/Feature/Capsule/GenerateCapsule/NewRoot.cer.gFmpDevicePkgTokenSpaceGuid.PcdFmpDevicePkcs7CertBufferXdr.inc
!endif
