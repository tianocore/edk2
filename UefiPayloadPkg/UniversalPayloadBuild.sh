#!/usr/bin/env bash
#
# Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
#
# SPDX-License-Identifier: BSD-2-Clause-Patent
#

if command -v ${PYTHON_COMMAND} >/dev/null 2>&1; then
    echo python_exe=${PYTHON_COMMAND}
fi

# Get file path of UniversalPayloadBuild.sh
uplbld_filepath=${BASH_SOURCE:-$0}
# Remove ".sh" extension
uplbld_filepath_noext=${uplbld_filepath%.*}
# execute UniversalPayloadBuild.py to build UefiPayloadPkg
exec "${python_exe:-python}" "$uplbld_filepath_noext.py" "$@"
