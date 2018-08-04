#!python2
##
# Copyright (c) 2016, Microsoft Corporation

# All rights reserved.
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##

##
# This is a sample tool and should not be used in production environments.
#  
# This tool creates a signed Windows Capsule.
###


import time
import sys
import logging
import os
import shutil

print "This is a sample tool and should not be used in production environments\n"

#raw_input('Press any key to continue . . .\n')


### Parse input parameters ###

if len(sys.argv) == 1 or sys.argv[1] == "-h" or sys.argv[1] == "-H" or sys.argv[1] == "-?":
  print "This tool creates a signed Windows capsule\n"
  print "usage: CreateWindowsCapsule.py <ProductFwName> <CapsuleVersionDotString> <ProductFmpGuid> <CapsuleFileName> <CapsuleVersionHexString> <ProductFwProvider> <ProductFwMfgName> <ProductFwDesc> <PfxFileName>"
  print "example: CreateWindowsCapsule.py UEFI 1.2.3.4 ########-####-####-####-############ UEFI_1.2.3.bin 0x01020304 Microsoft Microsoft \"Microsoft UEFI\" OS_DRIVER.pfx"
  sys.exit(-1)

if len(sys.argv) != 10:
  print "Error: Incorrect number of parameters"
  print "usage: CreateWindowsCapsule.py <ProductFwName> <CapsuleVersionDotString> <ProductFmpGuid> <CapsuleFileName> <CapsuleVersionHexString> <ProductFwProvider> <ProductFwMfgName> <ProductFwDesc> <PfxFileName>"
  sys.exit(-1)

ProductName = sys.argv[1];
CapsuleVersion_DotString = sys.argv[2];
ProductFmpGuid = sys.argv[3];
CapsuleFileName = sys.argv[4];
CapsuleVersion_HexString = sys.argv[5];
ProductFwProvider = sys.argv[6];
ProductFwMfgName = sys.argv[7];
ProductFwDesc = sys.argv[8];
PfxFile = sys.argv[9];

logging.debug("CapsulePackage: Create Windows Capsule Files")
InfFileName = ProductName + ".inf"
CatFileName = ProductName + ".cat"

capsuleFolderPath = "WindowsCapsule"
if(os.path.exists(capsuleFolderPath)):
  shutil.rmtree(capsuleFolderPath)
  time.sleep(2)

os.mkdir(capsuleFolderPath)
shutil.copy2(CapsuleFileName, capsuleFolderPath)
os.chdir(capsuleFolderPath)

#Make INF
cmd = "..\CreateWindowsInf.py" 
cmd = cmd + " " + "\"" + ProductName + "\"" + " " + "\"" + CapsuleVersion_DotString + "\""
cmd = cmd + " " + "\"" + ProductFmpGuid + "\"" + " " + "\"" + CapsuleFileName + "\""
cmd = cmd + " " + "\"" + CapsuleVersion_HexString + "\"" + " " + "\"" + ProductFwProvider + "\""
cmd = cmd + " " + "\"" + ProductFwMfgName + "\"" + " " + "\"" + ProductFwDesc + "\"" 
ret = os.system(cmd)
if(ret != 0):
  raise Exception("CreateWindowsInf Failed with errorcode %d" % ret)

#Find Signtool 
SignToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "x64", "signtool.exe")
if not os.path.exists(SignToolPath):
  SignToolPath = SignToolPath.replace('10', '8.1')
if not os.path.exists(SignToolPath):
  raise Exception("Can't find signtool on this machine.")

#Find Inf2Cat tool
Inf2CatToolPath = os.path.join(os.getenv("ProgramFiles(x86)"), "Windows Kits", "10", "bin", "x86", "Inf2Cat.exe")
if not os.path.exists(Inf2CatToolPath):
  raise Exception("Can't find Inf2Cat on this machine.  Please install the Windows 10 WDK - https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit")

#Make Cat file
cmd = "\"" + Inf2CatToolPath + "\"" + " /driver:. /os:10_X64 /verbose"
ret = os.system(cmd)
if(ret != 0):
  raise Exception("Creating Cat file Failed with errorcode %d" % ret)

if(PfxFile is not None):
  #dev sign the cat file
  cmd = "\"" + SignToolPath + "\"" + " sign /a /v /fd SHA256 /f " + "..\\" + PfxFile + " " + CatFileName
  ret = os.system(cmd)
  if(ret != 0):
    raise Exception("Signing Cat file Failed with errorcode %d" % ret)

print "\nWindows capsule created successfully in folder WindowsCapsule!!!"

sys.exit()
