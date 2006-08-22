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
	private static ModuleInfo mi;
	
	private static final Pattern ptninfequation = Pattern.compile("([^\\s]*)\\s*=\\s*([^\\s]*)");
	private static final Pattern ptnsection = Pattern.compile("\\[([^\\[\\]]*)\\]([^\\[\\]]*)\\n", Pattern.MULTILINE);
	private static final Pattern ptnfilename = Pattern.compile("[^\\s]+");
	
	public static final void ModuleScan(ModuleInfo m) throws Exception {
		mi = m;
		
		Common.toDoAll(mi.modulepath, ModuleInfo.class.getMethod("enroll", String.class), mi, null, Common.FILE);
		
		String filename = null;
		if (mi.msaorinf.isEmpty()) {
			ModuleInfo.ui.println("No INF nor MSA file found!");
			System.exit(0);
		} else {
			filename = ModuleInfo.ui.choose("Found .inf or .msa file for module\n" + mi.modulepath + "\nChoose one Please", mi.msaorinf.toArray());
		}
		if (filename.contains(".inf")) {
			readInf(filename);
		} else if (filename.contains(".msa")) {
			readMsa(filename);
		}
		
		CommentOutNonLocalHFile();
		parsePreProcessedSourceCode();

	}
	
	private static final void readMsa(String name) throws Exception {
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
	
	private static final void readInf(String name) throws Exception {
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
							ModuleInfo.ui.println("DPX File Missing! : " + mtrinfequation.group(2));
						}
					}
				}
			}
			if (mtrsection.group(1).contains("sources.")) {
				mtrfilename = ptnfilename.matcher(mtrsection.group(2));
				while (mtrfilename.find()) {
					if (!mi.localmodulesources.contains(mtrfilename.group())) {
						ModuleInfo.ui.println("Source File Missing! : " + mtrfilename.group());
					}
				}
			}
		}
	}
	
	// add '//' to all non-local include lines
	private static final void CommentOutNonLocalHFile() throws IOException {
		BufferedReader rd;
		String line;
		String curFile;
		PrintWriter outfile;

		Pattern ptninclude = Pattern.compile("[\"<](.*[.]h)[\">]");
		Matcher mtrinclude;

		Iterator<String> ii = mi.localmodulesources.iterator();
		while ( ii.hasNext() ) {
			curFile = ii.next();
			rd = new BufferedReader(new FileReader(mi.modulepath + File.separator + curFile));
			Common.ensureDir(mi.modulepath + File.separator + "temp" + File.separator + curFile);
			outfile = new PrintWriter(new BufferedWriter(new FileWriter(mi.modulepath + File.separator + "temp" + File.separator + curFile)));
			while ((line = rd.readLine()) != null) {
				if (line.contains("#include")) {
					mtrinclude = ptninclude.matcher(line);
					if (mtrinclude.find() && mi.localmodulesources.contains(mtrinclude.group(1))) {
					} else {
						line = ModuleInfo.migrationcomment + line;
					}
				}
				outfile.append(line + '\n');
			}
			outfile.flush();
			outfile.close();
		}
	}

	private static final void parsePreProcessedSourceCode() throws Exception {
		//Cl cl = new Cl(modulepath);
		//cl.execute("Fat.c");
		//cl.generateAll(preprocessedccodes);
		//
		//System.out.println("Note!!!! The CL is not implemented now , pls do it manually!!! RUN :");
		//System.out.println("cl " + modulepath + "\\temp\\*.c" + " -P");
		//String[] list = new File(modulepath + File.separator + "temp").list();	// without CL , add
		BufferedReader rd = null;
		String ifile = null;
		String line = null;
		String temp = null;
		
		Iterator<String> ii = mi.localmodulesources.iterator();
		while (ii.hasNext()) {
			temp = ii.next();
			if (temp.contains(".c")) {
				mi.preprocessedccodes.add(temp);
			}
		}
		
		ii = mi.preprocessedccodes.iterator();
		
		Pattern patefifuncc = Pattern.compile("g?(BS|RT)\\s*->\\s*([a-zA-Z_]\\w*)",Pattern.MULTILINE);
		Pattern patentrypoint = Pattern.compile("EFI_([A-Z]*)_ENTRY_POINT\\s*\\(([^\\(\\)]*)\\)",Pattern.MULTILINE);
		Matcher matguid;
		Matcher matfuncc;
		Matcher matfuncd;
		Matcher matenclosereplace;
		Matcher matefifuncc;
		Matcher matentrypoint;
		Matcher matmacro;
		
		while (ii.hasNext()) {
			StringBuffer wholefile = new StringBuffer();
			ifile = ii.next();
			rd = new BufferedReader(new FileReader(mi.modulepath + File.separator + "temp" + File.separator + ifile));
			while ((line = rd.readLine()) != null) {
				wholefile.append(line + '\n');
			}
			line = wholefile.toString();
			
			// if this is a Pei phase module , add these library class to .msa
			matentrypoint = patentrypoint.matcher(line);
			if (matentrypoint.find()) {
				mi.entrypoint = matentrypoint.group(2);
				if (matentrypoint.group(1).matches("PEIM")) {
					mi.hashrequiredr9libs.add("PeimEntryPoint");
				} else {
					mi.hashrequiredr9libs.add("UefiDriverEntryPoint");
				}
			}
			
			// find guid
			matguid = Guid.ptnguid.matcher(line);										// several ways to implement this , which one is faster ? :
			while (matguid.find()) {													// 1.currently , find once , then call to identify which is it
				if ((temp = Guid.register(matguid, mi, MigrationTool.db)) != null) {				// 2.use 3 different matchers , search 3 times to find each
					//matguid.appendReplacement(result, ModuleInfo.db.getR9Guidname(temp));		// search the database for all 3 kinds of guids , high cost
				}
			}
			//matguid.appendTail(result);
			//line = result.toString();

			// find EFI call in form of '->' , many 'gUnicodeCollationInterface->' like things are not changed
			// This item is not simply replaced , special operation is required.
			matefifuncc = patefifuncc.matcher(line);
			while (matefifuncc.find()) {
				mi.hashEFIcall.add(matefifuncc.group(2));
			}

			// find function call
			matfuncc = Func.ptnfuncc.matcher(line);
			while (matfuncc.find()) {
				if ((temp = Func.register(matfuncc, mi, MigrationTool.db)) != null) {
					//ModuleInfo.ui.println(ifile + "  dofunc  " + temp);
					//matfuncc.appendReplacement(result, ModuleInfo.db.getR9Func(temp));
				}
			}
			//matfuncc.appendTail(result);
			//line = result.toString();

			// find macro
			matmacro = Macro.ptntmacro.matcher(line);
			while (matmacro.find()) {
				if ((temp = Macro.register(matmacro, mi, MigrationTool.db)) != null) {
				}
			}
			
			// find function definition
			// replace all {} to @
			while ((matenclosereplace = Func.ptnbrace.matcher(line)).find()) {
				line = matenclosereplace.replaceAll("@");
			}

			matfuncd = Func.ptnfuncd.matcher(line);
			while (matfuncd.find()) {
				if ((temp = Func.register(matfuncd, mi, MigrationTool.db)) != null) {
				}
			}
		}
		
		// op on hash
		Iterator<String> funcci = mi.hashfuncc.iterator();
		while (funcci.hasNext()) {
			if (!mi.hashfuncd.contains(temp = funcci.next()) && !mi.hashEFIcall.contains(temp)) {
				mi.hashnonlocalfunc.add(temp);					// this set contains both changed and not changed items
			}
		}
	}
}
