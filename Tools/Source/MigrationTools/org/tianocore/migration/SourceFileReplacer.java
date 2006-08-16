/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.migration;

import java.io.*;
import java.util.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class SourceFileReplacer {
	SourceFileReplacer(String path, ModuleInfo moduleinfo, Database database, UI fp) {
		modulepath = path;
		mi = moduleinfo;
		db = database;
		ui = fp;
	}
	private String modulepath;
	private ModuleInfo mi;
	private Database db;
	private UI ui;
	private boolean showdetails = false;
	
	private class r8tor9 {
		r8tor9(String r8, String r9) {
			r8thing = r8;
			r9thing = r9;
		}
		public String r8thing;
		public String r9thing;
	}
	
	// these sets are used only for printing log of the changes in current file
	private Set<r8tor9> filefunc = new HashSet<r8tor9>();
	private Set<r8tor9> filemacro = new HashSet<r8tor9>();
	private Set<r8tor9> fileguid = new HashSet<r8tor9>();
	private Set<r8tor9> fileppi = new HashSet<r8tor9>();
	private Set<r8tor9> fileprotocol = new HashSet<r8tor9>();
	private Set<String> filer8only = new HashSet<String>();
	
	private String r8only = "EfiLibInstallDriverBinding " +
			"EfiLibInstallAllDriverProtocols " +
			"EfiLibCompareLanguage " +
			"BufToHexString " +
			"EfiStrTrim " +							//is the r8only lib going to be enlarged????  Caution !!!!
			"EfiValueToHexStr " +
			"HexStringToBuf " +
			"IsHexDigit " +
			"NibbleToHexChar " +
			"GetHob " +
			"GetHobListSize " +
			"GetHobVersion " +
			"GetHobBootMode " +
			"GetCpuHobInfo " +
			"GetDxeCoreHobInfo " +
			"GetNextFirmwareVolumeHob " +
			"GetNextGuidHob " +
			"GetPalEntryHobInfo " +
			"GetIoPortSpaceAddressHobInfo ";
	
	public void flush() throws Exception {
		PrintWriter outfile;
		String outname = null;
		String inname = null;
		if (ui.yesOrNo("Changes will be made to the Source Code.  View details?")) {
			showdetails = true;
		}
		
		Iterator<String> di = mi.localmodulesources.iterator();
		while (di.hasNext()) {
			inname = di.next();
			if (inname.contains(".c") || inname.contains(".C")) {
				if (inname.contains(".C")) {
					outname = inname.replaceFirst(".C", ".c");
				} else {
					outname = inname;
				}
				ui.println("\nModifying file: " + inname);
				Common.string2file(sourcefilereplace(modulepath + File.separator + "temp" + File.separator + inname), modulepath + File.separator + "result" + File.separator + outname);
				/*
				Common.ensureDir(modulepath + File.separator + "result" + File.separator + outname);
				outfile = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "result" + File.separator + outname)));
				outfile.append(sourcefilereplace(modulepath + File.separator + "temp" + File.separator + inname));
				outfile.flush();
				outfile.close();
				*/
			} else if (inname.contains(".h") || inname.contains(".H") || inname.contains(".dxs") || inname.contains(".uni")) {
				if (inname.contains(".H")) {
					outname = inname.replaceFirst(".H", ".h");
				} else {
					outname = inname;
				}
				ui.println("\nCopying file: " + inname);
				Common.string2file(Common.file2string(modulepath + File.separator + "temp" + File.separator + inname), modulepath + File.separator + "result" + File.separator + outname);
				/*
				Common.ensureDir(modulepath + File.separator + "result" + File.separator + outname);
				outfile = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "result" + File.separator + outname)));
				outfile.append(Common.file2string(modulepath + File.separator + "temp" + File.separator + inname));
				outfile.flush();
				outfile.close();
				*/
			}
		}

		if (!mi.hashr8only.isEmpty()) {
			addr8only();
		}
	}
	
	private void addr8only() throws Exception {
		String paragraph = null;
		String line = Common.file2string(Database.defaultpath + File.separator + "R8Lib.c");
		Common.ensureDir(modulepath + File.separator + "result" + File.separator + "R8Lib.c");
		PrintWriter outfile1 = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "result" + File.separator + "R8Lib.c")));
		PrintWriter outfile2 = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "result" + File.separator + "R8Lib.h")));
		Pattern ptnr8only = Pattern.compile("////#?(\\w*)?.*?R8_(\\w*).*?////~", Pattern.DOTALL);
		Matcher mtrr8only = ptnr8only.matcher(line);
		Matcher mtrr8onlyhead;
		while (mtrr8only.find()) {
			if (mi.hashr8only.contains(mtrr8only.group(2))) {
				paragraph = mtrr8only.group();
				outfile1.append(paragraph + "\n\n");
				if (mtrr8only.group(1).length() != 0) {
					mi.hashrequiredr9libs.add(mtrr8only.group(1));
				}
				//generate R8lib.h
				while ((mtrr8onlyhead = Func.ptnbrace.matcher(paragraph)).find()) {
					paragraph = mtrr8onlyhead.replaceAll(";");
				}
				outfile2.append(paragraph + "\n\n");
			}
		}
		outfile1.flush();
		outfile1.close();
		outfile2.flush();
		outfile2.close();
		
		mi.localmodulesources.add("R8Lib.h");
		mi.localmodulesources.add("R8Lib.c");
	}
	
	// Caution : if there is @ in file , it will be replaced with \n , so is you use Doxygen ... God Bless you!
	private String sourcefilereplace(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(filename));
		StringBuffer wholefile = new StringBuffer();
		String line;
		String r8thing;
		String r9thing;
		r8tor9 temp;
		boolean addr8 = false;

		Pattern pat = Pattern.compile("g?(BS|RT)(\\s*->\\s*)([a-zA-Z_]\\w*)", Pattern.MULTILINE);					// ! only two level () bracket allowed !
		//Pattern ptnpei = Pattern.compile("\\(\\*\\*?PeiServices\\)[.-][>]?\\s*(\\w*[#$]*)(\\s*\\(([^\\(\\)]*(\\([^\\(\\)]*\\))?[^\\(\\)]*)*\\))", Pattern.MULTILINE);

		while ((line = rd.readLine()) != null) {
			wholefile.append(line + "\n");
		}
		line = wholefile.toString();
		
		// replace BS -> gBS , RT -> gRT
		Matcher mat = pat.matcher(line);
		if (mat.find()) {												// add a library here
			ui.println("Converting all BS->gBS, RT->gRT");
			line = mat.replaceAll("g$1$2$3");							//unknown correctiveness
		}
		mat.reset();
		while (mat.find()) {
			if (mat.group(1).matches("BS")) {
				mi.hashrequiredr9libs.add("UefiBootServicesTableLib");
			}
			if (mat.group(1).matches("RT")) {
				mi.hashrequiredr9libs.add("UefiRuntimeServicesTableLib");
			}
		}
		/*
		// remove EFI_DRIVER_ENTRY_POINT
		Pattern patentrypoint = Pattern.compile("EFI_DRIVER_ENTRY_POINT[^\\}]*\\}");
		Matcher matentrypoint = patentrypoint.matcher(line);
		if (matentrypoint.find()) {
			ui.println("Deleting Entry_Point");
			line = matentrypoint.replaceAll("");
		}
		*/
		// start replacing names
		Iterator<String> it;
		// Converting non-locla function
		it = mi.hashnonlocalfunc.iterator();
		while (it.hasNext()) {
			r8thing = it.next();
			if (r8thing.matches("EfiInitializeDriverLib")) {					//s
				mi.hashrequiredr9libs.add("UefiBootServicesTableLib");			//p
				mi.hashrequiredr9libs.add("UefiRuntimeServicesTableLib");		//e
			} else if (r8thing.matches("DxeInitializeDriverLib")) {				//c
				mi.hashrequiredr9libs.add("UefiBootServicesTableLib");			//i
				mi.hashrequiredr9libs.add("UefiRuntimeServicesTableLib");		//a
				mi.hashrequiredr9libs.add("DxeServicesTableLib");				//l
			} else {															//
				mi.hashrequiredr9libs.add(db.getR9Lib(r8thing));				// add a library here
			}
			
			if ((r9thing = db.getR9Func(r8thing)) != null) {
				if (!r8thing.equals(r9thing)) {
					if (line.contains(r8thing)) {
						line = line.replaceAll(r8thing, r9thing);
						filefunc.add(new r8tor9(r8thing, r9thing));
						Iterator<r8tor9> rt = filefunc.iterator();
						while (rt.hasNext()) {
							temp = rt.next();
							if (r8only.contains(temp.r8thing)) {
								filer8only.add(r8thing);
								mi.hashr8only.add(r8thing);
								addr8 = true;
							}
						}
					}
				}
			}
		}															//is any of the guids changed?
		if (addr8 == true) {
			line = line.replaceFirst("\\*/\n", "\\*/\n#include \"R8Lib.h\"\n");
		}
		
		// Converting macro
		it = mi.hashnonlocalmacro.iterator();
		while (it.hasNext()) {						//macros are all assumed MdePkg currently
			r8thing = it.next();
			//mi.hashrequiredr9libs.add(db.getR9Lib(r8thing));		
			if ((r9thing = db.getR9Macro(r8thing)) != null) {
				if (line.contains(r8thing)) {
					line = line.replaceAll(r8thing, r9thing);
					filemacro.add(new r8tor9(r8thing, r9thing));
				}
			}
		}

		// Converting guid
		replaceGuid(line, mi.guid, "guid", fileguid);
		replaceGuid(line, mi.ppi, "ppi", fileppi);
		replaceGuid(line, mi.protocol, "protocol", fileprotocol);

		// Converting Pei
		// First , find all (**PeiServices)-> or (*PeiServices). with arg "PeiServices" , change name and add #%
		Pattern ptnpei = Pattern.compile("\\(\\*\\*?PeiServices\\)[.-][>]?\\s*(\\w*)(\\s*\\(\\s*PeiServices\\s*,\\s*)", Pattern.MULTILINE);
		if (mi.moduletype.contains("PEIM")) {
			Matcher mtrpei = ptnpei.matcher(line);
			while (mtrpei.find()) {										// ! add a library here !
				line = mtrpei.replaceAll("PeiServices$1#%$2");
				mi.hashrequiredr9libs.add("PeiServicesLib");
			}
			mtrpei.reset();
			if (line.contains("PeiServicesCopyMem")) {
				line = line.replaceAll("PeiServicesCopyMem#%", "CopyMem");
				mi.hashrequiredr9libs.add("BaseMemoryLib");
			}
			if (line.contains("PeiServicesSetMem")) {
				line = line.replaceAll("PeiServicesSetMem#%", "SetMem");
				mi.hashrequiredr9libs.add("BaseMemoryLib");
			}

			// Second , find all #% to drop the arg "PeiServices"
			Pattern ptnpeiarg = Pattern.compile("#%+(\\s*\\(+\\s*)PeiServices\\s*,\\s*", Pattern.MULTILINE);
			Matcher mtrpeiarg = ptnpeiarg.matcher(line);
			while (mtrpeiarg.find()) {
				line = mtrpeiarg.replaceAll("$1");
			}
		}
		
		Matcher mtrmac;
		mtrmac = Pattern.compile("EFI_IDIV_ROUND\\((.*), (.*)\\)").matcher(line);
		if (mtrmac.find()) {
			line = mtrmac.replaceAll("\\($1 \\/ $2 \\+ \\(\\(\\(2 \\* \\($1 \\% $2\\)\\) \\< $2\\) \\? 0 \\: 1\\)\\)");
		}
		mtrmac = Pattern.compile("EFI_MIN\\((.*), (.*)\\)").matcher(line);
		if (mtrmac.find()) {
			line = mtrmac.replaceAll("\\(\\($1 \\< $2\\) \\? $1 \\: $2\\)");
		}
		mtrmac = Pattern.compile("EFI_MAX\\((.*), (.*)\\)").matcher(line);
		if (mtrmac.find()) {
			line = mtrmac.replaceAll("\\(\\($1 \\> $2\\) \\? $1 \\: $2\\)");
		}
		mtrmac = Pattern.compile("EFI_UINTN_ALIGNED\\((.*)\\)").matcher(line);
		if (mtrmac.find()) {
			line = mtrmac.replaceAll("\\(\\(\\(UINTN\\) $1\\) \\& \\(sizeof \\(UINTN\\) \\- 1\\)\\)");
		}
		if (line.contains("EFI_UINTN_ALIGN_MASK")) {
			line = line.replaceAll("EFI_UINTN_ALIGN_MASK", "(sizeof (UINTN) - 1)");
		}

		show(filefunc, "function");
		show(filemacro, "macro");
		show(fileguid, "guid");
		show(fileppi, "ppi");
		show(fileprotocol, "protocol");
		if (!filer8only.isEmpty()) {
			ui.println("Converting r8only : " + filer8only);
		}

		filefunc.clear();
		filemacro.clear();
		fileguid.clear();
		fileppi.clear();
		fileprotocol.clear();
		filer8only.clear();

		return line;
	}
	
	private void show(Set<r8tor9> hash, String sh) {
		Iterator<r8tor9> it = hash.iterator();
		r8tor9 temp;
		if (showdetails) {
			if (!hash.isEmpty()) {
				ui.print("Converting " + sh + " : ");
				while (it.hasNext()) {
					temp = it.next();
					ui.print("[" + temp.r8thing + "->" + temp.r9thing + "] ");
				}
				ui.println("");
			}
		}
	}
	
	private void replaceGuid(String line, Set<String> hash, String kind, Set<r8tor9> filehash) {
		Iterator<String> it;
		String r8thing;
		String r9thing;
		it = hash.iterator();
		while (it.hasNext()) {
			r8thing = it.next();
			if ((r9thing = db.getR9Guidname(r8thing)) != null) {
				if (!r8thing.equals(r9thing)) {
					if (line.contains(r8thing)) {
						line = line.replaceAll(r8thing, r9thing);
						filehash.add(new r8tor9(r8thing, r9thing));
					}
				}
			}
		}
	}
}
