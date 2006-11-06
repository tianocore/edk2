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
import java.util.HashMap;
import java.util.Iterator;
import java.util.Set;

import javax.swing.JFileChooser;

/**
 * The class is used as the main class of the MigrationTool, maintains the main
 * work flow, and all the global variables and constants. It extends nothing.
 * 
 */
public class MigrationTool {

	//
	// These two objects are serves globally, it is always required, and only
	// one instance is ever allowed.
	//
	public static UI ui = null;

	public static Database db = null;

	//
	// The global constant for MigrationTool generated comments.
	//
	public static String MIGRATIONCOMMENT = "//@MT:";

	//
	// Global switches that are changed by user by the FirstPanel.
	//
	public static boolean printModuleInfo = false;

	public static boolean doCritic = false;

	public static boolean defaultoutput = false;

	//
	// A hashmap that associates the reference to a ModuleInfo with its
	// outputpath.
	//
	public static final HashMap<ModuleInfo, String> ModuleInfoMap = new HashMap<ModuleInfo, String>();

	//
	// The starting point of the MigrationTool.
	//
	private static String startpath = null;

	/**
	 * This method defines the overall main work flow of the MigrationTool.
	 * 
	 * @param mi
	 * @throws Exception
	 */
	private static final void mainFlow(ModuleInfo mi) throws Exception {
		ModuleReader.aimAt(mi);
		SourceFileReplacer.fireAt(mi); // some adding library actions are taken
										// here,so it must be put before
										// "MsaWriter"

		// show result
		if (MigrationTool.printModuleInfo) {
			MigrationTool.ui.println("\nModule Information : ");
			MigrationTool.ui.println("Entrypoint : " + mi.entrypoint);
			show(mi.protocols, "Protocol : ");
			show(mi.ppis, "Ppi : ");
			show(mi.guids, "Guid : ");
			show(mi.hashfuncc, "call : ");
			show(mi.hashfuncd, "def : ");
			show(mi.hashEFIcall, "EFIcall : ");
			show(mi.hashnonlocalmacro, "macro : ");
			show(mi.hashnonlocalfunc, "nonlocal : ");
			show(mi.hashr8only, "hashr8only : ");
		}
		new MsaWriter(mi).flush();

		// mi.getMsaOwner().flush(MigrationTool.ModuleInfoMap.get(mi) +
		// File.separator + "Migration_" + mi.modulename + File.separator +
		// mi.modulename + ".___");

		if (MigrationTool.doCritic) {
			Critic.fireAt(ModuleInfoMap.get(mi) + File.separator + "Migration_"
					+ mi.modulename);
		}

		MigrationTool.ui.println("Errors Left : " + MigrationTool.db.error);
		MigrationTool.ui.println("Complete!");
	}

	/**
	 * This method is specially written to print the message for ModuleInfo,
	 * just for less code repeating.
	 * 
	 * @param hash
	 * @param show
	 */
	private static final void show(Set<String> hash, String show) {
		MigrationTool.ui.println(show + hash.size());
		MigrationTool.ui.println(hash);
	}

	/**
	 * This method designates the location of temp directory.
	 * 
	 * @param modulepath
	 * @return String
	 */
	public static final String getTempDir(String modulepath) {
		return "C:" + File.separator + "MigrationTool_Temp"
				+ modulepath.replace(startpath, "");
	}

	/**
	 * This method is the default output path generating scheme.
	 * 
	 * @param inputpath
	 * @return String
	 */
	private static final String assignOutPutPath(String inputpath) {
		if (MigrationTool.defaultoutput) {
			return inputpath.replaceAll(Common.STRSEPARATER, "$1");
		} else {
			return MigrationTool.ui.getFilepath(
					"Please choose where to place the output module",
					JFileChooser.DIRECTORIES_ONLY);
		}
	}

	/**
	 * This function is called by main loop of the MigrationTool which
	 * verifies whether a dir contains a module, thus generating a map
	 * which shows the corresponding path for each module.
	 * 
	 * @param filepath
	 * @throws Exception
	 */
	public static final void seekModule(String filepath) throws Exception {
		if (ModuleInfo.isModule(filepath)) {
			ModuleInfoMap.put(new ModuleInfo(filepath),
					assignOutPutPath(filepath));
		}
	}

	/**
	 * This is the main loop of the tool.
	 * 
	 * @param path
	 * @throws Exception
	 */
	public static final void startMigrateAll(String path) throws Exception {
		startpath = path;
		MigrationTool.ui.println("Project Migration");
		MigrationTool.ui.println("Copyright (c) 2006, Intel Corporation");

		if (new File("C:" + File.separator + "MigrationTool_Temp").exists()) {
			Common.deleteDir("C:" + File.separator + "MigrationTool_Temp");
		}

		Common.toDoAll(path, MigrationTool.class.getMethod("seekModule",
				String.class), null, null, Common.DIR);

		Iterator<ModuleInfo> miit = ModuleInfoMap.keySet().iterator();
		while (miit.hasNext()) {
			mainFlow(miit.next());
		}

		ModuleInfoMap.clear();

		Common.deleteDir("C:" + File.separator + "MigrationTool_Temp");
	}

	/**
	 * This main method initializes the environment. 
	 * 
	 * @param args
	 * @throws Exception
	 */
	public static void main(String[] args) throws Exception {
		ui = FirstPanel.getInstance();
		db = Database.getInstance();
	}
}