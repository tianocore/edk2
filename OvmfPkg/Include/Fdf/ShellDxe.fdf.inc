##
#    SPDX-License-Identifier: BSD-2-Clause-Patent
##

!if $(BUILD_SHELL) == TRUE

!if $(TOOL_CHAIN_TAG) != "XCODE5"
INF  ShellPkg/DynamicCommand/TftpDynamicCommand/TftpDynamicCommand.inf
INF  ShellPkg/DynamicCommand/HttpDynamicCommand/HttpDynamicCommand.inf
INF  OvmfPkg/LinuxInitrdDynamicShellCommand/LinuxInitrdDynamicShellCommand.inf
!endif

INF  ShellPkg/Application/Shell/Shell.inf
!endif
