## @file
# This file is used to define the Fmmt Logger.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

##

import logging
import sys

FmmtLogger = logging.getLogger('FMMT')
FmmtLogger.setLevel(logging.INFO)

lh=logging.StreamHandler(sys.stdout)
lf=logging.Formatter("%(levelname)-8s: %(message)s")
lh.setFormatter(lf)
FmmtLogger.addHandler(lh)