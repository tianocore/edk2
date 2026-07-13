## @file
# pytest configuration for BaseTools tests
#
# Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

import pytest


def pytest_addoption(parser):
    parser.addoption(
        "--toolchain",
        default="VS2022",
        help="EDK II toolchain tag (default: VS2022)"
    )
