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
import java.lang.reflect.*;

public final class Common {
	public static final int BOTH = 0;
	public static final int FILE = 1;
	public static final int DIR = 2;
	
	public static final String strseparate = "(.*)\\\\([^\\\\]*)";
	public static final Pattern ptnseparate = Pattern.compile("(.*)\\\\([^\\\\]*)");

	//-------------------------------------regex------------------------------------------//
	
	public static final String replaceAll(String line, Pattern ptn, String des) {
		Matcher mtr = ptn.matcher(line);
		if (mtr.find()) {
			 return mtr.replaceAll(des);
		}
		return line;
	}

	//-------------------------------------regex------------------------------------------//
	
	//-----------------------------------file&string---------------------------------------//
	
	public static final String file2string(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(filename));
		StringBuffer wholefile = new StringBuffer();
		String line;
		while ((line = rd.readLine()) != null) {
			wholefile.append(line + "\n");
		}
		return wholefile.toString();
	}

	public static final void string2file(String content, String filename) throws Exception {
		ensureDir(filename);
		PrintWriter outfile = new PrintWriter(new BufferedWriter(new FileWriter(filename)));
		outfile.append(content);
		outfile.flush();
		outfile.close();
	}

	//-----------------------------------file&string---------------------------------------//

	//--------------------------------------dir--------------------------------------------//
	
	public static final void ensureDir(String objFileWhole) {
		File tempdir;
		Matcher mtrseparate = ptnseparate.matcher(objFileWhole);
		if (mtrseparate.find()) {
			tempdir = new File(mtrseparate.group(1));
			if (!tempdir.exists()) tempdir.mkdirs();
		}
	}
	
	public static final void deleteDir(String objFileWhole) {
		String[] list = new File(objFileWhole).list();
		File temp;
		for (int i = 0 ; i < list.length ; i++) {
			temp = new File(objFileWhole + File.separator + list[i]);
			if (temp.isDirectory()) {
				deleteDir(objFileWhole + File.separator + list[i]);
			} else {
				temp.delete();
			}
		}
		new File(objFileWhole).delete();
	}
	
	public static final String dirCopy_(String src) throws Exception {
		Matcher mtrseparate = Common.ptnseparate.matcher(src);
		if (mtrseparate.find()) {
			dirCopy(src, mtrseparate.group(1) + File.separator + "_" + mtrseparate.group(2));
		}
		return mtrseparate.group(1) + File.separator + "_" + mtrseparate.group(2);
	}
	
	public static final void dirCopy(String src, String des) throws Exception {
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

	//--------------------------------------dir--------------------------------------------//

	//-------------------------------like python walk-----------------------------------------//
	
	public static final void toDoAll(String path, Method md, Object obj, Object[] args, int type) throws Exception {
		String[] list = new File(path).list();
		ArrayList<Object> _args = new ArrayList<Object>();
		
		_args.add(path);
		if (args != null) {
			for (int i = 0; i < args.length; i++) {
				_args.add(args[i]);
			}
		}

		if (type == DIR || type == BOTH) {
			md.invoke(obj, _args.toArray());
		}
		for (int i = 0 ; i < list.length ; i++) {
			if (new File(path + File.separator + list[i]).isDirectory()) {
				toDoAll(path + File.separator + list[i], md, obj, args, type);
			} else {
				if (type == FILE || type == BOTH) {
					_args.set(0, path + File.separator + list[i]);
					md.invoke(obj, _args.toArray());
				}
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
