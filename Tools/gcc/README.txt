You may run these scripts to build a Tiano Cross compiler. They have been
tested on Cygwin, OS X and Linux. You should expect to hack on these scripts to
make them work on your system. You may need to install additional tools on your
system to make the scripts work.

You will need

  A recent version of gcc that is able to produce executables for the machine
    that you want to run this compiler on (the host machine).
  wget or curl
  tar
  bzip
  gzip
  bash
  and possibly others

CYGWIN Notes

You must have the directory mounted as binary, or the build will not succeed.
In the example below, /workspace is mounted as binary.

C:\cygwin\bin on /usr/bin type user (textmode)
C:\cygwin\lib on /usr/lib type user (textmode)
c:\workspace on /workspace type system (binmode)
C:\cygwin on / type user (textmode)
c: on /cygdrive/c type user (textmode,noumount)
n: on /cygdrive/n type user (textmode,noumount)


