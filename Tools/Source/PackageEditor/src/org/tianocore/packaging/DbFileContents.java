/** @file
  Java class DbFileContents is used to deal with FrameworkDatabase.db file cotents.
 
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
import java.util.LinkedList;
import java.util.List;
import java.util.ListIterator;
import java.util.Date;
import java.text.SimpleDateFormat;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;

import org.tianocore.*;

/**
 This class provides methods for add, remove, query FrameworkDatabase.db file.
  
 @since PackageEditor 1.0
**/
public class DbFileContents {

    ///
    /// return values for various conditions. 
    ///
    static final int BASE_PACKAGE_NOT_INSTALLED = 1;

    static final int VERSION_NOT_EQUAL = 2;

    static final int GUID_NOT_EQUAL = 3;

    static final int SAME_ALL = 4;

    private File dbFile = null;

    private FrameworkDatabaseDocument fdd = null;

    private FrameworkDatabaseDocument.FrameworkDatabase fddRoot = null;
    
    private PackageListDocument.PackageList pkgList = null;

    public DbFileContents() {
        super();
        // TODO Auto-generated constructor stub
    }

    /**
       Parse file f, store its xml data in fdd, store root xml element in fddRoot.
       
       @param f DB file to parse
    **/
    public DbFileContents(File f) {
        try {
            dbFile = f;
            if (fdd == null) {
                fdd = ((FrameworkDatabaseDocument) XmlObject.Factory.parse(dbFile));
            }
            fddRoot = fdd.getFrameworkDatabase();
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }
    
    /**
     Generate the Package element in FrameworkDatabase.db
     
     @param baseName Base name of package
     @param ver Version of package
     @param guid GUID of package
     @param path Where the package installed
     @param installDate When the package installed
    **/
    public void genPackage (String baseName, String ver, String guid, String path, String installDate) {
        if (getPkgList() == null) {
            pkgList = fddRoot.addNewPackageList();
        }
        PackageListDocument.PackageList.Package p = pkgList.addNewPackage();
        p.addNewPackageName().setStringValue(baseName);
        p.addNewGuid().setStringValue(guid);
        p.addVersion(ver);
        p.addNewPath().setStringValue(path);
        p.addNewInstalledDate().setStringValue(installDate);
    }
    
    /**
     Get PackageList
     
     @return PackageListDocument.PackageList
    **/
    public PackageListDocument.PackageList getPkgList() {
        if (pkgList == null) {
            pkgList = fddRoot.getPackageList();
        }
        return pkgList;
    }
    
    /**
     Remove PackageList and all elements under it.
    **/
    public void removePackageList() {
        XmlObject o = fddRoot.getPackageList();
        if (o == null)
            return;
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
    }
    /**
     Get the number of Package elements.
     
     @return int
    **/
    public int getPackageCount () {
        return fddRoot.getPackageList().getPackageList().size();
    }
    
    /**
     Get all Package contents into String array
     
     @param pkg Two dimentional array to store Package info.
    **/
    public void getPackageList(String[][] pkg) {
        List<PackageListDocument.PackageList.Package> l = fddRoot.getPackageList().getPackageList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) li
                                                                                                                                      .next();
            if (p.getPackageNameArray(0)!= null) {
                pkg[i][0] = p.getPackageNameArray(0).getStringValue();
            }
            
            pkg[i][1] = p.getVersionArray(0);
            
            if (p.getGuidArray(0) != null) {
                pkg[i][2] = p.getGuidArray(0).getStringValue();
            }
            if (p.getPathArray(0) != null) {
                pkg[i][3] = p.getPathArray(0).getStringValue();
            }
            if (p.getInstalledDateArray(0) != null) {
                pkg[i][4] = p.getInstalledDateArray(0);
            }
            i++;
        }
    }
    /**
     Check whether destDir has been used by one Package
     
     @param destDir The directory to check.
     @retval <1> destDir has been used
     @retval <0> destDir has not been used
     @return int
    **/
    public int checkDir(String destDir) {
        List<PackageListDocument.PackageList.Package> lp = fddRoot.getPackageList().getPackageList();

        ListIterator lpi = lp.listIterator();
        while (lpi.hasNext()) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) lpi.next();
            if (p.getPathArray(0).getStringValue().equals(destDir)) {
                return 1;
            }
        }
        return 0;
    }

    /**
     Find the package info. and store results into list of same base name or list
     of same version.
     
     @param base The base name of package
     @param version The version of package
     @param guid the GUID of package
     @param lpSameBase The list to store package info with the same base name with "base"
     @param lpSameVersion The list to store package info from lpSameBase and same version
            with "version"
     @retval <0> No package installed has base name "base"
     @retval <VERSION_NOT_EQUAL> At least one package installed with "base" but no "version"
     @retval <GUID_NOT_EQUAL> At least one package installed with "base" and "version" but no "guid"
     @retval <SAME_ALL> One installed package has the same base, version and guid
     @return int
    **/
    public int query(String base, String version, String guid,
                     List<PackageListDocument.PackageList.Package> lpSameBase,
                     List<PackageListDocument.PackageList.Package> lpSameVersion) {

        List<PackageListDocument.PackageList.Package> lp = fddRoot.getPackageList().getPackageList();

        ListIterator lpi = lp.listIterator();
        while (lpi.hasNext()) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) lpi.next();
            if (p.getPackageNameArray(0).getStringValue().equals(base)) {
                lpSameBase.add(p);
            }
        }

        if (lpSameBase.size() == 0) {
            return 0;
        }

        for (ListIterator li = lpSameBase.listIterator(); li.hasNext();) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) li.next();
            if (p.getVersionArray(0).equals(version)) {
                lpSameVersion.add(p);
            }
        }

        if (lpSameVersion.size() == 0) {
            return VERSION_NOT_EQUAL;
        }

        for (ListIterator li = lpSameVersion.listIterator(); li.hasNext();) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) li.next();
            if (!p.getGuidArray(0).getStringValue().equals(guid)) {
                return GUID_NOT_EQUAL;
            }
        }

        return SAME_ALL;

    }

    /**
     Update package info (name, version, guid) with installDir, newVer, newGuid.
     And update install date with current date. if no package info available, add 
     a new entry.
     
     @param name Original base name
     @param version Original version
     @param guid Original GUID
     @param installDir original path
     @param newVer Version value of package to be installed
     @param newGuid GUID value of package to be installed
     @throws IOException Exception during file operation
    **/
    public void updatePkgInfo(String name, String version, String guid, String installDir, String newVer, String newGuid)
                                                                                                                         throws IOException {
        List<PackageListDocument.PackageList.Package> lp = fddRoot.getPackageList().getPackageList();

        ListIterator lpi = lp.listIterator();
        while (lpi.hasNext()) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) lpi.next();
            if (p.getPackageNameArray(0).getStringValue().equals(name)) {
                if (p.getVersionArray(0).equals(version)) {
                    if (p.getGuidArray(0).getStringValue().equals(guid)) {
                        p.setVersionArray(0, newVer);
                        p.getGuidArray(0).setStringValue(newGuid);
                        p.getPathArray(0).setStringValue(installDir);
                        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm");
                        Date date = new Date();
                        p.setInstalledDateArray(0, format.format(date));
                        saveAs();
                        return;

                    }
                }
            }
        }

        addNewPkgInfo(name, newVer, newGuid, installDir);
    }

    /**
     Add one new package entry.
     
     @param name Package base name
     @param version Package version
     @param guid Package Guid
     @param installDir Package path
     @throws IOException Exception during file operation
    **/
    public void addNewPkgInfo(String name, String version, String guid, String installDir) throws IOException {

        PackageListDocument.PackageList.Package p = fddRoot.getPackageList().addNewPackage();
        p.addNewPackageName().setStringValue(name);
        p.addNewGuid().setStringValue(guid);
        p.addNewVersion().setStringValue(version);
        p.addNewPath().setStringValue(installDir + "/");

        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        Date date = new Date();
        p.addNewInstalledDate().setStringValue(format.format(date));
        saveAs();
    }

    /**
     Save the fdd into file with format options
    **/
    public void saveAs() {
        XmlOptions options = new XmlOptions();

        options.setCharacterEncoding("UTF-8");
        options.setSavePrettyPrint();
        options.setSavePrettyPrintIndent(2);
        try {
            fdd.save(dbFile, options);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

}
