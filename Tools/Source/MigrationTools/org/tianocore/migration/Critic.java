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

import java.util.regex.*;

public class Critic implements Common.ForDoAll {
	Critic() {
		filepath = null;
	}
	Critic(String path) {
		filepath = path;
	}
	
	private String filepath = null;
	
	private static Pattern ptnheadcomment = Pattern.compile("^\\/\\*\\+\\+(.*?)\\-\\-\\*\\/",Pattern.DOTALL);
	private static Matcher mtrheadcomment;
	private static Pattern ptnfunccomment = Pattern.compile("([\\w\\d]*\\s*[_\\w][_\\w\\d]*\\s*\\([^\\)\\(]*\\)\\s*)(\\/\\*\\+\\+.*?)(\\-\\-\\*\\/\\s*)(.*?)([\\{;])",Pattern.DOTALL);
	private static Matcher mtrfunccomment;
	private static Pattern ptncommentstructure = Pattern.compile("Routine Description:\\s*(\\w.*?\\w)\\s*Arguments:(\\s*\\w.*?\\w\\s*)Returns:(\\s*\\w.*?\\w\\s*)&%",Pattern.DOTALL);
	private static Matcher mtrcommentstructure;
	private static Pattern ptntempcomment = Pattern.compile("\\/\\*\\+\\+(.*?)\\-\\-\\*\\/\\s*[\\w\\d]*\\s*[_\\w][_\\w\\d]*\\s*\\([^\\)\\(]*\\)",Pattern.DOTALL);
	private static Matcher mtrtempcomment;
	private static Pattern ptninfequation = Pattern.compile("([^\\s]*)\\s*-\\s*(\\w.*\\w)");
	private static Matcher mtrinfequation;
	
	public void toDo(String filepath) throws Exception {
		String funccomment = null;
		if (filepath.contains(".c") || filepath.contains(".h")) {
			System.out.println("Criticing   " + filepath);
			String wholeline = Common.file2string(filepath);
			
			// find head comment
			mtrheadcomment = ptnheadcomment.matcher(wholeline);
			if (mtrheadcomment.find()) {			//as we find only the head comment here, use 'if' not 'while'
				wholeline = mtrheadcomment.replaceFirst("/** @file$1**/");
			}
			
			// find func comment
			mtrfunccomment = ptnfunccomment.matcher(wholeline);
			while (mtrfunccomment.find()) {
				funccomment = mtrfunccomment.group(2) + "&%";
				mtrcommentstructure = ptncommentstructure.matcher(funccomment);
				wholeline = mtrfunccomment.replaceAll("$2$4$3$1$5");
			}
			
			// edit func comment
			mtrtempcomment = ptntempcomment.matcher(wholeline);
			while (mtrtempcomment.find()) {
				System.out.println("-----------------------------");
				System.out.println(mtrtempcomment.group());
				System.out.println("-----------------------------");
			}
			Common.string2file(wholeline, filepath);
		}
	}
	
	public static void fireAt(String path) throws Exception {
		Critic critic = new Critic();
		Common.toDoAll(Common.dirCopy_(path), critic);
		System.out.println("Critic Done");
	}
}
//analyze func comment
/*if (mtrcommentstructure.find()) {
	newcomment.append("/*++\n\n" + mtrcommentstructure.group(1) + "\n\n");
	
	//System.out.println("-------1-------");
	//System.out.println(mtrcommentstructure.group(1));
	
	// arg
	//System.out.println("-------2-------");
	//System.out.println(mtrcommentstructure.group(2));
	mtrinfequation = ptninfequation.matcher(mtrcommentstructure.group(2));
	while (mtrinfequation.find()) {
		newcomment.append("@param   " + mtrinfequation.group(1) + "            " + mtrinfequation.group(2) + "\n");
		//System.out.println("@param   " + mtrinfequation.group(1) + "   " + mtrinfequation.group(2));
	}
	newcomment.append("\n");
	// return
	//System.out.println("-------3-------");
	//System.out.println(mtrcommentstructure.group(3));
	mtrinfequation = ptninfequation.matcher(mtrcommentstructure.group(3));
	while (mtrinfequation.find()) {
		newcomment.append("@retval   " + mtrinfequation.group(1) + "            " + mtrinfequation.group(2) + "\n");
		//System.out.println("@retval   " + mtrinfequation.group(1) + "   " + mtrinfequation.group(2));
	}
	System.out.println(newcomment);
} else {
	System.out.println("Error: Comment Style Incorrect");
}*/