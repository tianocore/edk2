#/*++
#
# Copyright (c) 2004 - 2007, Intel Corporation                                                         
# All rights reserved. This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
#  Module Name:
#
#    Nt32.dsc
#
#  Abstract:
#
#    This is the build description file containing the platform 
#    build definitions.
#
#
#  Notes:
#    
#    The info in this file is broken down into sections. The start of a section
#    is designated by a "[" in the first column. So the [=====] separater ends
#    a section.
#    
#--*/


[=============================================================================]
#
# This section gets processed first by the utility. Define any
# macros that you may use elsewhere in this description file. This is the
# mechanism by which you can pass parameters and defines to the makefiles
# generated for each component. You can define it here, and then make an
# assignment in the [makefile.common] section. For example, if here you
# define MY_VAR = my_var_value, then you can add MY_VAR = $(MY_VAR) in
# the [makefile.common] section and it becomes MY_VAR = my_var_value in
# the output makefiles for each component.
#
[Defines]
PLATFORM                  = $(PROJECT_NAME)

[=============================================================================]
#
# Include other common build descriptions
#
!include "$(EDK_SOURCE)\Sample\Platform\Common.dsc"
!include "$(EDK_SOURCE)\Sample\Platform\Common$(PROCESSOR).dsc"

[=============================================================================]
#
# These control the generation of the FV files
#
[=============================================================================]
[Fv.Fv.Attributes]

[Fv.Fv.options]

[Build.Fv.Fv]

[=============================================================================]
#
# These are the libraries that will be built by the master makefile
#
[=============================================================================]
[Libraries]
DEFINE EDK_PREFIX=

!include "$(EDK_SOURCE)\Sample\Platform\EdkLibAll.dsc"

#
# EdkII Glue Library
#
#!include "$(EDK_SOURCE)\Sample\Platform\EdkIIGlueLibAll.dsc"

[=============================================================================]
#
# These are platform specific libraries that must be built prior to building
# certain drivers that depend upon them.
#
[=============================================================================]
[Libraries.Platform]
Sample\Platform\Nt32\Protocol\EdkNt32ProtocolLib.inf
Sample\Library\Dxe\WinNt\WinNtLib.inf
#Sample\Platform\Generic\MonoStatusCode\Library\Pei\MemoryStatusCode\MemoryStatusCode.inf
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\BsDataHubStatusCode\BsDataHubStatusCode.inf
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\RtMemoryStatusCode\RtMemoryStatusCode.inf
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\RtPlatformStatusCode\$(PROJECT_NAME)\RtPlatformStatusCode.inf

Other\Maintained\Application\Shell\Library\EfiShellLib.inf

[=============================================================================]
#
# These are the components that will be built by the master makefile
#
[=============================================================================]
[Components]
DEFINE PACKAGE=Default

#Other\Maintained\Application\Shell\Shell.inf
#Other\Maintained\Application\Shell\ShellFull.inf
Other\Maintained\Application\Shell\attrib\attrib.inf
Other\Maintained\Application\Shell\cls\cls.inf
Other\Maintained\Application\Shell\comp\comp.inf
Other\Maintained\Application\Shell\cp\cp.inf
Other\Maintained\Application\Shell\date\date.inf
Other\Maintained\Application\Shell\dblk\dblk.inf
Other\Maintained\Application\Shell\devices\devices.inf
Other\Maintained\Application\Shell\DeviceTree\devicetree.inf
Other\Maintained\Application\Shell\dmem\dmem.inf
Other\Maintained\Application\Shell\dmpstore\dmpstore.inf
Other\Maintained\Application\Shell\drivers\drivers.inf
Other\Maintained\Application\Shell\drvcfg\drvcfg.inf
Other\Maintained\Application\Shell\drvdiag\drvdiag.inf
Other\Maintained\Application\Shell\edit\edit.inf
Other\Maintained\Application\Shell\EfiCompress\compress.inf
Other\Maintained\Application\Shell\EfiDecompress\Decompress.inf
Other\Maintained\Application\Shell\err\err.inf
Other\Maintained\Application\Shell\guid\guid.inf
Other\Maintained\Application\Shell\hexedit\hexedit.inf
Other\Maintained\Application\Shell\IfConfig\IfConfig.inf
Other\Maintained\Application\Shell\IpConfig\IpConfig.inf
Other\Maintained\Application\Shell\load\load.inf
Other\Maintained\Application\Shell\LoadPciRom\LoadPciRom.inf
Other\Maintained\Application\Shell\ls\ls.inf
Other\Maintained\Application\Shell\mem\mem.inf
Other\Maintained\Application\Shell\memmap\memmap.inf
Other\Maintained\Application\Shell\mkdir\mkdir.inf
Other\Maintained\Application\Shell\mm\mm.inf
Other\Maintained\Application\Shell\mode\mode.inf
Other\Maintained\Application\Shell\mount\mount.inf
Other\Maintained\Application\Shell\mv\mv.inf
Other\Maintained\Application\Shell\newshell\nshell.inf
Other\Maintained\Application\Shell\openinfo\openinfo.inf
Other\Maintained\Application\Shell\pci\pci.inf
Other\Maintained\Application\Shell\Ping\Ping.inf
Other\Maintained\Application\Shell\reset\reset.inf
Other\Maintained\Application\Shell\rm\rm.inf
Other\Maintained\Application\Shell\sermode\sermode.inf
Other\Maintained\Application\Shell\SmbiosView\Smbiosview.inf
Other\Maintained\Application\Shell\stall\stall.inf
Other\Maintained\Application\Shell\TelnetMgmt\TelnetMgmt.inf
Other\Maintained\Application\Shell\time\time.inf
Other\Maintained\Application\Shell\touch\touch.inf
Other\Maintained\Application\Shell\type\type.inf
Other\Maintained\Application\Shell\tzone\timezone.inf
Other\Maintained\Application\Shell\unload\unload.inf
Other\Maintained\Application\Shell\ver\Ver.inf
Other\Maintained\Application\Shell\vol\Vol.inf

[=============================================================================]

