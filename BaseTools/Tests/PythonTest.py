## @file
# Test whether PYTHON_COMMAND is available and the
# minimum Python version is installed.
#
# Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import sys

if __name__ == '__main__':
    # Check if the major and minor versions required were specified.
    if len(sys.argv) >= 3:
        req_major_version = int(sys.argv[1])
        req_minor_version = int(sys.argv[2])
    else:
        # If the minimum version wasn't specified on the command line,
        # default to 3.6 because BaseTools uses syntax from PEP 526
        # (https://peps.python.org/pep-0526/)
        req_major_version = 3
        req_minor_version = 6

    if sys.version_info.major == req_major_version and \
       sys.version_info.minor >= req_minor_version:
        sys.exit(0)
    else:
        sys.exit(1)
