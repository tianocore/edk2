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
public final class ModuleInfo {
	ModuleInfo(String modulepath) throws Exception {
		this.modulepath = modulepath;
		
		if (ModuleInfo.defaultoutput) {
			this.outputpath = this.modulepath.replaceAll(Common.strseparate, "$1");
		} else {
			ModuleInfo.ui.println("Choose where to place the result");
			if ((outputpath = ModuleInfo.ui.getFilepath("Please choose where to place the output module")) == null) {
				outputpath = modulepath; 
			}
			ModuleInfo.ui.println("Output to: " + outputpath);
		}
	}

	public final String modulepath;
	
	public String outputpath = null;
	
	public String modulename = null;
	public String guidvalue = null;
	public String moduletype = null;
	public String entrypoint = null;
	
	public final Set<String> localmodulesources = new HashSet<String>();		//contains both .c and .h
	public final Set<String> preprocessedccodes = new HashSet<String>();
	public final Set<String> msaorinf = new HashSet<String>();				//only a little, hash may be too big for this
	
	public final Set<String> hashfuncc = new HashSet<String>();
	public final Set<String> hashfuncd = new HashSet<String>();
	public final Set<String> hashnonlocalfunc = new HashSet<String>();
	public final Set<String> hashnonlocalmacro = new HashSet<String>();
	public final Set<String> hashEFIcall = new HashSet<String>();
	public final Set<String> hashr8only = new HashSet<String>();
	
	public final Set<String> hashrequiredr9libs = new HashSet<String>();	// hashrequiredr9libs is now all added in SourceFileReplacer 
	public final Set<String> guid = new HashSet<String>();
	public final Set<String> protocol = new HashSet<String>();
	public final Set<String> ppi = new HashSet<String>();

	public final void enroll(String filepath) throws Exception {
		if (filepath.contains(".c") || filepath.contains(".C") || filepath.contains(".h") || 
				filepath.contains(".H") || filepath.contains(".dxs") || filepath.contains(".uni")) {
			localmodulesources.add(filepath.replace(modulepath + "\\", ""));
		} else if (filepath.contains(".inf") || filepath.contains(".msa")) {
			msaorinf.add(filepath.replace(modulepath + "\\", ""));
		}
	}

	public static final boolean isModule(String path) {
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

	//---------------------------------------------------------------------------//
	
	private static final void manipulate(ModuleInfo mi) throws Exception {
		
		ModuleReader.ModuleScan(mi);
		//ModuleInfo.ui.yesOrNo("go on replace?");
		SourceFileReplacer.flush(mi);	// some adding library actions are taken here,so it must be put before "MsaWriter"

		//ModuleInfo.ui.yesOrNo("go on show?");
		// show result
		if (ModuleInfo.printModuleInfo) {
			ModuleInfo.ui.println("\nModule Information : ");
			ModuleInfo.ui.println("Entrypoint : " + mi.entrypoint);
			show(mi.protocol, "Protocol : ");
			show(mi.ppi, "Ppi : ");
			show(mi.guid, "Guid : ");
			show(mi.hashfuncc, "call : ");
			show(mi.hashfuncd, "def : ");
			show(mi.hashEFIcall, "EFIcall : ");
			show(mi.hashnonlocalmacro, "macro : ");
			show(mi.hashnonlocalfunc, "nonlocal : ");
			show(mi.hashr8only, "hashr8only : ");
		}

		//ModuleInfo.ui.yesOrNo("go on msawrite?");
		new MsaWriter(mi).flush();
		//ModuleInfo.ui.yesOrNo("go on critic?");

		if (ModuleInfo.doCritic) {
    		Critic.fireAt(mi.outputpath + File.separator + "Migration_" + mi.modulename);
		}

		//ModuleInfo.ui.yesOrNo("go on delete?");
		Common.deleteDir(mi.modulepath + File.separator + "temp");
		
		ModuleInfo.ui.println("Errors Left : " + ModuleInfo.db.error);
		ModuleInfo.ui.println("Complete!");
		//ModuleInfo.ui.println("Your R9 module was placed here: " + mi.modulepath + File.separator + "result");
		//ModuleInfo.ui.println("Your logfile was placed here: " + mi.modulepath);
	}
	
	private static final void show(Set<String> hash, String show) {
		ModuleInfo.ui.println(show + hash.size());
		ModuleInfo.ui.println(hash);
	}
	
	public static final void seekModule(String filepath) throws Exception {
		if (ModuleInfo.isModule(filepath)) {
			manipulate(new ModuleInfo(filepath));
		}
	}

	public static final void triger(String path) throws Exception {
		ModuleInfo.ui.println("Project Migration");
		ModuleInfo.ui.println("Copyright (c) 2006, Intel Corporation");
		Common.toDoAll(path, ModuleInfo.class.getMethod("seekModule", String.class), null, null, Common.DIR);
	}
	
	public static UI ui = null;
	public static Database db = null;
	
	public static final String migrationcomment = "//%$//";
	
	public static boolean printModuleInfo = false;
	public static boolean doCritic = false;
	public static boolean defaultoutput = false;
	
	public static void main(String[] args) throws Exception {
		ui = FirstPanel.init();
		db = Database.init();
	}
}