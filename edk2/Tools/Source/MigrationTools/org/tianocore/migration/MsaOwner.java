package org.tianocore.migration;

import java.util.*;

import org.tianocore.*;
import org.tianocore.SupportedArchitectures.Enum;

public class MsaOwner {
	public static final String COPYRIGHT = "Copyright (c) 2006, Intel Corporation";
	public static final String VERSION = "1.0";
	public static final String ABSTRACT = "Component name for module ";
	public static final String DESCRIPTION = "FIX ME!";
	public static final String LICENSE = "All rights reserved.\n" +
    "      This software and associated documentation (if any) is furnished\n" +
    "      under a license and may only be used or copied in accordance\n" +
    "      with the terms of the license. Except as permitted by such\n" +
    "      license, no part of this software or documentation may be\n" +
    "      reproduced, stored in a retrieval system, or transmitted in any\n" +
    "      form or by any means without the express written consent of\n" +
    "      Intel Corporation.";
	public static final String SPECIFICATION = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";
	
	public static final Enum IA32 = SupportedArchitectures.IA_32;
	public static final Enum X64 = SupportedArchitectures.X_64;
	public static final Enum IPF = SupportedArchitectures.IPF;
	public static final Enum EBC = SupportedArchitectures.EBC;
	
    private ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.newInstance();
    
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;
    private MsaHeaderDocument.MsaHeader msaheader = null;
    private LicenseDocument.License license = null;
    private ModuleDefinitionsDocument.ModuleDefinitions moduledefinitions = null;
    private SourceFilesDocument.SourceFiles sourcefiles = null;    //found local .h files are not written
    private GuidsDocument.Guids guids = null;
    private ProtocolsDocument.Protocols protocols = null;
    private PPIsDocument.PPIs ppis = null;
    private PackageDependenciesDocument.PackageDependencies packagedependencies = null;
    private LibraryClassDefinitionsDocument.LibraryClassDefinitions libclassdefs = null;
    private ExternsDocument.Externs externs = null;
    
    private List<Enum> listarch = new ArrayList<Enum>();
    private Map<String, Enum> mapfilenames = new HashMap<String, Enum>();	//this need to be installed manually when msa is to be written
    private Map<String, UsageTypes.Enum> mapprotocols = new HashMap<String, UsageTypes.Enum>();

    //-----------------------------msaheader-------------------------------------//
    private final boolean installProtocols () {
    	if (mapprotocols.isEmpty()) {
    		return false;
    	}
    	Set<String> setprotocols = mapprotocols.keySet();
    	ProtocolsDocument.Protocols.Protocol protocol;
    	Iterator<String> it = setprotocols.iterator();
    	while (it.hasNext()) {
    		protocol = protocols.addNewProtocol();
    		protocol.setProtocolCName(it.next());
    		protocol.setUsage(mapprotocols.get(protocol.getProtocolCName()));
    	}
    	return true;
    }
    
    public final boolean addProtocols (String protocol, UsageTypes.Enum usage) {
    	if (mapprotocols.containsKey(protocol)) {
    		return false;
    	} else {
    		mapprotocols.put(protocol, usage);
    		return true;
    	}
    }
    
    private final boolean installHashFilename () {
    	if (mapfilenames.isEmpty()) {
    		return false;
    	}
    	Set<String> setfilename = mapfilenames.keySet();
    	FilenameDocument.Filename filename;
    	List<Enum> arch = new ArrayList<Enum>();
    	Iterator<String> it = setfilename.iterator();
    	while (it.hasNext()) {
        	filename = sourcefiles.addNewFilename();
        	filename.setStringValue(it.next());
        	arch.add(mapfilenames.get(filename.getStringValue()));
        	filename.setSupArchList(arch);
    	}
    	return true;
    }
    
    public final boolean addSourceFile (String filename, Enum arch) {		// dummy & null how to imply?
    	if (mapfilenames.containsKey(filename)) {
    		return false;
    	} else {
            mapfilenames.put(filename, arch);
            return true;
    	}
    }
    
    // entry point todo
    
    public final boolean setupExternSpecification () {
    	addExternSpecification("EFI_SPECIFICATION_VERSION 0x00020000");
    	addExternSpecification("EDK_RELEASE_VERSION 0x00020000");
    	return true;
    }
    
    public final boolean addExternSpecification (String specification) {
    	if (externs.getSpecificationList().contains(specification)) {
    		return false;
    	} else {
    		externs.addSpecification(specification);
    		return true;
    	}
    }
    
    public final boolean setupPackageDependencies() {
    	addPackage("5e0e9358-46b6-4ae2-8218-4ab8b9bbdcec");
    	addPackage("68169ab0-d41b-4009-9060-292c253ac43d");
    	return true;
    }
    
    public final boolean addPackage (String guid) {
    	if (packagedependencies.getPackageList().contains(guid)) {
    		return false;
    	} else {
            packagedependencies.addNewPackage().setPackageGuid(guid);
            return true;
    	}
    }
    
    public final boolean setupModuleDefinitions () {				//????????? give this job to moduleinfo
    	moduledefinitions.setBinaryModule(false);
    	moduledefinitions.setOutputFileBasename(msaheader.getModuleName());
    	return true;
    }
    public final boolean addSupportedArchitectures (Enum arch) {
    	if (listarch.contains(arch)) {
    		return false;
    	} else {
    		listarch.add(arch);
    		return true;
    	}
    }
    
    public final boolean addSpecification (String specification) {
        if (msaheader.getSpecification() == null) {
            if (specification == null) {
            	msaheader.setSpecification(SPECIFICATION);
            } else {
            	msaheader.setSpecification(specification);
            }
            return true;
        } else {
    		MigrationTool.ui.println ("Warning: Duplicate Specification");
    		return false;
        }
    }
    
    public final boolean addLicense (String licensecontent) {
        if (msaheader.getLicense() == null) {
        	license = msaheader.addNewLicense();
            if (licensecontent == null) {
            	license.setStringValue(LICENSE);
            } else {
            	license.setStringValue(licensecontent);
            }
            return true;
        } else {
    		MigrationTool.ui.println ("Warning: Duplicate License");
    		return false;
        }
    }
    
    public final boolean addDescription (String description) {
        if (msaheader.getDescription() == null) {
            if (description == null) {
            	msaheader.setDescription(DESCRIPTION);
            } else {
            	msaheader.setDescription(description);
            }
            return true;
        } else {
    		MigrationTool.ui.println ("Warning: Duplicate Description");
    		return false;
        }
    }
    
    public final boolean addAbstract (String abs) {
        if (msaheader.getAbstract() == null) {
            if (abs == null) {
            	msaheader.setAbstract(ABSTRACT + msaheader.getModuleName());
            } else {
            	msaheader.setVersion(abs);
            }
            return true;
        } else {
    		MigrationTool.ui.println ("Warning: Duplicate Abstract");
    		return false;
        }
    }
    
    public final boolean addVersion (String version) {
        if (msaheader.getVersion() == null) {
            if (version == null) {
            	msaheader.setVersion(VERSION);
            } else {
            	msaheader.setVersion(version);
            }
            return true;
        } else {
    		MigrationTool.ui.println ("Warning: Duplicate Version");
    		return false;
        }
    }
    
    public final boolean addCopyRight (String copyright) {
    	if (msaheader.getCopyright() == null) {
        	if (copyright == null) {
                msaheader.setCopyright(COPYRIGHT);
        	} else {
        		msaheader.setCopyright(copyright);
        	}
        	return true;
    	} else {
    		MigrationTool.ui.println ("Warning: Duplicate CopyRight");
    		return false;
    	}
    }
    
    public final boolean addModuleType (String moduletype) {
    	if (msaheader.getModuleType() == null) {
    		msaheader.setModuleType(ModuleTypeDef.Enum.forString(moduletype));
    		return true;
    	} else {
    		MigrationTool.ui.println ("Warning: Duplicate ModuleType");
        	return false;
    	}
    }
    
    public final boolean addGuidValue (String guidvalue) {
    	if (msaheader.getGuidValue() == null) {
    		msaheader.setGuidValue(guidvalue);
    		return true;
    	} else  {
    		MigrationTool.ui.println ("Warning: Duplicate GuidValue");
        	return false;
    	}
    }
    
    public final boolean addModuleName (String modulename) {
    	if (msaheader.getModuleName() == null) {
        	msaheader.setModuleName(modulename);
        	return true;
    	} else {
    		MigrationTool.ui.println ("Warning: Duplicate ModuleName");
        	return false;
    	}
    }
    //-----------------------------msaheader-------------------------------------//
    
    public final void addSourceFiles (String filename, int arch) {
    	
    }
    
    private final MsaOwner init () {
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