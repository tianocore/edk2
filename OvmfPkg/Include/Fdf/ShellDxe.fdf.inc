##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(BUILD_SHELL) == TRUE

!if $(TOOL_CHAIN_TAG) != "XCODE5"
!if $(NETWORK_ENABLE) == TRUE
INF  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf
INF  ShellPkg/DynamicCommand/HttpDynamicCommand/HttpDynamicCommand.inf
!endif
INF  OvmfPkg/LinuxInitrdDynamicShellCommand/LinuxInitrdDynamicShellCommand.inf
!endif

INF  ShellPkg/Application/Shell/Shell.inf
!endif
