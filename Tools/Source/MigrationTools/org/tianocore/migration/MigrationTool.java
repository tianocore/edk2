package org.tianocore.migration;

public class MigrationTool {
	public static UI ui = null;
	public static Database db = null;
	
	public static void main(String[] args) throws Exception {
		ui = FirstPanel.init();
		db = Database.init();
	}
}
