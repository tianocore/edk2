package org.tianocore.migration;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class Common {
	public static String sourcefiletostring(String filename) throws Exception {
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
}
