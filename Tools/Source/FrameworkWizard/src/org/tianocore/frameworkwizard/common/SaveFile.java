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
        // TODO Auto-generated method stub

    }

    /**
     Save file as msa
     
     **/
    public static void saveMsaFile(String path, ModuleSurfaceArea msa) throws Exception {
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
    }

    /**
     Save file as spd
     
     **/
    public static void saveSpdFile(String path, PackageSurfaceArea spd) throws Exception {
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
    }

    /**
     Save file as fpd
     
     **/
    public static void saveFpdFile(String path, PlatformSurfaceArea fpd) throws Exception {
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
    }
}
