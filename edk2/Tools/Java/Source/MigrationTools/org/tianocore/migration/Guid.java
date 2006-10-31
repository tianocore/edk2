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

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.tianocore.UsageTypes;

public class Guid {
	Guid(String r8, String t, String n, String r9, String gv, String p) {
		r8name = r8;
		type = t;
		name = n;
		r9name = r9;
		guidvalue = gv;
		pack = p;
	}

	Guid(String[] linecontext, String t) {
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
		if (MigrationTool.db.hasGuid(temp)) { // only changed guids
												// registered, because both
												// changed and not changed guids
												// are included in database
			type = MigrationTool.db.getGuidType(temp);
			if (type.matches("Protocol")) {
				mi.addProtocol(temp, UsageTypes.ALWAYS_CONSUMED);
				// mi.protocols.add(temp);
			} else if (type.matches("Ppi")) {
				mi.addPpi(temp, UsageTypes.ALWAYS_CONSUMED);
				// mi.ppis.add(temp);
			} else if (type.matches("Guid")) {
				mi.addGuid(temp, UsageTypes.ALWAYS_CONSUMED);
				// mi.guids.add(temp);
			}
			return temp;
		}
		return null;
	}
}