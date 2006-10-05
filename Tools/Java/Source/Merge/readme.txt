#
# Copyright (c) 2006, Intel Corporation   All rights reserved.
#
# This program and the accompanying materials are licensed and made
# available under the terms and conditions of the BSD License which
# accompanies this distribution.  The full text of the license may 
# be found at  http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Overview
--------
The Merge program is designed to merge the contents of similar modules into a
single module for size reduction. The combined module will link in one copy of
the libraries, rather than have multiple copies of the libraries linked in to
each individual driver.

Rules:
  The ModuleType must be identical for each of the (leaf) modules.
  At least one architecture type must be common for all modules, and the merged
module will only support the common architecture types.
  ALL modules to be merged must be in a directory structure below the location
of the merged module.
  The package must be within a directly directory line with the merged module's
MSA file. (Parent directories.)
  The copying of the files from the "leaf" directory into the merge module's
directory structure must be handled by an external program.
  The merge program must be run everytime a leaf module is modified.
  The external copy program must also be run everytime a leaf module is modified.
  Two or more leaf modules must be specified.
  The merged module must be added to a package (SPD) file before it can be used.
  PCD Driver Modules cannot be merged, nor combined with other modules.
  Leaf Module Global BuildOptions and UserExtensions are not merged.


merge Usage:  
  merge [-v] -t target [-u UiName] [-p PackageFile] dir1\leaf1 ... dirN\leafN [-h | -? | --help]
    where:
      -h | -? | --help            OPTIONAL - This Help Text
      -t Target                   REQUIRED - The Name of the new Merge Module MSA file
      -p Package                  OPTIONAL - The Name of the Package (SPD) file to add the target
      -u UiName                   OPTIONAL - The User Interface Name for the Target Module
      -v                          OPTIONAL - Verbose, print information messages.
      -o OutputFileBasename       OPTIONAL - Set the Output Filename for this module to Basename
      dir1\leaf1 ... dirN\leafN   REQUIRED The path to two or more MSA files that will be merged


