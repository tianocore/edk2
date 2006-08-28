package org.tianocore.migration;

import java.io.File;
import java.util.Set;

public class MigrationTool {
	private static final void manipulate(ModuleInfo mi) throws Exception {

		ModuleReader.ModuleScan(mi);
		//MigrationTool.ui.yesOrNo("go on replace?");
		SourceFileReplacer.flush(mi);	// some adding library actions are taken here,so it must be put before "MsaWriter"

		//MigrationTool.ui.yesOrNo("go on show?");
		// show result
		if (MigrationTool.printModuleInfo) {
			MigrationTool.ui.println("\nModule Information : ");
			MigrationTool.ui.println("Entrypoint : " + mi.entrypoint);
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

		//MigrationTool.ui.yesOrNo("go on msawrite?");
		new MsaWriter(mi).flush();
		//MigrationTool.ui.yesOrNo("go on critic?");

		if (MigrationTool.doCritic) {
			Critic.fireAt(mi.outputpath + File.separator + "Migration_" + mi.modulename);
		}

		//MigrationTool.ui.yesOrNo("go on delete?");
		Common.deleteDir(mi.modulepath + File.separator + "temp");

		MigrationTool.ui.println("Errors Left : " + MigrationTool.db.error);
		MigrationTool.ui.println("Complete!");
		//MigrationTool.ui.println("Your R9 module was placed here: " + mi.modulepath + File.separator + "result");
		//MigrationTool.ui.println("Your logfile was placed here: " + mi.modulepath);
	}

	private static final void show(Set<String> hash, String show) {
		MigrationTool.ui.println(show + hash.size());
		MigrationTool.ui.println(hash);
	}

	public static final void seekModule(String filepath) throws Exception {
		if (ModuleInfo.isModule(filepath)) {
			manipulate(new ModuleInfo(filepath));
		}
	}

	public static final void triger(String path) throws Exception {
		MigrationTool.ui.println("Project Migration");
		MigrationTool.ui.println("Copyright (c) 2006, Intel Corporation");
		Common.toDoAll(path, MigrationTool.class.getMethod("seekModule", String.class), null, null, Common.DIR);
	}

	public static UI ui = null;
	public static Database db = null;

	public static final String MIGRATIONCOMMENT = "//%$//";

	public static boolean printModuleInfo = false;
	public static boolean doCritic = false;
	public static boolean defaultoutput = false;

	public static void main(String[] args) throws Exception {
		ui = FirstPanel.init();
		db = Database.init();
	}
}
