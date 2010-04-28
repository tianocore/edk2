#/*++
#
# Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials                          
# are licensed and made available under the terms and conditions of the BSD License         
# which accompanies this distribution.  The full text of the license may be found at        
# http://opensource.org/licenses/bsd-license.php                                            
#                                                                                           
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             
# 
#  Module Name:
# 
#   CommonIa32.dsc
#
#  Abstract:
#
#    This is the build description file containing the processor architecture
#    dependent build instructions.
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
# These are the package descriptions. They are tagged as
# [Package.$(COMPONENT_TYPE).$(PACKAGE)], where COMPONENT_TYPE is typically
# defined in the component INF file, and PACKAGE is typically specified
# in the components section in the main DSC file. Main DSC file can also define
# platform specific package descriptions. 
#

[=============================================================================]
[Package.APPLICATION.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_APPLICATION
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(DEST_DIR)\$(BASE_NAME).pe32
               $(DEST_DIR)\$(BASE_NAME).ui
               $(DEST_DIR)\$(BASE_NAME).ver
            -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
  }
}

[=============================================================================]
[Package.Apriori.Default|DefaultStripped,Package.FILE.Default|DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FREEFORM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ 
  $(BASE_NAME).sec 
}

[=============================================================================]
[Package.RAWFILE.Default|DefaultStripped,Package.Config.Default|DefaultStripped|Config,Package.Microcode.Default|DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_RAW
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  $(BASE_NAME).Bin
}

[=============================================================================]
[Package.BINARY.Default,Package.Legacy16.Default,Package.Logo.Default|Logo]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FREEFORM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool ( $(OEMTOOLPATH)\GenCRC32Section
      ARGS = -i $(DEST_DIR)\$(BASE_NAME).sec
             -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
  }
}

[=============================================================================]
#
# Package definition for TE files
#
[Package.PE32_PEIM.TE_PEIM]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ 
  $(BASE_NAME).dpx 
  $(BASE_NAME).tes
  $(BASE_NAME).ui 
  $(BASE_NAME).ver 
}

[=============================================================================]
#
# Package definition to put the IFR data in a separate section in the
# FFS file.
#
[Package.BS_DRIVER.Ifr_Bin]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_DRIVER
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(DEST_DIR)\$(BASE_NAME).dpx
               $(DEST_DIR)\$(BASE_NAME).pe32
               $(DEST_DIR)\$(BASE_NAME).ui
               $(DEST_DIR)\$(BASE_NAME).ver
               $(DEST_DIR)\$(BASE_NAME)IfrBin.sec
            -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
  }
}

[=============================================================================]
[Package.PEI_CORE.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEI_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{          \
  $(BASE_NAME).pe32 \
  $(BASE_NAME).ui \
  $(BASE_NAME).ver \
}

[=============================================================================]
[Package.PEI_CORE.TE_PEIM]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEI_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{          \
  $(BASE_NAME).tes \
  $(BASE_NAME).ui \
  $(BASE_NAME).ver \
}

[=============================================================================]
[Package.PE32_PEIM.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ \
  $(BASE_NAME).dpx \
  $(BASE_NAME).pe32 \
  $(BASE_NAME).ui \
  $(BASE_NAME).ver \
}

[=============================================================================]
[Package.PE32_PEIM.Relocatable]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ \
  $(BASE_NAME).dpx \
  $(BASE_NAME).pe32 \
}

[=============================================================================]
[Package.PE32_PEIM.CompressPEIM]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ 
  $(BASE_NAME).dpx 
  Compress ($(COMPRESS_METHOD)) {
    $(BASE_NAME).pe32
    $(BASE_NAME).ui 
    $(BASE_NAME).ver
  }
}

[=============================================================================]
[Package.COMBINED_PEIM_DRIVER.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ \
  $(BASE_NAME).dpx \
  $(BASE_NAME).dpxd \
  $(BASE_NAME).pe32 \
  $(BASE_NAME).ui \
  $(BASE_NAME).ver \
}

[=============================================================================]
[Package.BS_DRIVER.DxeMain]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_DXE_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(BASE_NAME).pe32
    $(BASE_NAME).ui
    $(BASE_NAME).ver
  }    
}

[=============================================================================]
[Package.BS_DRIVER.Default,Package.RT_DRIVER.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_DRIVER
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(DEST_DIR)\$(BASE_NAME).dpx
               $(DEST_DIR)\$(BASE_NAME).pe32
               $(DEST_DIR)\$(BASE_NAME).ui
               $(DEST_DIR)\$(BASE_NAME).ver
            -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
  }
}

[=============================================================================]
[Package.FvImageFile.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(DEST_DIR)\$(SOURCE_FV)Fv.sec
            -o $(DEST_DIR)\$(BASE_NAME)fv.crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME)fv.crc32
    )
  }
}

[=============================================================================]
#
# Define a package that "signs" our capsule cargo FV
#
[Package.FvImageFile.SignedFVPackage]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(DEST_DIR)\$(SOURCE_FV)Fv.sec
            -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
}

[=============================================================================]
[Package.FvImageFile.FvMainCompact]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Blank.pad
    $(SOURCE_FV)Fv.sec
  }
}

[=============================================================================]
#
# Stripped package descriptions for size reduction.
#
[=============================================================================]
[Package.APPLICATION.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_APPLICATION
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(DEST_DIR)\$(BASE_NAME).pe32
  }
}

[=============================================================================]
[Package.BINARY.DefaultStripped,Package.Legacy16.DefaultStripped,Package.Logo.DefaultStripped|LogoStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FREEFORM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(DEST_DIR)\$(BASE_NAME).sec
  }
}

[=============================================================================]
[Package.PEI_CORE.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEI_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{          \
  $(BASE_NAME).pe32 \
}

[=============================================================================]
[Package.PEI_CORE.TE_PEIMStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEI_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  $(BASE_NAME).tes
}

[=============================================================================]
[Package.PE32_PEIM.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ \
  $(BASE_NAME).dpx \
  $(BASE_NAME).pe32 \
}

[=============================================================================]
[Package.PE32_PEIM.CompressPEIMStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  $(BASE_NAME).dpx
  Compress ($(COMPRESS_METHOD)) {
    $(BASE_NAME).pe32
  }
}

[=============================================================================]
#
# Package definition for TE files
#
[Package.PE32_PEIM.TE_PEIMStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_PEIM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  $(BASE_NAME).dpx
  $(BASE_NAME).tes
}

[=============================================================================]
[Package.COMBINED_PEIM_DRIVER.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{ \
  $(BASE_NAME).dpx \
  $(BASE_NAME).dpxd \
  $(BASE_NAME).pe32 \
}

[=============================================================================]
[Package.BS_DRIVER.DxeMainStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_DXE_CORE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(BASE_NAME).pe32
  }
}

[=============================================================================]
[Package.BS_DRIVER.DefaultStripped,Package.RT_DRIVER.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_DRIVER
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(DEST_DIR)\$(BASE_NAME).dpx
    $(DEST_DIR)\$(BASE_NAME).pe32
  }
}

[=============================================================================]
[Package.FvImageFile.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(DEST_DIR)\$(SOURCE_FV)Fv.sec
  }
}

[=============================================================================]
[Package.SECURITY_CORE.Default|DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_SECURITY_CORE
FFS_ATTRIB_CHECKSUM         = FALSE

IMAGE_SCRIPT =
{          \
  Blank1.pad \
  $(BASE_NAME).tes \
  ResetVec.raw \
}

[=============================================================================]
[Package.AcpiTable.Default]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FREEFORM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    Tool (
      $(OEMTOOLPATH)\GenCRC32Section
      ARGS= -i $(SECTION_TARGETS)
               $(DEST_DIR)\$(BASE_NAME).ui
            -o $(DEST_DIR)\$(BASE_NAME).crc32
      OUTPUT = $(DEST_DIR)\$(BASE_NAME).crc32
    )
  }
}

[=============================================================================]
[Package.AcpiTable.DefaultStripped]
PACKAGE.INF
\[.]
BASE_NAME                   = $(BASE_NAME)
FFS_FILEGUID                = $(FILE_GUID)
FFS_FILETYPE                = EFI_FV_FILETYPE_FREEFORM
FFS_ATTRIB_CHECKSUM         = TRUE

IMAGE_SCRIPT =
{
  Compress ($(COMPRESS_METHOD)) {
    $(SECTION_TARGETS)
  }
}

[=============================================================================]
