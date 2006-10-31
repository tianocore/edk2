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
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class Common {
	public static final int BOTH = 0;

	public static final int FILE = 1;

	public static final int DIR = 2;

	public static final String STRSEPARATER = "(.*)\\\\([^\\\\]*)";

	public static final Pattern PTNSEPARATER = Pattern
			.compile("(.*)\\\\([^\\\\]*)");

	// -------------------------------------regex------------------------------------------//

	public static final String replaceAll(String line, Pattern ptn, String des) {
		Matcher mtr = ptn.matcher(line);

		if (mtr.find()) {
			return mtr.replaceAll(des);
		}

		return line;
	}

	public static final boolean find(String line, String regex) {
		Pattern ptn = Pattern.compile(regex);

		return ptn.matcher(line).find();
	}

	// -------------------------------------regex------------------------------------------//

	// -----------------------------------file&string---------------------------------------//

	public static final String file2string(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(filename));
		StringBuffer wholefile = new StringBuffer();
		String line;
		while ((line = rd.readLine()) != null) {
			wholefile.append(line + "\n");
		}
		rd.close();
		return wholefile.toString();
	}

	public static final void string2file(String content, String filename)
			throws Exception {
		ensureDir(filename);
		PrintWriter outfile = new PrintWriter(new BufferedWriter(
				new FileWriter(filename)));
		outfile.append(content);
		outfile.flush();
		outfile.close();
	}

	public static final void fileCopy(String src, String des) throws Exception {
		string2file(file2string(src), des);
	}

	// -----------------------------------file&string---------------------------------------//

	// --------------------------------------dir--------------------------------------------//
	/*
	 * public static final HashSet<String> walkDir(String path, int mode)
	 * throws Exception { HashSet<String> pathlist = new HashSet<String>();
	 * Common.toDoAll(path, Common.class.getMethod("walkDir", String.class),
	 * null, null, mode); return pathlist; }
	 */
	public static final void ensureDir(String objFileWhole) {
		File tempdir;
		Matcher mtrseparate = PTNSEPARATER.matcher(objFileWhole);
		if (mtrseparate.find()) {
			tempdir = new File(mtrseparate.group(1));
			if (!tempdir.exists())
				tempdir.mkdirs();
		}
	}

	public static final void deleteDir(String objFileWhole) {
		String[] list = new File(objFileWhole).list();
		File temp;
		for (int i = 0; i < list.length; i++) {
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
		Matcher mtrseparate = Common.PTNSEPARATER.matcher(src);
		if (mtrseparate.find()) {
			dirCopy(src, mtrseparate.group(1) + File.separator + "_"
					+ mtrseparate.group(2));
		}
		return mtrseparate.group(1) + File.separator + "_"
				+ mtrseparate.group(2);
	}

	public static final void dirCopy(String src, String des) throws Exception {
		String[] list = new File(src).list();
		File test;

		ensureDir(des);
		for (int i = 0; i < list.length; i++) {
			test = new File(src + File.separator + list[i]);
			if (test.isDirectory()) {
				dirCopy(src + File.separator + list[i], des + File.separator
						+ list[i]);
			} else {
				// ensureDir(des + File.separator + list[i]);
				string2file(file2string(src + File.separator + list[i]), des
						+ File.separator + list[i]);
			}
		}
	}

	public static final void oneLevelDirCopy(String src, String des, String type)
			throws Exception {
		String[] list = new File(src).list();

		ensureDir(des);
		for (int i = 0; i < list.length; i++) {
			if (list[i].contains(type)) {
				string2file(file2string(src + File.separator + list[i]), des
						+ File.separator + list[i]);
			}
		}
	}

	// --------------------------------------dir--------------------------------------------//

	// -------------------------------like python
	// walk-----------------------------------------//

	public static final void toDoAll(String path, Method md, Object obj,
			Object[] args, int type) throws Exception {
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
		for (int i = 0; i < list.length; i++) {
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

	public static final void toDoAll(Set<String> set, ForDoAll fda)
			throws Exception {
		Iterator<String> di = set.iterator();
		while (di.hasNext()) {
			fda.run(di.next());
		}
	}

	public static final void toDoAll(String path, ForDoAll fda, int type)
			throws Exception { // filter of file type can be done in toDo
		String[] list = new File(path).list();
		File test;

		if (type == DIR || type == BOTH) {
			fda.run(path);
		}
		for (int i = 0; i < list.length; i++) {
			test = new File(path + File.separator + list[i]);
			if (test.isDirectory()) {
				if (fda.filter(test)) {
					toDoAll(path + File.separator + list[i], fda, type);
				}
			} else {
				if (type == FILE || type == BOTH) {
					fda.run(path + File.separator + list[i]);
				}
			}
		}
	}

	public static interface ForDoAll {
		public void run(String filepath) throws Exception;

		public boolean filter(File dir);
	}

	public static abstract class Laplace {
		public void transform(String src, String des) throws Exception {
			Common.string2file(operation(Common.file2string(src)), des);
		}

		public abstract String operation(String wholeline);

		public abstract boolean recognize(String filename);

		public abstract String namechange(String oldname);
	}

	public static interface Element {

		// public int replace = 0;
		// public int type = 1;

		public String getReplace(String key);

		// public void getType(String key);
		//        
		// public void setReplace(int num);
		//        
		// public void setType(int num);

	}
}
