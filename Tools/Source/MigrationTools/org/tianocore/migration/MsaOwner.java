package org.tianocore.migration;

import org.tianocore.*;

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
	
    private ModuleSurfaceAreaDocument msadoc = ModuleSurfaceAreaDocument.Factory.newInstance();
    
    private ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = null;
    private MsaHeaderDocument.MsaHeader msaheader = null;
    private LicenseDocument.License license = null;
    private ModuleDefinitionsDocument.ModuleDefinitions md = null;
    private SourceFilesDocument.SourceFiles sourcefiles = null;    //found local .h files are not written
    private GuidsDocument.Guids guids = null;
    private ProtocolsDocument.Protocols protocols = null;
    private PPIsDocument.PPIs ppis = null;
    private PackageDependenciesDocument.PackageDependencies pd = null;
    private LibraryClassDefinitionsDocument.LibraryClassDefinitions libclassdefs = null;
    private ExternsDocument.Externs externs = null;

    //-----------------------------msaheader-------------------------------------//
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
    	md = msa.addNewModuleDefinitions();
    	sourcefiles = msa.addNewSourceFiles();
    	pd = msa.addNewPackageDependencies();
    	libclassdefs = msa.addNewLibraryClassDefinitions();
    	externs = msa.addNewExterns();
    	return this;
    }
    
    public static final MsaOwner initNewMsaOwner() {
    	return new MsaOwner().init();
    }
}