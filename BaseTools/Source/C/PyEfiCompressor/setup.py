## @file
# package and install PyEfiCompressor extension
#
#  Copyright (c) 2008, Intel Corporation. All rights reserved.<BR>
#
#  SPDX-License-Identifier: BSD-2-Clause-Patent
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
    name="EfiCompressor",
    version="0.01",
    ext_modules=[
        Extension(
            'EfiCompressor',
            sources=[
                os.path.join(BaseToolsDir, 'Source', 'C', 'Common', 'Decompress.c'),
                'EfiCompressor.c'
                ],
            include_dirs=[
                os.path.join(BaseToolsDir, 'Source', 'C', 'Include'),
                os.path.join(BaseToolsDir, 'Source', 'C', 'Include', 'Ia32'),
                os.path.join(BaseToolsDir, 'Source', 'C', 'Common')
                ],
            )
        ],
  )

