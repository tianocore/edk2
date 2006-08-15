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
import java.util.*;

import org.tianocore.*;
import org.tianocore.SupportedArchitectures.Enum;
import org.apache.xmlbeans.*;

public class MsaWriter {
	MsaWriter(String path, ModuleInfo moduleinfo, Database database) {
		modulepath = path;
		mi = moduleinfo;
		db = database;
	}

	private String modulepath;
	private ModuleInfo mi;
	private Database db;
	
	private ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.newInstance();
	
	private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = msadoc.addNewModuleSurfaceArea();
	private MsaHeaderDocument.MsaHeader msaheader = msa.addNewMsaHeader();
	private ModuleDefinitionsDocument.ModuleDefinitions md = msa.addNewModuleDefinitions();
	private SourceFilesDocument.SourceFiles sourcefiles = msa.addNewSourceFiles();	//found local .h files are not written
	private GuidsDocument.Guids guids;
	private ProtocolsDocument.Protocols protocols;
	private PPIsDocument.PPIs ppis;
	private PackageDependenciesDocument.PackageDependencies pd = msa.addNewPackageDependencies();
	private LibraryClassDefinitionsDocument.LibraryClassDefinitions libclassdefs = msa.addNewLibraryClassDefinitions();
	private ExternsDocument.Externs externs = msa.addNewExterns();
	
	private String Query (String requirement) throws Exception {
		String answer;
		BufferedReader rd = new BufferedReader(new InputStreamReader(System.in));
		System.out.println(requirement);
		while ((answer = rd.readLine()).length() == 0) ;
		return answer;
	}
	
	private ModuleSurfaceAreaDocument fulfillMsadoc() throws Exception {
		Iterator<String> it;
		String temp;
		
		if (mi.modulename != null) {
			msaheader.setModuleName(mi.modulename);
		} else {
			msaheader.setModuleName(mi.modulename = Query("Module Name Not Found!  Please Input ModuleName"));
		}
		if (mi.guidvalue != null) {
			msaheader.setGuidValue(mi.guidvalue);
		} else {
			msaheader.setGuidValue(mi.guidvalue = Query("Guid Value Not Found!  Please Input Guid Value"));
		}
		if (mi.moduletype != null) {
			if (mi.moduletype.contains("PEI")) {
				msaheader.setModuleType(ModuleTypeDef.Enum.forString("PEIM"));
			} else {
				msaheader.setModuleType(ModuleTypeDef.Enum.forString("DXE_DRIVER"));
			}
		} else {
			msaheader.setModuleType(ModuleTypeDef.Enum.forString(mi.moduletype = Query("Guid Value Not Found!  Please Input Guid Value")));
		}

		msaheader.setCopyright("Copyright (c) 2006, Intel Corporation");
		msaheader.setVersion("1.0");
		msaheader.setAbstract("Component name for module " + mi.modulename);
		msaheader.setDescription("FIX ME!");
		msaheader.addNewLicense().setStringValue("All rights reserved.\n" +
				"      This software and associated documentation (if any) is furnished\n" +
				"      under a license and may only be used or copied in accordance\n" +
				"      with the terms of the license. Except as permitted by such\n" +
				"      license, no part of this software or documentation may be\n" +
				"      reproduced, stored in a retrieval system, or transmitted in any\n" +
				"      form or by any means without the express written consent of\n" +
				"      Intel Corporation.");
		msaheader.setSpecification("FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052");
		
		List<Enum> arch = new ArrayList<Enum>();
		arch.add(SupportedArchitectures.IA_32);
		arch.add(SupportedArchitectures.X_64);
		arch.add(SupportedArchitectures.IPF);
		arch.add(SupportedArchitectures.EBC);
		md.setSupportedArchitectures(arch);
		md.setBinaryModule(false);
		md.setOutputFileBasename(mi.modulename);
		
		pd.addNewPackage().setPackageGuid("5e0e9358-46b6-4ae2-8218-4ab8b9bbdcec");
		externs.addNewSpecification().setStringValue("EFI_SPECIFICATION_VERSION 0x00020000");
		externs.addNewSpecification().setStringValue("EDK_RELEASE_VERSION 0x00020000");
		externs.addNewExtern().setModuleEntryPoint(mi.entrypoint);
		
		FilenameDocument.Filename filename;
		it = mi.localmodulesources.iterator();
		//System.out.println(mi.localmodulesources);
		while (it.hasNext()) {
			temp = it.next();
			filename = sourcefiles.addNewFilename();
			filename.setStringValue(temp);
			//if (temp.contains("x64" + File.separator)) {
				//System.out.println("find");
				//filename.setSupArchList();
			//}
		}
		if (!mi.protocol.isEmpty()) {
			protocols = msa.addNewProtocols();
			it = mi.protocol.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					ProtocolsDocument.Protocols.Protocol pr = protocols.addNewProtocol();
					pr.setProtocolCName(temp);
					pr.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		if (!mi.ppi.isEmpty()) {
			ppis = msa.addNewPPIs();
			it = mi.ppi.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					PPIsDocument.PPIs.Ppi pp = ppis.addNewPpi();
					pp.setPpiCName(temp);
					pp.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		if (!mi.guid.isEmpty()) {
			guids = msa.addNewGuids();
			it = mi.guid.iterator();
			while (it.hasNext()) {
				if ((temp = it.next()) != null) {
					GuidsDocument.Guids.GuidCNames gcn = guids.addNewGuidCNames();
					gcn.setGuidCName(temp);
					gcn.setUsage(UsageTypes.ALWAYS_CONSUMED);
				}
			}
		}
		it = mi.hashrequiredr9libs.iterator();
		while (it.hasNext()) {
			if ((temp = it.next()) != null && !temp.matches("%")) {
				LibraryClassDocument.LibraryClass lc = libclassdefs.addNewLibraryClass();
				lc.setKeyword(temp);
				lc.setUsage(UsageTypes.ALWAYS_CONSUMED);
			}
		}
		
		return msadoc;
	}
	
	public void flush() throws Exception {
        XmlOptions options = new XmlOptions();

        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);
        options.setUseDefaultNamespace();
        
		BufferedWriter bw = new BufferedWriter(new FileWriter(modulepath + File.separator + "result" + File.separator + mi.modulename + ".msa"));
		fulfillMsadoc().save(bw, options);
		bw.flush();
		bw.close();
	}
}
