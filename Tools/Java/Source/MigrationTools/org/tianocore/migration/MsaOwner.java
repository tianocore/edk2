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

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.apache.xmlbeans.XmlOptions;
import org.tianocore.ExternsDocument;
import org.tianocore.FilenameDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.LicenseDocument;
import org.tianocore.ModuleDefinitionsDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.PPIsDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.SourceFilesDocument;
import org.tianocore.SupportedArchitectures;
import org.tianocore.UsageTypes;
import org.tianocore.SupportedArchitectures.Enum;

public class MsaOwner {
	public static final String COPYRIGHT = "Copyright (c) 2006, Intel Corporation";

	public static final String VERSION = "1.0";

	public static final String ABSTRACT = "Component name for module ";

	public static final String DESCRIPTION = "FIX ME!";

	public static final String LICENSE = "All rights reserved.\n"
			+ "      This software and associated documentation (if any) is furnished\n"
			+ "      under a license and may only be used or copied in accordance\n"
			+ "      with the terms of the license. Except as permitted by such\n"
			+ "      license, no part of this software or documentation may be\n"
			+ "      reproduced, stored in a retrieval system, or transmitted in any\n"
			+ "      form or by any means without the express written consent of\n"
			+ "      Intel Corporation.";

	public static final String SPECIFICATION = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";

	public static final Enum IA32 = SupportedArchitectures.IA_32;

	public static final Enum X64 = SupportedArchitectures.X_64;

	public static final Enum IPF = SupportedArchitectures.IPF;

	public static final Enum EBC = SupportedArchitectures.EBC;

	private ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory
			.newInstance();

	private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;

	private MsaHeaderDocument.MsaHeader msaheader = null;

	private LicenseDocument.License license = null;

	private ModuleDefinitionsDocument.ModuleDefinitions moduledefinitions = null;

	private SourceFilesDocument.SourceFiles sourcefiles = null; // found local
																// .h files are
																// not written

	private GuidsDocument.Guids guids = null;

	private ProtocolsDocument.Protocols protocols = null;

	private PPIsDocument.PPIs ppis = null;

	private PackageDependenciesDocument.PackageDependencies packagedependencies = null;

	private LibraryClassDefinitionsDocument.LibraryClassDefinitions libclassdefs = null;

	private ExternsDocument.Externs externs = null;

	private List<Enum> listarch = new ArrayList<Enum>();

	// private Map<String, Enum> mapfilenames = new HashMap<String, Enum>();
	// //this need to be installed manually when msa is to be written
	// private Map<String, UsageTypes.Enum> mapprotocols = new HashMap<String,
	// UsageTypes.Enum>();

	// -----------------------------msaheader-------------------------------------//

	public final boolean addLibraryClass(String name, UsageTypes.Enum usage) {
		/*
		 * if (!libclassdefs.getLibraryClassList().contains(name)) {
		 * LibraryClassDocument.LibraryClass classname; classname =
		 * libclassdefs.addNewLibraryClass(); classname.setKeyword(name);
		 * classname.setUsage(usage); return true; } else { return false; }
		 */
		if (name == null) {
			return false;
		} else {
			Iterator<LibraryClassDocument.LibraryClass> classit = libclassdefs
					.getLibraryClassList().iterator();
			while (classit.hasNext()) {
				if (classit.next().getKeyword().matches(name)) {
					// MigrationTool.ui.println ("Warning: Duplicate
					// LibraryClass");
					return false;
				}
			}

			LibraryClassDocument.LibraryClass classname;
			classname = libclassdefs.addNewLibraryClass();
			classname.setKeyword(name);
			classname.setUsage(usage);
			return true;

		}
	}

	public final boolean addGuid(String guidname, UsageTypes.Enum usage) {
		if (guids == null) {
			guids = msa.addNewGuids();
		}

		Iterator<GuidsDocument.Guids.GuidCNames> guidit = guids
				.getGuidCNamesList().iterator();
		while (guidit.hasNext()) {
			if (guidit.next().getGuidCName() == guidname) {
				// MigrationTool.ui.println ("Warning: Duplicate Guid");
				return false;
			}
		}

		GuidsDocument.Guids.GuidCNames guid;
		guid = guids.addNewGuidCNames();
		guid.setGuidCName(guidname);
		guid.setUsage(usage);
		return true;
	}

	public final boolean addPpi(String ppiname, UsageTypes.Enum usage) {
		if (ppis == null) {
			ppis = msa.addNewPPIs();
		}

		Iterator<PPIsDocument.PPIs.Ppi> ppiit = ppis.getPpiList().iterator();
		while (ppiit.hasNext()) {
			if (ppiit.next().getPpiCName() == ppiname) {
				// MigrationTool.ui.println ("Warning: Duplicate Ppi");
				return false;
			}
		}

		PPIsDocument.PPIs.Ppi ppi;
		ppi = ppis.addNewPpi();
		ppi.setPpiCName(ppiname);
		ppi.setUsage(usage);
		return true;
	}

	public final boolean addProtocol(String proname, UsageTypes.Enum usage) {
		if (protocols == null) {
			protocols = msa.addNewProtocols();
		}

		Iterator<ProtocolsDocument.Protocols.Protocol> proit = protocols
				.getProtocolList().iterator();
		while (proit.hasNext()) {
			if (proit.next().getProtocolCName() == proname) {
				// MigrationTool.ui.println ("Warning: Duplicate Protocol");
				return false;
			}
		}

		ProtocolsDocument.Protocols.Protocol protocol;
		protocol = protocols.addNewProtocol();
		protocol.setProtocolCName(proname);
		protocol.setUsage(usage);
		return true;
	}

	public final boolean addSourceFile(String name, Enum en) {
		Iterator<FilenameDocument.Filename> fileit = sourcefiles
				.getFilenameList().iterator();
		while (fileit.hasNext()) {
			if (fileit.next().getStringValue() == name) {
				MigrationTool.ui.println("Warning: Duplicate SourceFileName");
				return false;
			}
		}

		FilenameDocument.Filename filename;
		List<Enum> arch = new ArrayList<Enum>();
		filename = sourcefiles.addNewFilename();
		filename.setStringValue(name);
		arch.add(en);
		filename.setSupArchList(arch);
		return true;
	}

	// entry point todo

	public final boolean setupExternSpecification() {
		addExternSpecification("EFI_SPECIFICATION_VERSION 0x00020000");
		addExternSpecification("EDK_RELEASE_VERSION 0x00020000");
		return true;
	}

	public final boolean addExternSpecification(String specification) {
		if (externs.getSpecificationList().contains(specification)) {
			return false;
		} else {
			externs.addSpecification(specification);
			return true;
		}
	}

	public final boolean setupPackageDependencies() {
		Iterator<String> it;
		//
		// For now, simply add all package guids in the database.
		// 
		it = MigrationTool.db.dumpAllPkgGuid();
		while (it.hasNext()) {
			packagedependencies.addNewPackage().setPackageGuid(it.next());
		}
		return true;
	}

	public final boolean addPackage(String guid) {
		if (packagedependencies.getPackageList().contains(guid)) {
			return false;
		} else {
			packagedependencies.addNewPackage().setPackageGuid(guid);
			return true;
		}
	}

	public final boolean setupModuleDefinitions() { // ????????? give this job
													// to moduleinfo
		moduledefinitions.setBinaryModule(false);
		moduledefinitions.setOutputFileBasename(msaheader.getModuleName());
		return true;
	}

	public final boolean addSupportedArchitectures(Enum arch) {
		if (listarch.contains(arch)) {
			return false;
		} else {
			listarch.add(arch);
			return true;
		}
	}

	public final boolean addSpecification(String specification) {
		if (msaheader.getSpecification() == null) {
			if (specification == null) {
				msaheader.setSpecification(SPECIFICATION);
			} else {
				msaheader.setSpecification(specification);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate Specification");
			return false;
		}
	}

	public final boolean addLicense(String licensecontent) {
		if (msaheader.getLicense() == null) {
			license = msaheader.addNewLicense();
			if (licensecontent == null) {
				license.setStringValue(LICENSE);
			} else {
				license.setStringValue(licensecontent);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate License");
			return false;
		}
	}

	public final boolean addDescription(String description) {
		if (msaheader.getDescription() == null) {
			if (description == null) {
				msaheader.setDescription(DESCRIPTION);
			} else {
				msaheader.setDescription(description);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate Description");
			return false;
		}
	}

	public final boolean addAbstract(String abs) {
		if (msaheader.getAbstract() == null) {
			if (abs == null) {
				msaheader.setAbstract(ABSTRACT + msaheader.getModuleName());
			} else {
				msaheader.setVersion(abs);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate Abstract");
			return false;
		}
	}

	public final boolean addVersion(String version) {
		if (msaheader.getVersion() == null) {
			if (version == null) {
				msaheader.setVersion(VERSION);
			} else {
				msaheader.setVersion(version);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate Version");
			return false;
		}
	}

	public final boolean addCopyRight(String copyright) {
		if (msaheader.getCopyright() == null) {
			if (copyright == null) {
				msaheader.setCopyright(COPYRIGHT);
			} else {
				msaheader.setCopyright(copyright);
			}
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate CopyRight");
			return false;
		}
	}

	public final boolean addModuleType(String moduletype) {
		if (msaheader.getModuleType() == null) {
			msaheader.setModuleType(ModuleTypeDef.Enum.forString(moduletype));
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate ModuleType");
			return false;
		}
	}

	public final boolean addGuidValue(String guidvalue) {
		if (msaheader.getGuidValue() == null) {
			msaheader.setGuidValue(guidvalue);
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate GuidValue");
			return false;
		}
	}

	public final boolean addModuleName(String modulename) {
		if (msaheader.getModuleName() == null) {
			msaheader.setModuleName(modulename);
			return true;
		} else {
			MigrationTool.ui.println("Warning: Duplicate ModuleName");
			return false;
		}
	}

	// -----------------------------msaheader-------------------------------------//

	private final void fullfill() throws Exception {
		addCopyRight(null);
		addVersion(null);
		addAbstract(null);
		addDescription(null);
		addLicense(null);
		addSpecification(null);
	}

	public final void flush(String outputpath) throws Exception {
		XmlOptions options = new XmlOptions();

		options.setCharacterEncoding("UTF-8");
		options.setSavePrettyPrint();
		options.setSavePrettyPrintIndent(2);
		options.setUseDefaultNamespace();

		BufferedWriter bw = new BufferedWriter(new FileWriter(outputpath));
		fullfill();
		msadoc.save(bw, options);
		bw.flush();
		bw.close();
	}

	private final MsaOwner init() {
		msa = msadoc.addNewModuleSurfaceArea();
		msaheader = msa.addNewMsaHeader();
		moduledefinitions = msa.addNewModuleDefinitions();
		moduledefinitions.setSupportedArchitectures(listarch);

		sourcefiles = msa.addNewSourceFiles();
		packagedependencies = msa.addNewPackageDependencies();
		libclassdefs = msa.addNewLibraryClassDefinitions();
		externs = msa.addNewExterns();
		return this;
	}

	public static final MsaOwner initNewMsaOwner() {
		return new MsaOwner().init();
	}
}