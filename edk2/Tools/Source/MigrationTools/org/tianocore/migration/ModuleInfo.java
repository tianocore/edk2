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

/*
	Class ModuleInfo is built for scanning the source files, it contains all the needed
information and all the temporary data.
*/
public class ModuleInfo {
	ModuleInfo(String modulepath) throws Exception {
		this.modulepath = modulepath;
		
		MigrationTool.ui.println("Choose where to place the result");
		if ((outputpath = MigrationTool.ui.getFilepath()) == null) {
			outputpath = modulepath; 
		}
		MigrationTool.ui.println(outputpath);
		
		mainFlow();
	}

	public String modulepath = null;
	
	public String outputpath = null;
	
	public String modulename = null;
	public String guidvalue = null;
	public String moduletype = null;
	public String entrypoint = null;
	
	public Set<String> localmodulesources = new HashSet<String>();		//contains both .c and .h
	public Set<String> preprocessedccodes = new HashSet<String>();
	public Set<String> msaorinf = new HashSet<String>();				//only a little, hash may be too big for this
	
	public Set<String> hashfuncc = new HashSet<String>();
	public Set<String> hashfuncd = new HashSet<String>();
	public Set<String> hashnonlocalfunc = new HashSet<String>();
	public Set<String> hashnonlocalmacro = new HashSet<String>();
	public Set<String> hashEFIcall = new HashSet<String>();
	public Set<String> hashr8only = new HashSet<String>();
	
	public Set<String> hashrequiredr9libs = new HashSet<String>();	// hashrequiredr9libs is now all added in SourceFileReplacer 
	public Set<String> guid = new HashSet<String>();
	public Set<String> protocol = new HashSet<String>();
	public Set<String> ppi = new HashSet<String>();
	
	private void mainFlow() throws Exception {
		
		ModuleReader.ModuleScan(this);
		
		SourceFileReplacer.flush(this);	// some adding library actions are taken here,so it must be put before "MsaWriter"
		
		// show result
		if (MigrationTool.printModuleInfo) {
			MigrationTool.ui.println("\nModule Information : ");
			MigrationTool.ui.println("Entrypoint : " + entrypoint);
			show(protocol, "Protocol : ");
			show(ppi, "Ppi : ");
			show(guid, "Guid : ");
			show(hashfuncc, "call : ");
			show(hashfuncd, "def : ");
			show(hashEFIcall, "EFIcall : ");
			show(hashnonlocalmacro, "macro : ");
			show(hashnonlocalfunc, "nonlocal : ");
			show(hashr8only, "hashr8only : ");
		}
		
		new MsaWriter(this).flush();

		if (MigrationTool.doCritic) {
    		Critic.fireAt(outputpath + File.separator + "Migration_" + modulename);
		}
		
		Common.deleteDir(modulepath + File.separator + "temp");
		
		MigrationTool.ui.println("Errors Left : " + MigrationTool.db.error);
		MigrationTool.ui.println("Complete!");
		MigrationTool.ui.println("Your R9 module was placed here: " + modulepath + File.separator + "result");
		MigrationTool.ui.println("Your logfile was placed here: " + modulepath);
	}
	
	private void show(Set<String> hash, String show) {
		MigrationTool.ui.println(show + hash.size());
		MigrationTool.ui.println(hash);
	}
	
	public final void enroll(String filepath) throws Exception {
		String[] temp;
		if (filepath.contains(".c") || filepath.contains(".C") || filepath.contains(".h") || 
				filepath.contains(".H") || filepath.contains(".dxs") || filepath.contains(".uni")) {
			temp = filepath.split("\\\\");
			localmodulesources.add(temp[temp.length - 1]);
		} else if (filepath.contains(".inf") || filepath.contains(".msa")) {
			temp = filepath.split("\\\\");
			msaorinf.add(temp[temp.length - 1]);
		}
	}

	public static final void seekModule(String filepath) throws Exception {
		if (isModule(filepath)) {
			new ModuleInfo(filepath);
		}
	}

	private static final boolean isModule(String path) {
		String[] list = new File(path).list();
		for (int i = 0 ; i < list.length ; i++) {
			if (!new File(list[i]).isDirectory()) {
				if (list[i].contains(".inf") || list[i].contains(".msa")) {
					return true;
				}
			}
		}
		return false;
	}

	public static final void triger(String path) throws Exception {
		MigrationTool.ui.println("Project Migration");
		MigrationTool.ui.println("Copyright (c) 2006, Intel Corporation");
		Common.toDoAll(path, ModuleInfo.class.getMethod("seekModule", String.class), null, null, Common.DIR);
	}
}