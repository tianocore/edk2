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
import java.io.File;
import java.io.FileReader;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.DbPathAndFilename;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.FrameworkDatabaseDocument.FrameworkDatabase;
import org.tianocore.GuidDeclarationsDocument.GuidDeclarations;
import org.tianocore.LibraryClassDeclarationsDocument.LibraryClassDeclarations;
import org.tianocore.LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PpiDeclarationsDocument.PpiDeclarations;
import org.tianocore.ProtocolDeclarationsDocument.ProtocolDeclarations;

public final class Database {
	private static final Database INSTANCE = Database.init();;

	Database(String path) {
		DatabasePath = path;

		try {
			// collectWorkSpaceDatabase();
			importPkgGuid("PkgGuid.csv");
			importDBLib("Library.csv");
			importDBGuid("Guid.csv", "Guid");
			importDBGuid("Ppi.csv", "Ppi");
			importDBGuid("Protocol.csv", "Protocol");
			importDBMacro("Macro.csv");
			importListR8Only();
		} catch (Exception e) {
			System.out.println(e.getMessage());
		}
	}

	public String DatabasePath;

	public Set<String> error = new HashSet<String>();

	public Set<String> r8only = new HashSet<String>();

	private Map<String, Guid> hashguid = new HashMap<String, Guid>();

	private Map<String, Func> hashfunc = new HashMap<String, Func>();

	private Map<String, Macro> hashmacro = new HashMap<String, Macro>();

	private Map<String, String> hashPkgGuid = new HashMap<String, String>();

	// -------------------------------------import------------------------------------------//
	private void importPkgGuid(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath
				+ File.separator + filename));
		String line;
		String[] linecontext;

		if (rd.ready()) {
			System.out.println("Found " + filename
					+ ", Importing Package Guid Database.");
			//
			// Skip the title row.
			// 
			line = rd.readLine();
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
					hashPkgGuid.put(linecontext[0], linecontext[1]);
				}
			}
		}
	}

	public Iterator<String> dumpAllPkgGuid() {
		return hashPkgGuid.values().iterator();
	}

	private void importDBLib(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath
				+ File.separator + filename));
		String line;
		String[] linecontext;
		Func lf;

		if (rd.ready()) {
			System.out.println("Found " + filename
					+ ", Importing Library Database.");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
					lf = new Func(linecontext);
					hashfunc.put(lf.r8funcname, lf);
				}
			}
		}
	}

	private void importDBGuid(String filename, String type) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath
				+ File.separator + filename));
		String line;
		String[] linecontext;
		Guid gu;

		if (rd.ready()) {
			System.out.println("Found " + filename + ", Importing " + type
					+ " Database.");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
					gu = new Guid(linecontext, type);
					hashguid.put(gu.r8name, gu);
				}
			}
		}
	}

	private void importDBMacro(String filename) throws Exception {
		BufferedReader rd = new BufferedReader(new FileReader(DatabasePath
				+ File.separator + filename));
		String line;
		String[] linecontext;
		Macro mc;

		if (rd.ready()) {
			System.out.println("Found " + filename
					+ ", Importing Macro Database.");
			while ((line = rd.readLine()) != null) {
				if (line.length() != 0) {
					linecontext = line.split(",");
					mc = new Macro(linecontext);
					hashmacro.put(mc.r8name, mc);
				}
			}
		}
	}

	private void importListR8Only() throws Exception {
		Pattern ptnr8only = Pattern.compile(
				"////#?(\\w*)?.*?R8_(.*?)\\s*\\(.*?////~", Pattern.DOTALL);
		String wholeline = Common.file2string(DatabasePath + File.separator
				+ "R8Lib.c");
		System.out
				.println("Found " + "R8Lib.c" + ", Importing R8Lib Database.");
		Matcher mtrr8only = ptnr8only.matcher(wholeline);
		while (mtrr8only.find()) {
			r8only.add(mtrr8only.group(2));
		}
	}

	// -------------------------------------import------------------------------------------//

	// -------------------------------------get------------------------------------------//

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

	public String getR9Macro(String r8macro) {
		return hashmacro.get(r8macro).r9name; // the verification job of if
												// the macro exists in the
												// database is done when
												// registering it
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
			temp = hashguid.get(r8Guid).type;
		} catch (NullPointerException e) {
			error.add("getR9Guidname :" + r8Guid);
		}
		return temp;
	}

	// -------------------------------------get------------------------------------------//

	// -------------------------------------has------------------------------------------//

	public boolean hasFunc(String r8lib) {
		return hashfunc.containsKey(r8lib);
	}

	public boolean hasGuid(String r8guid) {
		return hashguid.containsKey(r8guid);
	}

	public boolean hasMacro(String r8macro) {
		return hashmacro.containsKey(r8macro);
	}

	// -------------------------------------has------------------------------------------//

	// -------------------------------------init------------------------------------------//

	private static final Database init() {
		if (System.getenv("WORKSPACE") == null) {
			return new Database("C:" + File.separator + "tianocore"
					+ File.separator + "edk2" + File.separator + "Tools"
					+ File.separator + "Conf" + File.separator + "Migration");
		} else {
			return new Database(System.getenv("WORKSPACE") + File.separator
					+ "Tools" + File.separator + "Conf" + File.separator
					+ "Migration");
		}
	}

	public static final Database getInstance() {
		return INSTANCE;
	}

	private String workspacePath;

	private HashMap<String, String> hashDbGuids = new HashMap<String, String>();

	private HashMap<String, String> hashDbPpis = new HashMap<String, String>();

	private HashMap<String, String> hashDbProtocols = new HashMap<String, String>();

	private HashMap<String, String> hashDbLibSymbols = new HashMap<String, String>();

	private HashMap<String, String> hashDbLibFunctions = new HashMap<String, String>();

	private HashMap<String, String> hashDbLibExterns = new HashMap<String, String>();

	private final String regLibClassName = ".*\\W(\\w[\\w\\d]*)\\.h";

	private final Pattern ptnLibClassName = Pattern.compile(regLibClassName);

	private final String regLibSymbol = "#define\\s+(\\w[\\w\\d]*)";

	private final Pattern ptnLibSymbol = Pattern.compile(regLibSymbol);

	private final String regLibDataType = "[A-Z][A-Z0-9_]*\\s*\\**";

	private final String regLibFunction = regLibDataType
			+ "\\s*(?:EFIAPI)?\\s+" + "(\\w[\\w\\d]*)\\s*\\([^)]*\\)\\s*;";

	private Pattern ptnLibFunction = Pattern.compile(regLibFunction);

	private final String regLibExtern = "extern\\s+" + regLibDataType
			+ "\\s*(\\w[\\w\\d]*)";

	private final Pattern ptnLibExtern = Pattern.compile(regLibExtern);

	private final String convertToOsFilePath(String filePath) {
		return filePath.replace("/", File.separator).replace("\\",
				File.separator);
	}

	private final void collectLibHeaderFileInfo(String libHeaderFile,
			String pkgGuid) throws Exception {
		String fileContents;
		String libClassName;
		String libContainer;
		Matcher mtrLibClass;
		Matcher mtrLibSymbol;
		Matcher mtrLibFunction;
		Matcher mtrLibExtern;

		System.out.println("Parsing: " + libHeaderFile);
		mtrLibClass = ptnLibClassName.matcher(libHeaderFile);
		if (!mtrLibClass.matches()) {
			throw new Exception("Illegal libary header file");
		}
		libClassName = mtrLibClass.group(1);
		libContainer = libClassName + "@" + pkgGuid;

		fileContents = Common.file2string(libHeaderFile);
		mtrLibSymbol = ptnLibSymbol.matcher(fileContents);
		while (mtrLibSymbol.find()) {
			String libSymbol;
			String oldLibContainer;

			libSymbol = mtrLibSymbol.group(1);
			oldLibContainer = hashDbLibSymbols.put(libSymbol, libContainer);
			if (oldLibContainer != null) {
				String warnMessage;

				warnMessage = "Duplicated Lib Symbol:" + libSymbol + " Found. "
						+ "Later package will overide the previous one";
				System.out.println(warnMessage);
			}
		}

		mtrLibFunction = ptnLibFunction.matcher(fileContents);
		while (mtrLibFunction.find()) {
			String libFunction;
			String oldLibContainer;

			libFunction = mtrLibFunction.group(1);
			oldLibContainer = hashDbLibFunctions.put(libFunction, libContainer);
			if (oldLibContainer != null) {
				String warnMessage;

				warnMessage = "Duplicated Lib Function:" + libFunction
						+ " Found. "
						+ "Later package will overide the previous one";
				System.out.println(warnMessage);
			}
		}

		mtrLibExtern = ptnLibExtern.matcher(fileContents);
		while (mtrLibExtern.find()) {
			String libExtern;
			String oldLibContainer;

			libExtern = mtrLibExtern.group(1);
			oldLibContainer = hashDbLibExterns.put(libExtern, libContainer);
			if (oldLibContainer != null) {
				String warnMessage;

				warnMessage = "Duplicated Lib Extern:" + libExtern + " Found. "
						+ "Later package will overide the previous one";
				System.out.println(warnMessage);
			}
		}
	}

	private final void collectLibDataBase(PackageSurfaceArea spdDatabase,
			String pkgDirectory) throws Exception {
		String pkgGuid;
		LibraryClassDeclarations libClassDeclarations;

		pkgGuid = spdDatabase.getSpdHeader().getGuidValue();
		libClassDeclarations = spdDatabase.getLibraryClassDeclarations();
		if (libClassDeclarations != null) {
			Iterator<LibraryClass> itLibClass;

			itLibClass = libClassDeclarations.getLibraryClassList().iterator();
			while (itLibClass.hasNext()) {
				String libHeaderFile;

				libHeaderFile = pkgDirectory + File.separator
						+ itLibClass.next().getIncludeHeader();
				libHeaderFile = convertToOsFilePath(libHeaderFile);
				try {
					collectLibHeaderFileInfo(libHeaderFile, pkgGuid);
				} catch (Exception e) {
					String errorMessage;

					errorMessage = "Error (" + e.getMessage()
							+ ")occurs when parsing " + libHeaderFile;
					System.out.println(errorMessage);
				}
			}
		}
	}

	private final void collectGuidDatabase(PackageSurfaceArea spdDatabase)
			throws Exception {
		String pkgGuid;
		GuidDeclarations guidDeclarations;

		pkgGuid = spdDatabase.getSpdHeader().getGuidValue();
		guidDeclarations = spdDatabase.getGuidDeclarations();
		if (guidDeclarations != null) {
			Iterator<GuidDeclarations.Entry> itGuids;

			itGuids = guidDeclarations.getEntryList().iterator();
			while (itGuids.hasNext()) {
				hashDbGuids.put(itGuids.next().getCName(), pkgGuid);
			}
		}

	}

	private final void collectPpiDatabase(PackageSurfaceArea spdDatabase)
			throws Exception {
		String pkgGuid;
		PpiDeclarations ppiDeclarations;

		pkgGuid = spdDatabase.getSpdHeader().getGuidValue();
		ppiDeclarations = spdDatabase.getPpiDeclarations();

		if (ppiDeclarations != null) {
			Iterator<PpiDeclarations.Entry> itPpis;

			itPpis = ppiDeclarations.getEntryList().iterator();
			while (itPpis.hasNext()) {
				hashDbPpis.put(itPpis.next().getCName(), pkgGuid);
			}
		}

	}

	private final void collectProtocolDatabase(PackageSurfaceArea spdDatabase)
			throws Exception {
		String pkgGuid;
		ProtocolDeclarations protocolDeclarations;

		pkgGuid = spdDatabase.getSpdHeader().getGuidValue();
		protocolDeclarations = spdDatabase.getProtocolDeclarations();

		if (protocolDeclarations != null) {
			Iterator<ProtocolDeclarations.Entry> itProtocols;

			itProtocols = protocolDeclarations.getEntryList().iterator();
			while (itProtocols.hasNext()) {
				hashDbGuids.put(itProtocols.next().getCName(), pkgGuid);
			}
		}

	}

	private final void collectPackageDatabase(String packageFileName)
			throws Exception {
		XmlObject xmlPackage;
		PackageSurfaceArea spdDatabase;
		File pkgFile;

		pkgFile = new File(packageFileName);
		xmlPackage = XmlObject.Factory.parse(pkgFile);
		spdDatabase = ((PackageSurfaceAreaDocument) xmlPackage)
				.getPackageSurfaceArea();

		collectGuidDatabase(spdDatabase);
		collectProtocolDatabase(spdDatabase);
		collectPpiDatabase(spdDatabase);
		collectLibDataBase(spdDatabase, pkgFile.getParent());

	}

	public final void collectWorkSpaceDatabase() throws Exception {
		String databaseFileName;
		File databaseFile;
		XmlObject xmlDatabase;
		FrameworkDatabase frameworkDatabase;
		Iterator<DbPathAndFilename> packageFile;

		workspacePath = System.getenv("WORKSPACE");

		if (workspacePath == null) {
			String errorMessage = "Envivornment variable \"WORKSPACE\" is not set!";
			throw new Exception(errorMessage);
		}
		databaseFileName = workspacePath + File.separator + "Tools"
				+ File.separator + "Conf" + File.separator
				+ "FrameworkDatabase.db";
		System.out.println("Open " + databaseFileName);
		databaseFile = new File(databaseFileName);
		xmlDatabase = XmlObject.Factory.parse(databaseFile);
		frameworkDatabase = ((FrameworkDatabaseDocument) xmlDatabase)
				.getFrameworkDatabase();
		packageFile = frameworkDatabase.getPackageList().getFilenameList()
				.iterator();

		while (packageFile.hasNext()) {
			String packageFileName = packageFile.next().getStringValue();
			packageFileName = workspacePath + File.separator + packageFileName;
			packageFileName = convertToOsFilePath(packageFileName);

			System.out.println("Parsing: " + packageFileName);
			try {
				collectPackageDatabase(packageFileName);
			} catch (Exception e) {
				System.out.println("Error occured when opening "
						+ packageFileName + e.getMessage());
			}
		}
	}
}
