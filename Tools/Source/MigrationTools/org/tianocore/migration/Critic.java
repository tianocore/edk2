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
import java.io.*;

public class Critic {
	private static Pattern ptnheadcomment = Pattern.compile("^\\/\\*\\+\\+(.*?)\\-\\-\\*\\/",Pattern.DOTALL);
	private static Pattern ptnfunccomment = Pattern.compile("([\\};\\/]\\s*)([\\w\\s]*?[_\\w][_\\w\\d]*\\s*\\([^\\)\\(]*\\)\\s*)\\/\\*\\+\\+(.*?)\\-\\-\\*\\/(\\s*.*?)([\\{;])",Pattern.DOTALL);
	//private static Pattern ptncommentstructure = Pattern.compile("\\/\\*\\+\\+\\s*Routine Description:\\s*(.*?)\\s*Arguments:\\s*(.*?)\\s*Returns:\\s*(.*?)\\s*\\-\\-\\*\\/",Pattern.DOTALL);
	private static Pattern ptncommentequation = Pattern.compile("([^\\s]*)\\s+-\\s+(.*)\\s*");
	private static Matcher mtrcommentequation;
	private static Pattern ptnnewcomment = Pattern.compile("(\\s*@(param|retval)\\s+[^\\s]+)\\s+(.*)");
	private static Matcher mtrnewcomment;
	
	private static final int totallinelength = 82;
	
	public static final void critic(String filepath) throws Exception {
		if (filepath.contains(".c") || filepath.contains(".h")) {
			BufferedReader rd = null;
			String line = null;
			StringBuffer templine = new StringBuffer();
			boolean incomment = false;

			System.out.println("Criticing   " + filepath);
			String wholeline = Common.file2string(filepath);

			wholeline = wholeline.replaceAll("\t", "  ");
			wholeline = Common.replaceAll(wholeline, ptnheadcomment, "/** @file$1**/");
			wholeline = Common.replaceAll(wholeline, ptnfunccomment, "$1/**$3**/$4$2$5");
			//wholeline = Common.replaceAll(wholeline, ptncommentstructure, "/**\n#%\n$1\n%#\n#%%\n$2\n%%#\n#%%%\n$3\n%%%#\n**/");

			// first scan
			boolean description = false;
			boolean arguments = false;
			boolean returns = false;
			boolean inequation = false;
			rd = new BufferedReader(new StringReader(wholeline));
			while ((line = rd.readLine()) != null) {
				if (line.matches("\\/\\*\\*")) {
					incomment = true;
					templine.append(line + "\n");
				} else if (line.matches("\\*\\*\\/")) {
					incomment = false;
					templine.append(line + "\n");
				} else if (incomment && line.contains("Routine Description:")) {
					description = true;
					arguments = false;
					returns = false;
				} else if (incomment && line.contains("Arguments:")) {
					description = false;
					arguments = true;
					returns = false;
				} else if (incomment && line.contains("Returns:")) {
					description = false;
					arguments = false;
					returns = true;
				} else if (incomment && description) {
					templine.append("  " + line.trim() + "\n");
				} else if (incomment && arguments) {
					mtrcommentequation = ptncommentequation.matcher(line);
					if (mtrcommentequation.find()) {
						inequation = true;
						templine.append("  @param  " + mtrcommentequation.group(1) + "     " + mtrcommentequation.group(2) + "\n");
					} else if (inequation && line.trim().length() == 0) {
						inequation = false;
						templine.append(line + "\n");
					} else if (inequation && line.trim().length() != 0) {
						templine.append("#%#%" + line + "\n");
					} else {
						templine.append("  " + line.trim() + "\n");
					}
				} else if (incomment && returns) {
					mtrcommentequation = ptncommentequation.matcher(line);
					if (mtrcommentequation.find()) {
						inequation = true;
						templine.append("  @retval " + mtrcommentequation.group(1) + "     " + mtrcommentequation.group(2) + "\n");
					} else if (inequation && line.trim().length() == 0) {
						inequation = false;
						templine.append(line + "\n");
					} else if (inequation && line.trim().length() != 0) {
						templine.append("#%#%" + line + "\n");
					} else {
						templine.append("  " + line.trim() + "\n");
					}
				} else {
					templine.append(line + "\n");
				}
			}
			wholeline = templine.toString();
			wholeline = wholeline.replaceAll("\n#%#%\\s*", " ");
			//
			
			// secend scan
			int startmax = 0;
			rd = new BufferedReader(new StringReader(wholeline));
			while ((line = rd.readLine()) != null) {
				if (line.matches("\\/\\*\\*")) {
					incomment = true;
					templine.append(line + "\n");
				} else if (line.matches("\\*\\*\\/")) {
					incomment = false;
					templine.append(line + "\n");
				} else if (incomment) {
					mtrnewcomment = ptnnewcomment.matcher(line);
					if (mtrnewcomment.find()) {
						startmax = mtrnewcomment.group(1).length() > startmax ? mtrnewcomment.group(1).length() : startmax;
					}
				}
			}
			startmax++;
			//
			
			// third scan
			int n = 0;
			String temp = null;
			String[] tempcont = null;
			int count = 0;
			templine = new StringBuffer();
			rd = new BufferedReader(new StringReader(wholeline));
			while ((line = rd.readLine()) != null) {
				if (line.matches("\\/\\*\\*")) {
					incomment = true;
					templine.append(line + "\n");
				} else if (line.matches("\\*\\*\\/")) {
					incomment = false;
					templine.append(line + "\n");
				} else if (incomment) {
					mtrnewcomment = ptnnewcomment.matcher(line);
					if (mtrnewcomment.find()) {
						n = startmax - mtrnewcomment.group(1).length();
						templine.append(mtrnewcomment.group(1));
						while (n-- >= 0) {
							templine.append(" ");
						}
						temp = mtrnewcomment.group(3);
						tempcont = temp.split(" ");							// use \\s+ ?
						
						count = 0;
						for (int i = 0; i < tempcont.length; i++) {
							count += tempcont[i].length();
							if (count <= (totallinelength - startmax)) {
								templine.append(tempcont[i] + " ");
								count += 1;
							} else {
								templine.append("\n");
								n = startmax;
								while (n-- >= 0) {
									templine.append(" ");
								}
								templine.append(tempcont[i] + " ");
								count = tempcont[i].length() + 1;
							}
						}
						templine.append("\n");
					} else {
						templine.append(line + "\n");
					}
				} else {
					templine.append(line + "\n");
				}
			}
			wholeline = templine.toString();
			//
			
			/* -----slow edition of replacefirst with stringbuffer-----
			line.append(wholeline);
			mtrfunccomment = ptnfunccomment.matcher(line);
			while (mtrfunccomment.find()) {
				line.replace(0, line.length()-1, mtrfunccomment.replaceFirst("$2$4$3$1$5"));
			}
			*/
			/* -----slow edition of replacefirst with string-----
			while ((mtrfunccomment = ptnfunccomment.matcher(wholeline)).find()) {
				//funccomment = mtrfunccomment.group(2);
				//mtrcommentstructure = ptncommentstructure.matcher(funccomment);
				wholeline = mtrfunccomment.replaceFirst("$2$4$3$1$5");
			}
			*/
			/*
			// edit func comment
			mtrtempcomment = ptntempcomment.matcher(wholeline);
			while (mtrtempcomment.find()) {
				System.out.println("-----------------------------");
				System.out.println(mtrtempcomment.group());
				System.out.println("-----------------------------");
			}
			*/
			Common.string2file(wholeline, filepath);
		}
	}
	
	public static final void fireAt(String path) throws Exception {
		Common.toDoAll(Common.dirCopy_(path), Critic.class.getMethod("critic", String.class), null, null, Common.FILE);
		//Common.toDoAll(Common.dirCopy_(path), critic, Common.FILE);
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