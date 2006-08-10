package org.tianocore.migration;

import java.util.regex.*;

public class Macro {
	Macro(String r8, String r9) {
		r8name = r8;
		r9name = r9;
	}
	Macro(String[] linecontext) {
		r8name = linecontext[0];
		r9name = linecontext[1];
	}
	
	public String r8name;
	public String r9name;

	public static Pattern ptntmacro = Pattern.compile("\\b[A-Z_]+\\s*?\\(?\\b",Pattern.MULTILINE);

	private static String unmacro = "VOID UINTN BOOLEAN ASSERT OPTIONAL STATIC NULL TRUE IN OUT FALSE";

	public static String register(Matcher mtr, ModuleInfo mi, Database db) {
		String temp = null;
		
		temp = mtr.group();
		if (db.hasMacro(temp)) {					// only changed macros registered, because the database of macro has only changed ones
			if (!unmacro.contains(temp)) {
				mi.hashnonlocalmacro.add(temp);
			}
			return temp;
		}
		return null;
	}
}