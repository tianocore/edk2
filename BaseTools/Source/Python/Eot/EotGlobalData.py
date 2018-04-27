## @file
# This file is used to save global datas
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

from collections import OrderedDict
from Common.LongFilePathSupport import OpenLongFilePath as open

gEFI_SOURCE = ''
gEDK_SOURCE = ''
gWORKSPACE = ''
gSHELL_INF = 'Application\Shell'
gMAKE_FILE = ''
gDSC_FILE = ''
gFV_FILE = []
gFV = []
gMAP_FILE = []
gMap = {}


gDb = ''
gIdentifierTableList = []

# Global macro
gMACRO = {}
gMACRO['EFI_SOURCE'] = gEFI_SOURCE
gMACRO['EDK_SOURCE'] = gEDK_SOURCE
gMACRO['SHELL_INF'] = gSHELL_INF
gMACRO['CAPSULE_INF'] = ''

gNOT_FOUND_FILES = []
gSOURCE_FILES = []
gINF_FILES = {}
gDEC_FILES = []

# Log file for unmatched variables
gUN_MATCHED_LOG = 'Log_UnMatched.log'
gOP_UN_MATCHED = open(gUN_MATCHED_LOG, 'w+')

# Log file for all INF files
gINF_FILES = 'Log_Inf_File.log'
gOP_INF = open(gINF_FILES, 'w+')

# Log file for not dispatched PEIM/DRIVER
gUN_DISPATCHED_LOG = 'Log_UnDispatched.log'
gOP_UN_DISPATCHED = open(gUN_DISPATCHED_LOG, 'w+')

# Log file for unmatched variables in function calling
gUN_MATCHED_IN_LIBRARY_CALLING_LOG = 'Log_UnMatchedInLibraryCalling.log'
gOP_UN_MATCHED_IN_LIBRARY_CALLING = open(gUN_MATCHED_IN_LIBRARY_CALLING_LOG, 'w+')

# Log file for order of dispatched PEIM/DRIVER
gDISPATCH_ORDER_LOG = 'Log_DispatchOrder.log'
gOP_DISPATCH_ORDER = open(gDISPATCH_ORDER_LOG, 'w+')

# Log file for source files not found
gUN_FOUND_FILES = 'Log_UnFoundSourceFiles.log'
gOP_UN_FOUND_FILES = open(gUN_FOUND_FILES, 'w+')

# Log file for found source files
gSOURCE_FILES = 'Log_SourceFiles.log'
gOP_SOURCE_FILES = open(gSOURCE_FILES, 'w+')

# Dict for GUID found in DEC files
gGuidDict = dict()

# Dict for PPI
gPpiList = {}

# Dict for PROTOCOL
gProtocolList = {}

# Dict for consumed PPI function calling
gConsumedPpiLibrary = OrderedDict()
gConsumedPpiLibrary['EfiCommonLocateInterface'] = 0
gConsumedPpiLibrary['PeiServicesLocatePpi'] = 0

# Dict for produced PROTOCOL function calling
gProducedProtocolLibrary = OrderedDict()
gProducedProtocolLibrary['RegisterEsalClass'] = 0
gProducedProtocolLibrary['CoreInstallProtocolInterface'] = 1
gProducedProtocolLibrary['CoreInstallMultipleProtocolInterfaces'] = -1
gProducedProtocolLibrary['EfiInstallProtocolInterface'] = 1
gProducedProtocolLibrary['EfiReinstallProtocolInterface'] = 1
gProducedProtocolLibrary['EfiLibNamedEventSignal'] = 0
gProducedProtocolLibrary['LibInstallProtocolInterfaces'] = 1
gProducedProtocolLibrary['LibReinstallProtocolInterfaces'] = 1

# Dict for consumed PROTOCOL function calling
gConsumedProtocolLibrary = OrderedDict()
gConsumedProtocolLibrary['EfiHandleProtocol'] = 0
gConsumedProtocolLibrary['EfiLocateProtocolHandleBuffers'] = 0
gConsumedProtocolLibrary['EfiLocateProtocolInterface'] = 0
gConsumedProtocolLibrary['EfiHandleProtocol'] = 1

# Dict for callback PROTOCOL function callling
gCallbackProtocolLibrary = OrderedDict()
gCallbackProtocolLibrary['EfiRegisterProtocolCallback'] = 2
