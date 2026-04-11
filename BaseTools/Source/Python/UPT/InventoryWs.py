## @file
# Inventory workspace's distribution package information.
#
# Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#
"""
Inventory workspace's distribution package information.
"""
##
# Import Modules
#
from sys import platform
from traceback import format_exc
from platform import python_version

from Logger import StringTable as ST
from Logger.ToolError import FatalError
from Logger.ToolError import ABORT_ERROR
from Logger.ToolError import CODE_ERROR
import Logger.Log as Logger

from Library import GlobalData

## InventoryDistInstalled
#
# This method retrieves the installed distribution information from the internal UPT database
#
# @param  DataBase: the UPT database
#
def InventoryDistInstalled(DataBase):
    DistInstalled = DataBase.InventoryDistInstalled()

    #
    # find the max length for each item
    #
    DpNameStr = "DpName"
    DpGuidStr = "DpGuid"
    DpVerStr = "DpVer"
    DpOriginalNameStr = "DpOriginalName"
    MaxGuidlen = len(DpGuidStr)
    MaxVerlen = len(DpVerStr)
    MaxDpAliasFileNameLen = len(DpNameStr)
    MaxDpOrigFileNamelen = len(DpOriginalNameStr)

    for (DpGuid, DpVersion, DpOriginalName, DpAliasFileName) in DistInstalled:
        MaxGuidlen = max(MaxGuidlen, len(DpGuid))
        MaxVerlen = max(MaxVerlen, len(DpVersion))
        MaxDpAliasFileNameLen = max(MaxDpAliasFileNameLen, len(DpAliasFileName))
        MaxDpOrigFileNamelen = max(MaxDpOrigFileNamelen, len(DpOriginalName))

    OutMsgFmt = "%-*s\t%-*s\t%-*s\t%-s"
    OutMsg = OutMsgFmt % (MaxDpAliasFileNameLen,
                          DpNameStr,
                          MaxGuidlen,
                          DpGuidStr,
                          MaxVerlen,
                          DpVerStr,
                          DpOriginalNameStr)
    Logger.Info(OutMsg)

    for (DpGuid, DpVersion, DpFileName, DpAliasFileName) in DistInstalled:
        OutMsg = OutMsgFmt % (MaxDpAliasFileNameLen,
                            DpAliasFileName,
                            MaxGuidlen,
                            DpGuid,
                            MaxVerlen,
                            DpVersion,
                            DpFileName)
        Logger.Info(OutMsg)

## Tool entrance method
#
# This method mainly dispatch specific methods per the command line options.
# If no error found, return zero value so the caller of this tool can know
# if it's executed successfully or not.
#
# @param  Options: command Options
#
def Main(Options = None):
    if Options:
        pass

    try:
        DataBase = GlobalData.gDB
        InventoryDistInstalled(DataBase)
        ReturnCode = 0
    except FatalError as XExcept:
        ReturnCode = XExcept.args[0]
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + format_exc())
    except KeyboardInterrupt:
        ReturnCode = ABORT_ERROR
        if Logger.GetLevel() <= Logger.DEBUG_9:
            Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(), platform) + format_exc())
    except:
        ReturnCode = CODE_ERROR
        Logger.Error("\nInventoryWs",
                    CODE_ERROR,
                    ST.ERR_UNKNOWN_FATAL_INVENTORYWS_ERR,
                    ExtraData=ST.MSG_SEARCH_FOR_HELP % ST.MSG_EDKII_MAIL_ADDR,
                    RaiseError=False
                    )
        Logger.Quiet(ST.MSG_PYTHON_ON % (python_version(),
            platform) + format_exc())

    if ReturnCode == 0:
        Logger.Quiet(ST.MSG_FINISH)

    return ReturnCode
