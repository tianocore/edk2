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

public class ModuleReader {
	ModuleReader(String path, ModuleInfo moduleinfo, Database database, UI u) {
		modulepath = path;
		mi = moduleinfo;
		db = database;
		ui = u;
	}
	private String modulepath;
	private ModuleInfo mi;
	private Database db;
	private UI ui;
	
	private static Pattern ptninfequation = Pattern.compile("([^\\s]*)\\s*=\\s*([^\\s]*)");
	private static Pattern ptnsection = Pattern.compile("\\[([^\\[\\]]*)\\]([^\\[\\]]*)\\n", Pattern.MULTILINE);
	private static Pattern ptnfilename = Pattern.compile("[^\\s]+");
	
	public void readMsa(String name) throws Exception {
		ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.parse(new File(modulepath + File.separator + name));
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
	
	public void readInf(String name) throws Exception {
		System.out.println("\nParsing INF file: " + name);
		String wholeline;
		Matcher mtrinfequation;
		Matcher mtrsection;
		Matcher mtrfilename;

		wholeline = Common.file2string(modulepath + File.separator + name);
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
