You may run these scripts to build a Tiano Cross compiler. They have been
tested on Cygwin, OS X and Linux. You should expect to hack on these scripts to
make them work on your system. You may need to install additional tools on your
system to make the scripts work.

You will need

  A recent version (3.0 or later should be fine) of gcc that is able to produce
    executables for the machine that you want to run this compiler on (the host
    machine).
  wget or curl
  tar
  bzip
  gzip
  bash
  and possibly others

CYGWIN Notes

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

