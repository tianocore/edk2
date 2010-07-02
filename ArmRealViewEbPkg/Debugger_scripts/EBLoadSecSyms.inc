// returns the base address of the SEC FV in flash on the EB board
// change this address for where your platform's SEC FV is located
// (or make it more intelligent to search for it)
define /r FindFv()
{
  return 0x40000000;
}
.

include /s 'ZZZZZZ/EfiFuncs.inc'
error=continue
unload ,all
error=abort
LoadPeiSec()
include C:\loadfiles.inc

