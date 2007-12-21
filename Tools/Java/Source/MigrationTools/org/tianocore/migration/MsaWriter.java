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
import java.io.FileWriter;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.UUID;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlOptions;
import org.tianocore.ExternsDocument;
import org.tianocore.FilenameDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.LibraryClassDocument;
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

public class MsaWriter {
	MsaWriter(ModuleInfo moduleinfo) {
		mi = moduleinfo;
	}

	private ModuleInfo mi;

	private ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory
			.newInstance();

	private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = msadoc
			.addNewModuleSurfaceArea();

	private MsaHeaderDocument.MsaHeader msaheader = msa.addNewMsaHeader();

	private ModuleDefinitionsDocument.ModuleDefinitions md = msa
			.addNewModuleDefinitions();

	private SourceFilesDocument.SourceFiles sourcefiles = msa
			.addNewSourceFiles(); // found local .h files are not written

	private GuidsDocument.Guids guids;

	private ProtocolsDocument.Protocols protocols;

	private PPIsDocument.PPIs ppis;

	private PackageDependenciesDocument.PackageDependencies pd = msa
			.addNewPackageDependencies();

	private LibraryClassDefinitionsDocument.LibraryClassDefinitions libclassdefs = msa
			.addNewLibraryClassDefinitions();

	private ExternsDocument.Externs externs = msa.addNewExterns();

	private String Query(String requirement) throws Exception {
		String answer;
		BufferedReader rd = new BufferedReader(new InputStreamReader(System.in));
		System.out.println(requirement);
		while ((answer = rd.readLine()).length() == 0)
			;
		return answer;
	}

	private void addSourceFiles(String name) { // furthur modification needed
		List<Enum> arch = new ArrayList<Enum>();
		FilenameDocument.Filename filename;
		filename = sourcefiles.addNewFilename();
		filename.setStringValue(name);

		if (name.contains("x64" + File.separator)) { // filename ???
			arch.add(SupportedArchitectures.X_64);
			System.out.println("x64" + File.separator);
			filename.setSupArchList(arch);
		} else if (name.contains("Ia32" + File.separator)) { // filename ???
			arch.add(SupportedArchitectures.IA_32);
			System.out.println("Ia32" + File.separator);
			filename.setSupArchList(arch);
		} else if (name.contains("Ipf" + File.separator)) { // filename ???
			arch.add(SupportedArchitectures.IPF);
			System.out.println("Ipf" + File.separator);
			filename.setSupArchList(arch);
		} else if (name.contains("Ebc" + File.separator)) { // filename ???
			arch.add(SupportedArchitectures.EBC);
			System.out.println("Ebc" + File.separator);
			filename.setSupArchList(arch);
		}
	}

	private void addWrapper() {
		XmlCursor cursor = msa.newCursor();
		String uri = "http://www.TianoCore.org/2006/Edk2.0";
		cursor.push();
		cursor.toNextToken();
		cursor.insertNamespace("", uri);
		cursor.insertNamespace("xsi",
				"http://www.w3.org/2001/XMLSchema-instance");
		cursor.pop();
		msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) cursor.getObject();
	}

	private ModuleSurfaceAreaDocument fulfillMsadoc() throws Exception {
		Iterator<String> it;
		String temp;

		if (mi.modulename != null) {
			msaheader.setModuleName(mi.modulename);
		} else {
			msaheader
					.setModuleName(mi.modulename = Query("Module Name Not Found!  Please Input ModuleName"));
		}
		if (mi.guidvalue == null) {
			mi.guidvalue = UUID.randomUUID().toString();
			MigrationTool.ui
					.println("Guid value can not be retrieved from inf file. Generate "
							+ mi.guidvalue + " at random!");
		}
		msaheader.setGuidValue(mi.guidvalue);
		if (mi.moduletype != null) {
			msaheader.setModuleType(ModuleTypeDef.Enum.forString(mi
					.getModuleType()));
		} else {
			msaheader
					.setModuleType(ModuleTypeDef.Enum
							.forString(mi.moduletype = Query("Guid Value Not Found!  Please Input Guid Value")));
		}

		msaheader
				.setCopyright("Copyright (c) 2007, Intel Corporation. All rights reserved.");
		msaheader.setVersion("1.0");
		msaheader.setAbstract("Component name for module " + mi.modulename);
		msaheader.setDescription("FIX ME!");

		if (mi.license == null) {
			mi.license = "FIX ME!";
			MigrationTool.ui
					.println("Fail to extract license info in inf file");
		}
		msaheader.addNewLicense().setStringValue(mi.license);
		msaheader
				.setSpecification("FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052");

		List<Enum> arch = new ArrayList<Enum>();
		arch.add(SupportedArchitectures.IA_32);
		arch.add(SupportedArchitectures.X_64);
		arch.add(SupportedArchitectures.IPF);
		arch.add(SupportedArchitectures.EBC);
		md.setSupportedArchitectures(arch);
		md.setBinaryModule(false);
		md.setOutputFileBasename(mi.modulename);
		//
		// For now, simply add all package guids in the database.
		// 
		it = MigrationTool.db.dumpAllPkgGuid();
		while (it.hasNext()) {
			pd.addNewPackage().setPackageGuid(it.next());
		}
		externs.addNewSpecification().setStringValue(
				"EFI_SPECIFICATION_VERSION 0x00020000");
		externs.addNewSpecification().setStringValue(
				"EDK_RELEASE_VERSION 0x00020000");
		if (mi.entrypoint != null) {
			externs.addNewExtern().setModuleEntryPoint(mi.entrypoint);
			org.tianocore.ModuleTypeDef.Enum moduleType = msaheader
					.getModuleType();
			if (moduleType == ModuleTypeDef.PEIM) {
				mi.hashrequiredr9libs.add("PeimEntryPoint");
			} else {
				mi.hashrequiredr9libs.add("UefiDriverEntryPoint");
			}
		}

		it = mi.localmodulesources.iterator();
		while (it.hasNext()) {
			addSourceFiles(it.next());
		}
		if (!mi.protocols.isEmpty()) {
			protocols = msa.addNewProtocols();
			it = mi.protocols.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					ProtocolsDocument.Protocols.Protocol pr = protocols
							.addNewProtocol();
					pr.setProtocolCName(temp);
					pr.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		if (!mi.ppis.isEmpty()) {
			ppis = msa.addNewPPIs();
			it = mi.ppis.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					PPIsDocument.PPIs.Ppi pp = ppis.addNewPpi();
					pp.setPpiCName(temp);
					pp.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		if (!mi.guids.isEmpty()) {
			guids = msa.addNewGuids();
			it = mi.guids.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					GuidsDocument.Guids.GuidCNames gcn = guids
							.addNewGuidCNames();
					gcn.setGuidCName(temp);
					gcn.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		if (mi.isLibrary) {
			LibraryClassDocument.LibraryClass lc = libclassdefs
					.addNewLibraryClass();
			lc.setKeyword(mi.modulename);
			lc.setUsage(UsageTypes.ALWAYS_PRODUCED);
		}
		it = mi.hashrequiredr9libs.iterator();
		while (it.hasNext()) {
			if ((temp = it.next()) != null && !temp.matches("%")
					&& !temp.matches("n/a")) {
				LibraryClassDocument.LibraryClass lc = libclassdefs
						.addNewLibraryClass();
				lc.setKeyword(temp);
				lc.setUsage(UsageTypes.ALWAYS_CONSUMED);
			}
		}
		addWrapper();
		msadoc.setModuleSurfaceArea(msa);
		return msadoc;
	}

	public void flush() throws Exception {
		XmlOptions options = new XmlOptions();

		options.setCharacterEncoding("UTF-8");
		options.setSavePrettyPrint();
		options.setSavePrettyPrintIndent(2);
		options.setUseDefaultNamespace();

		BufferedWriter bw = new BufferedWriter(new FileWriter(
				MigrationTool.ModuleInfoMap.get(mi) + File.separator
						+ "Migration_" + mi.modulename + File.separator
						+ mi.modulename + ".msa"));
		fulfillMsadoc().save(bw, options);
		// MsaTreeEditor.init(mi, ui, msadoc);
		bw.flush();
		bw.close();
	}

	private static void flush(String path, ModuleSurfaceAreaDocument msadoc)
			throws Exception {
		XmlOptions options = new XmlOptions();

		options.setCharacterEncoding("UTF-8");
		options.setSavePrettyPrint();
		options.setSavePrettyPrintIndent(2);
		options.setUseDefaultNamespace();

		BufferedWriter bw = new BufferedWriter(new FileWriter(path));
		msadoc.save(bw, options);
		bw.flush();
		bw.close();
	}

	public static final void parse(String msafile) throws Exception {
		ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory
				.parse(msafile);
		flush("c:\\temp.msa", msadoc);
	}
}
