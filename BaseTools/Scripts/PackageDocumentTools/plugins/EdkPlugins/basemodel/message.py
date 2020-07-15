## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

def GetEdkLogger():
    import logging
    return logging.getLogger('edk')

class EdkException(Exception):
    def __init__(self, message, fName=None, fNo=None):
        self._message = message
        ErrorMsg(message, fName, fNo)

    def GetMessage(self):
        return '[EDK Failure]: %s' %self._message

def ErrorMsg(mess, fName=None, fNo=None):
    GetEdkLogger().error(NormalMessage('#ERR#', mess, fName, fNo))

def LogMsg(mess, fName=None, fNo=None):
    GetEdkLogger().info(NormalMessage('@LOG@', mess, fName, fNo))

def WarnMsg(mess, fName=None, fNo=None):
    GetEdkLogger().warning(NormalMessage('!WAR!', mess, fName, fNo))

def NormalMessage(type, mess, fName=None, fNo=None):
    strMsg = type

    if fName is not None:
        strMsg += ' %s' % fName.replace('/', '\\')
        if fNo is not None:
            strMsg += '(%d):' % fNo
        else:
            strMsg += ' :'

    if fName is None and fNo is None:
        strMsg += ' '
    strMsg += mess

    return strMsg



