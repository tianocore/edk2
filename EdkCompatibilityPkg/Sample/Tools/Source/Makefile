#/*++
#
#  Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
#  This program and the accompanying materials                          
#  are licensed and made available under the terms and conditions of the BSD License         
#  which accompanies this distribution.  The full text of the license may be found at        
#  http://opensource.org/licenses/bsd-license.php                                            
#                                                                                            
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
#  
#  Module Name:  makefile
#
#  Abstract:
#
#    This file is used to build the EFI build tools.
#
#--*/

#
# Define our toolchain before we include the master settings file
#
TOOLCHAIN = TOOLCHAIN_MSVC

!INCLUDE $(BUILD_DIR)\PlatformTools.env

#
# Define all the makefiles we want to call
#

MAKEFILES = $(EDK_TOOLS_SOURCE)\Common\Makefile             \
            $(CUSTOMIZEDCOMPRESS_MAKEFILE)                  \
            $(EDK_TOOLS_SOURCE)\GenCRC32Section\Makefile    \
            $(EDK_TOOLS_SOURCE)\GenSection\Makefile         \
            $(EDK_TOOLS_SOURCE)\GenDepex\Makefile           \
            $(EDK_TOOLS_SOURCE)\GenFfsFile\Makefile         \
            $(EDK_TOOLS_SOURCE)\GenFvImage\Makefile         \
            $(EDK_TOOLS_SOURCE)\GenTEImage\makefile         \
            $(EDK_TOOLS_SOURCE)\FwImage\Makefile            \
            $(EDK_TOOLS_SOURCE)\ProcessDsc\makefile         \
            $(EDK_TOOLS_SOURCE)\GuidChk\makefile            \
            $(EDK_TOOLS_SOURCE)\MakeDeps\makefile           \
            $(EDK_TOOLS_SOURCE)\SetStamp\makefile           \
!IF "$(EFI_SPECIFICATION_VERSION)" >= "0x0002000A"
            $(EDK_TOOLS_SOURCE)\UefiVfrCompile\makefile     \
            $(EDK_TOOLS_SOURCE)\UefiStrGather\makefile      \
            $(EDK_TOOLS_SOURCE)\UefiHiiPack\Makefile        \
!ELSE
            $(EDK_TOOLS_SOURCE)\VfrCompile\makefile         \
            $(EDK_TOOLS_SOURCE)\StrGather\makefile          \
            $(EDK_TOOLS_SOURCE)\HiiPack\Makefile            \
!ENDIF
            $(EDK_TOOLS_SOURCE)\SplitFile\Makefile          \
            $(EDK_TOOLS_SOURCE)\Strip\Makefile              \
            $(EDK_TOOLS_SOURCE)\EfiCompress\Makefile        \
            $(EDK_TOOLS_SOURCE)\EfiRom\Makefile             \
            $(EDK_TOOLS_SOURCE)\GenAprioriFile\Makefile     \
            $(EDK_TOOLS_SOURCE)\ModifyInf\Makefile

#
# Define default all target which calls all our makefiles. The special
# bang (!) tells nmake to do the command for each out-of-date dependent.
#
# Create the BIN directory, which will only exist if you pull the source tree
# from version control.
#
all : $(MAKEFILES)
  -if not exist $(EDK_TOOLS_OUTPUT) mkdir $(EDK_TOOLS_OUTPUT)
  !$(MAKE) -f $? TOOLCHAIN=$(TOOLCHAIN) BUILD_DIR=$(BUILD_DIR) all
  
#
# Call all the tools makefiles with a clean target.
#
clean : $(MAKEFILES)
  !$(MAKE) -f $? TOOLCHAIN=$(TOOLCHAIN) clean
