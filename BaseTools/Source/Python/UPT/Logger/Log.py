## @file
# This file implements the log mechanism for Python tools.
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

'''
Logger
'''

## Import modules
from sys import argv
from sys import stdout
from sys import stderr
import os.path
from os import remove
from logging import getLogger
from logging import Formatter
from logging import StreamHandler
from logging import FileHandler
from traceback import extract_stack

from Logger.ToolError import FatalError
from Logger.ToolError import WARNING_AS_ERROR
from Logger.ToolError import gERROR_MESSAGE
from Logger.ToolError import UNKNOWN_ERROR
from Library import GlobalData

#
# Log level constants
#
DEBUG_0 = 1
DEBUG_1 = 2
DEBUG_2 = 3
DEBUG_3 = 4
DEBUG_4 = 5
DEBUG_5 = 6
DEBUG_6 = 7
DEBUG_7 = 8
DEBUG_8 = 9
DEBUG_9 = 10
VERBOSE = 15
INFO    = 20
WARN    = 30
QUIET   = 40
QUIET_1 = 41
ERROR   = 50
SILENT  = 60

IS_RAISE_ERROR = True
SUPRESS_ERROR = False

#
# Tool name
#
_TOOL_NAME = os.path.basename(argv[0])
#
# For validation purpose
#
_LOG_LEVELS = [DEBUG_0, DEBUG_1, DEBUG_2, DEBUG_3, DEBUG_4, DEBUG_5, DEBUG_6, \
              DEBUG_7, DEBUG_8, DEBUG_9, VERBOSE, WARN, INFO, ERROR, QUIET, \
              QUIET_1, SILENT]
#
# For DEBUG level (All DEBUG_0~9 are applicable)
#
_DEBUG_LOGGER = getLogger("tool_debug")
_DEBUG_FORMATTER = Formatter("[%(asctime)s.%(msecs)d]: %(message)s", \
                            datefmt="%H:%M:%S")
#
# For VERBOSE, INFO, WARN level
#
_INFO_LOGGER = getLogger("tool_info")
_INFO_FORMATTER = Formatter("%(message)s")
#
# For ERROR level
#
_ERROR_LOGGER = getLogger("tool_error")
_ERROR_FORMATTER = Formatter("%(message)s")

#
# String templates for ERROR/WARN/DEBUG log message
#
_ERROR_MESSAGE_TEMPLATE = \
('\n\n%(tool)s...\n%(file)s(%(line)s): error %(errorcode)04X: %(msg)s\n\t%(extra)s')

__ERROR_MESSAGE_TEMPLATE_WITHOUT_FILE = \
'\n\n%(tool)s...\n : error %(errorcode)04X: %(msg)s\n\t%(extra)s'

_WARNING_MESSAGE_TEMPLATE = '%(tool)s...\n%(file)s(%(line)s): warning: %(msg)s'
_WARNING_MESSAGE_TEMPLATE_WITHOUT_FILE = '%(tool)s: : warning: %(msg)s'
_DEBUG_MESSAGE_TEMPLATE = '%(file)s(%(line)s): debug: \n    %(msg)s'


#
# Log INFO message
#
#Info    = _INFO_LOGGER.info

def Info(msg, *args, **kwargs):
    _INFO_LOGGER.info(msg, *args, **kwargs)

#
# Log information which should be always put out
#
def Quiet(msg, *args, **kwargs):
    _ERROR_LOGGER.error(msg, *args, **kwargs)

## Log debug message
#
#   @param  Level       DEBUG level (DEBUG0~9)
#   @param  Message     Debug information
#   @param  ExtraData   More information associated with "Message"
#
def Debug(Level, Message, ExtraData=None):
    if _DEBUG_LOGGER.level > Level:
        return
    if Level > DEBUG_9:
        return
    #
    # Find out the caller method information
    #
    CallerStack = extract_stack()[-2]
    TemplateDict = {
        "file"      : CallerStack[0],
        "line"      : CallerStack[1],
        "msg"       : Message,
    }

    if ExtraData is not None:
        LogText = _DEBUG_MESSAGE_TEMPLATE % TemplateDict + "\n    %s" % ExtraData
    else:
        LogText = _DEBUG_MESSAGE_TEMPLATE % TemplateDict

    _DEBUG_LOGGER.log(Level, LogText)

## Log verbose message
#
#   @param  Message     Verbose information
#
def Verbose(Message):
    return _INFO_LOGGER.log(VERBOSE, Message)

## Log warning message
#
#   Warning messages are those which might be wrong but won't fail the tool.
#
#   @param  ToolName    The name of the tool. If not given, the name of caller
#                       method will be used.
#   @param  Message     Warning information
#   @param  File        The name of file which caused the warning.
#   @param  Line        The line number in the "File" which caused the warning.
#   @param  ExtraData   More information associated with "Message"
#
def Warn(ToolName, Message, File=None, Line=None, ExtraData=None):
    if _INFO_LOGGER.level > WARN:
        return
    #
    # if no tool name given, use caller's source file name as tool name
    #
    if ToolName is None or ToolName == "":
        ToolName = os.path.basename(extract_stack()[-2][0])

    if Line is None:
        Line = "..."
    else:
        Line = "%d" % Line

    TemplateDict = {
        "tool"      : ToolName,
        "file"      : File,
        "line"      : Line,
        "msg"       : Message,
    }

    if File is not None:
        LogText = _WARNING_MESSAGE_TEMPLATE % TemplateDict
    else:
        LogText = _WARNING_MESSAGE_TEMPLATE_WITHOUT_FILE % TemplateDict

    if ExtraData is not None:
        LogText += "\n    %s" % ExtraData

    _INFO_LOGGER.log(WARN, LogText)
    #
    # Raise an exception if indicated
    #
    if GlobalData.gWARNING_AS_ERROR == True:
        raise FatalError(WARNING_AS_ERROR)

## Log ERROR message
#
# Once an error messages is logged, the tool's execution will be broken by
# raising an exception. If you don't want to break the execution later, you
# can give "RaiseError" with "False" value.
#
#   @param  ToolName    The name of the tool. If not given, the name of caller
#                       method will be used.
#   @param  ErrorCode   The error code
#   @param  Message     Warning information
#   @param  File        The name of file which caused the error.
#   @param  Line        The line number in the "File" which caused the warning.
#   @param  ExtraData   More information associated with "Message"
#   @param  RaiseError  Raise an exception to break the tool's execution if
#                       it's True. This is the default behavior.
#
def Error(ToolName, ErrorCode, Message=None, File=None, Line=None, \
          ExtraData=None, RaiseError=IS_RAISE_ERROR):
    if ToolName:
        pass
    if Line is None:
        Line = "..."
    else:
        Line = "%d" % Line

    if Message is None:
        if ErrorCode in gERROR_MESSAGE:
            Message = gERROR_MESSAGE[ErrorCode]
        else:
            Message = gERROR_MESSAGE[UNKNOWN_ERROR]

    if ExtraData is None:
        ExtraData = ""

    TemplateDict = {
        "tool"      : _TOOL_NAME,
        "file"      : File,
        "line"      : Line,
        "errorcode" : ErrorCode,
        "msg"       : Message,
        "extra"     : ExtraData
    }

    if File is not None:
        LogText =  _ERROR_MESSAGE_TEMPLATE % TemplateDict
    else:
        LogText = __ERROR_MESSAGE_TEMPLATE_WITHOUT_FILE % TemplateDict

    if not SUPRESS_ERROR:
        _ERROR_LOGGER.log(ERROR, LogText)
    if RaiseError:
        raise FatalError(ErrorCode)


## Initialize log system
#
def Initialize():
    #
    # Since we use different format to log different levels of message into
    # different place (stdout or stderr), we have to use different "Logger"
    # objects to do this.
    #
    # For DEBUG level (All DEBUG_0~9 are applicable)
    _DEBUG_LOGGER.setLevel(INFO)
    _DebugChannel = StreamHandler(stdout)
    _DebugChannel.setFormatter(_DEBUG_FORMATTER)
    _DEBUG_LOGGER.addHandler(_DebugChannel)
    #
    # For VERBOSE, INFO, WARN level
    #
    _INFO_LOGGER.setLevel(INFO)
    _InfoChannel = StreamHandler(stdout)
    _InfoChannel.setFormatter(_INFO_FORMATTER)
    _INFO_LOGGER.addHandler(_InfoChannel)
    #
    # For ERROR level
    #
    _ERROR_LOGGER.setLevel(INFO)
    _ErrorCh = StreamHandler(stderr)
    _ErrorCh.setFormatter(_ERROR_FORMATTER)
    _ERROR_LOGGER.addHandler(_ErrorCh)


## Set log level
#
#   @param  Level   One of log level in _LogLevel
#
def SetLevel(Level):
    if Level not in _LOG_LEVELS:
        Info("Not supported log level (%d). Use default level instead." % \
             Level)
        Level = INFO
    _DEBUG_LOGGER.setLevel(Level)
    _INFO_LOGGER.setLevel(Level)
    _ERROR_LOGGER.setLevel(Level)

## Get current log level
#
def GetLevel():
    return _INFO_LOGGER.getEffectiveLevel()

## Raise up warning as error
#
def SetWarningAsError():
    GlobalData.gWARNING_AS_ERROR = True

## Specify a file to store the log message as well as put on console
#
#   @param  LogFile     The file path used to store the log message
#
def SetLogFile(LogFile):
    if os.path.exists(LogFile):
        remove(LogFile)

    _Ch = FileHandler(LogFile)
    _Ch.setFormatter(_DEBUG_FORMATTER)
    _DEBUG_LOGGER.addHandler(_Ch)

    _Ch = FileHandler(LogFile)
    _Ch.setFormatter(_INFO_FORMATTER)
    _INFO_LOGGER.addHandler(_Ch)

    _Ch = FileHandler(LogFile)
    _Ch.setFormatter(_ERROR_FORMATTER)
    _ERROR_LOGGER.addHandler(_Ch)



