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

import java.io.BufferedReader;
import java.io.StringReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class Critic {
	public static final Pattern PTN_NEW_HEAD_COMMENT = Pattern.compile(
			"^\\/\\*\\*.*?\\*\\*\\/", Pattern.DOTALL);

	private static final Pattern ptnheadcomment = Pattern.compile(
			"^\\/\\*\\+\\+(.*?)\\-\\-\\*\\/", Pattern.DOTALL);

	private static final Pattern ptnfunccomment = Pattern
			.compile(
					"([\\};\\/\">]\\s*)([\\w\\s\\*]*?[_\\w][_\\w\\d]*\\s*\\([^\\)\\(]*\\)\\s*)\\/\\*\\+\\+(.*?)\\-\\-\\*\\/\\s*(.*?)(?=[\\{;])",
					Pattern.DOTALL); // find function with {;">/ , may be
										// unsafe

	// private static Pattern ptncommentstructure =
	// Pattern.compile("\\/\\*\\+\\+\\s*Routine
	// Description:\\s*(.*?)\\s*Arguments:\\s*(.*?)\\s*Returns:\\s*(.*?)\\s*\\-\\-\\*\\/",Pattern.DOTALL);
	private static final Pattern ptncommentequation = Pattern
			.compile("([^\\s]*)\\s+-\\s+(.*)\\s*");

	private static Matcher mtrcommentequation;

	private static final Pattern ptnnewcomment = Pattern
			.compile("(\\s*@(param|retval)\\s+[^\\s]+)\\s+(.*)");

	private static Matcher mtrnewcomment;

	private static final int totallinelength = 82;

	public static final void run(String filepath) throws Exception {
		if (MigrationTool.doCritic) { // this is left here to set an example
										// for future structure
			critic(filepath);
		}
	}

	private static final void critic(String filepath) throws Exception {
		if (filepath.contains(".c") || filepath.contains(".h")) {
			BufferedReader rd = null;
			String line = null;
			StringBuffer templine = new StringBuffer();
			boolean incomment = false;

			System.out.println("Criticing   " + filepath);
			String wholeline = Common.file2string(filepath);

			wholeline = wholeline.replaceAll("\t", "  ");
			wholeline = Common.replaceAll(wholeline, ptnheadcomment,
					"/** @file$1**/");
			wholeline = Common.replaceAll(wholeline, ptnfunccomment,
					"$1\n/**$3\n**/\n$4$2");
			// wholeline = Common.replaceAll(wholeline, ptncommentstructure,
			// "/**\n#%\n$1\n%#\n#%%\n$2\n%%#\n#%%%\n$3\n%%%#\n**/");

			// first scan
			boolean description = false;
			boolean arguments = false;
			boolean returns = false;
			boolean inequation = false;
			rd = new BufferedReader(new StringReader(wholeline));
			while ((line = rd.readLine()) != null) {
				if (line.matches("\\/\\*\\*")) {
					incomment = true;
					description = false;
					arguments = false;
					returns = false;
					templine.append(line + "\n");
				} else if (line.matches("\\*\\*\\/")) {
					incomment = false;
					templine.append("\n" + line + "\n");
				} else if (incomment) {
					if (line.contains("Routine Description:")) {
						description = true;
						arguments = false;
						returns = false;
					} else if (line.contains("Arguments:")) {
						description = false;
						arguments = true;
						returns = false;
						templine.append("\n");
					} else if (line.contains("Returns:")) {
						description = false;
						arguments = false;
						returns = true;
						templine.append("\n");
					} else if (description) {
						if (line.trim().length() != 0) {
							templine.append("  " + line.trim() + "\n");
						}
					} else if (arguments) {
						mtrcommentequation = ptncommentequation.matcher(line);
						if (mtrcommentequation.find()) {
							inequation = true;
							templine.append("  @param  "
									+ mtrcommentequation.group(1) + "     "
									+ mtrcommentequation.group(2) + "\n");
						} else if (inequation && line.trim().length() == 0) {
							inequation = false;
						} else if (inequation && line.trim().length() != 0) {
							templine.append("#%#%" + line + "\n");
						} else {
							if (line.trim().length() != 0) {
								templine.append("  " + line.trim() + "\n");
							}
						}
					} else if (returns) {
						mtrcommentequation = ptncommentequation.matcher(line);
						if (mtrcommentequation.find()) {
							inequation = true;
							templine.append("  @retval "
									+ mtrcommentequation.group(1) + "     "
									+ mtrcommentequation.group(2) + "\n");
						} else if (inequation && line.trim().length() == 0) {
							inequation = false;
						} else if (inequation && line.trim().length() != 0) {
							templine.append("#%#%" + line + "\n");
						} else {
							if (line.trim().length() != 0) {
								templine.append("  @return " + line.trim()
										+ "\n");
							}
						}
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
						startmax = mtrnewcomment.group(1).length() > startmax ? mtrnewcomment
								.group(1).length()
								: startmax;
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
						tempcont = temp.split(" "); // use \\s+ ?

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
			// Remove trailing blanks.
			// 
			wholeline = wholeline.replaceAll(" +\n", "\n");
			Common.string2file(wholeline, filepath);
		}
	}

	public static final void fireAt(String path) throws Exception {
		// Common.toDoAll(Common.dirCopy_(path),
		// Critic.class.getMethod("critic", String.class), null, null,
		// Common.FILE);
		Common.toDoAll(path, Critic.class.getMethod("run", String.class), null,
				null, Common.FILE);
		// Common.toDoAll(Common.dirCopy_(path), critic, Common.FILE);
		System.out.println("Critic Done");
	}
}