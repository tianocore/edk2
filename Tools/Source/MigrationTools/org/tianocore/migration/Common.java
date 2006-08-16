package org.tianocore.migration;

import java.io.*;
import java.util.regex.*;
import java.util.*;

public class Common {
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
		Pattern ptnseparate = Pattern.compile("(.*)\\\\[^\\\\]*");
		Matcher mtrseparate;
		File tempdir;

		mtrseparate = ptnseparate.matcher(objFileWhole);
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
