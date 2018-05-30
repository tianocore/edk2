#/** @file
# FMP Certificates shared by multiple FmpDxe drivers for firmware update.
#
# Copyright (c) 2018, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available under
# the terms and conditions of the BSD License that accompanies this distribution.
# The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php.
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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
