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
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.tianocore.GuidDeclarationsDocument.GuidDeclarations;
import org.tianocore.IncludeHeaderDocument.IncludeHeader;
import org.tianocore.LibraryClassDeclarationDocument.LibraryClassDeclaration;
import org.tianocore.LibraryClassDeclarationsDocument.LibraryClassDeclarations;
import org.tianocore.PackageHeadersDocument.PackageHeaders;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PpiDeclarationsDocument.PpiDeclarations;
import org.tianocore.PpiDeclarationsDocument.PpiDeclarations.Entry;
import org.tianocore.ProtocolDeclarationsDocument.ProtocolDeclarations;

/**
 
  This class is to generate a global table for the content of spd file.
  
**/
public class Spd {
    ///
    /// Map of module name and package it belongs to.
    /// Key : Module BaseName
    /// Value: Relative Path to Package
    ///
    Map<String, String[]> msaInfo = new HashMap<String, String[]>();

    ///
    /// Map of module info. 
    /// Key : moduletype
    /// Value: moduletype related include file
    ///
    Map<String, String> moduleInfo = new HashMap<String, String>();

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
    /// Map of library class and its exposed header file.
    /// Key : library class name
    /// value : library class corresponding header file
    ///
    Map<String, String> libClassHeaderList = new HashMap<String, String>();

    ///
    /// Package path.
    ///
    String packagePath = null;

    /**
      Constructor function
      
      This function mainly initialize some member variables. 
   
      @param spdDoc      Handle of spd document.
      @param spdPath     Path of spd file.
     **/
    Spd (PackageSurfaceAreaDocument spdDoc, String spdPath) {

        PackageSurfaceArea spd = spdDoc.getPackageSurfaceArea();
        this.packagePath = spdPath;

        GuidDeclarations spdGuidInfo = spd.getGuidDeclarations();
        genGuidInfoList(spdGuidInfo);

        PpiDeclarations spdPpiInfo = spd.getPpiDeclarations();
        genPpiInfoList(spdPpiInfo);

        ProtocolDeclarations spdProtocolInfo = spd.getProtocolDeclarations();
        genProtocolInfoList(spdProtocolInfo);

        LibraryClassDeclarations spdLibClassDeclare = spd
                        .getLibraryClassDeclarations();
        genLibClassDeclare(spdLibClassDeclare);

        PackageHeaders spdPackageHeaderInfo = spd.getPackageHeaders();
        genModuleInfoList(spdPackageHeaderInfo);

    }

    /**
      genModuleInfoList
      
      This function is to generate Module info map.
      
      @param packageHeader   The information of packageHeader which descripted
                             in spd file.    
    **/
    public void genModuleInfoList(PackageHeaders packageHeader) {

        if (packageHeader != null) {
            List<IncludeHeader> headerList = packageHeader.getIncludeHeaderList();
            IncludeHeader       header;

            for (int i = 0; i < headerList.size(); i++) {
                header = (IncludeHeader)headerList.get(i);
                try {
                    this.moduleInfo.put(header.getModuleType().toString(), header.getStringValue());
                } catch (Exception e) {
                    System.out.print("can't find ModuleHeaders ModuleType & includeHeader!\n");
                }
            }
        }
    }

  /**
    genPpiInfoList
    
    This function is to generate Ppi info map.
    
    @param  ppiInfo           The information of PpiDeclarations which descripted
                              in spd file.    
  **/
    public void genPpiInfoList(PpiDeclarations ppiInfo) {
        String[] cNameGuid = new String[2];
        String   guidString;

        if (ppiInfo != null) {
            List<PpiDeclarations.Entry> ppiEntryList = ppiInfo.getEntryList();
            PpiDeclarations.Entry       ppiEntry;

            for (int i = 0; i < ppiEntryList.size(); i++) {
                ppiEntry = (PpiDeclarations.Entry)ppiEntryList.get(i);
                try {
                    if (ppiEntry.isSetGuidValue()) {
                        guidString = ppiEntry.getGuidValue();
                    } else {
                        guidString = ppiEntry.getGuid().getStringValue();
                    }

                    cNameGuid[0] = ppiEntry.getCName();
                    cNameGuid[1] = formatGuidName(guidString);
                    this.ppiInfo.put(ppiEntry.getName(), new String[] { cNameGuid[0], cNameGuid[1] });
                } catch (Exception e) {
                    System.out.print("can't find GuidDeclarations C_Name & Guid!\n");
                }
            }
        }
    }

    /**
      genProtocolInfoList 
      
      This function is to generate Protocol info map.
      
      @param   proInfo    The information of ProtocolDeclarations which 
                          descripted in spd file.
    **/
    public void genProtocolInfoList(ProtocolDeclarations proInfo) {
        String[] cNameGuid = new String[2];
        String   guidString;

        if (proInfo != null) {
            List<ProtocolDeclarations.Entry> protocolEntryList = proInfo.getEntryList();
            ProtocolDeclarations.Entry       protocolEntry;
            for (int i = 0; i < protocolEntryList.size(); i++) {
                protocolEntry = (ProtocolDeclarations.Entry)protocolEntryList.get(i);
                try {
                    if (protocolEntry.isSetGuidValue()) {
                        guidString = protocolEntry.getGuidValue();
                    } else {
                        guidString = protocolEntry.getGuid().getStringValue();
                    }
                    cNameGuid[0] = protocolEntry.getCName();
                    cNameGuid[1] = formatGuidName(guidString);

                    String temp = new String(protocolEntry.getName());
                    this.protocolInfo.put(temp, new String[] { cNameGuid[0], cNameGuid[1] });
                } catch (Exception e) {
                    System.out.print("can't find ProtocolDeclarations C_Name & Guid!\n");
                }
            }
        }
    }

    /**
      genGuidInfoList
      
      This function is to generate GUID inf map.
      
      @param guidInfo     The information of GuidDeclarations which descripted
                          in spd file.
      
    **/
    public void genGuidInfoList(GuidDeclarations guidInfo) {
        String[] cNameGuid = new String[2];
        String   guidString;

        if (guidInfo != null) {
            
            List<GuidDeclarations.Entry>    guidEntryList = guidInfo.getEntryList();
            GuidDeclarations.Entry          guidEntry;
            for (int i = 0; i < guidEntryList.size(); i++) {
                guidEntry = (GuidDeclarations.Entry)guidEntryList.get(i);
                if (guidEntry.isSetGuidValue()) {
                    guidString = guidEntry.getGuidValue();
                } else {
                    guidString = guidEntry.getGuid().getStringValue();
                }
                    
                cNameGuid[0] = guidEntry.getCName();
                cNameGuid[1] = formatGuidName(guidString);
                this.guidInfo.put(guidEntry.getName(), new String[] {cNameGuid[0], cNameGuid[1] });
            }
        }
    }

    /**
      genLibClassDeclare
      
      This function is to generate the libClassHeader list.
      
      @param libClassDeclares  The information of LibraryClassDeclarations which
                               descripted in spd file.
    **/
    public void genLibClassDeclare(LibraryClassDeclarations libClassDeclares) {
        if (libClassDeclares != null && libClassDeclares.getLibraryClassDeclarationList() != null) {
            if (libClassDeclares.getLibraryClassDeclarationList().size() > 0) {
                List<LibraryClassDeclaration> libDeclareList = libClassDeclares.getLibraryClassDeclarationList();
                for (int i = 0; i < libDeclareList.size(); i++) {
                    libClassHeaderList.put(libDeclareList.get(i).getLibraryClass()
                                    .getStringValue(), libDeclareList.get(i)
                                    .getIncludeHeader().getStringValue());
                }
            }
        }
    }

    /**
      getPpiGuid
      
      This function is to get ppi GUID according ppi name.
    
      @param   ppiStr    Name of ppi.
      @return            PPi's GUID.
    **/
    public String getPpiGuid(String ppiStr) {
        if (ppiInfo.get(ppiStr) != null) {
            return ppiInfo.get(ppiStr)[1];
        } else {
            return null;
        }

    }

    /**
      getPpiCnameGuidArray
      
      This function is to get the ppi CName and it's GUID according to ppi name.
      
      @param   ppiName      Name of ppi.
      @return               Ppi CName and it's GUID.
    **/
    public String[] getPpiCnameGuidArray(String ppiName) {
        return this.ppiInfo.get(ppiName);
    }

    /**
      getProtocolGuid
      
      This function is to get the protocol GUID according to protocol's name.
      
      @param   protocolStr    Name of protocol.
      @return                 Protocol's GUID.
    **/
    public String getProtocolGuid(String protocolStr) {
        if (protocolInfo.get(protocolStr) != null) {
            return this.protocolInfo.get(protocolStr)[0];
        } else {
            return null;
        }
    }

    /**
      getProtocolNameGuidArray
      
      This function is to get the protocol's CName ant it's GUID according to
      protocol's namej.
      
      @param  protocolName   Name of protocl.
      @return                Protocol's CName and it's GUID.
    **/
    public String[] getProtocolNameGuidArray(String protocolName) {
        return this.protocolInfo.get(protocolName);
    }

    /**
      getGUIDGuid
      
      This function is to get the GUID according to GUID's name
      
      @param  guidStr        Name of GUID
      @return                GUID.
    **/
    public String getGUIDGuid(String guidStr) {
        if (guidInfo.get(guidStr) != null) {
            return guidInfo.get(guidStr)[1];
        } else {
            return null;
        }

    }

    /**
      getGuidNameArray
      
      This function is to get the GUID's CName and it's GUID according to 
      GUID's name
      
      @param   guidName     Name of GUID
      @return               CName and GUID.
    **/
    public String[] getGuidNameArray(String guidName) {
        return this.guidInfo.get(guidName);
    }

    /**
      getLibClassInclude 
      
      This function is to get the library exposed header file name according 
      library class name.
      
      @param     libName    Name of library class   
      @return               Name of header file
    **/
    String getLibClassIncluder(String libName) {
        return libClassHeaderList.get(libName);
    }

    /**
      getModuleTypeIncluder
      
      This function is to get the header file name from module info map 
      according to module type.
     
      @param   moduleType    Module type.
      @return                Name of header file.
    **/
    String getModuleTypeIncluder(String moduleType) {
        return moduleInfo.get(moduleType);
    }

    /**
      formateGuidName
      
      This function is to formate GUID to ANSI c form.
     
      @param  guidNameCon      String of GUID.
      @return                  Formated GUID.
    **/
    public static String formatGuidName (String guidNameConv) {
        String[] strList;
        String guid = "";
        int index = 0;
        if (guidNameConv
                        .matches("[a-fA-F0-9]{8}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{4}-[a-fA-F0-9]{12}")) {
            strList = guidNameConv.split("-");
            guid = "0x" + strList[0] + ", ";
            guid = guid + "0x" + strList[1] + ", ";
            guid = guid + "0x" + strList[2] + ", ";
            guid = guid + "{";
            guid = guid + "0x" + strList[3].substring(0, 2) + ", ";
            guid = guid + "0x" + strList[3].substring(2, 4);

            while (index < strList[4].length()) {
                guid = guid + ", ";
                guid = guid + "0x" + strList[4].substring(index, index + 2);
                index = index + 2;
            }
            guid = guid + "}";
            return guid;
        } else if (guidNameConv
                        .matches("0x[a-fA-F0-9]{1,8},( )*0x[a-fA-F0-9]{1,4},( )*0x[a-fA-F0-9]{1,4}(,( )*\\{)?(,?( )*0x[a-fA-F0-9]{1,2}){8}( )*(\\})?")) {
            strList = guidNameConv.split(",");
            
            //
            // chang Microsoft specific form to ANSI c form
            //
            for (int i = 0; i < 3; i++){
                guid = guid + strList[i] + ",";
            }
            guid = guid + "{";
            
            for (int i = 3; i < strList.length; i++){
                if (i == strList.length - 1){
                    guid = guid + strList[i];
                } else {
                    guid = guid + strList[i] + ",";
                }
            }
            guid = guid + "}";            
            return guid;
        } else {
            System.out.println("Check GUID Value, it don't conform to the schema!!!");
            return "0";

        }
    }
}
