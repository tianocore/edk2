/** @file
  Java class SpdFileContents is used to parse spd xml file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.packaging.ui;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;
import java.util.Vector;


import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.apache.xmlbeans.XmlCursor;

import org.tianocore.GuidDeclarationsDocument;

import org.tianocore.LibraryClassDeclarationsDocument;

import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaFilesDocument;
import org.tianocore.PackageDefinitionsDocument;
import org.tianocore.PackageHeadersDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PcdDataTypes;
import org.tianocore.PcdDeclarationsDocument;
import org.tianocore.PpiDeclarationsDocument;
import org.tianocore.ProtocolDeclarationsDocument;
import org.tianocore.SpdHeaderDocument;
import org.tianocore.GuidDeclarationsDocument.GuidDeclarations;

/**
 This class processes spd file contents such as add remove xml elements.
  
 @since PackageEditor 1.0
**/

public class SpdFileContents {

    private File file = null;

    private PackageSurfaceAreaDocument psad = null;

    private PackageSurfaceAreaDocument.PackageSurfaceArea psaRoot = null;

    private SpdHeaderDocument.SpdHeader spdHdr = null;
    
    private PackageDefinitionsDocument.PackageDefinitions spdPkgDefs = null;
    
    private LibraryClassDeclarationsDocument.LibraryClassDeclarations spdLibClassDeclarations = null;

    private MsaFilesDocument.MsaFiles spdMsaFiles = null;

    private PackageHeadersDocument.PackageHeaders spdModHdrs = null;

    private GuidDeclarationsDocument.GuidDeclarations spdGuidDeclarations = null;

    private ProtocolDeclarationsDocument.ProtocolDeclarations spdProtocolDeclarations = null;

    private PpiDeclarationsDocument.PpiDeclarations spdPpiDeclarations = null;

    private PcdDeclarationsDocument.PcdDeclarations spdPcdDefinitions = null;

    /**
     Constructor to create a new spd file
    **/
    public SpdFileContents() {

        psad = PackageSurfaceAreaDocument.Factory.newInstance();
        psaRoot = psad.addNewPackageSurfaceArea();

    }

    /**
     Constructor for existing document object
       @param psa
    **/
    public SpdFileContents(PackageSurfaceAreaDocument.PackageSurfaceArea psa) {
        psaRoot = psa;
        spdHdr = psaRoot.getSpdHeader();
        spdPkgDefs = psaRoot.getPackageDefinitions();
    }
    /**
     Constructor based on an existing spd file
     
     @param f Existing spd file
    **/
    public SpdFileContents(File f) {
        try {
            psad = PackageSurfaceAreaDocument.Factory.parse(f);
            psaRoot = psad.getPackageSurfaceArea();
            file = f;
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    /**
     Remove existing pcd definitions elements using XmlCursor
    **/
    public void removeSpdPcdDefinition() {
        XmlObject o = psaRoot.getPcdDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdPcdDefinitions = null;
        cursor.dispose();
    }
    
    public void removeSpdPcdDefinition(int i){
        XmlObject o = psaRoot.getPcdDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdPcdDefinitionCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        } 
        cursor.dispose();
    }

    /**
     Remove existing ppi declarations using XmlCursor
    **/
    public void removeSpdPpiDeclaration() {
        XmlObject o = psaRoot.getPpiDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdPpiDeclarations = null;
        cursor.dispose();
    }

    public void removeSpdPpiDeclaration(int i){
        XmlObject o = psaRoot.getPpiDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdPpiDeclarationCount() == 0){
                cursor.toParent();
                cursor.removeXml();
            }
        } 
        cursor.dispose();
    }
    /**
     Remove existing protocols declarations using XmlCursor
    **/
    public void removeSpdProtocolDeclaration() {
        XmlObject o = psaRoot.getProtocolDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdProtocolDeclarations = null;
        cursor.dispose();
    }
    
    public void removeSpdProtocolDeclaration(int i) {
        XmlObject o = psaRoot.getProtocolDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdProtocolDeclarationCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    /**
     Remove existing GUID declarations using XmlCursor
    **/
    public void removeSpdGuidDeclaration() {
        XmlObject o = psaRoot.getGuidDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdGuidDeclarations = null; 
        cursor.dispose();
    }
    
    public void removeSpdGuidDeclaration(int i) {
        XmlObject o = psaRoot.getGuidDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdGuidDeclarationCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        } 
        cursor.dispose();
    }

    /**
     Remove existing spd package include files using XmlCursor
    **/
    public void removeSpdPkgHeader() {
        XmlObject o = psaRoot.getPackageHeaders();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdModHdrs = null;
        cursor.dispose();
    }
    
    public void removeSpdPkgHeader(int i){
        XmlObject o = psaRoot.getPackageHeaders();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdPackageHeaderCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        } 
        cursor.dispose();
    }

    /**
     Remove existing msa files using XmlCursor
    **/
    public void removeSpdMsaFile() {
        XmlObject o = psaRoot.getMsaFiles();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdMsaFiles = null;
        cursor.dispose();
    }
    
    public void removeSpdMsaFile(int i){
        XmlObject o = psaRoot.getMsaFiles();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdMsaFileCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }

    /**
     Remove existing library class declarations using XmlCursor
    **/
    public void removeSpdLibClass() {
        XmlObject o = psaRoot.getLibraryClassDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdLibClassDeclarations = null;
        cursor.dispose();
    }
    
    public void removeSpdLibClass(int i) {
        XmlObject o = psaRoot.getLibraryClassDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getSpdLibClassDeclarationCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    
    public void updateSpdLibClass(int i, String lib, String hdr, String hlp, String clsUsage, String instanceVer, String hdrAttribArch, String hdrAttribModType) {
        XmlObject o = psaRoot.getLibraryClassDeclarations();
        if (o == null)
            return;
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass lc = (LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass)cursor.getObject();
            lc.setName(lib);
            lc.setIncludeHeader(hdr);
            lc.setHelpText(hlp);
            if (clsUsage != null) {
              lc.setRecommendedInstanceGuid(clsUsage);
              if (instanceVer != null){
                lc.setRecommendedInstanceVersion(instanceVer);
              } else {
                if (lc.isSetRecommendedInstanceVersion()) {
                  lc.unsetRecommendedInstanceVersion();
                }
              }
            } else {
              if (lc.isSetRecommendedInstanceGuid()) {
                lc.unsetRecommendedInstanceGuid();
              }
              if (lc.isSetRecommendedInstanceVersion()) {
                  lc.unsetRecommendedInstanceVersion();
              }
            }

            if (stringToList(hdrAttribArch) != null){
              lc.setSupArchList(stringToList(hdrAttribArch));
            } else {
              if (lc.isSetSupArchList()) {
                lc.unsetSupArchList();
              }
            }
            if (stringToList(hdrAttribModType) != null){
              lc.setSupModuleList(stringToList(hdrAttribModType));
            } else {
              if (lc.isSetSupModuleList()) {
                lc.unsetSupModuleList();
              }
            }
        }
        
        cursor.dispose();
    }
    
    public void updateSpdMsaFile(int i, String msaFile, String mName, String v, String g){
        XmlObject o = psaRoot.getMsaFiles();
        if (o == null)
            return;
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.setTextValue(msaFile);

        }
        
        cursor.dispose();
    }

    public void updateSpdGuidDecl(int i, String name, String cName, String guid, String hlp, String archList, 
                                  String modTypeList, String guidTypeList){
        XmlObject o = psaRoot.getGuidDeclarations();
        if (o == null){
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            GuidDeclarationsDocument.GuidDeclarations.Entry e = (GuidDeclarationsDocument.GuidDeclarations.Entry)cursor.getObject();
            e.setName(name);
            e.setCName(cName);
            e.setGuidValue(guid);
            e.setHelpText(hlp);
            if (stringToList(guidTypeList) != null) {
              e.setGuidTypeList(stringToList(guidTypeList));
            }
            else{
              if (e.isSetGuidTypeList()) {
                e.unsetGuidTypeList();
              }
            }
            if (stringToList(archList) != null){
                e.setSupArchList(stringToList(archList));
            }
            else{
              if (e.isSetSupArchList()) {
                e.unsetSupArchList();
              }
            }
            if (stringToList(modTypeList) != null) {
                e.setSupModuleList(stringToList(modTypeList));
            }
            else{
              if (e.isSetSupModuleList()) {
                e.unsetSupModuleList();
              }
            }
            
        }
        cursor.dispose();
    }
    
    public void updateSpdPpiDecl(int i, String name, String cName, String guid, String hlp, String archList, 
                                 String modTypeList, String guidTypeList){
        XmlObject o = psaRoot.getPpiDeclarations();
        if (o == null){
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            PpiDeclarationsDocument.PpiDeclarations.Entry e = (PpiDeclarationsDocument.PpiDeclarations.Entry)cursor.getObject();
            e.setName(name);
            e.setCName(cName);
            e.setGuidValue(guid);
            e.setHelpText(hlp);
            if (stringToList(guidTypeList) != null) {
                e.setGuidTypeList(stringToList(guidTypeList));
            }
            else{
                if (e.isSetGuidTypeList()) {
                  e.unsetGuidTypeList();
                }
            }
            if (stringToList(archList) != null){
                e.setSupArchList(stringToList(archList));
            }
            else{
              if (e.isSetSupArchList()) {
                e.unsetSupArchList();
              }
            }
            if (stringToList(modTypeList) != null) {
                e.setSupModuleList(stringToList(modTypeList));
            }
            else{
              if (e.isSetSupModuleList()) {
                e.unsetSupModuleList();
              }
            }
        }
        cursor.dispose();
    }
    
    public void updateSpdProtocolDecl(int i, String name, String cName, String guid, String hlp, String archList, 
                                      String modTypeList, String guidTypeList){
        XmlObject o = psaRoot.getProtocolDeclarations();
        if (o == null){
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry e = (ProtocolDeclarationsDocument.ProtocolDeclarations.Entry)cursor.getObject();
            e.setName(name);
            e.setCName(cName);
            e.setGuidValue(guid);
            e.setHelpText(hlp);
            if (stringToList(guidTypeList) != null) {
                e.setGuidTypeList(stringToList(guidTypeList));
              }
            else{
                if (e.isSetGuidTypeList()) {
                  e.unsetGuidTypeList();
                }
            }
            if (stringToList(archList) != null){
                e.setSupArchList(stringToList(archList));
            }
            else{
              if (e.isSetSupArchList()) {
                e.unsetSupArchList();
              }
            }
            if (stringToList(modTypeList) != null) {
                e.setSupModuleList(stringToList(modTypeList));
            }
            else{
              if (e.isSetSupModuleList()) {
                e.unsetSupModuleList();
              }
            }
        }
        cursor.dispose();
    }
    
    public void updateSpdPkgHdr(int i, String pkgName, String hdrName){
        XmlObject o = psaRoot.getPackageHeaders();
        if (o == null){
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader iph = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader)cursor.getObject();
            iph.setModuleType(ModuleTypeDef.Enum.forString(pkgName));
            iph.setStringValue(hdrName);
        }
        cursor.dispose();
    }
    
    public void updateSpdPcdDefinition(int i, String cName, String token, String dataType, String pcdItemTypes, 
                                       String spaceGuid, String defaultString, String help, String archList, String modTypeList){
        XmlObject o = psaRoot.getPcdDeclarations();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            PcdDeclarationsDocument.PcdDeclarations.PcdEntry e = (PcdDeclarationsDocument.PcdDeclarations.PcdEntry)cursor.getObject();
            e.setCName(cName);
            e.setToken(token);
            e.setDatumType(PcdDataTypes.Enum.forString(dataType));
            if (pcdItemTypes != null && pcdItemTypes.length() > 0) {
                String usage[] = pcdItemTypes.split("( )+");
                List<String> l = new ArrayList<String>();
                for (int k = 0; k < usage.length; ++k) {
                    l.add(usage[k]);  
                }
                e.setValidUsage(l);
            }
            
            e.setTokenSpaceGuidCName(spaceGuid);
            e.setDefaultValue(defaultString);
            e.setHelpText(help);
            if (stringToList(archList) != null){
                e.setSupArchList(stringToList(archList));
            }
            else{
              if (e.isSetSupArchList()) {
                e.unsetSupArchList();
              }
            }
            if (stringToList(modTypeList) != null) {
                e.setSupModuleList(stringToList(modTypeList));
            }
            else{
              if (e.isSetSupModuleList()) {
                e.unsetSupModuleList();
              }
            }
            
        } 
        cursor.dispose();
    }
    /**
     Get spd file header contents into String array
     
     @param s Caller allocated String array
    **/
    public void getSpdHdrDetails(String[] s) {
        if (getSpdHdr() == null) {
            spdHdr = psaRoot.addNewSpdHeader();
        }
        s[0] = getSpdHdrPkgName();
        s[1] = getSpdHdr().getGuidValue();
        s[2] = getSpdHdrVer();
//        s[3] = getSpdHdr().getAbstract();
        s[4] = getSpdHdr().getDescription();
        s[5] = getSpdHdr().getCopyright();
        s[6] = getSpdHdrLicense();
        
    }

    /**
     Get the number of library class declarations from the size of List
     
     @return int
    **/
    public int getSpdLibClassDeclarationCount() {
        if (psaRoot.getLibraryClassDeclarations() == null
            || psaRoot.getLibraryClassDeclarations().getLibraryClassList() == null) {
            return 0;
        }
        return psaRoot.getLibraryClassDeclarations().getLibraryClassList().size();
    }

    /**
     Get available library class declaration into String array
     @param libClass Caller allocated two-dimentional String array
    **/
    public void getSpdLibClassDeclarations(String[][] libClass) {
        if (psaRoot.getLibraryClassDeclarations() == null){
            return;
        }
        
        List<LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass> l = psaRoot.getLibraryClassDeclarations().getLibraryClassList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass lc = (LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass) li.next();
            if (lc != null) {
                libClass[i][0] = lc.getName();
                libClass[i][1] = lc.getIncludeHeader();
                libClass[i][2] = lc.getHelpText();
// LAH added logic so you cannot set the version unless the GUID is defined.

//                if (lc.getRecommendedInstanceGuid() != null) {
                  libClass[i][3] = lc.getRecommendedInstanceGuid();
//                  if (lc.getRecommendedInstanceVersion() != null) {
                    libClass[i][4] = lc.getRecommendedInstanceVersion();
//                  }
//                }

                if (lc.getSupArchList() != null) {
                    libClass[i][5] = listToString(lc.getSupArchList());
                }
                if (lc.getSupModuleList() != null) {
                    libClass[i][6] = listToString(lc.getSupModuleList());
                }
                
            }
            
            i++;
        }

    }
    
    /**
    Get the number of Msa files from the size of List
    
    @return int
   **/
    public int getSpdMsaFileCount() {
        if (psaRoot.getMsaFiles() == null || psaRoot.getMsaFiles().getFilenameList() == null) {
            return 0;
        }
        return psaRoot.getMsaFiles().getFilenameList().size();
    }
    
    /**
    Get available Msa file into String array
    
    @param msaFile Caller allocated two-dimentional String array
   **/
    public void getSpdMsaFiles(String[][] msaFile) {
        if (psaRoot.getMsaFiles() == null) {
            return;
        }
        List<String> l = psaRoot.getMsaFiles().getFilenameList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            msaFile[i][0] = (String)li.next();

            i++;
        }
    }

    /**
    Get the number of include header files in PackageHeaders from the size of List
    
    @return int
   **/
    public int getSpdPackageHeaderCount() {
        if (psaRoot.getPackageHeaders() == null || psaRoot.getPackageHeaders().getIncludePkgHeaderList() == null) {
            return 0;
        }
        return psaRoot.getPackageHeaders().getIncludePkgHeaderList().size();
    }

    /**
    Get available package header contents into String array
    
    @param pkgHeader Caller allocated two-dimentional String array
   **/
    public void getSpdPackageHeaders(String[][] pkgHeader) {
        if (psaRoot.getPackageHeaders() == null) {
            return;
        }
        
        List<PackageHeadersDocument.PackageHeaders.IncludePkgHeader> l = psaRoot.getPackageHeaders().getIncludePkgHeaderList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader ih = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader) li.next();
            if (ih.getModuleType() != null) {
                pkgHeader[i][0] = ih.getModuleType().toString();
            }

            pkgHeader[i][1] = ih.getStringValue();
            i++;
        }
    }

    public void getSpdGuidDeclWithType (Vector<String> vGuidCName, String type) {
        if (psaRoot.getGuidDeclarations() == null) {
            return;
        }
        List<GuidDeclarationsDocument.GuidDeclarations.Entry> l = psaRoot.getGuidDeclarations().getEntryList();
        for (int i = 0; i < l.size(); ++i) {
            if (l.get(i).getGuidTypeList() == null || l.get(i).getGuidTypeList().contains(type)) {
                vGuidCName.add(l.get(i).getCName());
            }
        }
    }
    /**
    Get the number of GUID declarations from the size of List
    
    @return int
   **/
    public int getSpdGuidDeclarationCount() {
        if (psaRoot.getGuidDeclarations() == null || psaRoot.getGuidDeclarations().getEntryList() == null) {
            return 0;
        }
        return psaRoot.getGuidDeclarations().getEntryList().size();
    }

    /**
    Get available Guid declaration contents into String array
    
    @param guid Caller allocated two-dimentional String array
   **/
    public void getSpdGuidDeclarations(String[][] guid) {
        if (psaRoot.getGuidDeclarations() == null) {
            return;
        }
        
        List<GuidDeclarationsDocument.GuidDeclarations.Entry> l = psaRoot.getGuidDeclarations().getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            GuidDeclarationsDocument.GuidDeclarations.Entry e = (GuidDeclarationsDocument.GuidDeclarations.Entry) li
                                                                                                                    .next();
            guid[i][0] = e.getName();
            guid[i][1] = e.getCName();
            if (e.getGuidValue() != null) {
                guid[i][2] = e.getGuidValue();
            }
            guid[i][3] = e.getHelpText();
            guid[i][4] = listToString(e.getSupArchList());
            guid[i][5] = listToString(e.getSupModuleList());
            guid[i][6] = listToString(e.getGuidTypeList());
            i++;
        }
    }

    /**
    Get the number of protocol declarations from the size of List
    
    @return int
   **/
    public int getSpdProtocolDeclarationCount() {
        if (psaRoot.getProtocolDeclarations() == null || psaRoot.getProtocolDeclarations().getEntryList() == null) {
            return 0;
        }
        return psaRoot.getProtocolDeclarations().getEntryList().size();
    }

    /**
    Get available protocol declaration contents into String array
    
    @param protocol Caller allocated two-dimentional String array
   **/
    public void getSpdProtocolDeclarations(String[][] protocol) {
        if (psaRoot.getProtocolDeclarations() == null) {
            return;
        }
        
        List<ProtocolDeclarationsDocument.ProtocolDeclarations.Entry> l = psaRoot.getProtocolDeclarations()
                                                                                 .getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry e = (ProtocolDeclarationsDocument.ProtocolDeclarations.Entry) li
                                                                                                                                    .next();
            protocol[i][0] = e.getName();
            protocol[i][1] = e.getCName();
            if (e.getGuidValue() != null) {
                protocol[i][2] = e.getGuidValue();
            }
            protocol[i][3] = e.getHelpText();
            protocol[i][4] = listToString(e.getSupArchList());
            protocol[i][5] = listToString(e.getSupModuleList());
            protocol[i][6] = listToString(e.getGuidTypeList());
            i++;
        }
    }

    /**
    Get the number of Ppi declarations from the size of List
    
    @return int
   **/
    public int getSpdPpiDeclarationCount() {
        if (psaRoot.getPpiDeclarations() == null || psaRoot.getPpiDeclarations().getEntryList() == null) {
            return 0;
        }
        return psaRoot.getPpiDeclarations().getEntryList().size();
    }

    /**
    Get available Ppi declaration contents into String array
    
    @param ppi Caller allocated two-dimentional String array
   **/
    public void getSpdPpiDeclarations(String[][] ppi) {
        if (psaRoot.getPpiDeclarations() == null) {
            return;
        }
        
        List<PpiDeclarationsDocument.PpiDeclarations.Entry> l = psaRoot.getPpiDeclarations().getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PpiDeclarationsDocument.PpiDeclarations.Entry e = (PpiDeclarationsDocument.PpiDeclarations.Entry) li.next();
            ppi[i][0] = e.getName();
            ppi[i][1] = e.getCName();
            if (e.getGuidValue() != null) {
                ppi[i][2] = e.getGuidValue();
            }
            ppi[i][3] = e.getHelpText();
            ppi[i][4] = listToString(e.getSupArchList());
            ppi[i][5] = listToString(e.getSupModuleList());
            ppi[i][6] = listToString(e.getGuidTypeList());
            i++;
        }
    }

    /**
    Get the number of Pcd definitions from the size of List
    
    @return int
   **/
    public int getSpdPcdDefinitionCount() {
        if (psaRoot.getPcdDeclarations() == null || psaRoot.getPcdDeclarations().getPcdEntryList() == null) {
            return 0;
        }
        return psaRoot.getPcdDeclarations().getPcdEntryList().size();
    }

    /**
    Get available Pcd definition contents into String array
    
    @param pcd Caller allocated two-dimentional String array
   **/
    public void getSpdPcdDefinitions(String[][] pcd, String pcdUsage[][]) {
        if (psaRoot.getPcdDeclarations() == null) {
            return;
        }
        
        List<PcdDeclarationsDocument.PcdDeclarations.PcdEntry> l = psaRoot.getPcdDeclarations().getPcdEntryList();
        int i = 0;
        while (i < l.size()) {
            PcdDeclarationsDocument.PcdDeclarations.PcdEntry e = (PcdDeclarationsDocument.PcdDeclarations.PcdEntry)l.get(i);
            pcd[i][0] = e.getCName();
            pcd[i][1] = e.getToken() + "";           
            pcd[i][2] = e.getTokenSpaceGuidCName();
            if (e.getDatumType() != null) {
                pcd[i][3] = e.getDatumType().toString();
            }
            pcd[i][4] = e.getDefaultValue()+"";
            pcd[i][5] = e.getHelpText();
            String archList = listToString(e.getSupArchList());
            if (archList != null) {
                pcd[i][6] = archList;
            }
            String modTypeList = listToString(e.getSupModuleList());
            if (modTypeList != null) {
                pcd[i][7] = modTypeList;
            }
            
            int j = 0;
            while (j < e.getValidUsage().size() && j < 5) {
                pcdUsage[i][j] = (String)e.getValidUsage().get(j);
                ++j;
            }
            i++;
        }
    }

    /**
     Save the processed xml contents to file
     
     @param spdFile The file to save xml contents
     @throws IOException Exceptions during file operation
    **/
    public void saveAs(File spdFile) throws IOException {

        XmlOptions options = new XmlOptions();

        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);
        try {
            psad.save(spdFile, options);
        } catch (IOException e) {
            e.printStackTrace();
        }

    }

    /**
     Generate SpdHeader contents using parameters passed in.
     
     @param pkgName PackageName 
     @param pkgGuid Guid
     @param pkgVer Version
     @param pkgAbs Abstract
     @param pkgDes Description
     @param pkgCpRight Copyright
     @param pkgLicense License
     @param pkgCreateDate Created
     @param pkgUpdateDate Updated
     @param pkgURL URL
     @param pkgType PackageType
     @param pkgRdOnly ReadOnly
     @param pkgRePkg RePackage
     @param pkgSpec Reserved
     @param pkgOutDir Reserved
    **/
    public void genSpdHeader(String pkgName, String pkgGuid, String pkgVer, String pkgAbs, String pkgDes,
                             String pkgCpRight, String pkgLicense, String pkgCreateDate, String pkgUpdateDate,
                             String pkgURL, String pkgType, String pkgRdOnly, String pkgRePkg, String pkgSpec,
                             String pkgOutDir) {
        if (getSpdHdr() == null) {
            spdHdr = psaRoot.addNewSpdHeader();
        }

        setSpdHdrPkgName(pkgName);
        setSpdHdrGuidValue(pkgGuid);
        setSpdHdrVer(pkgVer);
        setSpdHdrAbs(pkgAbs);
        setSpdHdrDescription(pkgDes);
        setSpdHdrCopyright(pkgCpRight);
        setSpdHdrLicense(pkgLicense);
      

        setSpdHdrSpec(pkgSpec);
    }

    /**
     Generate library class declaration element using parameters passed in
     
     @param libClassBaseName LibraryClass element value
     @param libClassUsage Reserved
     @param incHdrFileName IncludeHeader element value
     @param incHdrAttribGuid Reserved
     @param incHdrAttribArch Reserved
     @param incHdrAttribPath Reserved
     @param incHdrAttribClass Reserved
     @param incHdrAttribVer Reserved
     @param incHdrAttribOverrideID Reserved
     @param incHdrAttribModuleType Reserved
    **/
    public void genSpdLibClassDeclarations(String libClassBaseName, String instanceUsage, String incHdrFileName,
                                           String help, String incHdrAttribArch, String incHdrAttribPath,
                                           String incHdrAttribClass, String incHdrAttribVer,
                                           String incHdrAttribOverrideID, String incHdrAttribModuleType) {
        if (getSpdLibClassDeclarations() == null) {
            spdLibClassDeclarations = psaRoot.addNewLibraryClassDeclarations();
        }
        //
        // add contents under LibraryClassDeclarations tag 
        //
        setSpdLibClassDeclaration(libClassBaseName, instanceUsage, incHdrFileName, help, incHdrAttribArch,
                                  incHdrAttribPath, incHdrAttribClass, incHdrAttribVer, incHdrAttribOverrideID,
                                  incHdrAttribModuleType, spdLibClassDeclarations);
    }
    
    public void getSpdLibClassDeclaration(String[] sa, int i) {
        if (getSpdLibClassDeclarations() == null) {
            return;
        }
        XmlCursor cursor = getSpdLibClassDeclarations().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass lc = (LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass)cursor.getObject();
            sa[0] = lc.getName();
            sa[1] = lc.getIncludeHeader();
            sa[2] = lc.getHelpText();
            sa[3] = lc.getRecommendedInstanceGuid();
            sa[4] = lc.getRecommendedInstanceVersion();
            sa[5] = listToString(lc.getSupArchList());
            sa[6] = listToString(lc.getSupModuleList());
        }
        cursor.dispose();
    }

    /**
     Set library class declaration contents under parent tag
     
     @param clsName LibraryClass element value
     @param clsUsage Reserved
     @param hdrFile IncludeHeader element value
     @param hdrAttribGuid Reserved
     @param hdrAttribArch Reserved
     @param hdrAttribPath Reserved
     @param hdrAttribClass Reserved
     @param hdrAttribVer Reserved
     @param hdrAttribOverID Reserved
     @param hdrAttribModType Reserved
     @param parent The tag under which library class declaration goes to
    **/
    public void setSpdLibClassDeclaration(String clsName, String clsUsage, String hdrFile, String help,
                                          String hdrAttribArch, String hdrAttribPath, String hdrAttribClass,
                                          String instanceVer, String hdrAttribOverID, String hdrAttribModType,
                                          XmlObject parent) {

        setSpdLibraryClass(clsName, hdrFile, help, clsUsage, instanceVer, hdrAttribArch, hdrAttribModType, parent);

    }

    /**
     Set the contents of LibraryClass under parent element
     
     @param clsName LibraryClass element value 
     @param clsUsage Reserved
     @param parent The tag under which library class declaration goes to
    **/
    public void setSpdLibraryClass(String clsName, String clsIncludeFile, String help, String clsUsage, String instanceVer, String hdrAttribArch, String hdrAttribModType, XmlObject parent) {
        LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass lc = ((LibraryClassDeclarationsDocument.LibraryClassDeclarations) parent).addNewLibraryClass();
        lc.setName(clsName);
        lc.setIncludeHeader(clsIncludeFile);
        lc.setHelpText(help);
// LAH added logic so you cannot set the version unless the GUID is defined.

        if (clsUsage != null) {
          lc.setRecommendedInstanceGuid(clsUsage);
          if (instanceVer != null) {
            lc.setRecommendedInstanceVersion(instanceVer);
          }
        }
        else {
          if (lc.isSetRecommendedInstanceGuid()) {
              lc.unsetRecommendedInstanceGuid();
          }
          if (lc.isSetRecommendedInstanceVersion()) {
              lc.unsetRecommendedInstanceVersion();
          }
        }

        if (hdrAttribArch != null) {
            lc.setSupArchList(stringToList(hdrAttribArch));
        } else {
          if (lc.isSetSupArchList()) {
            lc.unsetSupArchList();
          }
        }

        if (hdrAttribModType != null) {
          lc.setSupModuleList(stringToList(hdrAttribModType));
        } else {
          if (lc.isSetSupModuleList()) {
            lc.unsetSupModuleList();
          }
        }
        
    }

    /**
     Set contents of IncludeHeader under parent element
     
     @param modType Reserved
     @param hdrFile IncludeHeader element value
     @param hdrAttribGuid Reserved
     @param hdrAttribArch Reserved
     @param hdrAttribPath Reserved
     @param hdrAttribClass Reserved
     @param hdrAttribVer Reserved
     @param hdrAttribOverID Reserved
     @param parent The tag under which library class declaration goes to
    **/
    public void setSpdIncludeHeader(String modType, String hdrFile, String hdrAttribGuid, String hdrAttribArch,
                                    String hdrAttribPath, String hdrAttribClass, String hdrAttribVer,
                                    String hdrAttribOverID, XmlObject parent) {
        
        if (parent instanceof LibraryClassDeclarationsDocument.LibraryClassDeclarations) {
        } else if (parent instanceof PackageHeadersDocument.PackageHeaders) {
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader ih = null;
            ih = ((PackageHeadersDocument.PackageHeaders) parent).addNewIncludePkgHeader();
            ih.setStringValue(hdrFile);
            ih.setModuleType(ModuleTypeDef.Enum.forString(modType));
        } else {
            return;
        }

        if (hdrAttribGuid != null) {
        }
        if (hdrAttribPath != null) {
        }
        if (hdrAttribClass != null) {
        }
        if (hdrAttribVer != null) {
        }
        if (hdrAttribOverID != null) {
        }

    }

    /**
     Generate MsaFile element.
     
     @param msaFileName MsaFile element value
     @param archType Reserved
    **/
    public void genSpdMsaFiles(String msaFileName, String moduleName, String ver, String guid) {
        if (getSpdMsaFiles() == null) {
            spdMsaFiles = psaRoot.addNewMsaFiles();
        }
        setSpdMsaFile(msaFileName, moduleName, ver, guid, spdMsaFiles);
        
    }
    
    public void getSpdMsaFile (String[] sa, int i) {
        if (getSpdMsaFiles() == null) {
            return;
        }
        XmlCursor cursor = getSpdMsaFiles().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            sa[0] = cursor.getTextValue();
        }
        cursor.dispose();
    }

    /**
     Set MsaFile contents under parent element.
     
     @param msaFileName MsaFile element value
     @param parent Element under which MsaFile goes to
    **/
    public void setSpdMsaFile(String msaFileName, String moduleName, String ver, String guid, XmlObject parent) {
        MsaFilesDocument.MsaFiles f = (MsaFilesDocument.MsaFiles)parent;
        f.addNewFilename().setStringValue(msaFileName);
//        f.setFilename(msaFileName);
//        f.setModuleName(moduleName);
//        f.setModuleVersion(ver);
//        f.setModuleGuid(guid);
    }

    /**
     Generate PackageHeader element using parameters passed in.
     
     @param ModHdrModType ModuleType attribute of IncludeHeader element
     @param hdrFile IncludeHeader element value
     @param hdrAttribGuid Reserved
     @param hdrAttribArch Reserved
     @param hdrAttribPath Reserved
     @param hdrAttribClass Reserved
     @param hdrAttribVer Reserved
     @param hdrAttribOverID Reserved
    **/
    public void genSpdModuleHeaders(String ModHdrModType, String hdrFile, String hdrAttribGuid, String hdrAttribArch,
                                    String hdrAttribPath, String hdrAttribClass, String hdrAttribVer,
                                    String hdrAttribOverID) {
        if (getSpdModHdrs() == null) {
            spdModHdrs = psaRoot.addNewPackageHeaders();
        }

        //
        // add IncludeHeader under PackageHeaders element
        //
        setSpdIncludeHeader(ModHdrModType, hdrFile, hdrAttribGuid, hdrAttribArch, hdrAttribPath, hdrAttribClass,
                            hdrAttribVer, hdrAttribOverID, spdModHdrs);
    }
    
    public void getSpdModuleHeader(String[] sa, int i) {
        if (getSpdModHdrs() == null) {
            return;
        }
        XmlCursor cursor = getSpdModHdrs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader ih = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader)cursor.getObject();
            sa[0] = ih.getModuleType()+"";
            sa[1] = ih.getStringValue();
        }
        cursor.dispose();
    }

    /**
     Generate GUID declaration element using parameters passed in.
     
     @param guidDeclEntryName Name attribute of Entry element
     @param guidDeclCName CName element value
     @param guidDeclGuid Guid element value
     @param guidDeclFeatureFlag Reserved
    **/
    public void genSpdGuidDeclarations(String guidDeclEntryName, String guidDeclCName, String guidDeclGuid,
                                       String guidDeclHelp, Vector<String> archList, Vector<String> modTypeList,
                                       Vector<String> guidTypeList) {
        if (getSpdGuidDeclarations() == null) {
            spdGuidDeclarations = psaRoot.addNewGuidDeclarations();
        }

        setSpdEntry(guidDeclEntryName, guidDeclCName, guidDeclGuid, guidDeclHelp, archList, modTypeList, guidTypeList, spdGuidDeclarations);
    }

    /**
    Generate protocol declaration element using parameters passed in.
    
    @param protocolDeclEntryName Name attribute of Entry element
    @param protocolDeclCName CName element value
    @param protocolDeclGuid Guid element value
    @param protocolDeclFeatureFlag Reserved
   **/
    public void genSpdProtocolDeclarations(String protocolDeclEntryName, String protocolDeclCName,
                                           String protocolDeclGuid, String protocolDeclFeatureFlag,
                                           Vector<String> archList, Vector<String> modTypeList, Vector<String> guidTypeList) {
        if (getSpdProtocolDeclarations() == null) {
            spdProtocolDeclarations = psaRoot.addNewProtocolDeclarations();
        }

        setSpdEntry(protocolDeclEntryName, protocolDeclCName, protocolDeclGuid, protocolDeclFeatureFlag,
                    archList, modTypeList, guidTypeList, spdProtocolDeclarations);
    }

    /**
    Generate PPI declaration element using parameters passed in.
    
    @param ppiDeclEntryName Name attribute of Entry element
    @param ppiDeclCName CName element value
    @param ppiDeclGuid Guid element value
    @param ppiDeclFeatureFlag Reserved
   **/
    public void genSpdPpiDeclarations(String ppiDeclEntryName, String ppiDeclCName, String ppiDeclGuid,
                                      String ppiDeclFeatureFlag, Vector<String> archList, Vector<String> modTypeList, Vector<String> guidTypeList) {
        if (getSpdPpiDeclarations() == null) {
            spdPpiDeclarations = psaRoot.addNewPpiDeclarations();
        }

        setSpdEntry(ppiDeclEntryName, ppiDeclCName, ppiDeclGuid, ppiDeclFeatureFlag, archList, modTypeList, guidTypeList, spdPpiDeclarations);
    }
    
    public void getSpdGuidDeclaration(String[] sa, int i) {
        if (getSpdGuidDeclarations() == null) {
            return;
        }
        getSpdEntry(sa, i, getSpdGuidDeclarations());
    }
    
    public void getSpdProtocolDeclaration(String[] sa, int i) {
        if (getSpdProtocolDeclarations() == null) {
            return;
        }
        getSpdEntry(sa, i, getSpdProtocolDeclarations());
    }
    
    public void getSpdPpiDeclaration(String[] sa, int i) {
        if (getSpdPpiDeclarations() == null) {
            return;
        }
        getSpdEntry(sa, i, getSpdPpiDeclarations());
    }

    public void getSpdEntry(String[] sa, int i, XmlObject parent) {
        XmlCursor cursor = parent.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            if (parent instanceof GuidDeclarationsDocument.GuidDeclarations) {
                GuidDeclarationsDocument.GuidDeclarations.Entry e = (GuidDeclarationsDocument.GuidDeclarations.Entry)cursor.getObject();
                sa[0] = e.getName();
                sa[1] = e.getCName();
                sa[2] = e.getGuidValue();
                sa[3] = e.getHelpText();
                sa[4] = listToString(e.getSupArchList());
                sa[5] = listToString(e.getSupModuleList());
                sa[6] = listToString(e.getGuidTypeList());
            }
            
            if (parent instanceof ProtocolDeclarationsDocument.ProtocolDeclarations) {
                ProtocolDeclarationsDocument.ProtocolDeclarations.Entry e = (ProtocolDeclarationsDocument.ProtocolDeclarations.Entry)cursor.getObject();
                sa[0] = e.getName();
                sa[1] = e.getCName();
                sa[2] = e.getGuidValue();
                sa[3] = e.getHelpText();
                sa[4] = listToString(e.getSupArchList());
                sa[5] = listToString(e.getSupModuleList());
                sa[6] = listToString(e.getGuidTypeList());
            }
            
            if (parent instanceof PpiDeclarationsDocument.PpiDeclarations) {
                PpiDeclarationsDocument.PpiDeclarations.Entry e = (PpiDeclarationsDocument.PpiDeclarations.Entry)cursor.getObject();
                sa[0] = e.getName();
                sa[1] = e.getCName();
                sa[2] = e.getGuidValue();
                sa[3] = e.getHelpText();
                sa[4] = listToString(e.getSupArchList());
                sa[5] = listToString(e.getSupModuleList());
                sa[6] = listToString(e.getGuidTypeList());
            }
            
        }
        cursor.dispose();
    }
    /**
     Set Entry contents using parameters passed in
     
     @param entryName Name attribute of Entry element
     @param cName CName element value
     @param guid Guid element value
     @param featureFlag Reserved
     @param parent The tag under which Entry element goes to
    **/
    public void setSpdEntry(String entryName, String cName, String guid, String help, 
                            Vector<String> archList, Vector<String> modTypeList,
                            Vector<String> guidTypeList, XmlObject parent) {

        if (parent instanceof GuidDeclarationsDocument.GuidDeclarations) {
            GuidDeclarationsDocument.GuidDeclarations.Entry e = ((GuidDeclarations) parent).addNewEntry();
            e.setName(entryName);
            e.setCName(cName);
            e.setGuidValue(guid);
            e.setHelpText(help);
            if (guidTypeList != null) {
                e.setGuidTypeList(guidTypeList);
            }
            else{
                if (e.isSetGuidTypeList()) {
                    e.unsetGuidTypeList();
                }
            }
            if (archList != null) {
                e.setSupArchList(archList);
            }
            else {
                if (e.isSetSupArchList()) {
                    e.unsetSupArchList();
                }
            }
            if (modTypeList != null){
                e.setSupModuleList(modTypeList);
            }
            else {
                if (e.isSetSupModuleList()) {
                    e.unsetSupModuleList();
                }
            }

            return;
        }
        if (parent instanceof ProtocolDeclarationsDocument.ProtocolDeclarations) {
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry pe = ((ProtocolDeclarationsDocument.ProtocolDeclarations) parent)
                                                                                                                                     .addNewEntry();
            pe.setName(entryName);
            pe.setCName(cName);
            pe.setGuidValue(guid);
            pe.setHelpText(help);
            if (guidTypeList != null) {
                pe.setGuidTypeList(guidTypeList);
            }
            else{
                if (pe.isSetGuidTypeList()) {
                    pe.unsetGuidTypeList();
                }
            }
            if (archList != null) {
                pe.setSupArchList(archList);
            }
            else {
                if (pe.isSetSupArchList()) {
                    pe.unsetSupArchList();
                }
            }
            if (modTypeList != null){
                pe.setSupModuleList(modTypeList);
            }
            else {
                if (pe.isSetSupModuleList()) {
                    pe.unsetSupModuleList();
                }
            }

            return;
        }
        if (parent instanceof PpiDeclarationsDocument.PpiDeclarations) {
            PpiDeclarationsDocument.PpiDeclarations.Entry ppe = ((PpiDeclarationsDocument.PpiDeclarations) parent)
                                                                                                                  .addNewEntry();
            ppe.setName(entryName);
            ppe.setCName(cName);
            ppe.setGuidValue(guid);
            ppe.setHelpText(help);
            if (guidTypeList != null) {
                ppe.setGuidTypeList(guidTypeList);
            }
            else{
                if (ppe.isSetGuidTypeList()) {
                    ppe.unsetGuidTypeList();
                }
            }
            if (archList != null) {
                ppe.setSupArchList(archList);
            }
            else {
                if (ppe.isSetSupArchList()) {
                    ppe.unsetSupArchList();
                }
            }
            if (modTypeList != null){
                ppe.setSupModuleList(modTypeList);
            }
            else {
                if (ppe.isSetSupModuleList()) {
                    ppe.unsetSupModuleList();
                }
            }

            return;
        }

        return;

    }

    /**
     Generate Pcd definition using parameters passed in
     
     @param pcdItemTypes ItemType attribute of PcdEntry element
     @param cName C_Name element value
     @param token Token element value
     @param dataType DatumType element value
     @param skuEnable Reserved
     @param sku Reserved
     @param maxSku Reserved
     @param hiiEnable Reserved
     @param varGuid Reserved
     @param varName Reserved
     @param defaultString DefaultString element value
    **/
    public void genSpdPcdDefinitions(String cName, String token, String dataType, String pcdItemTypes, 
                                     String spaceGuid, String defaultString, String help, String archList,
                                     String modTypeList) {
        if (getSpdPcdDefinitions() == null) {
            spdPcdDefinitions = psaRoot.addNewPcdDeclarations();
        }

        setSpdPcdEntry(pcdItemTypes, cName, token, dataType, spaceGuid, help,
                       defaultString, archList, modTypeList, spdPcdDefinitions);
    }
    
    public void getSpdPcdDeclaration(String[] sa, int i) {
        if (getSpdPcdDefinitions() == null) {
            return;
        }
        
        XmlCursor cursor = getSpdPcdDefinitions().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            PcdDeclarationsDocument.PcdDeclarations.PcdEntry pe = (PcdDeclarationsDocument.PcdDeclarations.PcdEntry)cursor.getObject();
            sa[0] = pe.getCName();
            sa[1] = pe.getToken()+"";
            sa[2] = pe.getTokenSpaceGuidCName();
            sa[3] = pe.getDatumType()+"";
            sa[4] = pe.getDefaultValue();
            sa[5] = pe.getHelpText();
            sa[6] = listToString(pe.getValidUsage());
            sa[7] = listToString(pe.getSupArchList());
            sa[8] = listToString(pe.getSupModuleList());
        }
        cursor.dispose();
    }

    /**
     Set Pcd entry contents under parent tag
     
     @param pcdItemTypes ItemType attribute of PcdEntry element
     @param cName C_Name element value
     @param token Token element value
     @param dataType DatumType element value
     @param spaceGuid Reserved
     @param help Reserved
     @param defaultString DefaultString element value
     @param parent Tag under which PcdEntry goes to
    **/
    public void setSpdPcdEntry(String pcdItemTypes, String cName, String token, String dataType, String spaceGuid, String help,
                               String defaultString, String archList, String modTypeList, XmlObject parent) {

        PcdDeclarationsDocument.PcdDeclarations.PcdEntry pe = ((PcdDeclarationsDocument.PcdDeclarations) parent).addNewPcdEntry();
        
        //ToDo: maybe multiple types in, need parse pcdItemTypes.
        String usage[] = pcdItemTypes.split("( )+");
        List<String> l = new ArrayList<String>();
        for (int i = 0; i < usage.length; ++i) {
            l.add(usage[i]);  
        }
        pe.setValidUsage(l);
        pe.setCName(cName);
        pe.setToken(token);
        pe.setDatumType(PcdDataTypes.Enum.forString(dataType));
        pe.setDefaultValue(defaultString);
        pe.setTokenSpaceGuidCName(spaceGuid);
        pe.setHelpText(help);
        if (archList != null){
          pe.setSupArchList(stringToList(archList));
        } else {
          if (pe.isSetSupArchList()) {
            pe.unsetSupArchList();
          }
        }
        if (modTypeList != null){
          pe.setSupModuleList(stringToList(modTypeList));
        } else {
          if (pe.isSetSupModuleList()) {
            pe.unsetSupModuleList();
          }
        }
    }

    /**
     Get PpiDeclarations element
     
     @return PpiDeclarationsDocument.PpiDeclarations
    **/
    public PpiDeclarationsDocument.PpiDeclarations getSpdPpiDeclarations() {
        if (spdPpiDeclarations == null) {
            spdPpiDeclarations = psaRoot.getPpiDeclarations();
        }
        return spdPpiDeclarations;
    }

    /**
    Get ProtocolDeclarations element
    
    @return ProtocolDeclarationsDocument.ProtocolDeclarations
    **/
    public ProtocolDeclarationsDocument.ProtocolDeclarations getSpdProtocolDeclarations() {
        if (spdProtocolDeclarations == null) {
            spdProtocolDeclarations = psaRoot.getProtocolDeclarations();
        }
        return spdProtocolDeclarations;
    }

    /**
    Get GuidDeclarations element
    
    @return GuidDeclarationsDocument.GuidDeclarations
    **/
    public GuidDeclarationsDocument.GuidDeclarations getSpdGuidDeclarations() {
        if (spdGuidDeclarations == null) {
            spdGuidDeclarations = psaRoot.getGuidDeclarations();
        }
        return spdGuidDeclarations;
    }

    /**
     Get PcdDefinitions element
     
     @return PcdDefinitionsDocument.PcdDefinitions
    **/
    public PcdDeclarationsDocument.PcdDeclarations getSpdPcdDefinitions() {
        if (spdPcdDefinitions == null) {
            spdPcdDefinitions = psaRoot.getPcdDeclarations();
        }
        return spdPcdDefinitions;
    }

    /**
     Get PackageHeaders element
     
     @return PackageHeadersDocument.PackageHeaders
    **/
    public PackageHeadersDocument.PackageHeaders getSpdModHdrs() {
        if (spdModHdrs == null) {
            spdModHdrs = psaRoot.getPackageHeaders();
        }
        return spdModHdrs;
    }

    /**
     Get MsaFiles element
     
     @return MsaFilesDocument.MsaFiles
    **/
    public MsaFilesDocument.MsaFiles getSpdMsaFiles() {
        if (spdMsaFiles == null) {
            spdMsaFiles = psaRoot.getMsaFiles();
        }
        return spdMsaFiles;
    }

    /**
     Get LibraryClassDeclarations element
     
     @return LibraryClassDeclarationsDocument.LibraryClassDeclarations
    **/
    public LibraryClassDeclarationsDocument.LibraryClassDeclarations getSpdLibClassDeclarations() {
        if (spdLibClassDeclarations == null) {
            spdLibClassDeclarations = psaRoot.getLibraryClassDeclarations();
        }
        return spdLibClassDeclarations;
    }

    public PackageDefinitionsDocument.PackageDefinitions getSpdPkgDefs() {
        if (spdPkgDefs == null) {
            spdPkgDefs = psaRoot.addNewPackageDefinitions();
        }
        return spdPkgDefs;
    }
    /**
     Get SpdHeader element
     
     @return SpdHeaderDocument.SpdHeader
    **/
    public SpdHeaderDocument.SpdHeader getSpdHdr() {
        if (spdHdr == null) {
            spdHdr = psaRoot.addNewSpdHeader();
        }
        return spdHdr;
    }

    /**
     Set value to Guid element
     
     @param guid The value set to Guid element
    **/
    public void setSpdHdrGuidValue(String guid) {
        getSpdHdr().setGuidValue(guid);
    }

    /**
    Get Version element under SpdHdr
    
    @return String
   **/
    public String getSpdHdrVer() {
        return getSpdHdr().getVersion();
    }

    /**
    Set value to Version element
    
    @param ver The value set to Version element
   **/
    public void setSpdHdrVer(String ver) {
        getSpdHdr().setVersion(ver);
    }

    public String getSpdHdrAbs(){
        return getSpdHdr().getAbstract();
        
    }
    
    /**
     Set value to Abstract element
     
     @param abs The value set to Abstract element
     **/
    public void setSpdHdrAbs(String abs) {

        getSpdHdr().setAbstract(abs);
    }
    
    public String getSpdHdrDescription(){
       return getSpdHdr().getDescription(); 
    }
    /**
    Set value to Description element
    
    @param des The value set to Description element
    **/
    public void setSpdHdrDescription(String des) {
       getSpdHdr().setDescription(des);
    }
    
    public String getSpdHdrCopyright(){
        return getSpdHdr().getCopyright();
    }
    /**
     Set value to Copyright element
     
     @param cpRit The value set to Copyright element
     **/
    public void setSpdHdrCopyright(String cpRit) {

        getSpdHdr().setCopyright(cpRit);

    }
    /**
     Get License element under SpdHdr
     
     @return String
     **/
    public String getSpdHdrLicense() {
        if (getSpdHdr().getLicense() != null) {
            return getSpdHdr().getLicense().getStringValue();
        }
        return null;
    }

    /**
    Set value to License element
    
    @param license The value set to License element
   **/
    public void setSpdHdrLicense(String license) {
        if (getSpdHdr().getLicense() == null){
            getSpdHdr().addNewLicense().setStringValue(license);
        }
        else {
            getSpdHdr().getLicense().setStringValue(license);
        }
    }
    
    public String getSpdHdrUrl(){
        if (getSpdHdr().getLicense() != null) {
            return getSpdHdr().getLicense().getURL();
        }
        return null;
    }
    
    public void setSpdHdrUrl(String url){
        getSpdHdr().getLicense().setURL(url);
    }

    /**
    Get PackageName element under SpdHdr
    
    @return String
   **/
    public String getSpdHdrPkgName() {
        
        return getSpdHdr().getPackageName();
    }

    /**
    Set value to PackageName element
    
    @param pkgName The value set to PackageName element
   **/
    public void setSpdHdrPkgName(String pkgName) {
        getSpdHdr().setPackageName(pkgName);
    }

    public String getSpdHdrGuidValue(){
        return getSpdHdr().getGuidValue();
    }
    
    /**
    Reserved method
    
    @return SpecificationDocument.Specification
   **/
    public String getSpdHdrSpec() {
        return "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";
//        return getSpdHdr().getSpecification();
    }

    /**
    Reserved method
    
    @param spec 
   **/
    public void setSpdHdrSpec(String spec) {
        spec = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";
        getSpdHdr().setSpecification(spec);
        
    }

   public String getSpdPkgDefsRdOnly(){
        return getSpdPkgDefs().getReadOnly() + "";
    }
    /**
    Set value to ReadOnly element
    
    @param rdOnly The value set to ReadOnly element
   **/
    public void setSpdPkgDefsRdOnly(String rdOnly) {

        getSpdPkgDefs().setReadOnly(new Boolean(rdOnly));
    }

    public String getSpdPkgDefsRePkg(){
        return getSpdPkgDefs().getRePackage() + "";
    }
    /**
    Set value to RePackage element
    
    @param rePkg The value set to RePackage element
   **/
    public void setSpdPkgDefsRePkg(String rePkg) {

        getSpdPkgDefs().setRePackage(new Boolean(rePkg));
    }

    /**
    Set value to URL element
    
    @param url The value set to URL element
   **/
//    public void setSpdHdrURL(String url) {
//        getSpdHdr().setURL(url);
//    }

    /**
     Get xml file
     
     @return File
    **/
    public File getFile() {
        return file;
    }

    /**
     Set file
     
     @param file File with xml format
    **/
    public void setFile(File file) {
        this.file = file;
    }
    
    private List<String> stringToList(String s){
        if (s == null || s.length() == 0) {
            return null;
        }
        Vector<String> al = new Vector<String>();
        String[] sArray = s.split(" ");
        for(int i = 0; i < sArray.length; ++i){
            al.add(sArray[i]);
        }
        return al; 
    }
    
    private String listToString(List l) {
        if (l == null) {
            return null;
        }
        String s = " ";
        ListIterator li = l.listIterator();
        while(li.hasNext()) {
            s += li.next();
            s += " ";
        }
        return s.trim();
    }

}
