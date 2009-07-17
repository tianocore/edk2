You may run these scripts to build a UEFI/PI gcc cross compiler.

Cross compilers built with these scripts are tested on
Linux, OS X and Cygwin.

Please note that you may need to modify your edk2 tree's Conf/tools_def.txt
file to point to the location where you installed the cross compiler.

=== tianoCross-gcc-4.1 ===

This script will build an x86 (ia32) cross compiler.

The results of this script are very similar to the 'mingw' cross compiler
which is commonly available on linux and OS X.  But, since the cross
compiler produced by this script is tested, it is the only 'supported' way
to build UEFI/PI images.

To use this script, you will need:

 * A recent version (3.0 or later should be fine) of gcc that is able to produce
   executables for the machine that you want to run this compiler on (the host
   machine).
 * wget or curl
 * tar
 * bzip
 * gzip
 * bash
 * As well as (possibly) others tools and development packages

=== x86_64-mingw-gcc-build.py ==

This script will build an x86_64 (x64/Intel 64/amd64) cross compiler.

To use this script, you will need:

 * A recent version (3.0 or later should be fine) of gcc that is able to
   produce executables for the machine that you want to run this compiler
   on (the host machine).
 * Python 2.5
 * texinfo
 * bison
 * flex
 * libmpfr
 * libgmp
 * As well as (possibly) others tools and development packages

=== Ubuntu Notes ===

On Ubuntu, the following command should install all the necessary build
packages to utilize the x86_64-mingw-gcc-build.py script:

  sudo apt-get install build-essential texinfo bison flex libgmp3-dev libmpfr-dev

=== CYGWIN Notes ===

You should setup cygwin to use binmode on all mounts. When you initially
install cygwin it gives you the choice of Unix file mode (recommended) or DOS
file mode. Unix mode will cause all the cygwin directories to be mounted in
binmode, while DOS will mount the dirs in textmode. Here is an example of a
cygwin install where the dirs are (properly) mounted in binmode.

C:\cygwin\bin on /usr/bin type user (binmode)
C:\cygwin\lib on /usr/lib type user (binmode)
c:\workspace on /workspace type system (binmode)
C:\cygwin on / type user (binmode)

If you use textmode, it is likely that the build will fail in a way that is
hard to debug.

Cygwin is pretty slow, so it is not recommended for large builds.

