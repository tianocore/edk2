/** @file
  Java class ForceInstallPkg is used to install a package without DB check.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.io.*;
import java.util.jar.*;
import org.apache.xmlbeans.XmlException;

/**
 Derived class from FrameworkPkg, installation skipping some checks.
  
 @since PackageEditor 1.0
**/
public class ForceInstallPkg extends FrameworkPkg {

    private String oldVer = null;

    private String oldGuid = null;

    /**
       Constructor with parameters
       
       @param s Package path to be installed 
       @param d Destination directory
    **/
    public ForceInstallPkg(String s, String d) {
        super(s, d);
       
    }

    public void setOldVersion(String v) {
        oldVer = v;
    }

    public void setOldGuid(String guid) {
        oldGuid = guid;
    }

    /**
     Set jar file to package name to be installed
     **/
    protected void pre_install() throws DirSame, IOException {
        setJf(new JarFile(getPkg()));
        
    }

    /**
     Update database file contents after install
     **/
    protected void post_install() throws IOException, XmlException {
        //
        // Get package info. from FDPManifest.xml file
        //
        setJf(new JarFile(getPkg()));
        ManifestContents manFile = new ManifestContents(getManifestInputStream(getJf()));
        setBName(manFile.getBaseName());
        setPVer(manFile.getVersion());
        setPGuid(manFile.getGuid());
        getJf().close();

        //
        // Add force installed package info. into database file
        //
        setDbFile(new File(getWkSpace() + System.getProperty("file.separator") + FrameworkPkg.dbConfigFile));
        setDfc(new DbFileContents(new File(getWkSpace() + System.getProperty("file.separator") + dbConfigFile)));
        getDfc().updatePkgInfo(getBName(), oldVer, oldGuid, getWkDir().substring(getWkSpace().length() + 1), getPVer(),
                               getPGuid());
    }

}
