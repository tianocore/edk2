package org.tianocore.migration;

import java.io.*;
import java.util.*;
import java.util.regex.*;
import org.tianocore.*;

public class ModuleReader {
	ModuleReader(String path, ModuleInfo moduleinfo, Database database) {
		modulepath = path;
		mi = moduleinfo;
		db = database;
	}
	private String modulepath;
	private ModuleInfo mi;
	private Database db;
	
	private static Pattern ptninfequation = Pattern.compile("([^ ]*) *= *([^ ]*)");
	
	public void readMsa(String name) throws Exception {
		ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.parse(new File(modulepath + File.separator + name));
		ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = msadoc.getModuleSurfaceArea();
		MsaHeaderDocument.MsaHeader msaheader = msa.getMsaHeader();
		
		mi.modulename = msaheader.getModuleName();
		mi.guidvalue = msaheader.getGuidValue();
		mi.moduletype = msaheader.getModuleType().toString();		// ???
		
		SourceFilesDocument.SourceFiles sourcefiles = msa.getSourceFiles();
		
		String temp;
		Iterator<FilenameDocument.Filename> li = sourcefiles.getFilenameList().iterator();
		while (li.hasNext()) {
			if (!mi.localmodulesources.contains(temp = li.next().toString())) {
				System.out.println("Source File Missing ! : " + temp);
			}
		}
	}
	
	public void readInf(String name) throws Exception {
		System.out.println("Reading From Inf : " + name);
		BufferedReader rd = new BufferedReader(new FileReader(modulepath + File.separator + name));
		String line;
		String[] linecontext;
		boolean inSrc = false;
		Matcher mtrinfequation;

		while ((line = rd.readLine()) != null) {
			if (line.length() != 0) {
				if (inSrc) {
					if (line.contains("[")) {
						inSrc = false;
					} else {
						linecontext = line.split(" ");
						if (linecontext[2].length() != 0) {
							if (!mi.localmodulesources.contains(linecontext[2])) {
								System.out.println("Source File Missing ! : " + linecontext[2]);
							}
						}
					}
				} else {
					if ((mtrinfequation = ptninfequation.matcher(line)).find()) {
						if (mtrinfequation.group(1).matches("BASE_NAME")) {
							mi.modulename = mtrinfequation.group(2);
						}
						if (mtrinfequation.group(1).matches("FILE_GUID")) {
							mi.guidvalue = mtrinfequation.group(2);
						}
						if (mtrinfequation.group(1).matches("COMPONENT_TYPE")) {
							mi.moduletype = mtrinfequation.group(2);
						}
						if (mtrinfequation.group(1).matches("IMAGE_ENTRY_POINT")) {
							mi.entrypoint = mtrinfequation.group(2);
						}
					}
					if (line.contains("sources")) {
						inSrc = true;
					}
				}
			}
		}
	}
}