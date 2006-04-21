/** @file
  Java class ManifestContents is used to deal with FDPManifest.xml file related
  operations.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import org.apache.xmlbeans.XmlException;

import org.tianocore.*;
import java.io.*;

/**
 This class operates on FDPManifest.xml file
  
 @since PackageEditor 1.0
**/
public class ManifestContents {

    ///
    /// it is more convenient to get input stream from Jar entry of to-be-installed package file.
    /// so i use InputStream instead of File
    ///
    private InputStream manIs = null;

    FrameworkDevPkgManifestDocument manDoc = null;

    HeaderDocument hdr = null;

    FrameworkDevPkgManifestDocument.FrameworkDevPkgManifest manRoot = null;

    public ManifestContents(InputStream fis) throws XmlException, IOException {

        manIs = fis;
        manDoc = FrameworkDevPkgManifestDocument.Factory.parse(manIs);
        manRoot = manDoc.getFrameworkDevPkgManifest();

    }

    /**
     Get package name from manifest file header.
     
     @return String
    **/
    public String getBaseName() {
        return manRoot.getHeader().getPackageName().getStringValue();
    }

    /**
     Get package version from manifest file header.
     
     @return String
    **/
    public String getVersion() {
        return manRoot.getHeader().getVersion();
    }

    /**
     Get package GUID from manifest file header.
     
     @return String
    **/
    public String getGuid() {
        return manRoot.getHeader().getGuid().getStringValue();
    }
}
