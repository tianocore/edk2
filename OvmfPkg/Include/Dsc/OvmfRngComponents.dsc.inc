##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

  OvmfPkg/CheckRdrandX64Dxe/CheckRdrandX64Dxe.inf
  SecurityPkg/RandomNumberGenerator/RngDxe/RngDxe.inf {
    <LibraryClasses>
      RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
      NULL|OvmfPkg/Library/CheckRdrandX64Lib/CheckRdrandX64Lib.inf
  }
  OvmfPkg/VirtioRngDxe/VirtioRng.inf
