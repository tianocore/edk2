## @file
# package and install PyEfiCompressor extension
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
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
from distutils.core import setup, Extension
import os

if 'BASE_TOOLS_PATH' not in os.environ:
    raise "Please define BASE_TOOLS_PATH to the root of base tools tree"

BaseToolsDir = os.environ['BASE_TOOLS_PATH']
setup(
    name="PyUtility",
    version="0.01",
    ext_modules=[
        Extension(
            'PyUtility',
            sources=[
                'PyUtility.c'
                ],
            include_dirs=[
                os.path.join(BaseToolsDir, 'Source', 'C', 'Include'),
                os.path.join(BaseToolsDir, 'Source', 'C', 'Include', 'Ia32'),
                os.path.join(BaseToolsDir, 'Source', 'C', 'Common')
                ],
            )
        ],
  )

