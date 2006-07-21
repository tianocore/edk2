/** @file
 
 The file provides interface to open xml file.
 
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
import java.io.IOException;

import org.apache.xmlbeans.XmlException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.FrameworkDatabaseDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.FrameworkDatabaseDocument.FrameworkDatabase;
import org.tianocore.ModuleSurfaceAreaDocument.ModuleSurfaceArea;
import org.tianocore.PackageSurfaceAreaDocument.PackageSurfaceArea;
import org.tianocore.PlatformSurfaceAreaDocument.PlatformSurfaceArea;

public class OpenFile {
    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub

    }

    /**
     Open specificed Msa file and read its content
     
     @param strMsaFilePath The input data of Msa File Path
     
     **/
    public static ModuleSurfaceArea openMsaFile(String strMsaFilePath) throws IOException, XmlException, Exception {
        Log.log("Open Msa", strMsaFilePath);
        File msaFile = new File(strMsaFilePath);
        ModuleSurfaceAreaDocument xmlMsaDoc = (ModuleSurfaceAreaDocument) XmlObject.Factory.parse(msaFile);
        return xmlMsaDoc.getModuleSurfaceArea();
    }

    /**
     Open specificed Spd file and read its content
     
     @param strSpdFilePath The input data of Spd File Path
     
     **/
    public static PackageSurfaceArea openSpdFile(String strSpdFilePath) throws IOException, XmlException, Exception {
        Log.log("Open Spd", strSpdFilePath);
        File spdFile = new File(strSpdFilePath);
        PackageSurfaceAreaDocument xmlSpdDoc = (PackageSurfaceAreaDocument) XmlObject.Factory.parse(spdFile);
        return xmlSpdDoc.getPackageSurfaceArea();
    }

    /**
     Open specificed Fpd file and read its content
     
     @param strFpdFilePath The input data of Fpd File Path
     
     **/
    public static PlatformSurfaceArea openFpdFile(String strFpdFilePath) throws IOException, XmlException,
                                                                                 Exception {
        Log.log("Open Fpd", strFpdFilePath);
        File fpdFile = new File(strFpdFilePath);
        PlatformSurfaceAreaDocument xmlFpdDoc = null;
        xmlFpdDoc = (PlatformSurfaceAreaDocument) XmlObject.Factory.parse(fpdFile);
        return xmlFpdDoc.getPlatformSurfaceArea();
    }

    /**
     
     Open specificed Framework Database file and read its content
     
     */
    public static FrameworkDatabase openFrameworkDb(String strDbFilePath) throws IOException, XmlException, Exception {
        Log.log("Open Framework Database", strDbFilePath);
        File db = new File(strDbFilePath);
        FrameworkDatabaseDocument xmlDb = null;
        xmlDb = (FrameworkDatabaseDocument) XmlObject.Factory.parse(db);
        return xmlDb.getFrameworkDatabase();
    }
}
