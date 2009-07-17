## @file
# Convert an SPD Package class object ot a DEC Package class object by filling
# some fields required by DEC file.
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#

##
# Import Modules
#
import os
from Common.MigrationUtilities import *
from LoadSpd import LoadSpd
from StoreDec import StoreDec

#The default DEC version number tool generates.
gDecVersion = "0x00010005"


## Add required version information.
#
# Add the default DEC specification version to Package class object.
#
# @param  Package              An input Package class object.
#
def AddPackageMiscVersion(Package):
    PackageHeader = Package.Header
    PackageHeader.DecSpecification = gDecVersion

## Add package include information.
#
# Adds the default "Include" folder to if that directory exists.
#
# @param  Package              An input Package class object.
#
def AddPackageInclude(Package):
    PackageDir = os.path.dirname(Package.Header.FullPath)
    DefaultIncludeDir = os.path.join(PackageDir, "Include")
    if os.path.exists(DefaultIncludeDir):
        Include = IncludeClass()
        Include.FilePath = "Include"
        Package.Includes.insert(0, Include)

## Convert SPD Package class object to DEC Package class object.
#
# Convert SPD Package class ojbect to DEC Package class object by filling in
# several information required by DEC file.
#
# @param  Package              An input Package class object.
#
def ConvertSpdPackageToDecPackage(Package):
    AddPackageMiscVersion(Package)
    AddPackageInclude(Package)

# This acts like the main() function for the script, unless it is 'import'ed
# into another script.
if __name__ == '__main__':
    pass
    