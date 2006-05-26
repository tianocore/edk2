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
package org.tianocore.packaging;

import java.io.File;
import java.io.IOException;
import java.util.List;
import java.util.ListIterator;

import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.apache.xmlbeans.XmlCursor;

import org.tianocore.AbstractDocument;
import org.tianocore.GuidDeclarationsDocument;
import org.tianocore.GuidDocument;
import org.tianocore.IncludeHeaderDocument;
import org.tianocore.LibraryClassDeclarationDocument;
import org.tianocore.LibraryClassDeclarationsDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.LicenseDocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaFilesDocument;
import org.tianocore.OutputDirectoryDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.PackageHeadersDocument;
import org.tianocore.PackageNameDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PackageType;
import org.tianocore.PackageUsage;
import org.tianocore.PcdDataTypes;
import org.tianocore.PcdDefinitionsDocument;
import org.tianocore.PcdItemTypes;
import org.tianocore.PpiDeclarationsDocument;
import org.tianocore.ProtocolDeclarationsDocument;
import org.tianocore.SpdHeaderDocument;
import org.tianocore.SpecificationDocument;
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

    private PackageNameDocument.PackageName spdHdrPkgName = null;

    private GuidDocument.Guid spdHdrGuid = null;

    private AbstractDocument.Abstract spdHdrAbs = null;

    private LicenseDocument.License spdHdrLicense = null;

    private SpecificationDocument.Specification spdHdrSpec = null;

    private OutputDirectoryDocument.OutputDirectory spdHdrOutDir = null;

    private LibraryClassDeclarationsDocument.LibraryClassDeclarations spdLibClassDeclarations = null;

    private PackageDependenciesDocument.PackageDependencies spdPkgDeps = null;

    private MsaFilesDocument.MsaFiles spdMsaFiles = null;

    private PackageHeadersDocument.PackageHeaders spdModHdrs = null;

    private GuidDeclarationsDocument.GuidDeclarations spdGuidDeclarations = null;

    private ProtocolDeclarationsDocument.ProtocolDeclarations spdProtocolDeclarations = null;

    private PpiDeclarationsDocument.PpiDeclarations spdPpiDeclarations = null;

    private PcdDefinitionsDocument.PcdDefinitions spdPcdDefinitions = null;

    /**
     Constructor to create a new spd file
    **/
    public SpdFileContents() {

        psad = PackageSurfaceAreaDocument.Factory.newInstance();
        psaRoot = psad.addNewPackageSurfaceArea();

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
        XmlObject o = psaRoot.getPcdDefinitions();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        spdPcdDefinitions = null;
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
    }

    /**
     Get spd file header contents into String array
     
     @param s Caller allocated String array
    **/
    public void getSpdHdrDetails(String[] s) {
        if (getSpdHdr() == null) {
            spdHdr = psaRoot.addNewSpdHeader();
        }
        s[0] = getSpdHdrPkgName().getStringValue();
        s[1] = getSpdHdrGuid().getStringValue();
        s[2] = getSpdHdrVer();
        s[3] = getSpdHdrAbs().getStringValue();
        s[4] = getSpdHdr().getDescription();
        s[5] = getSpdHdr().getCopyright();
        s[6] = getSpdHdrLicense().getStringValue();
        s[7] = getSpdHdr().getCreated();
        s[8] = getSpdHdr().getURL();
        if (getSpdHdr().getPackageType() != null) {
            s[9] = getSpdHdr().getPackageType().toString();
        }
        //
        // convert boolean to String by adding empty String ""
        //
        s[10] = getSpdHdr().getReadOnly() + "";
        s[11] = getSpdHdr().getReadOnly() + "";
    }

    /**
     Get the number of library class declarations from the size of List
     
     @return int
    **/
    public int getSpdLibClassDeclarationCount() {
        if (psaRoot.getLibraryClassDeclarations() == null
            || psaRoot.getLibraryClassDeclarations().getLibraryClassDeclarationList() == null) {
            return 0;
        }
        return psaRoot.getLibraryClassDeclarations().getLibraryClassDeclarationList().size();
    }

    /**
     Get available library class declaration into String array
     @param libClass Caller allocated two-dimentional String array
    **/
    public void getSpdLibClassDeclarations(String[][] libClass) {
        List<LibraryClassDeclarationDocument.LibraryClassDeclaration> l = psaRoot.getLibraryClassDeclarations()
                                                                                 .getLibraryClassDeclarationList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            LibraryClassDeclarationDocument.LibraryClassDeclaration lcd = (LibraryClassDeclarationDocument.LibraryClassDeclaration) li
                                                                                                                                      .next();
            if (lcd.getLibraryClass() != null) {
                libClass[i][0] = lcd.getLibraryClass().getStringValue();
            }
            if (lcd.getIncludeHeader() != null) {
                libClass[i][1] = lcd.getIncludeHeader().getStringValue();
            }

            i++;
        }

    }
    
    /**
    Get the number of Msa files from the size of List
    
    @return int
   **/
    public int getSpdMsaFileCount() {
        if (psaRoot.getMsaFiles() == null || psaRoot.getMsaFiles().getMsaFileList() == null) {
            return 0;
        }
        return psaRoot.getMsaFiles().getMsaFileList().size();
    }
    
    /**
    Get available Msa file into String array
    
    @param msaFile Caller allocated two-dimentional String array
   **/
    public void getSpdMsaFiles(String[][] msaFile) {
        List<MsaFilesDocument.MsaFiles.MsaFile> l = psaRoot.getMsaFiles().getMsaFileList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            MsaFilesDocument.MsaFiles.MsaFile m = (MsaFilesDocument.MsaFiles.MsaFile) li.next();
            if (m.getFilename() != null) {
                msaFile[i][0] = m.getFilename().getStringValue();
            }

            i++;
        }
    }

    /**
    Get the number of include header files in PackageHeaders from the size of List
    
    @return int
   **/
    public int getSpdPackageHeaderCount() {
        if (psaRoot.getPackageHeaders() == null || psaRoot.getPackageHeaders().getIncludeHeaderList() == null) {
            return 0;
        }
        return psaRoot.getPackageHeaders().getIncludeHeaderList().size();
    }

    /**
    Get available package header contents into String array
    
    @param pkgHeader Caller allocated two-dimentional String array
   **/
    public void getSpdPackageHeaders(String[][] pkgHeader) {
        List<IncludeHeaderDocument.IncludeHeader> l = psaRoot.getPackageHeaders().getIncludeHeaderList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            IncludeHeaderDocument.IncludeHeader ih = (IncludeHeaderDocument.IncludeHeader) li.next();
            if (ih.getModuleType() != null) {
                pkgHeader[i][0] = ih.getModuleType().toString();
            }

            pkgHeader[i][1] = ih.getStringValue();
            i++;
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
        List<GuidDeclarationsDocument.GuidDeclarations.Entry> l = psaRoot.getGuidDeclarations().getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            GuidDeclarationsDocument.GuidDeclarations.Entry e = (GuidDeclarationsDocument.GuidDeclarations.Entry) li
                                                                                                                    .next();
            guid[i][0] = e.getName();
            guid[i][1] = e.getCName();
            if (e.getGuid() != null) {
                guid[i][2] = e.getGuid().getStringValue();
            }
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
        List<ProtocolDeclarationsDocument.ProtocolDeclarations.Entry> l = psaRoot.getProtocolDeclarations()
                                                                                 .getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry e = (ProtocolDeclarationsDocument.ProtocolDeclarations.Entry) li
                                                                                                                                    .next();
            protocol[i][0] = e.getName();
            protocol[i][1] = e.getCName();
            if (e.getGuid() != null) {
                protocol[i][2] = e.getGuid().getStringValue();
            }
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
        List<PpiDeclarationsDocument.PpiDeclarations.Entry> l = psaRoot.getPpiDeclarations().getEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PpiDeclarationsDocument.PpiDeclarations.Entry e = (PpiDeclarationsDocument.PpiDeclarations.Entry) li.next();
            ppi[i][0] = e.getName();
            ppi[i][1] = e.getCName();
            if (e.getGuid() != null) {
                ppi[i][2] = e.getGuid().getStringValue();
            }

            i++;
        }
    }

    /**
    Get the number of Pcd definitions from the size of List
    
    @return int
   **/
    public int getSpdPcdDefinitionCount() {
        if (psaRoot.getPcdDefinitions() == null || psaRoot.getPcdDefinitions().getPcdEntryList() == null) {
            return 0;
        }
        return psaRoot.getPcdDefinitions().getPcdEntryList().size();
    }

    /**
    Get available Pcd definition contents into String array
    
    @param pcd Caller allocated two-dimentional String array
   **/
    public void getSpdPcdDefinitions(String[][] pcd) {
        List<PcdDefinitionsDocument.PcdDefinitions.PcdEntry> l = psaRoot.getPcdDefinitions().getPcdEntryList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PcdDefinitionsDocument.PcdDefinitions.PcdEntry e = (PcdDefinitionsDocument.PcdDefinitions.PcdEntry) li
                                                                                                                  .next();
            if (e.getItemType() != null) {
                pcd[i][0] = e.getItemType().toString();
            }

            pcd[i][1] = e.getCName();
            pcd[i][2] = e.getToken();
            if (e.getDatumType() != null) {
                pcd[i][3] = e.getDatumType().toString();
            }

            if (e.getDefaultValue() != null) {
                pcd[i][4] = e.getDefaultValue().toString();
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
        setSpdHdrGuid(pkgGuid);
        setSpdHdrVer(pkgVer);
        setSpdHdrAbs(pkgAbs);
        setSpdHdrDes(pkgDes);
        setSpdHdrCpRit(pkgCpRight);
        setSpdHdrLicense(pkgLicense);
        setSpdHdrCreateDate(pkgCreateDate);
        setSpdHdrUpdateDate(pkgUpdateDate);
        setSpdHdrURL(pkgURL);
        setSpdHdrPkgType(pkgType);
        setSpdHdrRdOnly(pkgRdOnly);
        setSpdHdrRePkg(pkgRePkg);
        setSpdHdrSpec(pkgSpec);
        setSpdHdrOutDir(pkgOutDir);
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
    public void genSpdLibClassDeclarations(String libClassBaseName, String libClassUsage, String incHdrFileName,
                                           String incHdrAttribGuid, String incHdrAttribArch, String incHdrAttribPath,
                                           String incHdrAttribClass, String incHdrAttribVer,
                                           String incHdrAttribOverrideID, String incHdrAttribModuleType) {
        if (getSpdLibClassDeclarations() == null) {
            spdLibClassDeclarations = psaRoot.addNewLibraryClassDeclarations();
        }
        //
        // add contents under LibraryClassDeclarations tag 
        //
        setSpdLibClassDeclaration(libClassBaseName, libClassUsage, incHdrFileName, incHdrAttribGuid, incHdrAttribArch,
                                  incHdrAttribPath, incHdrAttribClass, incHdrAttribVer, incHdrAttribOverrideID,
                                  incHdrAttribModuleType, spdLibClassDeclarations);
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
    public void setSpdLibClassDeclaration(String clsName, String clsUsage, String hdrFile, String hdrAttribGuid,
                                          String hdrAttribArch, String hdrAttribPath, String hdrAttribClass,
                                          String hdrAttribVer, String hdrAttribOverID, String hdrAttribModType,
                                          XmlObject parent) {

        LibraryClassDeclarationDocument.LibraryClassDeclaration lcd = ((LibraryClassDeclarationsDocument.LibraryClassDeclarations) parent)
                                                                                                                                          .addNewLibraryClassDeclaration();

        setSpdLibraryClass(clsName, clsUsage, lcd);

        setSpdIncludeHeader(null, hdrFile, hdrAttribGuid, hdrAttribArch, hdrAttribPath, hdrAttribClass, hdrAttribVer,
                            hdrAttribOverID, lcd);
    }

    /**
     Set the contents of LibraryClass under parent element
     
     @param clsName LibraryClass element value 
     @param clsUsage Reserved
     @param parent The tag under which library class declaration goes to
    **/
    public void setSpdLibraryClass(String clsName, String clsUsage, XmlObject parent) {
        LibraryClassDocument.LibraryClass lc = ((LibraryClassDeclarationDocument.LibraryClassDeclaration) parent)
                                                                                                                 .addNewLibraryClass();
        lc.setStringValue(clsName);
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
        IncludeHeaderDocument.IncludeHeader ih = null;
        if (parent instanceof LibraryClassDeclarationDocument.LibraryClassDeclaration) {
            ih = ((LibraryClassDeclarationDocument.LibraryClassDeclaration) parent).addNewIncludeHeader();
        } else if (parent instanceof PackageHeadersDocument.PackageHeaders) {
            ih = ((PackageHeadersDocument.PackageHeaders) parent).addNewIncludeHeader();
        } else {
            return;
        }

        ih.setStringValue(hdrFile);
        if (hdrAttribGuid != null) {
            ih.setGuid(hdrAttribGuid);
        }
        if (hdrAttribPath != null) {
            ih.setPath(hdrAttribPath);
        }
        if (hdrAttribClass != null) {
            ih.setClass1(hdrAttribClass);
        }
        if (hdrAttribVer != null) {
            ih.setVersion(hdrAttribVer);
        }
        if (hdrAttribOverID != null) {
            ih.setOverrideID(Integer.parseInt(hdrAttribOverID));
        }
        if (modType != null) {
            ih.setModuleType(ModuleTypeDef.Enum.forString(modType));

        }

    }

    /**
     Reserved method
     
     @param pkgDepPkgName
     @param pkgDepPkgAttribGuid
     @param pkgDepPkgAttribVer
     @param pkgDepPkgAttribType
     @param pkgDepPkgAttribUsage
     @param pkgDepPkgAttribInstallDate
     @param pkgDepPkgAttribUpdateDate
     @param pkgDepPkgAttribPath
    **/
    public void genSpdPackageDependencies(String pkgDepPkgName, String pkgDepPkgAttribGuid, String pkgDepPkgAttribVer,
                                          String pkgDepPkgAttribType, String pkgDepPkgAttribUsage,
                                          String pkgDepPkgAttribInstallDate, String pkgDepPkgAttribUpdateDate,
                                          String pkgDepPkgAttribPath) {
        if (spdPkgDeps == null) {
            spdPkgDeps = psaRoot.addNewPackageDependencies();
        }

        setSpdPackageName(pkgDepPkgName, pkgDepPkgAttribGuid, pkgDepPkgAttribVer, pkgDepPkgAttribType,
                          pkgDepPkgAttribUsage, pkgDepPkgAttribInstallDate, pkgDepPkgAttribUpdateDate,
                          pkgDepPkgAttribPath, spdPkgDeps);
    }

    /**
     Reserved method
     
     @param pkgName
     @param pkgAttribGuid
     @param pkgAttribVer
     @param pkgAttribType
     @param pkgAttribUsage
     @param pkgAttribInstallDate
     @param pkgAttribUpdateDate
     @param pkgAttribPath
     @param parent
    **/
    public void setSpdPackageName(String pkgName, String pkgAttribGuid, String pkgAttribVer, String pkgAttribType,
                                  String pkgAttribUsage, String pkgAttribInstallDate, String pkgAttribUpdateDate,
                                  String pkgAttribPath, XmlObject parent) {

        PackageNameDocument.PackageName pn = ((PackageDependenciesDocument.PackageDependencies) parent)
                                                                                                       .addNewPackageName();
        pn.setStringValue(pkgName);
        pn.setPackageType(PackageType.Enum.forString(pkgAttribType));
        pn.setUsage(PackageUsage.Enum.forString(pkgAttribUsage));
        pn.setUpdatedDate(pkgAttribUpdateDate);
    }

    /**
     Generate MsaFile element.
     
     @param msaFileName MsaFile element value
     @param archType Reserved
    **/
    public void genSpdMsaFiles(String msaFileName, String archType) {
        if (getSpdMsaFiles() == null) {
            spdMsaFiles = psaRoot.addNewMsaFiles();
        }
        setSpdMsaFile(msaFileName, spdMsaFiles);
        
    }

    /**
     Set MsaFile contents under parent element.
     
     @param msaFileName MsaFile element value
     @param parent Element under which MsaFile goes to
    **/
    public void setSpdMsaFile(String msaFileName, XmlObject parent) {

        ((MsaFilesDocument.MsaFiles) parent).addNewMsaFile().addNewFilename().setStringValue(msaFileName);
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

    /**
     Generate GUID declaration element using parameters passed in.
     
     @param guidDeclEntryName Name attribute of Entry element
     @param guidDeclCName CName element value
     @param guidDeclGuid Guid element value
     @param guidDeclFeatureFlag Reserved
    **/
    public void genSpdGuidDeclarations(String guidDeclEntryName, String guidDeclCName, String guidDeclGuid,
                                       String guidDeclFeatureFlag) {
        if (getSpdGuidDeclarations() == null) {
            spdGuidDeclarations = psaRoot.addNewGuidDeclarations();
        }

        setSpdEntry(guidDeclEntryName, guidDeclCName, guidDeclGuid, guidDeclFeatureFlag, spdGuidDeclarations);
    }

    /**
    Generate protocol declaration element using parameters passed in.
    
    @param protocolDeclEntryName Name attribute of Entry element
    @param protocolDeclCName CName element value
    @param protocolDeclGuid Guid element value
    @param protocolDeclFeatureFlag Reserved
   **/
    public void genSpdProtocolDeclarations(String protocolDeclEntryName, String protocolDeclCName,
                                           String protocolDeclGuid, String protocolDeclFeatureFlag) {
        if (getSpdProtocolDeclarations() == null) {
            spdProtocolDeclarations = psaRoot.addNewProtocolDeclarations();
        }

        setSpdEntry(protocolDeclEntryName, protocolDeclCName, protocolDeclGuid, protocolDeclFeatureFlag,
                    spdProtocolDeclarations);
    }

    /**
    Generate PPI declaration element using parameters passed in.
    
    @param ppiDeclEntryName Name attribute of Entry element
    @param ppiDeclCName CName element value
    @param ppiDeclGuid Guid element value
    @param ppiDeclFeatureFlag Reserved
   **/
    public void genSpdPpiDeclarations(String ppiDeclEntryName, String ppiDeclCName, String ppiDeclGuid,
                                      String ppiDeclFeatureFlag) {
        if (getSpdPpiDeclarations() == null) {
            spdPpiDeclarations = psaRoot.addNewPpiDeclarations();
        }

        setSpdEntry(ppiDeclEntryName, ppiDeclCName, ppiDeclGuid, ppiDeclFeatureFlag, spdPpiDeclarations);
    }

    /**
     Set Entry contents using parameters passed in
     
     @param entryName Name attribute of Entry element
     @param cName CName element value
     @param guid Guid element value
     @param featureFlag Reserved
     @param parent The tag under which Entry element goes to
    **/
    public void setSpdEntry(String entryName, String cName, String guid, String featureFlag, XmlObject parent) {

        if (parent instanceof GuidDeclarationsDocument.GuidDeclarations) {
            GuidDeclarationsDocument.GuidDeclarations.Entry e = ((GuidDeclarations) parent).addNewEntry();
            e.setName(entryName);
            e.setCName(cName);
            e.addNewGuid().setStringValue(guid);
            if (featureFlag != null) {
                e.addNewFeatureFlag().setStringValue(featureFlag);
            }
            return;
        }
        if (parent instanceof ProtocolDeclarationsDocument.ProtocolDeclarations) {
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry pe = ((ProtocolDeclarationsDocument.ProtocolDeclarations) parent)
                                                                                                                                     .addNewEntry();
            pe.setName(entryName);
            pe.setCName(cName);
            pe.addNewGuid().setStringValue(guid);
            if (featureFlag != null) {
                pe.addNewFeatureFlag().setStringValue(featureFlag);
            }
            return;
        }
        if (parent instanceof PpiDeclarationsDocument.PpiDeclarations) {
            PpiDeclarationsDocument.PpiDeclarations.Entry ppe = ((PpiDeclarationsDocument.PpiDeclarations) parent)
                                                                                                                  .addNewEntry();
            ppe.setName(entryName);
            ppe.setCName(cName);
            ppe.addNewGuid().setStringValue(guid);
            if (featureFlag != null) {
                ppe.addNewFeatureFlag().setStringValue(featureFlag);
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
    public void genSpdPcdDefinitions(String pcdItemTypes, String cName, String token, String dataType,
                                     String skuEnable, String sku, String maxSku, String hiiEnable, String varGuid,
                                     String varName, String defaultString) {
        if (getSpdPcdDefinitions() == null) {
            spdPcdDefinitions = psaRoot.addNewPcdDefinitions();
        }

        setSpdPcdEntry(pcdItemTypes, cName, token, dataType, skuEnable, sku, maxSku, hiiEnable, varGuid, varName,
                       defaultString, spdPcdDefinitions);
    }

    /**
     Set Pcd entry contents under parent tag
     
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
     @param parent Tag under which PcdEntry goes to
    **/
    public void setSpdPcdEntry(String pcdItemTypes, String cName, String token, String dataType, String skuEnable,
                               String sku, String maxSku, String hiiEnable, String varGuid, String varName,
                               String defaultString, XmlObject parent) {

        PcdDefinitionsDocument.PcdDefinitions.PcdEntry pe = ((PcdDefinitionsDocument.PcdDefinitions) parent)
                                                                                                            .addNewPcdEntry();
        pe.setItemType(PcdItemTypes.Enum.forString(pcdItemTypes));
        pe.setCName(cName);
        pe.setToken(token);
        pe.setDatumType(PcdDataTypes.Enum.forString(dataType));
        pe.setDefaultValue(defaultString);
        
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
    public PcdDefinitionsDocument.PcdDefinitions getSpdPcdDefinitions() {
        if (spdPcdDefinitions == null) {
            spdPcdDefinitions = psaRoot.getPcdDefinitions();
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

    /**
     Get SpdHeader element
     
     @return SpdHeaderDocument.SpdHeader
    **/
    public SpdHeaderDocument.SpdHeader getSpdHdr() {
        if (spdHdr == null) {
            spdHdr = psaRoot.getSpdHeader();
        }
        return spdHdr;
    }

    /**
     Get Abstract element under tag SpdHeader
     
     @return AbstractDocument.Abstract
    **/
    public AbstractDocument.Abstract getSpdHdrAbs() {
        if (spdHdrAbs == null) {
            spdHdrAbs = getSpdHdr().getAbstract();
        }
        return spdHdrAbs;
    }

    /**
     Set value to Abstract element
     
     @param abs The value set to Abstract element
    **/
    public void setSpdHdrAbs(String abs) {

        if (getSpdHdrAbs() != null) {
            getSpdHdrAbs().setStringValue(abs);
        } else {
            spdHdrAbs = getSpdHdr().addNewAbstract();
            spdHdrAbs.setStringValue(abs);
        }
    }

    /**
     Set value to Copyright element
     
     @param cpRit The value set to Copyright element
    **/
    public void setSpdHdrCpRit(String cpRit) {

        getSpdHdr().setCopyright(cpRit);

    }

    /**
     Set value to Created element
     
     @param createDate The value set to Created element
    **/
    public void setSpdHdrCreateDate(String createDate) {

        getSpdHdr().setCreated(createDate);

    }

    /**
     Set value to Description element
     
     @param des The value set to Description element
    **/
    public void setSpdHdrDes(String des) {
        getSpdHdr().setDescription(des);
    }

    /**
     Get Guid element under SpdHdr
     
     @return GuidDocument.Guid
    **/
    public GuidDocument.Guid getSpdHdrGuid() {
        if (spdHdrGuid == null) {
            spdHdrGuid = getSpdHdr().getGuid();
        }
        return spdHdrGuid;
    }

    /**
     Set value to Guid element
     
     @param guid The value set to Guid element
    **/
    public void setSpdHdrGuid(String guid) {
        if (getSpdHdrGuid() != null) {
            getSpdHdrGuid().setStringValue(guid);
        } else {
            spdHdrGuid = getSpdHdr().addNewGuid();
            spdHdrGuid.setStringValue(guid);
        }
    }

    /**
    Get Version element under SpdHdr
    
    @return String
   **/
    public String getSpdHdrVer() {
        if (spdHdr != null)
            return spdHdr.getVersion();
        else
            return null;
    }

    /**
    Set value to Version element
    
    @param ver The value set to Version element
   **/
    public void setSpdHdrVer(String ver) {
        if (spdHdr != null) {
            spdHdr.setVersion(ver);
        }

    }

    /**
    Get License element under SpdHdr
    
    @return LicenseDocument.License
   **/
    public LicenseDocument.License getSpdHdrLicense() {
        if (spdHdrLicense == null) {
            spdHdrLicense = getSpdHdr().getLicense();
        }
        return spdHdrLicense;
    }

    /**
    Set value to License element
    
    @param license The value set to License element
   **/
    public void setSpdHdrLicense(String license) {
        if (getSpdHdrLicense() != null) {
            getSpdHdrLicense().setStringValue(license);
        } else {
            spdHdrLicense = getSpdHdr().addNewLicense();
            spdHdrLicense.setStringValue(license);
        }
    }

    /**
     Reserved method
     
     @return
    **/
    public OutputDirectoryDocument.OutputDirectory getSpdHdrOutDir() {
        return spdHdrOutDir;
    }

    /**
     Reserved method
     
     @param outdir
    **/
    public void setSpdHdrOutDir(String outdir) {
        if (outdir == null) {
            return;
        }
        if (getSpdHdrOutDir() != null) {
            getSpdHdrOutDir().setStringValue(outdir);
        } else {
            spdHdrOutDir = getSpdHdr().addNewOutputDirectory();
            spdHdrOutDir.setStringValue(outdir);
        }
    }

    /**
    Get PackageName element under SpdHdr
    
    @return PackageNameDocument.PackageName
   **/
    public PackageNameDocument.PackageName getSpdHdrPkgName() {
        if (spdHdrPkgName == null) {
            spdHdrPkgName = getSpdHdr().getPackageName();
        }
        return spdHdrPkgName;
    }

    /**
    Set value to PackageName element
    
    @param pkgName The value set to PackageName element
   **/
    public void setSpdHdrPkgName(String pkgName) {

        if (getSpdHdrPkgName() != null) {
            getSpdHdrPkgName().setStringValue(pkgName);
        } else {
            spdHdrPkgName = getSpdHdr().addNewPackageName();
            spdHdrPkgName.setStringValue(pkgName);
        }
    }

    /**
    Reserved method
    
    @return SpecificationDocument.Specification
   **/
    public SpecificationDocument.Specification getSpdHdrSpec() {
        return spdHdrSpec;
    }

    /**
    Reserved method
    
    @param spec 
   **/
    public void setSpdHdrSpec(String spec) {
        if (spec == null) {
            return;
        }
        if (getSpdHdrSpec() != null) {
            getSpdHdrSpec().setStringValue(spec);
        } else {
            spdHdrSpec = getSpdHdr().addNewSpecification();
            spdHdrSpec.setStringValue(spec);
        }
    }

    /**
    Set value to PackageType element
    
    @param pkgType The value set to PackageType element
   **/
    public void setSpdHdrPkgType(String pkgType) {
        getSpdHdr().setPackageType(PackageType.Enum.forString(pkgType));
    }

    /**
    Set value to ReadOnly element
    
    @param rdOnly The value set to ReadOnly element
   **/
    public void setSpdHdrRdOnly(String rdOnly) {

        getSpdHdr().setReadOnly(new Boolean(rdOnly));
    }

    /**
    Set value to RePackage element
    
    @param rePkg The value set to RePackage element
   **/
    public void setSpdHdrRePkg(String rePkg) {

        getSpdHdr().setRePackage(new Boolean(rePkg));
    }

    /**
    Set value to Updated element
    
    @param updateDate The value set to Updated element
   **/
    public void setSpdHdrUpdateDate(String updateDate) {
        getSpdHdr().setUpdated(updateDate);
    }

    /**
    Set value to URL element
    
    @param url The value set to URL element
   **/
    public void setSpdHdrURL(String url) {
        getSpdHdr().setURL(url);
    }

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

}
