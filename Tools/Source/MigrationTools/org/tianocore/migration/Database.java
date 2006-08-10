package org.tianocore.migration;

import java.io.*;
import java.util.*;

public class Database {
	Database() throws Exception {
		if (System.getenv("WORKSPACE") == null) {
			DatabasePath = "C:" + File.separator + "tianocore" + File.separator + "edk2" + File.separator + "Tools" + File.separator + "Conf" + File.separator + "Migration";
		} else {
			DatabasePath = System.getenv("WORKSPACE") + File.separator + "Tools" + File.separator + "Conf" + File.separator + "Migration";
		}
		
		importDBLib("Library.csv");
		importDBGuid("Guid.csv", "Guid");
		importDBGuid("Ppi.csv", "Ppi");
		importDBGuid("Protocol.csv", "Protocol");
		importDBMacro("Macro.csv");
	}
	
	public static String defaultpath = "C:" + File.separator + "tianocore" + File.separator + "edk2" + File.separator + "Tools" + File.separator + "Conf" + File.separator + "Migration";
	
	public String DatabasePath;
	public Set<String> error = new HashSet<String>();
	
	private Map<String,Guid> hashguid = new HashMap<String,Guid>();
	private Map<String,Func> hashfunc = new HashMap<String,Func>();
	private Map<String,Macro> hashmacro = new HashMap<String,Macro>();
	
	private void importDBLib(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath + File.separator + filename));
		String line;
		String[] linecontext;
		Func lf;
		
		if (rd.ready()) {
			System.out.println("Found " + filename + " , Importing Library Database");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
		        	lf = new Func(linecontext);
		        	hashfunc.put(lf.r8funcname,lf);
				}
			}
		}
	}
	
	private void importDBGuid(String filename, String type) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath + File.separator + filename));
		String line;
		String[] linecontext;
		Guid gu;
		
		if (rd.ready()) {
			System.out.println("Found " + filename + " , Importing " + type + " Database");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
		        	gu = new Guid(linecontext, type);
		            hashguid.put(gu.r8name,gu);
				}
			}
		}
	}
	
	private void importDBMacro(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath + File.separator + filename));
		String line;
		String[] linecontext;
		Macro mc;
		
		if (rd.ready()) {
			System.out.println("Found " + filename + " , Importing Macro Database");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
		        	mc = new Macro(linecontext);
		        	hashmacro.put(mc.r8name,mc);
				}
			}
		}
	}
	
	public String getR9Lib(String r8funcname) {
		String temp = null;
		if (hashfunc.containsKey(r8funcname)) {
			temp = hashfunc.get(r8funcname).r9libname;
		}
		return temp;
	}
	
	public String getR9Func(String r8funcname) {
		String temp = null;
		if (hashfunc.containsKey(r8funcname)) {
			temp = hashfunc.get(r8funcname).r9funcname;
		}
		return temp;
	}
	
	public boolean hasFunc(String r8lib) {
		return hashfunc.containsKey(r8lib);
	}

	public boolean hasGuid(String r8guid) {
		return hashguid.containsKey(r8guid);
	}

	public boolean hasMacro(String r8macro) {
		return hashmacro.containsKey(r8macro);
	}
	
	public String getR9Macro(String r8macro) {
		return hashmacro.get(r8macro).r9name;			// the verification job of if the macro exists in the database is done when registering it
	}
	
	public String getR9Guidname(String r8Guid) {
		String temp = null;
		try {
			temp = hashguid.get(r8Guid).r9name;
		} catch (NullPointerException e) {
			error.add("getR9Guidname :" + r8Guid);
		}
		return temp;
	}
	
	public String getGuidType(String r8Guid) {
		String temp = null;
		try {
			temp =  hashguid.get(r8Guid).type;
		} catch (NullPointerException e) {
			error.add("getR9Guidname :" + r8Guid);
		}
		return temp;
	}
}
