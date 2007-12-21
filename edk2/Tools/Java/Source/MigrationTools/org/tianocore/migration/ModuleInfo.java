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

import java.io.File;
import java.util.HashSet;
import java.util.Set;

import org.tianocore.UsageTypes;
import org.tianocore.SupportedArchitectures.Enum;

/*
 * Class ModuleInfo is built for scanning the source files, it contains all the
 * needed information and all the temporary data.
 */
public final class ModuleInfo {
	ModuleInfo(String modulepath) throws Exception {
		this.modulepath = modulepath;
		this.temppath = MigrationTool.getTempDir(this.modulepath);
	}

	public final String modulepath;

	public final String temppath;

	private MsaOwner msaowner = MsaOwner.initNewMsaOwner();

	public boolean isLibrary = false;

	public String modulename = null;

	public String guidvalue = null;

	public String moduletype = null;

	public String entrypoint = null;

	public String license = null;

	public final Set<String> localmodulesources = new HashSet<String>(); // contains
																			// both
																			// .c
																			// and
																			// .h

	public final Set<String> preprocessedccodes = new HashSet<String>();

	public final Set<String> msaorinf = new HashSet<String>(); // only a
																// little, hash
																// may be too
																// big for this

	public final Set<String> infincludes = new HashSet<String>();

	public final Set<String> infsources = new HashSet<String>();

	public final Set<String> hashfuncc = new HashSet<String>();

	public final Set<String> hashfuncd = new HashSet<String>();

	public final Set<String> hashnonlocalfunc = new HashSet<String>();

	public final Set<String> hashnonlocalmacro = new HashSet<String>();

	public final Set<String> hashEFIcall = new HashSet<String>();

	public final Set<String> hashr8only = new HashSet<String>();

	public final Set<String> hashmacro = new HashSet<String>();

	public final Set<String> hashrequiredr9libs = new HashSet<String>(); // hashrequiredr9libs
																			// is
																			// now
																			// all
																			// added
																			// in
																			// SourceFileReplacer

	public final Set<String> guids = new HashSet<String>();

	public final Set<String> protocols = new HashSet<String>();

	public final Set<String> ppis = new HashSet<String>();

	// -----------------------------------------------------------------------------------//

	// addModuleType
	// addGuidValue
	// addModuleName

	public final boolean addSourceFile(String filename, Enum en) {
		localmodulesources.add(filename);
		return msaowner.addSourceFile(filename, en);
	}

	public final boolean addProtocol(String proname, UsageTypes.Enum usage) {
		protocols.add(proname);
		return msaowner.addProtocol(proname, usage);
	}

	public final boolean addPpi(String ppiname, UsageTypes.Enum usage) {
		ppis.add(ppiname);
		return msaowner.addPpi(ppiname, usage);
	}

	public final boolean addGuid(String guidname, UsageTypes.Enum usage) {
		guids.add(guidname);
		return msaowner.addGuid(guidname, usage);
	}

	public final boolean addLibraryClass(String name, UsageTypes.Enum usage) {
		//
		// This section is only for adding library classes, this functionality
		// should be inside MsaOwner!!!
		//
		// if (!hashrequiredr9libs.contains(name)) {
		msaowner.addLibraryClass(name, usage);
		// }
		//
		hashrequiredr9libs.add(name);
		return true;
	}

	// -----------------------------------------------------------------------------------//

	public final String getModuleType() {
		if (moduletype.contains("PEI")) {
			return "PEIM";
		} else {
			return "DXE_DRIVER";
		}
	}

	public final void enroll(String filepath) throws Exception {
		String temp = null;
		if (filepath.contains(".inf") || filepath.contains(".msa")) {
			temp = filepath.replace(modulepath + File.separator, "");
			if (!temp.contains(File.separator)) { // .inf in subdirectory is
													// not regarded
				msaorinf.add(temp);
			}
		} else if (filepath.contains(".c") || filepath.contains(".C")
				|| filepath.contains(".h") || filepath.contains(".H")
				|| filepath.contains(".dxs") || filepath.contains(".uni")
				|| filepath.contains(".s") || filepath.contains(".S")
				|| filepath.contains(".i") || filepath.contains(".asm")) {
			addSourceFile(filepath.replace(modulepath + File.separator, ""),
					null);
		}
	}

	public static final boolean isModule(String path) {
		String[] list = new File(path).list();
		for (int i = 0; i < list.length; i++) {
			if (!new File(list[i]).isDirectory()) {
				if (list[i].contains(".inf") || list[i].contains(".msa")) {
					return true;
				}
			}
		}
		return false;
	}

	public final MsaOwner getMsaOwner() {
		return msaowner;
	}
}