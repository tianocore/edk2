/** @file
 Spd class.

 This class is to generate a global table for the content of spd file.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.platform.ui.global;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;

/**
 
 This class is to generate a global table for the content of spd file.
 
 **/
public class Spd {
    ///
    ///
    ///
    Map<ModuleIdentification, File> msaInfo = new HashMap<ModuleIdentification, File>();
    
    //
    // Xml Doc of Spd file, Msa file
    //
    public Map<String, XmlObject> spdDocMap = new HashMap<String, XmlObject>();
    public Map<ModuleIdentification, XmlObject> msaDocMap = new HashMap<ModuleIdentification, XmlObject>();
    ///
    /// Package path.
    ///
    PackageIdentification packageId;

    /**
     Constructor function
     
     This function mainly initialize some member variables. 
    **/
    Spd(File packageFile) throws Exception {
        try {
            XmlObject spdDoc = XmlObject.Factory.parse(packageFile);
            //
            // Verify SPD file, if is invalid, throw Exception
            //
//            if (! spdDoc.validate()) {
//                throw new Exception("Package Surface Area file [" + packageFile.getPath() + "] is invalid. ");
//            }
            // We can change Map to XmlObject
            
            spdDocMap.put("PackageSurfaceArea", spdDoc);
            SurfaceAreaQuery.setDoc(spdDocMap);
            //
            //
            //
            packageId = SurfaceAreaQuery.getSpdHeader();
            packageId.setSpdFile(packageFile);
            
            //
            // initialize Msa Files
            // MSA file is absolute file path
            //
            String[] msaFilenames = SurfaceAreaQuery.getSpdMsaFile();
            for (int i = 0; i < msaFilenames.length; i++){
                File msaFile = new File(packageId.getPackageDir() + File.separatorChar + msaFilenames[i]);
                if (!msaFile.exists()) {
                    continue;
                }
                Map<String, XmlObject> msaDoc = WorkspaceProfile.getNativeMsa( msaFile );
                SurfaceAreaQuery.push(msaDoc);
                ModuleIdentification moduleId = SurfaceAreaQuery.getMsaHeader();
                SurfaceAreaQuery.pop();
                moduleId.setPackage(packageId);
                msaInfo.put(moduleId, msaFile);
                msaDocMap.put(moduleId, msaDoc.get("ModuleSurfaceArea"));
            }
            
        }
        catch (Exception e) {
            
            throw new Exception("Parse package description file [" + packageId.getSpdFile() + "] Error.\n"
                                     + e.getMessage());
        }
    }

    public PackageIdentification getPackageId() {
        return packageId;
    }

    public File getModuleFile(ModuleIdentification moduleId) {
        return msaInfo.get(moduleId);
    }
    
    public Set<ModuleIdentification> getModules(){
        return msaInfo.keySet();
    }
    
}
