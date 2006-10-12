/** @file
 
 The file is used to create far file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.far;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Enumeration;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;
import java.util.jar.JarOutputStream;

import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;

public class Far {
    //
    // Class member Manifest
    //
    public Manifest manifest = null;

    //
    // Jar file
    //
    private JarFile jf = null;

    private File jarFile = null;

    //
    // Jar outputStream.
    //
    static public JarOutputStream jos = null;

    public Far(File jarFile) {
        this.jarFile = jarFile;
    }

    //
    // For install/updat jar
    //
    public Far(JarFile farFile) throws Exception {
        jf = farFile;
        this.manifest = new Manifest(getManifestFile());
    }

    public void creatFar(List<PackageIdentification> pkgList, List<PlatformIdentification> plfList,
                         Set<String> fileFilter, FarHeader fHeader) throws Exception {
        jos = new JarOutputStream(new FileOutputStream(jarFile));

        //
        // Generate Manifest and get file lists
        //
        this.manifest = new Manifest();
        this.manifest.setFarHeader(fHeader);
        this.manifest.createManifest(pkgList, plfList, fileFilter);

        this.manifest.hibernateToFile();

        //
        // Write Mainifest file to JAR.
        //
        if (this.manifest.mfFile != null) {
            writeToJar(this.manifest.mfFile, jos);
        }

        //
        // Write all files to JAR
        //
        Set<File> files = this.manifest.files;
        Iterator<File> iter = files.iterator();
        while (iter.hasNext()) {
            writeToJar(iter.next(), jos);
        }
        jos.close();
    }

    private void writeToJar(File file, JarOutputStream jos) throws Exception {
        byte[] buffer = new byte[(int) file.length()];
        FileInputStream fInput = new FileInputStream(file);
        JarEntry entry = new JarEntry(
                                      Tools
                                           .convertPathToUnixType(Tools
                                                                       .getRelativePath(file.getPath(),
                                                                                        Workspace.getCurrentWorkspace())));
        jos.putNextEntry(entry);
        fInput.read(buffer);
        jos.write(buffer);
        fInput.close();
    }

    public void InstallFar(String dir) throws Exception {
        List<FarFileItem> allFile = manifest.getAllFileItem();
        extract(allFile, dir);
    }

    public void InstallFar(Map<PlatformIdentification, File> plfMap, Map<PackageIdentification, File> pkgMap)
                                                                                                             throws Exception {
        Set<PlatformIdentification> plfKeys = plfMap.keySet();
        Iterator<PlatformIdentification> plfIter = plfKeys.iterator();
        while (plfIter.hasNext()) {
            PlatformIdentification item = plfIter.next();
            extract(this.manifest.getPlatformContents(item), plfMap.get(item).getPath());
        }

        Set<PackageIdentification> pkgKeys = pkgMap.keySet();
        Iterator<PackageIdentification> pkgIter = pkgKeys.iterator();
        while (pkgIter.hasNext()) {
            PackageIdentification item = pkgIter.next();
            installPackage(item, pkgMap.get(item));
        }
        jf.close();
    }

    public void installPackage(PackageIdentification packageId, File installPath) throws Exception {
        List<FarFileItem> contents = this.manifest.getPackageContents(packageId);
        extract(contents, installPath.getPath());
    }

    public InputStream getManifestFile() throws Exception {
        JarEntry je = null;
        for (Enumeration e = jf.entries(); e.hasMoreElements();) {
            je = (JarEntry) e.nextElement();
            if (je.getName().equalsIgnoreCase(Manifest.mfFileName))
                return jf.getInputStream(je);
        }
        return null;
    }

    public void createFar() {

    }

    public boolean hibernateToFile() {
        return true;
    }

    public void extract(List<FarFileItem> allFile, String dir) throws Exception {

        Iterator filesItem = allFile.iterator();
        FarFileItem ffItem;
        JarEntry je;
        new File(dir).mkdirs();
        dir += File.separatorChar;
        while (filesItem.hasNext()) {
            try {
                ffItem = (FarFileItem) filesItem.next();
                je = jf.getJarEntry(Tools.convertPathToUnixType(ffItem.getDefaultPath()));
                InputStream entryStream = jf.getInputStream(je);
                File file = new File(dir + ffItem.getRelativeFilename());
                file.getParentFile().mkdirs();
                try {
                    //
                    // Create the output file (clobbering the file if it
                    // exists).
                    //
                    FileOutputStream outputStream = new FileOutputStream(file);

                    try {
                        //
                        // Read the entry data and write it to the output
                        // file.
                        //
                        int fileLen = (int)je.getSize();
                        byte[] buffer = new byte[fileLen];
                        int len = entryStream.read(buffer);
                        if (len < fileLen){
                            File tempFile = new File("tempFile");
                            FileOutputStream fos = new FileOutputStream(tempFile);
                            fos.write(buffer, 0, len);
                            while (len < fileLen){
                                int tempLen = entryStream.read(buffer);
                                len = len + tempLen;
                                fos.write(buffer, 0, tempLen);
                            }
                            fos.close();
                            FileInputStream fin = new FileInputStream(tempFile);
                            fin.read(buffer);
                            tempFile.delete();
                        }
                        //
                        //  Check Md5
                        //
                        if (!ffItem.getMd5Value().equalsIgnoreCase(FarMd5.md5(buffer))){
                            throw new Exception (ffItem.getRelativeFilename() + " MD5 Checksum is invaild!");
                        }
                        outputStream.write(buffer);
                    } finally {
                        outputStream.close();
                    }
                } finally {
                    entryStream.close();
                }

            } finally {
            }
        }

    }

    public void addFileToFar(File file, JarOutputStream farOuputStream, String workDir) {

    }

    /**
     * 
     * @param pkgId
     * @return
     */
    public List<PackageIdentification> getPackageDependencies(PackageIdentification pkgId) {
        String[] entry = new String[2];
        PackageQuery pkgQ = new PackageQuery();
        List<PackageIdentification> result = new ArrayList<PackageIdentification>();

        entry = this.manifest.getPackgeSpd(pkgId);
        if (entry == null) {
            return result;
        }
        if (entry[0] != null) {
            try {
                JarEntry je;
                je = jf.getJarEntry(Tools.convertPathToUnixType(entry[1] + File.separatorChar + entry[0]));
                List<String> masList = pkgQ.getPackageMsaList(jf.getInputStream(je));
                Iterator item = masList.iterator();
                while (item.hasNext()) {
                    je = jf.getJarEntry(Tools.convertPathToUnixType(entry[1] + File.separatorChar + item.next()));
                    List<PackageIdentification> pkgIdList = pkgQ.getModuleDependencies(jf.getInputStream(je));
                    Iterator pkgItem = pkgIdList.iterator();
                    while (pkgItem.hasNext()) {
                        PackageIdentification id = (PackageIdentification) pkgItem.next();
                        if (!AggregationOperation.belongs(id, result)) {
                            result.add(id);
                        }
                    }
                }
            } catch (Exception e) {
                System.out.println(e.getMessage());
            }
        }
        return result;
    }
}
