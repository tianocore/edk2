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

public final class SourceFileReplacer implements Common.ForDoAll {
	private static final SourceFileReplacer SFReplacer = new SourceFileReplacer();
	private ModuleInfo mi;
	private static final Set<Common.Laplace> Laplaces = new HashSet<Common.Laplace>();

	// these sets are used only for printing log of the changes in current file
	private static final Set<r8tor9> filefunc = new HashSet<r8tor9>();
	private static final Set<r8tor9> filemacro = new HashSet<r8tor9>();
	private static final Set<r8tor9> fileguid = new HashSet<r8tor9>();
	private static final Set<r8tor9> fileppi = new HashSet<r8tor9>();
	private static final Set<r8tor9> fileprotocol = new HashSet<r8tor9>();
	private static final Set<String> filer8only = new HashSet<String>();
	
	//---------------------------------------inner classes---------------------------------------//
	private static class r8tor9 {
		r8tor9(String r8, String r9) {
			r8thing = r8;
			r9thing = r9;
		}
		public String r8thing;
		public String r9thing;
	}
	
	private class IdleLaplace extends Common.Laplace {
		public String operation(String wholeline) {
			return wholeline;
		}
		
		public boolean recognize(String filename) {
			return filename.contains(".h") || filename.contains(".H") || filename.contains(".uni");
		}

		public String namechange(String oldname) {
			if (oldname.contains(".H")) {
				return oldname.replaceFirst(".H", ".h");
			} else {
				return oldname;
			}
		}
	}
	private class DxsLaplace extends Common.Laplace {
		public String operation(String wholeline) {
			if (mi.getModuleType().equals("PEIM")) {
				return addincludefile(wholeline, "\\<PeimDepex.h\\>");
			} else {
				return addincludefile(wholeline, "\\<DxeDepex.h\\>");
			}
		}
		
		public boolean recognize(String filename) {
			return filename.contains(".dxs");
		}

		public String namechange(String oldname) {
			return oldname;
		}
	}
	
	private class CLaplace extends Common.Laplace {
		public  String operation(String wholeline) {
			boolean addr8 = false;

			Pattern pat = Pattern.compile("g?(BS|RT)(\\s*->\\s*)([a-zA-Z_]\\w*)", Pattern.MULTILINE);					// ! only two level () bracket allowed !
			//Pattern ptnpei = Pattern.compile("\\(\\*\\*?PeiServices\\)[.-][>]?\\s*(\\w*[#$]*)(\\s*\\(([^\\(\\)]*(\\([^\\(\\)]*\\))?[^\\(\\)]*)*\\))", Pattern.MULTILINE);

			// replace BS -> gBS , RT -> gRT
			Matcher mat = pat.matcher(wholeline);
			if (mat.find()) {												// add a library here
				MigrationTool.ui.println("Converting all BS->gBS, RT->gRT");
				wholeline = mat.replaceAll("g$1$2$3");							//unknown correctiveness
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
			// remove EFI_DRIVER_ENTRY_POINT
			wholeline = wholeline.replaceAll("(EFI_\\w+_ENTRY_POINT)", MigrationTool.MIGRATIONCOMMENT + " $1");
			
			// start replacing names
			String r8thing;
			String r9thing;
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
					mi.hashrequiredr9libs.add(MigrationTool.db.getR9Lib(r8thing));				// add a library here
				}

				r8tor9 temp;
				if ((r9thing = MigrationTool.db.getR9Func(r8thing)) != null) {
					if (!r8thing.equals(r9thing)) {
						if (wholeline.contains(r8thing)) {
							wholeline = wholeline.replaceAll(r8thing, r9thing);
							filefunc.add(new r8tor9(r8thing, r9thing));
							Iterator<r8tor9> rt = filefunc.iterator();
							while (rt.hasNext()) {
								temp = rt.next();
								if (MigrationTool.db.r8only.contains(temp.r8thing)) {
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
				wholeline = addincludefile(wholeline, "\"R8Lib.h\"");
			}
			
			// Converting macro
			it = mi.hashnonlocalmacro.iterator();
			while (it.hasNext()) {						//macros are all assumed MdePkg currently
				r8thing = it.next();
				//mi.hashrequiredr9libs.add(MigrationTool.db.getR9Lib(r8thing));		
				if ((r9thing = MigrationTool.db.getR9Macro(r8thing)) != null) {
					if (wholeline.contains(r8thing)) {
						wholeline = wholeline.replaceAll(r8thing, r9thing);
						filemacro.add(new r8tor9(r8thing, r9thing));
					}
				}
			}

			// Converting guid
			replaceGuid(wholeline, mi.guid, "guid", fileguid);
			replaceGuid(wholeline, mi.ppi, "ppi", fileppi);
			replaceGuid(wholeline, mi.protocol, "protocol", fileprotocol);

			// Converting Pei
			// First , find all (**PeiServices)-> or (*PeiServices). with arg "PeiServices" , change name and add #%
			Pattern ptnpei = Pattern.compile("\\(\\*\\*?PeiServices\\)[.-][>]?\\s*(\\w*)(\\s*\\(\\s*PeiServices\\s*,\\s*)", Pattern.MULTILINE);
			if (mi.moduletype.contains("PEIM")) {
				Matcher mtrpei = ptnpei.matcher(wholeline);
				while (mtrpei.find()) {										// ! add a library here !
					wholeline = mtrpei.replaceAll("PeiServices$1#%$2");
					mi.hashrequiredr9libs.add("PeiServicesLib");
				}
				mtrpei.reset();
				if (wholeline.contains("PeiServicesCopyMem")) {
					wholeline = wholeline.replaceAll("PeiServicesCopyMem#%", "CopyMem");
					mi.hashrequiredr9libs.add("BaseMemoryLib");
				}
				if (wholeline.contains("PeiServicesSetMem")) {
					wholeline = wholeline.replaceAll("PeiServicesSetMem#%", "SetMem");
					mi.hashrequiredr9libs.add("BaseMemoryLib");
				}

				// Second , find all #% to drop the arg "PeiServices"
				Pattern ptnpeiarg = Pattern.compile("#%+(\\s*\\(+\\s*)PeiServices\\s*,\\s*", Pattern.MULTILINE);
				Matcher mtrpeiarg = ptnpeiarg.matcher(wholeline);
				while (mtrpeiarg.find()) {
					wholeline = mtrpeiarg.replaceAll("$1");
				}
			}
			
			Matcher mtrmac;
			mtrmac = Pattern.compile("EFI_IDIV_ROUND\\((.*), (.*)\\)").matcher(wholeline);
			if (mtrmac.find()) {
				wholeline = mtrmac.replaceAll("\\($1 \\/ $2 \\+ \\(\\(\\(2 \\* \\($1 \\% $2\\)\\) \\< $2\\) \\? 0 \\: 1\\)\\)");
			}
			mtrmac = Pattern.compile("EFI_MIN\\((.*), (.*)\\)").matcher(wholeline);
			if (mtrmac.find()) {
				wholeline = mtrmac.replaceAll("\\(\\($1 \\< $2\\) \\? $1 \\: $2\\)");
			}
			mtrmac = Pattern.compile("EFI_MAX\\((.*), (.*)\\)").matcher(wholeline);
			if (mtrmac.find()) {
				wholeline = mtrmac.replaceAll("\\(\\($1 \\> $2\\) \\? $1 \\: $2\\)");
			}
			mtrmac = Pattern.compile("EFI_UINTN_ALIGNED\\((.*)\\)").matcher(wholeline);
			if (mtrmac.find()) {
				wholeline = mtrmac.replaceAll("\\(\\(\\(UINTN\\) $1\\) \\& \\(sizeof \\(UINTN\\) \\- 1\\)\\)");
			}
			if (wholeline.contains("EFI_UINTN_ALIGN_MASK")) {
				wholeline = wholeline.replaceAll("EFI_UINTN_ALIGN_MASK", "(sizeof (UINTN) - 1)");
			}

			show(filefunc, "function");
			show(filemacro, "macro");
			show(fileguid, "guid");
			show(fileppi, "ppi");
			show(fileprotocol, "protocol");
			if (!filer8only.isEmpty()) {
				MigrationTool.ui.println("Converting r8only : " + filer8only);
			}

			filefunc.clear();
			filemacro.clear();
			fileguid.clear();
			fileppi.clear();
			fileprotocol.clear();
			filer8only.clear();

			return wholeline;
		}
		
		public boolean recognize(String filename) {
			return filename.contains(".c") || filename.contains(".C");
		}

		public String namechange(String oldname) {
			if (oldname.contains(".C")) {
				return oldname.replaceFirst(".C", ".c");
			} else {
				return oldname;
			}
		}
	}
	//---------------------------------------inner classes---------------------------------------//
	
	private static final String addincludefile(String wholeline, String hfile) {
		return wholeline.replaceFirst("(\\*/\\s)", "$1\n#include " + hfile + "\n");
	}
	
	private static final void show(Set<r8tor9> hash, String sh) {
		Iterator<r8tor9> it = hash.iterator();
		r8tor9 temp;
		if (!hash.isEmpty()) {
			MigrationTool.ui.print("Converting " + sh + " : ");
			while (it.hasNext()) {
				temp = it.next();
				MigrationTool.ui.print("[" + temp.r8thing + "->" + temp.r9thing + "] ");
			}
			MigrationTool.ui.println("");
		}
	}

	private static final void replaceGuid(String line, Set<String> hash, String kind, Set<r8tor9> filehash) {
		Iterator<String> it;
		String r8thing;
		String r9thing;
		it = hash.iterator();
		while (it.hasNext()) {
			r8thing = it.next();
			if ((r9thing = MigrationTool.db.getR9Guidname(r8thing)) != null) {
				if (!r8thing.equals(r9thing)) {
					if (line.contains(r8thing)) {
						line = line.replaceAll(r8thing, r9thing);
						filehash.add(new r8tor9(r8thing, r9thing));
					}
				}
			}
		}
	}

	private final void addr8only() throws Exception {
		String paragraph = null;
		String line = Common.file2string(MigrationTool.db.DatabasePath + File.separator + "R8Lib.c");
		PrintWriter outfile1 = new PrintWriter(new BufferedWriter(new FileWriter(MigrationTool.ModuleInfoMap.get(mi) + File.separator + "Migration_" + mi.modulename + File.separator + "R8Lib.c")));
		PrintWriter outfile2 = new PrintWriter(new BufferedWriter(new FileWriter(MigrationTool.ModuleInfoMap.get(mi) + File.separator + "Migration_" + mi.modulename + File.separator + "R8Lib.h")));
		Pattern ptnr8only = Pattern.compile("////#?(\\w*)?(.*?R8_(\\w*).*?)////~", Pattern.DOTALL);
		Matcher mtrr8only = ptnr8only.matcher(line);
		Matcher mtrr8onlyhead;
		
		//add head comment
		Matcher mtrr8onlyheadcomment = Critic.PTN_NEW_HEAD_COMMENT.matcher(line);
		if (mtrr8onlyheadcomment.find()) {
			outfile1.append(mtrr8onlyheadcomment.group() + "\n\n");
			outfile2.append(mtrr8onlyheadcomment.group() + "\n\n");
		}
		
		//add functions body
		while (mtrr8only.find()) {
			if (mi.hashr8only.contains(mtrr8only.group(3))) {
				paragraph = mtrr8only.group(2);
				outfile1.append(paragraph + "\n\n");
				if (mtrr8only.group(1).length() != 0) {
					mi.hashrequiredr9libs.add(mtrr8only.group(1));
				}
				//generate R8lib.h
				while ((mtrr8onlyhead = Func.ptnbrace.matcher(paragraph)).find()) {
					paragraph = mtrr8onlyhead.replaceAll(";\n//");
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
	
	//-----------------------------------ForDoAll-----------------------------------//
	public void run(String filepath) throws Exception {
		String inname = filepath.replace(mi.modulepath + File.separator, "");
		String tempinpath = mi.modulepath + File.separator + "temp" + File.separator;
		String tempoutpath = MigrationTool.ModuleInfoMap.get(mi) + File.separator + "Migration_" + mi.modulename + File.separator;

		Iterator<Common.Laplace> itLaplace = Laplaces.iterator();
		while (itLaplace.hasNext()) {
			Common.Laplace lap = itLaplace.next();
			if (lap.recognize(inname)) {
				MigrationTool.ui.println("\nHandling file: " + inname);
				lap.transform(tempinpath + inname, tempoutpath + lap.namechange(inname));
			}
		}
	}
	
	public boolean filter(File dir) {
		return true;
	}
	//-----------------------------------ForDoAll-----------------------------------//
	
	private final void setModuleInfo(ModuleInfo moduleinfo) {
		mi = moduleinfo;
	}
	
	private final void start() throws Exception {
		Laplaces.add(new DxsLaplace());
		Laplaces.add(new CLaplace());
		Laplaces.add(new IdleLaplace());
		
		Common.toDoAll(mi.localmodulesources, this);
		
		if (!mi.hashr8only.isEmpty()) {
			addr8only();
		}
		
		Laplaces.clear();
	}
	
	public static final void fireAt(ModuleInfo moduleinfo) throws Exception {
		SFReplacer.setModuleInfo(moduleinfo);
		SFReplacer.start();
	}
}
