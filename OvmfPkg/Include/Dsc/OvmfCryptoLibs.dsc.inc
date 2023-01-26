##
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
# mostly following CryptoPkg/Readme.md recommendations
##

[LibraryClasses]
  HashApiLib|CryptoPkg/Library/BaseHashApiLib/BaseHashApiLib.inf
  IntrinsicLib|CryptoPkg/Library/IntrinsicLib/IntrinsicLib.inf

[LibraryClasses.common.SEC]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SecCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/RuntimeCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf


!if $(PEI_USE_CRYPTO_DRIVER) == TRUE

[LibraryClasses.common.PEIM]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/PeiCryptLib.inf

!else

[LibraryClasses.common.PEIM]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/PeiCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf

!endif


!if $(SMM_USE_CRYPTO_DRIVER) == TRUE

[LibraryClasses.common.DXE_SMM_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/SmmCryptLib.inf

!else

[LibraryClasses.common.DXE_SMM_DRIVER]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/SmmCryptLib.inf
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf

!endif


!if $(DXE_USE_CRYPTO_DRIVER) == TRUE

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf
  TlsLib|CryptoPkg/Library/BaseCryptLibOnProtocolPpi/DxeCryptLib.inf

!else

[LibraryClasses.common.DXE_DRIVER, LibraryClasses.common.UEFI_DRIVER, LibraryClasses.common.UEFI_APPLICATION]
  BaseCryptLib|CryptoPkg/Library/BaseCryptLib/BaseCryptLib.inf
!if $(NETWORK_TLS_ENABLE) == TRUE
  TlsLib|CryptoPkg/Library/TlsLib/TlsLib.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLib.inf
!else
  TlsLib|CryptoPkg/Library/TlsLibNull/TlsLibNull.inf
  OpensslLib|CryptoPkg/Library/OpensslLib/OpensslLibCrypto.inf
!endif

!endif
