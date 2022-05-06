## @file
# This file is used to define the Fmmt Logger.
#
# Copyright (c) 2021-, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

##

import logging
import sys
import os

logfile = 'FMMT_Build.log'
if os.path.exists(logfile):
    os.remove(logfile)

FmmtLogger = logging.getLogger('FMMT')
FmmtLogger.setLevel(logging.DEBUG)

log_stream_handler=logging.StreamHandler(sys.stdout)
log_file_handler=logging.FileHandler(logfile)
log_stream_handler.setLevel(logging.INFO)

stream_format=logging.Formatter("%(levelname)-8s: %(message)s")
file_format=logging.Formatter("%(levelname)-8s: %(message)s")

log_stream_handler.setFormatter(stream_format)
log_file_handler.setFormatter(file_format)

FmmtLogger.addHandler(log_stream_handler)
FmmtLogger.addHandler(log_file_handler)
