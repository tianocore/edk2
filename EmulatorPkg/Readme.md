## Overview

EmulatorPkg provides an environment where a UEFI environment can be
emulated under an environment where a full UEFI compatible
environment is not possible.  (For example, running under an OS
where an OS process hosts the UEFI emulation environment.)

https://github.com/tianocore/tianocore.github.io/wiki/EmulatorPkg

## Status

* Builds and runs under
  *  a posix-like environment with X windows
      - Linux
      - OS X
  *  Windows environment
      - Win10 (verified)
      - Win8 (not verified)

## How to Build & Run
**You can use the following command to build.**
  * 32bit emulator in Windows:

    `build -p EmulatorPkg\EmulatorPkg.dsc -t VS2017 -D WIN_SEC_BUILD -a IA32`

  * 64bit emulator in Windows:

    `build -p EmulatorPkg\EmulatorPkg.dsc -t VS2017 -D WIN_SEC_BUILD -a X64`

  * 32bit emulator in Linux:

    `build -p EmulatorPkg\EmulatorPkg.dsc -t GCC5 -D UNIX_SEC_BUILD -a IA32`

  * 64bit emulator in Linux:

    `build -p EmulatorPkg\EmulatorPkg.dsc -t GCC5 -D UNIX_SEC_BUILD -a X64`

**You can start/run the emulator using the following command:**
  * 32bit emulator in Windows:

    `cd Build\EmulatorIA32\DEBUG_VS2017\IA32\ && WinHost.exe`

  * 64bit emulator in Windows:

    `cd Build\EmulatorX64\DEBUG_VS2017\X64\ && WinHost.exe`

  * 32bit emulator in Linux:

    `cd Build/EmulatorIA32/DEBUG_GCC5/IA32/ && ./Host`

  * 64bit emulator in Linux:

    `cd Build/EmulatorX64/DEBUG_GCC5/X64/ && ./Host`

**On posix-like environment with the bash shell you can use EmulatorPkg/build.sh to simplify building and running
emulator.**

For example, to build + run:

`$ EmulatorPkg/build.sh`  
`$ EmulatorPkg/build.sh run`

The build architecture will match your host machine's architecture.

On X64 host machines, you can build + run IA32 mode as well:

`$ EmulatorPkg/build.sh -a IA32`  
`$ EmulatorPkg/build.sh -a IA32 run`
