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

public class Func {
	Func(String r8func, String r8lib, String r9func, String r9lib) {
		r8funcname = r8func;
		r8libname = r8lib;
		r9funcname = r9func;
		r9libname = r9lib;
	}

	Func(String[] linecontext) {
		r8funcname = linecontext[1];
		r8libname = linecontext[0];
		r9funcname = linecontext[2];
		if (r9funcname.contains("n/a")) {
			r9funcname = "#error Unknown or missing library function in EDKII: "
					+ r8funcname;
		}
		r9libname = linecontext[3];
	}

	public String r8funcname;

	public String r8libname;

	public String r9funcname;

	public String r9libname;

	public static Pattern ptnbrace = Pattern.compile("\\{[^\\{\\}]*\\}",
			Pattern.MULTILINE);

	public static Pattern ptnfuncc = Pattern.compile(
			"(?<!->)([a-zA-Z_]\\w*)\\s*\\(", Pattern.MULTILINE);

	public static Pattern ptnfuncd = Pattern.compile(
			"([a-zA-Z_]\\w*)\\s*\\([^\\)\\(]*\\)\\s*@", Pattern.MULTILINE);

	public static Pattern ptnlowcase = Pattern.compile("[a-z]"); // must be
																	// removed

	private static String reservedwords = "if for pack while switch return sizeof";

	public static String register(Matcher mtr, ModuleInfo mi, Database db) {
		String temp = null;

		temp = mtr.group(1); // both changed and not changed funcc are
								// registered , for finding all the non-local
								// function calls
		Matcher mtrlowcase = ptnlowcase.matcher(temp); // must be removed , so
														// the two funcs can be
														// merged
		if (!reservedwords.contains(temp) && mtrlowcase.find()) {
			mi.hashfuncc.add(temp);
		}
		return temp;
	}
}
