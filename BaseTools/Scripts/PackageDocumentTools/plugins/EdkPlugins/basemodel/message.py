## @file
#
# Copyright (c) 2011 - 2018, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available
# under the terms and conditions of the BSD License which accompanies this
# distribution. The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

    if fName != None:
        strMsg += ' %s' % fName.replace('/', '\\')
        if fNo != None:
            strMsg += '(%d):' % fNo
        else:
            strMsg += ' :'

    if fName == None and fNo == None:
        strMsg += ' '
    strMsg += mess

    return strMsg



