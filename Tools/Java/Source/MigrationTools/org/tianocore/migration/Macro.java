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

	public static Pattern ptntmacro = Pattern.compile("\\b\\w(\\w|\\d)*",
			Pattern.MULTILINE);

	private static String unmacro = "VOID UINTN BOOLEAN ASSERT OPTIONAL STATIC NULL TRUE IN OUT FALSE";

	public static String register(Matcher mtr, ModuleInfo mi, Database db) {
		String temp = null;

		temp = mtr.group();
		mi.hashmacro.add(temp);
		if (MigrationTool.db.hasMacro(temp)) { // only changed macros
												// registered, because the
												// database of macro has only
												// changed ones
			if (!unmacro.contains(temp)) {
				mi.hashnonlocalmacro.add(temp);
			}
			return temp;
		}
		return null;
	}
}