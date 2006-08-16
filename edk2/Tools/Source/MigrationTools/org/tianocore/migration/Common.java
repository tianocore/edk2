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

import java.io.*;
import java.util.regex.*;
import java.util.*;

public class Common {
	public static Pattern ptnseparate = Pattern.compile("(.*)\\\\([^\\\\]*)");
	
	public static String file2string(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(filename));
		StringBuffer wholefile = new StringBuffer();
		String line;
		while ((line = rd.readLine()) != null) {
			wholefile.append(line + "\n");
		}
		return wholefile.toString();
	}

	public static void ensureDir(String objFileWhole) {
		File tempdir;
		Matcher mtrseparate = ptnseparate.matcher(objFileWhole);
		if (mtrseparate.find()) {
			tempdir = new File(mtrseparate.group(1));
			if (!tempdir.exists()) tempdir.mkdirs();
		}
	}
	
	public static void string2file(String content, String filename) throws Exception {
		ensureDir(filename);
		PrintWriter outfile = new PrintWriter(new BufferedWriter(new FileWriter(filename)));
		outfile.append(content);
		outfile.flush();
		outfile.close();
	}
	
	public static HashSet<String> dirScan(String path) {			// use HashSet, persue speed rather than space
		HashSet<String> filelist = new HashSet<String>();
		String[] list = new File(path).list();
		File test;

		for (int i = 0 ; i < list.length ; i++) {
			test = new File(path + File.separator + list[i]);
			if (test.isDirectory()) {
				dirScan(path + File.separator + list[i]);
			} else {
				filelist.add(path + File.separator + list[i]);
			}
		}
		
		return filelist;
	}

	public static String dirCopy_(String src) throws Exception {
		Matcher mtrseparate = Common.ptnseparate.matcher(src);
		if (mtrseparate.find()) {
			dirCopy(src, mtrseparate.group(1) + File.separator + "_" + mtrseparate.group(2));
		}
		return mtrseparate.group(1) + File.separator + "_" + mtrseparate.group(2);
	}
	
	public static void dirCopy(String src, String des) throws Exception {
		String[] list = new File(src).list();
		File test;

		for (int i = 0 ; i < list.length ; i++) {
			test = new File(src + File.separator + list[i]);
			if (test.isDirectory()) {
				dirCopy(src + File.separator + list[i], des + File.separator + list[i]);
			} else {
				ensureDir(des + File.separator + list[i]);
				string2file(file2string(src + File.separator + list[i]), des + File.separator + list[i]);
			}
		}
	}
	
	public static void toDoAll(String path, ForDoAll fda) throws Exception { // filter of file type can be done in toDo
		String[] list = new File(path).list();
		File test;

		for (int i = 0 ; i < list.length ; i++) {
			test = new File(path + File.separator + list[i]);
			if (test.isDirectory()) {
				toDoAll(path + File.separator + list[i], fda);
			} else {
				fda.toDo(path + File.separator + list[i]);
			}
		}
	}
	
	public static interface ForDoAll {
		public void toDo(String filepath) throws Exception;
	}
}
