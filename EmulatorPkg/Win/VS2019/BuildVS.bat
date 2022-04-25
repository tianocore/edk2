@REM SPDX-License-Identifier: BSD-2-Clause-Patent
@REM

cd ../../../
@call edksetup.bat
build -p EmulatorPkg\EmulatorPkg.dsc -t VS2019 %*
