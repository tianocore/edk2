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
package org.tianocore.build.global;

import java.io.File;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.common.exception.EdkException;

/**
 
 This class is to generate a global table for the content of spd file.
 
**/
public class Spd {
    ///
    ///
    ///
    Map<ModuleIdentification, File> msaInfo = new HashMap<ModuleIdentification, File>();

    ///
    /// Map of module info. 
    /// Key : moduletype
    /// Value: moduletype related include file
    ///
    Map<String, String> packageHeaderInfo = new HashMap<String, String>();

    ///
    /// Map of PPI info.
    /// Key : PPI name
    /// value: String[] a. PPI C_NAME; b. PPI GUID;
    ///
    Map<String, String[]> ppiInfo = new HashMap<String, String[]>();

    ///
    /// Map of Protocol info.
    /// Key : Protocol name
    /// value: String[] a. Protocol C_NAME; b. Protocol GUID;
    ///
    Map<String, String[]> protocolInfo = new HashMap<String, String[]>();

    ///
    /// Map of Guid info.
    /// Key : Guid name
    /// value: String[] a. Guid C_NAME; b. Guid's GUID;
    ///
    Map<String, String[]> guidInfo = new HashMap<String, String[]>();

    ///
    /// Map of Guid info
    /// Key: GuidCName
    /// value: String Guid's GUID
    ///
    Map<String, String> guidCnameInfo = new HashMap<String, String>();
    
    /// Map of library class and its exposed header file.
    /// Key : library class name
    /// value : library class corresponding header file
    ///
    Map<String, String[]> libClassHeaderList = new HashMap<String, String[]>();

    ///
    /// Package path.
    ///
    PackageIdentification packageId;

    /**
     Constructor function
     
     This function mainly initialize some member variables. 
    **/
    Spd(File packageFile) throws EdkException {
        //
        // If specified package file not exists
        //
        if ( ! packageFile.exists()) {
            throw new EdkException("Package file [" + packageFile.getPath() + "] does not exist!");
        }
        try {
            XmlObject spdDoc = GlobalData.parseXmlFile(packageFile);
            //
            // We can change Map to XmlObject
            //
            Map<String, XmlObject> spdDocMap = new HashMap<String, XmlObject>();
            spdDocMap.put("PackageSurfaceArea", spdDoc);
            SurfaceAreaQuery saq = new SurfaceAreaQuery(spdDocMap);

            packageId = saq.getSpdHeader();
            packageId.setSpdFile(packageFile);
            
            //
            // initialize Msa Files
            // MSA file is absolute file path
            //
            String[] msaFilenames = saq.getSpdMsaFile();
            for (int i = 0; i < msaFilenames.length; i++){
                File msaFile = new File(packageId.getPackageDir() + File.separatorChar + msaFilenames[i]);
                Map<String, XmlObject> msaDoc = GlobalData.getNativeMsa( msaFile );
                saq.push(msaDoc);
                ModuleIdentification moduleId = saq.getMsaHeader();
                saq.pop();
                moduleId.setPackage(packageId);
                moduleId.setMsaFile(msaFile);
                if (msaInfo.containsKey(moduleId)) {
                    throw new EdkException("Found two modules with the same GUID and Version in package " + packageId + ".\nThey are  module [" + msaInfo.get(moduleId) + "] and MSA file [" + msaFile + "]!");
                }
                msaInfo.put(moduleId, msaFile);
            }
            
            //
            // initialize Package header files
            //
            Map<String, String> packageHeaders = saq.getSpdPackageHeaderFiles();
            Set keys = packageHeaders.keySet();
            Iterator iter = keys.iterator();
            while (iter.hasNext()){
                String moduleType = (String)iter.next();
                String header = packageId.getPackageRelativeDir() + File.separatorChar + packageHeaders.get(moduleType);
                
                //
                // Change path seperator to system-dependent path separator
                //
                File file = new File (header);
                header = file.getPath();
                packageHeaderInfo.put(moduleType, header);
            }
            
            //
            // initialize Guid Info
            //
            guidInfo.putAll(saq.getSpdGuid());
            
            //
            // For Pcd get TokenSpaceGuid
            //
            Set<String> key = guidInfo.keySet();
            Iterator item = key.iterator();
            String [] nameValue = new String[2];
            while(item.hasNext()){
                nameValue = guidInfo.get(item.next());
                guidCnameInfo.put(nameValue[0], nameValue[1]);
            }
            
            //
            // initialize PPI info
            //
            ppiInfo.putAll(saq.getSpdPpi());
            
            //
            // initialize Protocol info
            //
            protocolInfo.putAll(saq.getSpdProtocol());
            
            //
            // initialize library class declaration
            //
            Map<String, String[]> libraryClassHeaders = saq.getSpdLibraryClasses();
            keys = libraryClassHeaders.keySet();
            iter = keys.iterator();
            while (iter.hasNext()){
                String libraryClassName = (String)iter.next();
                String[] headerFiles = libraryClassHeaders.get(libraryClassName);
                for (int i = 0; i < headerFiles.length; i++){
                    headerFiles[i] = packageId.getPackageRelativeDir() + File.separatorChar + headerFiles[i];
                    
                    //
                    // Change path separator to system system-dependent path separator. 
                    //
                    File file = new File (headerFiles[i]);
                    headerFiles[i] = file.getPath();
                }
                libClassHeaderList.put(libraryClassName, headerFiles);
            }
        } catch (IOException ex) {
            EdkException edkException = new EdkException("Parse of the package description file [" + packageFile.getPath() + "] failed!\n" + ex.getMessage());
            edkException.setStackTrace(ex.getStackTrace());
            throw edkException;
        } catch (XmlException ex) {
            EdkException edkException = new EdkException("Parse of the package description file [" + packageFile.getPath() + "] failed!\n" + ex.getMessage());
            edkException.setStackTrace(ex.getStackTrace());
            throw edkException;
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

    /**
       return two value {CName, Guid}. If not found, return null.
     **/
    public String[] getPpi(String ppiName) {
        return ppiInfo.get(ppiName);
    }

    /**
        return two value {CName, Guid}. If not found, return null.
    **/
    public String[] getProtocol(String protocolName) {
        return protocolInfo.get(protocolName);
    }

    /**
      return two value {CName, Guid}. If not found, return null.
    **/
    public String[] getGuid(String guidName) {
        return guidInfo.get(guidName);
    }

    /**
     * return Guid Value. 
     */
    public String getGuidFromCname(String cName){
        return guidCnameInfo.get(cName);
    }
    
    /**
     getLibClassInclude 
     
     This function is to get the library exposed header file name according 
     library class name.
     
     @param     libName    Name of library class   
     @return               Name of header file
    **/
    String[] getLibClassIncluder(String libName) {
        return libClassHeaderList.get(libName);
    }

    /**
      getModuleTypeIncluder
    
      This function is to get the header file name from module info map 
      according to module type.
    
      @param   moduleType    Module type.
      @return                Name of header file.
    **/
    String getPackageIncluder(String moduleType) {
        return packageHeaderInfo.get(moduleType);
    }
    

}
