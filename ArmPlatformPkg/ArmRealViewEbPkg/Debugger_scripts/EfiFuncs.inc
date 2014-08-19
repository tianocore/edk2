error=abort

// NOTE: THIS MAY NEED TO BE ADJUSTED
// change to reflect the total amount of ram in your system
define /r GetMaxMem()
{
  return 0x10000000; // 256 MB
}
.

define /r GetWord(Addr)
{
  unsigned long data;

  if( (Addr & 0x2) == 0 )
  {
    data = dword(Addr);
    data = data & 0xffff;
    //$printf "getword data is %x\n", data$;
    return data;
  }
  else
  {
    data = dword(Addr & 0xfffffffc);
    //data = data >> 16;
    data = data / 0x10000;
    //$printf "getword data is %x (1)\n", data$;
    return data;
  }
}
.

define /r ProcessPE32(imgstart)
unsigned long imgstart;
{
  unsigned long filehdrstart;
  unsigned long debugdirentryrva;
	unsigned long debugtype;
  unsigned long debugrva;
	unsigned long dwarfsig;
  unsigned long baseofcode;
  unsigned long baseofdata;
  unsigned long elfbase;
  char *elfpath;

	$printf "PE32 image found at %x",imgstart$;

	//$printf "PE file hdr offset %x",dword(imgstart+0x3C)$;

  // offset from dos hdr to PE file hdr
  filehdrstart = imgstart + dword(imgstart+0x3C);

  // offset to debug dir in PE hdrs
	//$printf "debug dir is at %x",(filehdrstart+0xA8)$;
	debugdirentryrva = dword(filehdrstart + 0xA8);
  if(debugdirentryrva == 0)
	{
		$printf "no debug dir for image at %x",imgstart$;
	  return;
	}

	//$printf "debug dir entry rva is %x",debugdirentryrva$;

	debugtype = dword(imgstart + debugdirentryrva + 0xc);
  if( (debugtype != 0xdf) && (debugtype != 0x2) )
	{
		$printf "debug type is not dwarf for image at %x",imgstart$;
		$printf "debug type is %x",debugtype$;
		return;
	}

	debugrva = dword(imgstart + debugdirentryrva + 0x14);
	dwarfsig = dword(imgstart + debugrva);
	if(dwarfsig != 0x66727764)
	{
		$printf "dwarf debug signature not found for image at %x",imgstart$;
	  return;
	}

  elfpath = (char *)(imgstart + debugrva + 0xc);

  baseofcode = imgstart + dword(filehdrstart + 0x28);
  baseofdata = imgstart + dword(filehdrstart + 0x2c);

  if( (baseofcode < baseofdata) && (baseofcode != 0) )
  {
		elfbase = baseofcode;
  }
  else
  {
    elfbase = baseofdata;
  }

	$printf "found path %s",elfpath$;
	$fprintf 50, "load /ni /np /a %s &0x%x\n",elfpath,elfbase$;
}
.

define /r ProcessTE(imgstart)
unsigned long imgstart;
{
  unsigned long strippedsize;
	unsigned long debugdirentryrva;
	unsigned long debugtype;
  unsigned long debugrva;
	unsigned long dwarfsig;
  unsigned long elfbase;
  char *elfpath;

	$printf "TE image found at %x",imgstart$;

  // determine pe header bytes removed to account for in rva references
  //strippedsize = word(imgstart + 0x6);
  //strippedsize = (dword(imgstart + 0x4) & 0xffff0000) >> 16;
  strippedsize = (dword(imgstart + 0x4) & 0xffff0000) / 0x10000;
  strippedsize = strippedsize - 0x28;

	debugdirentryrva = dword(imgstart + 0x20);
  if(debugdirentryrva == 0)
	{
		$printf "no debug dir for image at %x",imgstart$;
	  return;
	}
  debugdirentryrva = debugdirentryrva - strippedsize;

	//$printf "debug dir entry rva is %x",debugdirentryrva$;

	debugtype = dword(imgstart + debugdirentryrva + 0xc);
  if( (debugtype != 0xdf) && (debugtype != 0x2) )
	{
		$printf "debug type is not dwarf for image at %x",imgstart$;
		$printf "debug type is %x",debugtype$;
		return;
	}

	debugrva = dword(imgstart + debugdirentryrva + 0x14);
  debugrva = debugrva - strippedsize;
	dwarfsig = dword(imgstart + debugrva);
	if( (dwarfsig != 0x66727764) && (dwarfsig != 0x3031424e) )
	{
		$printf "dwarf debug signature not found for image at %x",imgstart$;
		$printf "found %x", dwarfsig$;
	  return;
	}

	if( dwarfsig == 0x66727764 )
	{
		elfpath = (char *)(imgstart + debugrva + 0xc);
    $printf "looking for elf path at 0x%x", elfpath$;
  }
  else
  {
    elfpath = (char *)(imgstart + debugrva + 0x10);
    $printf "looking for elf path at 0x%x", elfpath$;
  }

  // elf base is baseofcode (we hope that for TE images it's not baseofdata)
  elfbase = imgstart + dword(imgstart + 0xc) - strippedsize;

	$printf "found path %s",elfpath$;
	$fprintf 50, "load /ni /np /a %s &0x%x\n",elfpath,elfbase$;
}
.

define /r ProcessFvSection(secstart)
unsigned long secstart;
{
  unsigned long sectionsize;
  unsigned char sectiontype;

	sectionsize = dword(secstart);
  //sectiontype = (sectionsize & 0xff000000) >> 24;
  sectiontype = (sectionsize & 0xff000000) / 0x1000000;
  sectionsize = sectionsize & 0x00ffffff;

	$printf "fv section at %x size %x type %x",secstart,sectionsize,sectiontype$;

	if(sectiontype == 0x10) // PE32
  {
		ProcessPE32(secstart+0x4);
	}
	else if(sectiontype == 0x12) // TE
	{
		ProcessTE(secstart+0x4);
 	}
}
.

define /r ProcessFfsFile(ffsfilestart)
unsigned long ffsfilestart;
{
  unsigned long ffsfilesize;
  unsigned long ffsfiletype;
  unsigned long secoffset;
  unsigned long secsize;

  //ffsfiletype = byte(ffsfilestart + 0x12);
  ffsfilesize = dword(ffsfilestart + 0x14);
  //ffsfiletype = (ffsfilesize & 0xff000000) >> 24;
  ffsfiletype = (ffsfilesize & 0xff000000) / 0x1000000;
  ffsfilesize = ffsfilesize & 0x00ffffff;

  if(ffsfiletype == 0xff) return;

	$printf "ffs file at %x size %x type %x",ffsfilestart,ffsfilesize,ffsfiletype$;

  secoffset = ffsfilestart + 0x18;

  // loop through sections in file
  while(secoffset < (ffsfilestart + ffsfilesize))
	{
		// process fv section and increment section offset by size
    secsize = dword(secoffset) & 0x00ffffff;
    ProcessFvSection(secoffset);
		secoffset = secoffset + secsize;

	  // align to next 4 byte boundary
    if( (secoffset & 0x3) != 0 )
		{
			secoffset = secoffset + (0x4 - (secoffset & 0x3));
		}
	} // end section loop
}
.

define /r LoadPeiSec()
{
  unsigned long fvbase;
  unsigned long fvlen;
  unsigned long fvsig;
	unsigned long ffsoffset;
  unsigned long ffsfilesize;

  fvbase = FindFv();
	$printf "fvbase %x",fvbase$;

  // get fv signature field
  fvsig = dword(fvbase + 0x28);
  if(fvsig != 0x4856465F)
  {
		$printf "FV does not have proper signature, exiting"$;
    return 0;
  }

	$printf "FV signature found"$;

  $fopen 50, 'C:\loadfiles.inc'$;

  fvlen = dword(fvbase + 0x20);

  // first ffs file is after fv header, use headerlength field
  //ffsoffset = (dword(fvbase + 0x30) & 0xffff0000) >> 16;
  ffsoffset = (dword(fvbase + 0x30) & 0xffff0000) / 0x10000;
  ffsoffset = fvbase + GetWord(fvbase + 0x30);

  // loop through ffs files
  while(ffsoffset < (fvbase+fvlen))
	{
		// process ffs file and increment by ffs file size field
 		ProcessFfsFile(ffsoffset);
 		ffsfilesize = (dword(ffsoffset + 0x14) & 0x00ffffff);
 		if(ffsfilesize == 0)
 		{
 		  break;
 		}
		ffsoffset = ffsoffset + ffsfilesize;


		// align to next 8 byte boundary
    if( (ffsoffset & 0x7) != 0 )
		{
			ffsoffset = ffsoffset + (0x8 - (ffsoffset & 0x7));
		}

  } // end fv ffs loop

	$vclose 50$;

}
.

define /r FindSystemTable(TopOfRam)
unsigned long TopOfRam;
{
  unsigned long offset;

  $printf "FindSystemTable"$;
  $printf "top of mem is %x",TopOfRam$;

  offset = TopOfRam;

  // align to highest 4MB boundary
  offset = offset & 0xFFC00000;

  // start at top and look on 4MB boundaries for system table ptr structure
  while(offset > 0)
  {
    //$printf "checking %x",offset$;
    //$printf "value is %x",dword(offset)$;

    // low signature match
    if(dword(offset) == 0x20494249)
    {
      // high signature match
      if(dword(offset+4) == 0x54535953)
      {
        // less than 4GB?
        if(dword(offset+0x0c) == 0)
        {
          // less than top of ram?
          if(dword(offset+8) < TopOfRam)
          {
            return(dword(offset+8));
          }
        }
      }

    }

    if(offset < 0x400000) break;
    offset = offset - 0x400000;
  }

  return 0;
}
.

define /r ProcessImage(ImageBase)
unsigned long ImageBase;
{
  $printf "ProcessImage %x", ImageBase$;
}
.

define /r FindDebugInfo(SystemTable)
unsigned long SystemTable;
{
  unsigned long   CfgTableEntries;
  unsigned long   ConfigTable;
  unsigned long   i;
  unsigned long   offset;
  unsigned long   dbghdr;
  unsigned long   dbgentries;
  unsigned long   dbgptr;
  unsigned long   dbginfo;
  unsigned long   loadedimg;

  $printf "FindDebugInfo"$;

  dbgentries = 0;
  CfgTableEntries = dword(SystemTable + 0x40);
  ConfigTable = dword(SystemTable + 0x44);

  $printf "config table is at %x (%d entries)", ConfigTable, CfgTableEntries$;

  // now search for debug info entry with guid 49152E77-1ADA-4764-B7A2-7AFEFED95E8B
  //	0x49152E77	0x47641ADA	0xFE7AA2B7	0x8B5ED9FE
  for(i=0; i<CfgTableEntries; i++)
  {
    offset = ConfigTable + (i*0x14);
    if(dword(offset) == 0x49152E77)
    {
      if(dword(offset+4) == 0x47641ADA)
      {
        if(dword(offset+8) == 0xFE7AA2B7)
        {
          if(dword(offset+0xc) == 0x8B5ED9FE)
          {
            dbghdr = dword(offset+0x10);
            dbgentries = dword(dbghdr + 4);
            dbgptr = dword(dbghdr + 8);
          }
        }
      }
    }
  }

  if(dbgentries == 0)
  {
    $printf "no debug entries found"$;
    return;
  }

  $printf "debug table at %x (%d entries)", dbgptr, dbgentries$;

  for(i=0; i<dbgentries; i++)
  {
    dbginfo = dword(dbgptr + (i*4));
    if(dbginfo != 0)
    {
      if(dword(dbginfo) == 1) // normal debug info type
      {
        loadedimg = dword(dbginfo + 4);
        ProcessPE32(dword(loadedimg + 0x20));
      }
    }
  }
}
.

define /r LoadDxe()
{
  unsigned long     maxmem;
  unsigned long     systbl;

  $printf "LoadDxe"$;

  $fopen 50, 'C:\loadfiles.inc'$;

  maxmem = GetMaxMem();
  systbl = FindSystemTable(maxmem);
  if(systbl != 0)
  {
    $printf "found system table at %x",systbl$;
    FindDebugInfo(systbl);
  }

  $vclose 50$;
}
.

define /r LoadRuntimeDxe()

{
  unsigned long   maxmem;
  unsigned long   SystemTable;
  unsigned long   CfgTableEntries;
  unsigned long   ConfigTable;
  unsigned long   i;
  unsigned long   offset;
  unsigned long   numentries;
  unsigned long   RuntimeDebugInfo;
  unsigned long   DebugInfoOffset;
  unsigned long   imgbase;

  $printf "LoadRuntimeDxe"$;

  $fopen 50, 'C:\loadfiles.inc'$;

  RuntimeDebugInfo = 0x80000010;

  if(RuntimeDebugInfo != 0)
  {
    numentries = dword(RuntimeDebugInfo);

    $printf "runtime debug info is at %x (%d entries)", RuntimeDebugInfo, numentries$;

    DebugInfoOffset = RuntimeDebugInfo + 0x4;
    for(i=0; i<numentries; i++)
    {
      imgbase = dword(DebugInfoOffset);
      if(imgbase != 0)
      {
        $printf "found image at %x",imgbase$;
        ProcessPE32(imgbase);
      }
      DebugInfoOffset = DebugInfoOffset + 0x4;
    }
  }

  $vclose 50$;
}
.
