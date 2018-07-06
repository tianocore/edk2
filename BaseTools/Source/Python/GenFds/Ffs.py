## @file
# process FFS generation
#
#  Copyright (c) 2007-2018, Intel Corporation. All rights reserved.<BR>
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
from CommonDataClass.FdfClass import FDClassObject
from Common.DataType import *

## generate FFS
#
#
class Ffs(FDClassObject):
    # mapping between FILE type in FDF and file type for GenFfs
    FdfFvFileTypeToFileType = {
        SUP_MODULE_SEC               : 'EFI_FV_FILETYPE_SECURITY_CORE',
        SUP_MODULE_PEI_CORE          : 'EFI_FV_FILETYPE_PEI_CORE',
        SUP_MODULE_PEIM              : 'EFI_FV_FILETYPE_PEIM',
        SUP_MODULE_DXE_CORE          : 'EFI_FV_FILETYPE_DXE_CORE',
        'FREEFORM'          : 'EFI_FV_FILETYPE_FREEFORM',
        'DRIVER'            : 'EFI_FV_FILETYPE_DRIVER',
        'APPLICATION'       : 'EFI_FV_FILETYPE_APPLICATION',
        'FV_IMAGE'          : 'EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE',
        'RAW'               : 'EFI_FV_FILETYPE_RAW',
        'PEI_DXE_COMBO'     : 'EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER',
        'SMM'               : 'EFI_FV_FILETYPE_SMM',
        SUP_MODULE_SMM_CORE          : 'EFI_FV_FILETYPE_SMM_CORE',
        SUP_MODULE_MM_STANDALONE     : 'EFI_FV_FILETYPE_MM_STANDALONE',
        SUP_MODULE_MM_CORE_STANDALONE : 'EFI_FV_FILETYPE_MM_CORE_STANDALONE'
    }
    
    # mapping between section type in FDF and file suffix
    SectionSuffix = {
        BINARY_FILE_TYPE_PE32                 : '.pe32',
        BINARY_FILE_TYPE_PIC                  : '.pic',
        BINARY_FILE_TYPE_TE                   : '.te',
        BINARY_FILE_TYPE_DXE_DEPEX            : '.dpx',
        'VERSION'              : '.ver',
        BINARY_FILE_TYPE_UI                   : '.ui',
        'COMPAT16'             : '.com16',
        'RAW'                  : '.raw',
        'FREEFORM_SUBTYPE_GUID': '.guid',
        'SUBTYPE_GUID'         : '.guid',        
        'FV_IMAGE'             : 'fv.sec',
        'COMPRESS'             : '.com',
        'GUIDED'               : '.guided',
        BINARY_FILE_TYPE_PEI_DEPEX            : '.dpx',
        BINARY_FILE_TYPE_SMM_DEPEX            : '.dpx'
    }
    
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsClassObject.__init__(self)
