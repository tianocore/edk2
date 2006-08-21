package org.tianocore.migration;

public class MigrationTool {
	public static void main(String[] args) throws Exception {
		ModuleInfo.ui = FirstPanel.init();
		ModuleInfo.db = new Database();
	}
}
