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
#    IPF.dsc
#
#  Abstract:
#
#    This is the build description file containing the platform 
#    specific build definitions.
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
PLATFORM=$(PROJECT_NAME)


[=============================================================================]
#
# Include other common build descriptions
#
!include "$(EDK_SOURCE)\Sample\Platform\Common.dsc"
!include "$(EDK_SOURCE)\Sample\Platform\Common$(PROCESSOR).dsc"



[=============================================================================]
[Fv.Fv.Attributes]

[Fv.Fv.options]

[build.fv.Fv]

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
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\RtMemoryStatusCode\RtMemoryStatusCode.inf
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\BsDataHubStatusCode\BsDataHubStatusCode.inf
Sample\Platform\Generic\RuntimeDxe\StatusCode\Lib\BsSerialStatusCode\BsSerialStatusCode.inf

#Other\Maintained\Application\Shell\Library\EfiShellLib.inf

[=============================================================================]
#
# These are the components that will be built by the master makefile
#
[=============================================================================]
[Components]
DEFINE PACKAGE=Default

#Other\Maintained\Application\Shell\Shell.inf
Other\Maintained\Application\Shell\ShellFull.inf

[=============================================================================]
