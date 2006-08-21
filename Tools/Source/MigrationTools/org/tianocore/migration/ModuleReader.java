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
import java.util.regex.*;
import org.tianocore.*;

public final class ModuleReader {
	ModuleReader(String path, ModuleInfo moduleinfo, Database database, UI u) {
		//modulepath = path;
		//mi = moduleinfo;
		db = database;
		ui = u;
	}
	//private static String modulepath;
	//private static ModuleInfo mi;
	private static Database db;
	private static UI ui;
	
	private static final Pattern ptninfequation = Pattern.compile("([^\\s]*)\\s*=\\s*([^\\s]*)");
	private static final Pattern ptnsection = Pattern.compile("\\[([^\\[\\]]*)\\]([^\\[\\]]*)\\n", Pattern.MULTILINE);
	private static final Pattern ptnfilename = Pattern.compile("[^\\s]+");
	
	public static final void readMsa(String name, ModuleInfo mi) throws Exception {
		ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.parse(new File(mi.modulepath + File.separator + name));
		ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = msadoc.getModuleSurfaceArea();
		MsaHeaderDocument.MsaHeader msaheader = msa.getMsaHeader();
		
		mi.modulename = msaheader.getModuleName();
		mi.guidvalue = msaheader.getGuidValue();
		mi.moduletype = msaheader.getModuleType().toString();		// ???
		
		SourceFilesDocument.SourceFiles sourcefiles = msa.getSourceFiles();
		
		String temp;
		Iterator<FilenameDocument.Filename> li = sourcefiles.getFilenameList().iterator();
		while (li.hasNext()) {
			if (!mi.localmodulesources.contains(temp = li.next().toString())) {
				System.out.println("Source File Missing! : " + temp);
			}
		}
	}
	
	public static final void readInf(String name, ModuleInfo mi) throws Exception {
		System.out.println("\nParsing INF file: " + name);
		String wholeline;
		Matcher mtrinfequation;
		Matcher mtrsection;
		Matcher mtrfilename;

		wholeline = Common.file2string(mi.modulepath + File.separator + name);
		mtrsection = ptnsection.matcher(wholeline);
		while (mtrsection.find()) {
			if (mtrsection.group(1).matches("defines")) {
				mtrinfequation = ptninfequation.matcher(mtrsection.group(2));
				while (mtrinfequation.find()) {
					if (mtrinfequation.group(1).matches("BASE_NAME")) {
						mi.modulename = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("FILE_GUID")) {
						mi.guidvalue = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("COMPONENT_TYPE")) {
						mi.moduletype = mtrinfequation.group(2);
					}
				}
			}
			if (mtrsection.group(1).matches("nmake.common")) {
				mtrinfequation = ptninfequation.matcher(mtrsection.group(2));
				while (mtrinfequation.find()) {
					if (mtrinfequation.group(1).matches("IMAGE_ENTRY_POINT")) {
						mi.entrypoint = mtrinfequation.group(2);
					}
					if (mtrinfequation.group(1).matches("DPX_SOURCE")) {
						if (!mi.localmodulesources.contains(mtrinfequation.group(2))) {
							ui.println("DPX File Missing! : " + mtrinfequation.group(2));
						}
					}
				}
			}
			if (mtrsection.group(1).contains("sources.")) {
				mtrfilename = ptnfilename.matcher(mtrsection.group(2));
				while (mtrfilename.find()) {
					if (!mi.localmodulesources.contains(mtrfilename.group())) {
						ui.println("Source File Missing! : " + mtrfilename.group());
					}
				}
			}
		}
	}
}
