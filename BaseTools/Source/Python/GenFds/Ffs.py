## @file
# process FFS generation
#
#  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
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

## generate FFS
#
#
class Ffs(FDClassObject):
    
    # mapping between MODULE type in FDF (from INF) and file type for GenFfs
    ModuleTypeToFileType = {
        'SEC'               : 'EFI_FV_FILETYPE_SECURITY_CORE',
        'PEI_CORE'          : 'EFI_FV_FILETYPE_PEI_CORE',
        'PEIM'              : 'EFI_FV_FILETYPE_PEIM',
        'DXE_CORE'          : 'EFI_FV_FILETYPE_DXE_CORE',
        'DXE_DRIVER'        : 'EFI_FV_FILETYPE_DRIVER',
        'DXE_SAL_DRIVER'    : 'EFI_FV_FILETYPE_DRIVER',
        'DXE_SMM_DRIVER'    : 'EFI_FV_FILETYPE_DRIVER',
        'DXE_RUNTIME_DRIVER': 'EFI_FV_FILETYPE_DRIVER',
        'UEFI_DRIVER'       : 'EFI_FV_FILETYPE_DRIVER',
        'UEFI_APPLICATION'  : 'EFI_FV_FILETYPE_APPLICATION',
        'SMM_CORE'          : 'EFI_FV_FILETYPE_SMM_CORE'
    }
    
    # mapping between FILE type in FDF and file type for GenFfs
    FdfFvFileTypeToFileType = {
        'SEC'               : 'EFI_FV_FILETYPE_SECURITY_CORE',
        'PEI_CORE'          : 'EFI_FV_FILETYPE_PEI_CORE',
        'PEIM'              : 'EFI_FV_FILETYPE_PEIM',
        'DXE_CORE'          : 'EFI_FV_FILETYPE_DXE_CORE',
        'FREEFORM'          : 'EFI_FV_FILETYPE_FREEFORM',
        'DRIVER'            : 'EFI_FV_FILETYPE_DRIVER',
        'APPLICATION'       : 'EFI_FV_FILETYPE_APPLICATION',
        'FV_IMAGE'          : 'EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE',
        'RAW'               : 'EFI_FV_FILETYPE_RAW',
        'PEI_DXE_COMBO'     : 'EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER',
        'SMM'               : 'EFI_FV_FILETYPE_SMM',
        'SMM_CORE'          : 'EFI_FV_FILETYPE_SMM_CORE'
    }
    
    # mapping between section type in FDF and file suffix
    SectionSuffix = {
        'PE32'                 : '.pe32',
        'PIC'                  : '.pic',
        'TE'                   : '.te',
        'DXE_DEPEX'            : '.dpx',
        'VERSION'              : '.ver',
        'UI'                   : '.ui',
        'COMPAT16'             : '.com16',
        'RAW'                  : '.raw',
        'FREEFORM_SUBTYPE_GUID': '.guid',
        'SUBTYPE_GUID'         : '.guid',        
        'FV_IMAGE'             : 'fv.sec',
        'COMPRESS'             : '.com',
        'GUIDED'               : '.guided',
        'PEI_DEPEX'            : '.dpx',
        'SMM_DEPEX'            : '.dpx'
    }
    
    ## The constructor
    #
    #   @param  self        The object pointer
    #
    def __init__(self):
        FfsClassObject.__init__(self)
