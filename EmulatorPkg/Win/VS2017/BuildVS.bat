cd ../../../
@call edksetup.bat
build -p EmulatorPkg\EmulatorPkg.dsc -t VS2017 -D WIN_SEC_BUILD %*
