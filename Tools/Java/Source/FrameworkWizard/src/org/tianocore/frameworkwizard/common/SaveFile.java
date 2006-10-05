/** @file
 
 The file provides interface to save xml file.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common;

import java.io.File;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.FrameworkDatabaseDocument.FrameworkDatabase;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;

public class SaveFile {

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        
    }
    
    private static void createDirectory(String path) throws Exception {
        File f = new File(path);
        path = f.getParent();
        FileOperation.newFolder(path);
    }

    /**
     Save file as msa
     
     **/
    public static void saveMsaFile(String path, ModuleSurfaceArea msa) throws Exception {
        //
        // Create the file's directory first
        //
        createDirectory(path);
        
        //
        // Remove all empty top level elements
        //
        XmlObject o = msa.getLibraryClassDefinitions();
        if (o != null) {
            if (msa.getLibraryClassDefinitions().getLibraryClassList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getSourceFiles();
        if (o != null) {
            if (msa.getSourceFiles().getFilenameList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getPackageDependencies();
        if (o != null) {
            if (msa.getPackageDependencies().getPackageList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getProtocols();
        if (o != null) {
            if (msa.getProtocols().getProtocolList().size() <= 0
                && msa.getProtocols().getProtocolNotifyList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getEvents();
        if (o != null) {
            if (msa.getEvents().getCreateEvents() != null || msa.getEvents().getSignalEvents() != null) {
                if (msa.getEvents().getCreateEvents() != null && msa.getEvents().getCreateEvents().getEventTypesList().size() <= 0) {
                    XmlCursor xmlCursor = o.newCursor();
                    xmlCursor.removeXml();
                    xmlCursor.dispose();
                }
                if (msa.getEvents().getSignalEvents() != null && msa.getEvents().getSignalEvents().getEventTypesList().size() <= 0) {
                    XmlCursor xmlCursor = o.newCursor();
                    xmlCursor.removeXml();
                    xmlCursor.dispose();    
                }
            } else {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getHobs();
        if (o != null) {
            if (msa.getHobs().getHobTypesList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getPPIs();
        if (o != null) {
            if (msa.getPPIs().getPpiList().size() <= 0 && msa.getPPIs().getPpiNotifyList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getVariables();
        if (o != null) {
            if (msa.getVariables().getVariableList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getBootModes();
        if (o != null) {
            if (msa.getBootModes().getBootModeList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getSystemTables();
        if (o != null) {
            if (msa.getSystemTables().getSystemTableCNamesList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getDataHubs();
        if (o != null) {
            if (msa.getDataHubs().getDataHubRecordList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getHiiPackages();
        if (o != null) {
            if (msa.getHiiPackages().getHiiPackageList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getGuids();
        if (o != null) {
            if (msa.getGuids().getGuidCNamesList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getExterns();
        if (o != null) {
            if (msa.getExterns().getExternList().size() <= 0 && msa.getExterns().getSpecificationList().size() <= 0
                && msa.getExterns().getPcdIsDriver() == null) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        o = msa.getPcdCoded();
        if (o != null) {
            if (msa.getPcdCoded().getPcdEntryList().size() <= 0) {
                XmlCursor xmlCursor = o.newCursor();
                xmlCursor.removeXml();
                xmlCursor.dispose();
            }
        }

        ModuleSurfaceAreaDocument msaDoc = ModuleSurfaceAreaDocument.Factory.newInstance();
        File f = new File(path);

        //
        //Init namespace
        //
        XmlCursor cursor = XmlConfig.setupXmlCursor(msa.newCursor());

        //
        //Config file format
        //
        XmlOptions options = XmlConfig.setupXmlOptions();

        //
        //Create finial doc
        //
        msaDoc.addNewModuleSurfaceArea();
        msaDoc.setModuleSurfaceArea((ModuleSurfaceArea) cursor.getObject());
        //
        //Save the file
        //
        msaDoc.save(f, options);
        cursor.dispose();
    }

    /**
     Save file as spd
     
     **/
    public static void saveSpdFile(String path, PackageSurfaceArea spd) throws Exception {
        //
        // Create the file's directory first
        //
        createDirectory(path);
        
        PackageSurfaceAreaDocument spdDoc = PackageSurfaceAreaDocument.Factory.newInstance();
        File f = new File(path);

        //
        //Init namespace
        //
        XmlCursor cursor = XmlConfig.setupXmlCursor(spd.newCursor());

        //
        //Config file format
        //
        XmlOptions options = XmlConfig.setupXmlOptions();

        //
        //Create finial doc
        //
        spdDoc.addNewPackageSurfaceArea();
        spdDoc.setPackageSurfaceArea((PackageSurfaceArea) cursor.getObject());
        //
        //Save the file
        //
        spdDoc.save(f, options);
        cursor.dispose();
    }

    /**
     Save file as fpd
     
     **/
    public static void saveFpdFile(String path, PlatformSurfaceArea fpd) throws Exception {
        //
        // Create the file's directory first
        //
        createDirectory(path);
        
        PlatformSurfaceAreaDocument fpdDoc = PlatformSurfaceAreaDocument.Factory.newInstance();
        File f = new File(path);

        //
        //Init namespace
        //
        XmlCursor cursor = XmlConfig.setupXmlCursor(fpd.newCursor());

        //
        //Config file format
        //
        XmlOptions options = XmlConfig.setupXmlOptions();

        //
        //Create finial doc
        //
        fpdDoc.addNewPlatformSurfaceArea();
        fpdDoc.setPlatformSurfaceArea((PlatformSurfaceArea) cursor.getObject());
        //
        //Save the file
        //
        fpdDoc.save(f, options);
        cursor.dispose();
    }

    /**
     Save file as framework db
     
     **/
    public static void saveDbFile(String path, FrameworkDatabase db) throws Exception {
        FrameworkDatabaseDocument dbDoc = FrameworkDatabaseDocument.Factory.newInstance();
        File f = new File(path);

        //
        //Init namespace
        //
        XmlCursor cursor = XmlConfig.setupXmlCursor(db.newCursor());

        //
        //Config file format
        //
        XmlOptions options = XmlConfig.setupXmlOptions();

        //
        //Create finial doc
        //
        dbDoc.addNewFrameworkDatabase();
        dbDoc.setFrameworkDatabase((FrameworkDatabase) cursor.getObject());

        //
        //Save the file
        //
        dbDoc.save(f, options);
        cursor.dispose();
    }
}
