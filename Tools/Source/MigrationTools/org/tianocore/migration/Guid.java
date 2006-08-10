package org.tianocore.migration;

import java.util.regex.*;

public class Guid {
	Guid (String r8, String t, String n, String r9, String gv, String p) {
		r8name = r8;
		type = t;
		name = n;
		r9name = r9;
		guidvalue = gv;
		pack = p;
	}
	Guid (String[] linecontext, String t) {
		r8name = linecontext[1];
		type = t;
		name = linecontext[0];
		r9name = linecontext[2];
		guidvalue = linecontext[3];
		pack = linecontext[4];
	}
	public String r8name;
	public String type;
	public String name;
	public String r9name;
	public String guidvalue;
	public String pack;

	public static Pattern ptnguid = Pattern.compile("g\\w*Guid");

	public static String register(Matcher mtr, ModuleInfo mi, Database db) {
		String type = null;
		String temp = null;
		
		temp = mtr.group();
		if (db.hasGuid(temp)) {		// only changed guids registered, because both changed and not changed guids are included in database
			type = db.getGuidType(temp);
			if (type.matches("Protocol")) {
				mi.protocol.add(temp);
			} else if (type.matches("Ppi")) {
				mi.ppi.add(temp);
			} else if (type.matches("Guid")) {
				mi.guid.add(temp);
			}
			return temp;
		}
		return null;
	}
}