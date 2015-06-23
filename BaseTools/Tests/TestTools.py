## @file
# Utility functions and classes for BaseTools unit tests
#
#  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
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
import base64
import os
import os.path
import random
import shutil
import subprocess
import sys
import types
import unittest

TestsDir = os.path.realpath(os.path.split(sys.argv[0])[0])
BaseToolsDir = os.path.realpath(os.path.join(TestsDir, '..'))
CSourceDir = os.path.join(BaseToolsDir, 'Source', 'C')
PythonSourceDir = os.path.join(BaseToolsDir, 'Source', 'Python')
TestTempDir = os.path.join(TestsDir, 'TestTempDir')

if PythonSourceDir not in sys.path:
    #
    # Allow unit tests to import BaseTools python modules. This is very useful
    # for writing unit tests.
    #
    sys.path.append(PythonSourceDir)

def MakeTheTestSuite(localItems):
    tests = []
    for name, item in localItems.iteritems():
        if isinstance(item, types.TypeType):
            if issubclass(item, unittest.TestCase):
                tests.append(unittest.TestLoader().loadTestsFromTestCase(item))
            elif issubclass(item, unittest.TestSuite):
                tests.append(item())
    return lambda: unittest.TestSuite(tests)

def GetBaseToolsPaths():
    if sys.platform in ('win32', 'win64'):
        return [ os.path.join(BaseToolsDir, 'Bin', sys.platform.title()) ]
    else:
        uname = os.popen('uname -sm').read().strip()
        for char in (' ', '/'):
            uname = uname.replace(char, '-')
        return [
                os.path.join(BaseToolsDir, 'Bin', uname),
                os.path.join(BaseToolsDir, 'BinWrappers', uname),
                os.path.join(BaseToolsDir, 'BinWrappers', 'PosixLike')
            ]

BaseToolsBinPaths = GetBaseToolsPaths()

class BaseToolsTest(unittest.TestCase):

    def cleanOutDir(self, dir):
        for dirItem in os.listdir(dir):
            if dirItem in ('.', '..'): continue
            dirItem = os.path.join(dir, dirItem)
            self.RemoveFileOrDir(dirItem)

    def CleanUpTmpDir(self):
        if os.path.exists(self.testDir):
            self.cleanOutDir(self.testDir)

    def HandleTreeDeleteError(self, function, path, excinfo):
        os.chmod(path, stat.S_IWRITE)
        function(path)
    
    def RemoveDir(self, dir):
        shutil.rmtree(dir, False, self.HandleTreeDeleteError)

    def RemoveFileOrDir(self, path):
        if not os.path.exists(path):
            return
        elif os.path.isdir(path):
            self.RemoveDir(path)
        else:
            os.remove(path)

    def DisplayBinaryData(self, description, data):
        print description, '(base64 encoded):'
        b64data = base64.b64encode(data)
        print b64data

    def DisplayFile(self, fileName):
        sys.stdout.write(self.ReadTmpFile(fileName))
        sys.stdout.flush()

    def FindToolBin(self, toolName):
        for binPath in BaseToolsBinPaths:
            bin = os.path.join(binPath, toolName)
            if os.path.exists(bin):
                break
        assert os.path.exists(bin)
        return bin

    def RunTool(self, *args, **kwd):
        if 'toolName' in kwd: toolName = kwd['toolName']
        else: toolName = None
        if 'logFile' in kwd: logFile = kwd['logFile']
        else: logFile = None

        if toolName is None: toolName = self.toolName
        bin = self.FindToolBin(toolName)
        if logFile is not None:
            logFile = open(os.path.join(self.testDir, logFile), 'w')
            popenOut = logFile
        else:
            popenOut = subprocess.PIPE

        args = [toolName] + list(args)

        Proc = subprocess.Popen(
            args, executable=bin,
            stdout=popenOut, stderr=subprocess.STDOUT
            )

        if logFile is None:
            Proc.stdout.read()

        return Proc.wait()

    def GetTmpFilePath(self, fileName):
        return os.path.join(self.testDir, fileName)

    def OpenTmpFile(self, fileName, mode = 'r'):
        return open(os.path.join(self.testDir, fileName), mode)

    def ReadTmpFile(self, fileName):
        f = open(self.GetTmpFilePath(fileName), 'r')
        data = f.read()
        f.close()
        return data

    def WriteTmpFile(self, fileName, data):
        f = open(self.GetTmpFilePath(fileName), 'w')
        f.write(data)
        f.close()

    def GenRandomFileData(self, fileName, minlen = None, maxlen = None):
        if maxlen is None: maxlen = minlen
        f = self.OpenTmpFile(fileName, 'w')
        f.write(self.GetRandomString(minlen, maxlen))
        f.close()

    def GetRandomString(self, minlen = None, maxlen = None):
        if minlen is None: minlen = 1024
        if maxlen is None: maxlen = minlen
        return ''.join(
            [chr(random.randint(0,255))
             for x in xrange(random.randint(minlen, maxlen))
            ])

    def setUp(self):
        self.savedEnvPath = os.environ['PATH']
        self.savedSysPath = sys.path[:]

        for binPath in BaseToolsBinPaths:
            os.environ['PATH'] = \
                os.path.pathsep.join((os.environ['PATH'], binPath))

        self.testDir = TestTempDir
        if not os.path.exists(self.testDir):
            os.mkdir(self.testDir)
        else:
            self.cleanOutDir(self.testDir)

    def tearDown(self):
        self.RemoveFileOrDir(self.testDir)

        os.environ['PATH'] = self.savedEnvPath
        sys.path = self.savedSysPath

