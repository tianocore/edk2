/** @file
  Java class FrameworkPkg is used to do package related operations.
 
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
import java.util.Enumeration;
import java.util.LinkedList;
import java.util.List;
import java.util.jar.*;
import org.apache.xmlbeans.*;

import org.tianocore.PackageListDocument;

/**
 This class deals with package related operations
  
 @since PackageEditor 1.0
**/
public class FrameworkPkg {

    private String pkg = null;

    private JarFile jf = null;

    ///
    /// where the package will be extracted to
    /// 
    private String wkDir = null;

    private String bName = null;

    private String pVer = null;

    private String pGuid = null;

    ///
    /// current WORKSPACE location
    ///
    private String wkSpace = null;

    private File dbFile = null;

    private DbFileContents dfc = null;

    ///
    /// relative path of FrameworkDatabase.db file
    ///
    final static String dbConfigFile = "Tools" + System.getProperty("file.separator") + "Conf"
                                       + System.getProperty("file.separator") + "FrameworkDatabase.db";


    public FrameworkPkg() {

    }

    public FrameworkPkg(String package_name, String work_space) {
        pkg = package_name;
        wkSpace = work_space;
    }

    /**
     install package (*.fdp file) to dir
     
     @param dir Destination directory
     @retval <0> Install successfully
     @return int
     @throws IOException
     @throws XmlException Xml file exception
     @throws DirSame One package already installed to dir
     @throws BasePkgNotInstalled Some package must be installed first
     @throws VerNotEqual At least one package info with same base name but version different
     @throws GuidNotEqual At least one package info with same base name and version but guid different
     @throws SameAll At least one package info with same base name, version and guid same
    **/
    public int install(final String dir) throws IOException, XmlException, DirSame, BasePkgNotInstalled, VerNotEqual,
                                        GuidNotEqual, SameAll {
        wkDir = dir;
        pre_install();
        extract(wkDir);
        post_install();
        return 0;
    }

    public int uninstall() {

        return 0;
    }

    /**
     Check package info. against Frameworkdatabase.db
     
     @throws IOException
     @throws XmlException Xml file exception
     @throws DirSame One package already installed to dir
     @throws BasePkgNotInstalled Some package must be installed first
     @throws VerNotEqual At least one package info with same base name but version different
     @throws GuidNotEqual At least one package info with same base name and version but guid different
     @throws SameAll At least one package info with same base name, version and guid same
    **/
    protected void pre_install() throws IOException, XmlException, DirSame, BasePkgNotInstalled, VerNotEqual,
                                GuidNotEqual, SameAll {

        jf = new JarFile(pkg);

        ManifestContents manFile = new ManifestContents(getManifestInputStream(jf));

        String baseName = manFile.getBaseName();
        String pkgVersion = manFile.getVersion();
        String pkgGuid = manFile.getGuid();
        bName = baseName;
        pVer = pkgVersion;
        pGuid = pkgGuid;

        if (dbFile == null) {
            dbFile = new File(wkSpace + System.getProperty("file.separator") + dbConfigFile);
        }
        //
        // the db file should exist if base packages have been installed
        //
        if (!dbFile.exists()) {
            throw new BasePkgNotInstalled();
        }

        if (dfc == null) {
            dfc = new DbFileContents(dbFile);
        }
        if (dfc.checkDir(wkDir) != 0) {
            throw new DirSame();
        }
        
        //
        // Get database info into lists
        //
        List<PackageListDocument.PackageList.Package> lpSameBase = new LinkedList<PackageListDocument.PackageList.Package>();
        List<PackageListDocument.PackageList.Package> lpSameVersion = new LinkedList<PackageListDocument.PackageList.Package>();
        int i = dfc.query(baseName, pkgVersion, pkgGuid, lpSameBase, lpSameVersion);

        //
        // throw various kind of exceptions according to query return value.
        //
        if (i == DbFileContents.VERSION_NOT_EQUAL) {

            jf.close();
            throw new VerNotEqual(lpSameBase);
        }
        if (i == DbFileContents.GUID_NOT_EQUAL) {

            jf.close();
            throw new GuidNotEqual(lpSameVersion);
        }
        if (i == DbFileContents.SAME_ALL) {
            jf.close();
            throw new SameAll(lpSameVersion);
        }

    }

    /**
     Add package info into db file.
     
     @throws IOException
     @throws XmlException
    **/
    protected void post_install() throws IOException, XmlException {

        dfc.addNewPkgInfo(bName, pVer, pGuid, wkDir.substring(wkSpace.length() + 1));

    }

    /**
     Extract package to dir
     
     @param dir Destination directory
     @throws DirSame
     @throws IOException
    **/
    private void extract(String dir) throws DirSame, IOException {

        new File(dir).mkdirs();
        dir += System.getProperty("file.separator");
        try {
            for (Enumeration e = jf.entries(); e.hasMoreElements();) {
                JarEntry je = (JarEntry) e.nextElement();
                
                //
                // jar entry contains directory only, make these directories
                //
                if (je.isDirectory()) {
                    new File(dir + je.getName()).mkdirs();
                    continue;
                }
                
                //
                // jar entry contains relative path and file name, make relative directories
                // under destination dir
                //
                int index = je.getName().lastIndexOf(System.getProperty("file.separator"));
                if (index != -1) {
                    String dirPath = je.getName().substring(0, index);
                    new File(dir + dirPath).mkdirs();
                }
                
                if (je != null) {
                    //
                    // Get an input stream for this entry.
                    //
                    InputStream entryStream = jf.getInputStream(je);

                    try {
                        //
                        // Create the output file (clobbering the file if it exists).
                        //
                        FileOutputStream file = new FileOutputStream(dir + je.getName());

                        try {

                            byte[] buffer = new byte[1024];
                            int bytesRead;
                            //
                            // Read the entry data and write it to the output file.
                            //
                            while ((bytesRead = entryStream.read(buffer)) != -1) {
                                file.write(buffer, 0, bytesRead);
                            }

                            System.out.println(je.getName() + " extracted.");
                        } finally {
                            file.close();
                        }
                    } finally {
                        entryStream.close();
                    }
                }

            }

        } finally {
            jf.close();

        }

    }

    public String getBName() {
        return bName;
    }

    public void setBName(String name) {
        bName = name;
    }

    public File getDbFile() {
        return dbFile;
    }

    public void setDbFile(File dbFile) {
        this.dbFile = dbFile;
    }

    public DbFileContents getDfc() {
        return dfc;
    }

    public void setDfc(DbFileContents dfc) {
        this.dfc = dfc;
    }

    public String getPGuid() {
        return pGuid;
    }

    public void setPGuid(String guid) {
        pGuid = guid;
    }

    public String getPVer() {
        return pVer;
    }

    public void setPVer(String ver) {
        pVer = ver;
    }

    public String getWkDir() {
        return wkDir;
    }

    public void setWkDir(String wkDir) {
        this.wkDir = wkDir;
    }

    public String getWkSpace() {
        return wkSpace;
    }

    public void setWkSpace(String wkSpace) {
        this.wkSpace = wkSpace;
    }

    public JarFile getJf() {
        return jf;
    }

    public void setJf(JarFile jf) {
        this.jf = jf;
    }

    public String getPkg() {
        return pkg;
    }

    public void setPkg(String pkg) {
        this.pkg = pkg;
    }

    /**
     Get the input stream of FDPManifest.xml file from jar entry
     
     @param jf The Jar file that contains FDPManifest.xml file
     @return InputStream
     @throws IOException
    **/
    protected InputStream getManifestInputStream(JarFile jf) throws IOException {
        JarEntry je = null;
        for (Enumeration e = jf.entries(); e.hasMoreElements();) {
            je = (JarEntry) e.nextElement();
            if (je.getName().contains("FDPManifest.xml"))
                return jf.getInputStream(je);
        }

        return null;
    }

}


/**
 Various Exception classes for what happened when database info and package info
 are compared.
  
 @since PackageEditor 1.0
**/
class DirSame extends Exception {
    final static long serialVersionUID = 0;
}

class BasePkgNotInstalled extends Exception {
    final static long serialVersionUID = 0;
}

class VerNotEqual extends Exception {
    final static long serialVersionUID = 0;

    //private String version = null;
    List<PackageListDocument.PackageList.Package> lppSameBase = null;

    VerNotEqual(List<PackageListDocument.PackageList.Package> ver) {
        lppSameBase = ver;
    }

    public List<PackageListDocument.PackageList.Package> getVersion() {
        return lppSameBase;
    }
}

class GuidNotEqual extends Exception {
    final static long serialVersionUID = 0;

    private List<PackageListDocument.PackageList.Package> lppSameVer = null;

    GuidNotEqual(List<PackageListDocument.PackageList.Package> ver) {
        lppSameVer = ver;
    }

    public List<PackageListDocument.PackageList.Package> getGuid() {
        return lppSameVer;
    }
}

class SameAll extends Exception {
    final static long serialVersionUID = 0;

    private List<PackageListDocument.PackageList.Package> version = null;

    SameAll(List<PackageListDocument.PackageList.Package> ver) {
        version = ver;
    }

    public List<PackageListDocument.PackageList.Package> getVersion() {
        return version;
    }
}
