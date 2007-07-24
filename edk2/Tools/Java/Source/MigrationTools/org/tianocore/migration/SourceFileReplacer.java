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

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.UsageTypes;

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

	private static final String[] specialhoblibfunc = { "BuildModuleHob",
			"BuildResourceDescriptorHob", "BuildFvHob", "BuildCpuHob",
			"BuildGuidDataHob", "BuildStackHob", "BuildBspStoreHob",
			"BuildMemoryAllocationHob" };

	private static final String[] peiserviceslibfunc = { "InstallPpi",
			"ReInstallPpi", "LocatePpi", "NotifyPpi", "GetBootMode",
			"SetBootMode", "GetHobList", "CreateHob", "FfsFindNextVolume",
			"FfsFindNextFile", "FfsFindSectionData", "InstallPeiMemory",
			"AllocatePages", "AllocatePool", "PeiResetSystem" };

	// ---------------------------------------inner
	// classes---------------------------------------//
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
			return replaceLibrary(wholeline, mi.hashmacro);
		}

		public boolean recognize(String filename) {
			return filename.contains(".h") || filename.contains(".H")
					|| filename.contains(".uni") || filename.contains(".s")
					|| filename.contains(".S") || filename.contains(".asm")
					|| (!filename.contains(".inf") && filename.contains(".i"));
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
			wholeline = replaceMacro(wholeline, mi.hashnonlocalmacro);
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
		public String operation(String wholeline) {
			// remove EFI_DRIVER_ENTRY_POINT
			wholeline = wholeline.replaceAll(
					"(EFI_[A-Z]+_ENTRY_POINT\\s*\\(\\s*\\w(\\w|\\d)*\\s*\\))",
					MigrationTool.MIGRATIONCOMMENT + " $1");
			// redefine module entry point for some self-relocated modules
			wholeline = wholeline.replaceAll(mi.entrypoint + "([^{]*?})",
					"_ModuleEntryPoint" + "$1");
			// remove R8 library contractor
			wholeline = wholeline.replaceAll(
					"(\\b(?:Efi|Dxe)InitializeDriverLib\\b)",
					MigrationTool.MIGRATIONCOMMENT + " $1");
			// Add Library Class for potential reference of gBS, gRT & gDS.
			if (Common.find(wholeline, "\\bg?BS\\b")) {
				mi.hashrequiredr9libs.add("UefiBootServicesTableLib");
			}
			if (Common.find(wholeline, "\\bg?RT\\b")) {
				mi.hashrequiredr9libs.add("UefiRuntimeServicesTableLib");
			}
			if (Common.find(wholeline, "\\bgDS\\b")) {
				mi.hashrequiredr9libs.add("DxeServicesTableLib");
			}

			wholeline = replaceLibrary(wholeline, mi.hashnonlocalfunc);
			wholeline = replaceLibrary(wholeline, mi.hashmacro);
			// Converting macro
			wholeline = replaceMacro(wholeline, mi.hashnonlocalmacro);

			// Converting guid
			replaceGuid(wholeline, mi.guids, "guid", fileguid);
			replaceGuid(wholeline, mi.ppis, "ppi", fileppi);
			replaceGuid(wholeline, mi.protocols, "protocol", fileprotocol);

			// Converting Pei
			if (mi.getModuleType().matches("PEIM")) {
				//
				// Try to remove PeiServicesTablePointer;
				// 
				wholeline = dropPeiServicesPointer(wholeline);
				//
				// Drop the possible return Status of Hob building function.
				// 
				wholeline = drophobLibReturnStatus(wholeline);
			}
			//
			// Expand obsolete R8 macro.
			// 
			wholeline = replaceObsoleteMacro(wholeline);

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

	// ---------------------------------------inner
	// classes---------------------------------------//

	// -------------------------------------process
	// functions-------------------------------------//
	private static final String addincludefile(String wholeline, String hfile) {
		return wholeline.replaceFirst("(\\*/\\s)", "$1\n#include " + hfile
				+ "\n");
	}

	private static final void show(Set<r8tor9> hash, String sh) {
		Iterator<r8tor9> it = hash.iterator();
		r8tor9 temp;
		if (!hash.isEmpty()) {
			MigrationTool.ui.print("Converting " + sh + " : ");
			while (it.hasNext()) {
				temp = it.next();
				MigrationTool.ui.print("[" + temp.r8thing + "->" + temp.r9thing
						+ "] ");
			}
			MigrationTool.ui.println("");
		}
	}

	private static final void replaceGuid(String line, Set<String> hash,
			String kind, Set<r8tor9> filehash) {
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

	private final String dropPeiServicesPointer(String wholeline) {
		String peiServicesTablePointer;
		String peiServicesTableCaller;
		String regPeiServices;
		Pattern ptnPei;
		Matcher mtrPei;

		peiServicesTablePointer = "\\w(?:\\w|[0-9]|->)*";
		peiServicesTableCaller = "\\(\\*\\*?\\s*(" + peiServicesTablePointer
				+ ")\\s*\\)[.-]>?\\s*";
		for (int i = 0; i < peiserviceslibfunc.length; i++) {
			regPeiServices = peiServicesTableCaller + peiserviceslibfunc[i]
					+ "\\s*\\(\\s*\\1\\s*,(\\t| )*";
			ptnPei = Pattern.compile(regPeiServices);
			mtrPei = ptnPei.matcher(wholeline);
			if (mtrPei.find()) {
				wholeline = mtrPei.replaceAll("PeiServices"
						+ peiserviceslibfunc[i] + " (");
				mi.hashrequiredr9libs.add("PeiServicesLib");
			}
		}
		regPeiServices = peiServicesTableCaller + "(CopyMem|SetMem)"
				+ "\\s*\\((\\t| )*";
		ptnPei = Pattern.compile(regPeiServices);
		mtrPei = ptnPei.matcher(wholeline);
		if (mtrPei.find()) {
			wholeline = mtrPei.replaceAll("$2 (");
			mi.hashrequiredr9libs.add("BaseMemoryLib");
		}

		ptnPei = Pattern.compile("#%+(\\s*\\(+\\s*)" + peiServicesTablePointer
				+ "\\s*,\\s*", Pattern.MULTILINE);
		mtrPei = ptnPei.matcher(wholeline);
		while (mtrPei.find()) {
			wholeline = mtrPei.replaceAll("$1");
		}

		return wholeline;
	}

	private final String drophobLibReturnStatus(String wholeline) { // or use
																	// regex to
																	// find
																	// pattern
																	// "Status =
																	// ..."
		Pattern ptnhobstatus;
		Matcher mtrhobstatus;
		String templine = wholeline;
		for (int i = 0; i < specialhoblibfunc.length; i++) {
			do {
				ptnhobstatus = Pattern.compile(
						"((?:\t| )*)(\\w(?:\\w|\\d)*)\\s*=\\s*"
								+ specialhoblibfunc[i] + "(.*?;)",
						Pattern.DOTALL);
				mtrhobstatus = ptnhobstatus.matcher(templine);
				if (!mtrhobstatus.find()) {
					break;
				}
				String captureIndent = mtrhobstatus.group(1);
				String captureStatus = mtrhobstatus.group(2);
				String replaceString = captureIndent + specialhoblibfunc[i]
						+ mtrhobstatus.group(3) + "\n";
				replaceString += captureIndent
						+ MigrationTool.MIGRATIONCOMMENT
						+ "R9 Hob-building library functions will assert if build failure.\n";
				replaceString += captureIndent + captureStatus
						+ " = EFI_SUCCESS;";
				templine = mtrhobstatus.replaceFirst(replaceString);
			} while (true);
		}
		return templine;
	}

	private final String replaceMacro(String wholeline, Set<String> symbolSet) {
		String r8thing;
		String r9thing;
		Iterator<String> it;

		it = symbolSet.iterator();
		while (it.hasNext()) { // macros are all assumed MdePkg currently
			r8thing = it.next();
			// mi.hashrequiredr9libs.add(MigrationTool.db.getR9Lib(r8thing));
			if ((r9thing = MigrationTool.db.getR9Macro(r8thing)) != null) {
				if (wholeline.contains(r8thing)) {
					String findString = "(?<!(?:\\d|\\w))" + r8thing
							+ "(?!(?:\\d|\\w))";
					wholeline = wholeline.replaceAll(findString, r9thing);
					filemacro.add(new r8tor9(r8thing, r9thing));
				}
			}
		}
		return wholeline;
	}

	private final String replaceLibrary(String wholeline, Set<String> symbolSet) {
		boolean addr8 = false;
		// start replacing names
		String r8thing;
		String r9thing;
		Iterator<String> it;
		// Converting non-locla function
		it = symbolSet.iterator();
		while (it.hasNext()) {
			r8thing = it.next();
			mi.addLibraryClass(MigrationTool.db.getR9Lib(r8thing),
					UsageTypes.ALWAYS_CONSUMED);
			// mi.hashrequiredr9libs.add(MigrationTool.db.getR9Lib(r8thing)); //
			// add a library here

			r8tor9 temp;
			if ((r9thing = MigrationTool.db.getR9Func(r8thing)) != null) {
				if (!r8thing.equals(r9thing)) {
					if (wholeline.contains(r8thing)) {
						String findString = "(?<!(?:\\d|\\w))" + r8thing
								+ "(?!(?:\\d|\\w))";
						wholeline = wholeline.replaceAll(findString, r9thing);
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
		} // is any of the guids changed?
		if (addr8 == true) {
			wholeline = addincludefile(wholeline, "\"R8Lib.h\"");
		}
		return wholeline;
	}

	private final String replaceObsoleteMacro(String wholeline) {
		Matcher mtrmac;
		mtrmac = Pattern.compile("EFI_IDIV_ROUND\\((.*), (.*)\\)").matcher(
				wholeline);
		if (mtrmac.find()) {
			wholeline = mtrmac
					.replaceAll("\\($1 \\/ $2 \\+ \\(\\(\\(2 \\* \\($1 \\% $2\\)\\) \\< $2\\) \\? 0 \\: 1\\)\\)");
		}
		mtrmac = Pattern.compile("EFI_MIN\\((.*), (.*)\\)").matcher(wholeline);
		if (mtrmac.find()) {
			wholeline = mtrmac
					.replaceAll("\\(\\($1 \\< $2\\) \\? $1 \\: $2\\)");
		}
		mtrmac = Pattern.compile("EFI_MAX\\((.*), (.*)\\)").matcher(wholeline);
		if (mtrmac.find()) {
			wholeline = mtrmac
					.replaceAll("\\(\\($1 \\> $2\\) \\? $1 \\: $2\\)");
		}
		mtrmac = Pattern.compile("EFI_UINTN_ALIGNED\\((.*)\\)").matcher(
				wholeline);
		if (mtrmac.find()) {
			wholeline = mtrmac
					.replaceAll("\\(\\(\\(UINTN\\) $1\\) \\& \\(sizeof \\(UINTN\\) \\- 1\\)\\)");
		}
		if (wholeline.contains("EFI_UINTN_ALIGN_MASK")) {
			wholeline = wholeline.replaceAll("EFI_UINTN_ALIGN_MASK",
					"(sizeof (UINTN) - 1)");
		}
		return wholeline;
	}

	private final void addr8only() throws Exception {
		String paragraph = null;
		String line = Common.file2string(MigrationTool.db.DatabasePath
				+ File.separator + "R8Lib.c");
		PrintWriter outfile1 = new PrintWriter(new BufferedWriter(
				new FileWriter(MigrationTool.ModuleInfoMap.get(mi)
						+ File.separator + "Migration_" + mi.modulename
						+ File.separator + "R8Lib.c")));
		PrintWriter outfile2 = new PrintWriter(new BufferedWriter(
				new FileWriter(MigrationTool.ModuleInfoMap.get(mi)
						+ File.separator + "Migration_" + mi.modulename
						+ File.separator + "R8Lib.h")));
		Pattern ptnr8only = Pattern.compile(
				"////#?(\\w*)?(.*?R8_(\\w*).*?)////~", Pattern.DOTALL);
		Matcher mtrr8only = ptnr8only.matcher(line);
		Matcher mtrr8onlyhead;

		// add head comment
        if (mi.license != null) {
            String header = "/**@file\n  Copyright (c) 2007, Intel Corporation\n\n" + 
            mi.license.replace("      ", "  ") + "**/\n\n";		
            outfile1.append(header);
    		outfile2.append(header);
        }
        
    // add functions body
		while (mtrr8only.find()) {
			if (mi.hashr8only.contains(mtrr8only.group(3))) {
				paragraph = mtrr8only.group(2);
				outfile1.append(paragraph + "\n\n");
				if (mtrr8only.group(1).length() != 0) {
					mi.hashrequiredr9libs.add(mtrr8only.group(1));
				}
				// generate R8lib.h
				while ((mtrr8onlyhead = Func.ptnbrace.matcher(paragraph))
						.find()) {
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

	// -------------------------------------process
	// functions-------------------------------------//

	// -----------------------------------ForDoAll-----------------------------------//
	public void run(String filepath) throws Exception {
		String inname = filepath.replace(mi.temppath + File.separator, "");
		String tempinpath = mi.temppath + File.separator;
		String tempoutpath = MigrationTool.ModuleInfoMap.get(mi)
				+ File.separator + "Migration_" + mi.modulename
				+ File.separator;

		Iterator<Common.Laplace> itLaplace = Laplaces.iterator();
		while (itLaplace.hasNext()) {
			Common.Laplace lap = itLaplace.next();
			if (lap.recognize(inname)) {
				MigrationTool.ui.println("\nHandling file: " + inname);
				lap.transform(tempinpath + inname, tempoutpath
						+ lap.namechange(inname));
			}
		}
	}

	public boolean filter(File dir) {
		return true;
	}

	// -----------------------------------ForDoAll-----------------------------------//

	private final void setModuleInfo(ModuleInfo moduleinfo) {
		mi = moduleinfo;
	}

	private final void start() throws Exception {
		Laplaces.add(new DxsLaplace());
		Laplaces.add(new CLaplace());
		Laplaces.add(new IdleLaplace());

		Common.toDoAll(mi.temppath, this, Common.FILE);

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
