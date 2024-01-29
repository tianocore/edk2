# DXE Phase Debug
Allow your system to boot to the point that the DXE
core is initialized (so that the System Table and Debug Information table is
present in memory) and execute this script (using the toolbar button or
'do EfiLoadDxe "0xGST_ADDRESS"' from the command area). 'GST_ADDRESS' is the
address of the EFI_SYSTEM_TABLE, and can be found by the global `gST`.
The script will scan memory for the debug info table and load modules in it.

# SEC/PEI Phase Debug
There is no way to autodetect where these images reside so you must pass an
address for the memory-mapped Firmware Volume containing these images.  To do
this, enter 'do EfiLoadFv <addr>' where <addr> is the base address for the
firmware volume containing the SEC or PEI code.  To be more efficient you may
want to create a script that calls this, like MyBoardLoadSec.cmm which contains
the call to EfiLoadFv.  You can them map this script to a T32 menu or toolbar
button for quick access.
