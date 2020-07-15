## @file
# This file is used to save global datas
#
# Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
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

# Log file for found source files
gSOURCE_FILES = 'Log_SourceFiles.log'
gOP_SOURCE_FILES = open(gSOURCE_FILES, 'w+')

# Dict for GUID found in DEC files
gGuidDict = dict()

# Dict for PROTOCOL
gProtocolList = {}
# Dict for PPI
gPpiList = {}


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

# Dict for callback PROTOCOL function calling
gCallbackProtocolLibrary = OrderedDict()
gCallbackProtocolLibrary['EfiRegisterProtocolCallback'] = 2

gArchProtocolGuids = {'665e3ff6-46cc-11d4-9a38-0090273fc14d',
                      '26baccb1-6f42-11d4-bce7-0080c73c8881',
                      '26baccb2-6f42-11d4-bce7-0080c73c8881',
                      '1da97072-bddc-4b30-99f1-72a0b56fff2a',
                      '27cfac87-46cc-11d4-9a38-0090273fc14d',
                      '27cfac88-46cc-11d4-9a38-0090273fc14d',
                      'b7dfb4e1-052f-449f-87be-9818fc91b733',
                      'a46423e3-4617-49f1-b9ff-d1bfa9115839',
                      'd2b2b828-0826-48a7-b3df-983c006024f0',
                      '26baccb3-6f42-11d4-bce7-0080c73c8881',
                      '1e5668e2-8481-11d4-bcf1-0080c73c8881',
                      '6441f818-6362-4e44-b570-7dba31dd2453',
                      '665e3ff5-46cc-11d4-9a38-0090273fc14d'}
