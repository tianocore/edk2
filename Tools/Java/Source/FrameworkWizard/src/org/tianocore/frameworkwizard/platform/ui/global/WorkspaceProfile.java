/** @file
  WorkspaceProfile class. 
  
  WorkspaceProfile provide initializing, instoring, querying and update global data.
  It is a bridge to intercommunicate between multiple component, such as AutoGen,
  PCD and so on.   
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.platform.ui.global;

import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PcdCodedDocument;
import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.Vector;

/**
  WorkspaceProfile provide initializing, instoring, querying and update global data.
  It is a bridge to intercommunicate between multiple component, such as AutoGen,
  PCD and so on. 
  
  <p>Note that all global information are initialized incrementally. All data will 
  parse and record only of necessary during build time. </p>
  
  @since GenBuild 1.0
**/
public class WorkspaceProfile {
    ///
    /// Record current WORKSPACE Directory
    ///
    private static String workspaceDir = "";
    
    /**
      Get the current WORKSPACE Directory. 
      
      @return current workspace directory
    **/
    public synchronized static String getWorkspacePath() {
        return workspaceDir;
    }

    public synchronized static PackageIdentification getPackageForModule(ModuleIdentification moduleId) {
        //
        // If package already defined in module
        //
        if (moduleId.getPackageId() != null) {
            return moduleId.getPackageId();
        }
        
        return null;
    }
    //
    // expanded by FrameworkWizard
    //
    public synchronized static ModuleSurfaceAreaDocument.ModuleSurfaceArea getModuleXmlObject(ModuleIdentification moduleId) {
        return GlobalData.openingModuleList.getModuleSurfaceAreaFromId(moduleId);
    }
    
    public synchronized static PackageSurfaceAreaDocument.PackageSurfaceArea getPackageXmlObject(PackageIdentification packageId) {
        return GlobalData.openingPackageList.getPackageSurfaceAreaFromId(packageId);
    }
    
    public static ModuleIdentification getModuleId(String key){
        //
        // Get ModuleGuid, ModuleVersion, PackageGuid, PackageVersion, Arch into string array.
        //
        String[] keyPart = key.split(" ");

        Iterator<ModuleIdentification> iMiList = GlobalData.vModuleList.iterator();
        
        while (iMiList.hasNext()) {
            ModuleIdentification mi = iMiList.next();
            if (mi.getGuid().equalsIgnoreCase(keyPart[0])){
                if (keyPart[1] != null && keyPart[1].length() > 0 && !keyPart[1].equals("null")){
                    if(!mi.getVersion().equals(keyPart[1])){
                        continue;
                    }
                }

                PackageIdentification pi = mi.getPackageId();
                if ( !pi.getGuid().equalsIgnoreCase(keyPart[2])){ 
                    continue;
                }
                if (keyPart[3] != null && keyPart[3].length() > 0 && !keyPart[3].equals("null")){
                    if(!pi.getVersion().equals(keyPart[3])){
                        continue;
                    }
                }
                return mi;
            }
        }
       
        return null;
    }
    
    public static Vector<String> getModuleSupArchs(ModuleIdentification mi){
        Vector<String> vArchs = null;
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)getModuleXmlObject(mi);
        if (msa.getModuleDefinitions() == null || msa.getModuleDefinitions().getSupportedArchitectures() == null) {
            return vArchs;
        }
        ListIterator li = msa.getModuleDefinitions().getSupportedArchitectures().listIterator();
        while (li.hasNext()) {
            if (vArchs == null) {
                vArchs = new Vector<String>();
            }
            vArchs.add((String)li.next());
        }
        
        return vArchs;
    }
    
    public static String getModuleType (ModuleIdentification mi) {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = getModuleXmlObject(mi);
        if (msa.getMsaHeader() == null || msa.getMsaHeader().getModuleType() == null) {
            return null;
        }
        return msa.getMsaHeader().getModuleType().toString();
    }
    
    public static boolean pcdInMsa (String cName, String tsGuid, String supArchList, ModuleIdentification mi) {
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)getModuleXmlObject(mi);
        if (msa.getPcdCoded() == null || msa.getPcdCoded().getPcdEntryList() == null) {
            return false;
        }
        ListIterator li = msa.getPcdCoded().getPcdEntryList().listIterator();
        while (li.hasNext()) {
            PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry)li.next();
            if (msaPcd.getCName().equals(cName) && msaPcd.getTokenSpaceGuidCName().equals(tsGuid)) {
                if (supArchList != null && msaPcd.getSupArchList() != null) {
                	if (msaPcd.getSupArchList().toString().toLowerCase().contains(supArchList.trim().toLowerCase())) {
                		return true;
                	}
                }
                else{
                	return true;
                }
            }
        }
        return false;
    }
    
}


