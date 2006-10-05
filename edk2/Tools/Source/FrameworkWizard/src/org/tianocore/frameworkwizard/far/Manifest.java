/** @file
 
 The file is used to create Far's manifest file
 
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
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.apache.xmlbeans.XmlException;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.platform.PlatformIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class Manifest implements ManifestInterface {
    // /
    // / manifest document
    // /
    Document manifestDoc = null;

    // /
    // / Manfiest file element name
    // /
    final static String mfFileName = "FrameworkArchiveManifest.xml";

    //
    // Header
    //
    final static String farHeader = "FarHeader";

    final static String farHeader_FarName = "FarName";

    final static String farHeader_Abstract = "Abstract";

    final static String farHeader_Description = "Description";

    final static String farHeader_CopyRight = "Copyright";

    final static String farHeader_License = "License";

    final static String farHeader_Specification = "Specification";

    //
    // Package list
    //
    final static String farPackageList = "FarPackageList";

    final static String farPlatformList = "FarPlatformList";

    final static String contents = "Contents";

    final static String userExtensions = "UserExtensions";

    //
    // Common node name
    //

    final static String guidValue = "GuidValue";

    final static String version = "Version";

    //
    // FarPackage node name
    //
    final static String farPackageList_FarPackage = "FarPackage";

    final static String farPackage_FarfileName = "FarFilename";

    final static String farPackage_DefaultPath = "DefaultPath";

    final static String farPlatformList_FarPlatform = "FarPlatform";

    final static String farFileName_FarGuid = "FarGuid";

    final static String farFileName_Md5sum = "Md5sum";

    //
    // manifest header information.
    //
    FarHeader fhInfo = new FarHeader();

    //
    // FarPackage list
    //
    List<FarPackage> fPkgList = new ArrayList<FarPackage>();

    //
    // FarPlatform list
    //
    List<FarPlatformItem> fPlfList = new ArrayList<FarPlatformItem>();

    Set<File> files = new LinkedHashSet<File>();

    //
    // Manifest file
    //
    public File mfFile = null;

    public FarHeader getHeader() {
        return fhInfo;
    }

    public Manifest() throws Exception {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        DocumentBuilder dombuilder = domfac.newDocumentBuilder();
        Document document = dombuilder.newDocument();
        this.manifestDoc = document;

    }

    public Manifest(InputStream manifestInputStream) throws Exception {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        try {
            if (manifestInputStream != null) {
                DocumentBuilder dombuilder = domfac.newDocumentBuilder();
                this.manifestDoc = dombuilder.parse(manifestInputStream);
                parseManifest();
            }

        } catch (Exception e) {
            //
            // Will change to throw Wizader exception.
            //
            throw new Exception(e.getMessage());
        }
    }

    public void setFarHeader(FarHeader fHeader) {
        this.fhInfo = fHeader;
    }

    public void createManifest(List<PackageIdentification> pkgList, List<PlatformIdentification> plfList,
                               Set<String> fileFilter) throws Exception {

        //
        // Add Package and it's contents to FarPackageList.
        //
        Iterator<PackageIdentification> pkgItem = pkgList.iterator();
        while (pkgItem.hasNext()) {
            PackageIdentification pkgId = pkgItem.next();

            //
            // Add package and it's contents to <FarPackageList>.
            //
            addPkgToPkgList(pkgId, fileFilter);
        }

        //
        // Add FarPlatform to this.farPlatformList.
        //
        Iterator<PlatformIdentification> plfItem = plfList.iterator();
        while (plfItem.hasNext()) {
            PlatformIdentification plfId = plfItem.next();

            //
            // Add platform to Mainifest.
            //
            addPlatformIdToFarPlatformItemList(plfId);
        }
    }

    public void setManifestFile(File manifestFile) throws Exception {
        DocumentBuilderFactory domfac = DocumentBuilderFactory.newInstance();
        DocumentBuilder dombuilder = domfac.newDocumentBuilder();
        InputStream is = new FileInputStream(manifestFile);
        this.manifestDoc = dombuilder.parse(is);
    }

    public void addPkgToPkgList(PackageIdentification packageId, Set<String> fileFilter) throws Exception {
        files.add(packageId.getSpdFile());

        FarPackage farPackage = new FarPackage();
        //
        // Add SPD file to FarPackage
        //
        File spdFile = new File(packageId.getPath());

        FarFileItem ffItem = new FarFileItem(spdFile.getName(), FarMd5.md5(spdFile));
        farPackage.setFarFile(ffItem);

        //
        // Add package guid value.
        //
        farPackage.setGuidValue(packageId.getGuid());

        //
        // Add package version
        //
        farPackage.setVersion(packageId.getVersion());

        //
        // Add DefaultPat: Package absoulte path - Workspace absolut
        // path.
        //
        String pkgDir = Tools.getFilePathOnly(packageId.getPath());
        File rootDir = new File(pkgDir);
        farPackage.setDefaultPath(Tools.getRelativePath(rootDir.getPath(), Workspace.getCurrentWorkspace()));

        //
        // Get package root dir contains.
        //
        Set<File> fileSet = new LinkedHashSet<File>();
        Set<File> fpdFileSet = new LinkedHashSet<File>();

        //
        // Get all files and fpd files
        //
        recursiveDirectory(fileSet, fpdFileSet, rootDir, fileFilter);

        //
        // Remove current package's SPD file
        //
        fileSet.remove(packageId.getSpdFile());

        files.addAll(fileSet);

        Iterator<File> iter = fileSet.iterator();
        List<FarFileItem> contents = new ArrayList<FarFileItem>();
        while (iter.hasNext()) {
            File normalFile = iter.next();
            String fileRelativePath = Tools.getRelativePath(normalFile.getPath(), pkgDir);
            ffItem = new FarFileItem(fileRelativePath, FarMd5.md5(normalFile));
            contents.add(ffItem);
        }

        farPackage.setContentList(contents);

        //        List<FarPlatformItem> fpfList = new ArrayList<FarPlatformItem>();
        //
        //        iter = fpdFileSet.iterator();
        //
        //        while (iter.hasNext()) {
        //            File fpdFile = iter.next();
        //            PlatformIdentification platformId = new PlatformIdentification(wsTool
        //                    .getId(fpdFile.getPath(), OpenFile.openFpdFile(fpdFile
        //                            .getPath())));
        //            addPlatformIdToFarPlatformItemList(fpfList, platformId);
        //        }
        //        farPackage.setFarPlatformList(fpfList);
        fPkgList.add(farPackage);
    }

    private void recursiveDirectory(Set<File> files, Set<File> fpds, File dir, Set<String> fileFilter) {
        if (isFilter(dir, fileFilter)) {
            return;
        }
        File[] allFilesInDir = dir.listFiles();
        for (int i = 0; i < allFilesInDir.length; i++) {
            if (allFilesInDir[i].isFile()) {
                if (isFilter(allFilesInDir[i], fileFilter)) {
                    continue;
                }
                //                if (allFilesInDir[i].getPath().toLowerCase().endsWith(".fpd")) {
                //                    fpds.add(allFilesInDir[i]);
                //                } else {
                files.add(allFilesInDir[i]);
                //                }
            } else {
                recursiveDirectory(files, fpds, allFilesInDir[i], fileFilter);
            }
        }
    }

    public void addPlatformIdToFarPlatformItemList(PlatformIdentification platformId) throws Exception {
        files.add(platformId.getFpdFile());

        FarPlatformItem fpfItem = new FarPlatformItem();
        FarFileItem ffItem;
        String fpfPath = platformId.getPath();
        File fpfFile = new File(fpfPath);
        //
        // Add farFileName
        //
        ffItem = new FarFileItem(Tools.getRelativePath(fpfFile.getPath(), Workspace.getCurrentWorkspace()),
                                 FarMd5.md5(fpfFile));
        fpfItem.setFarFile(ffItem);

        //
        // Add guid value
        //
        fpfItem.setGuidValue(platformId.getGuid());

        //
        // Add version
        //
        fpfItem.setVersion(platformId.getVersion());

        fPlfList.add(fpfItem);
    }

    /**
     * 
     * @param
     * @return Set<PackageIdentification> list of PackageIdentification.
     */
    public List<PackageIdentification> getPackageList() throws Exception {
        //
        // PackageIdentification set.
        //
        List<PackageIdentification> pkgList = new ArrayList<PackageIdentification>();
        //
        // 
        //
        Iterator pkgItem = this.fPkgList.iterator();
        while (pkgItem.hasNext()) {
            FarPackage fPkg = (FarPackage) pkgItem.next();
            //
            // Get package information from SPD file and create a package
            // identification.
            //

            PackageIdentification pkgId = new PackageIdentification(fPkg.getFarFile().getRelativeFilename(),
                                                                    fPkg.getGuidValue(), fPkg.getVersion());
            pkgId.setPath(Workspace.getCurrentWorkspace() + File.separatorChar + fPkg.getDefaultPath()
                          + File.separatorChar + fPkg.getFarFile().getRelativeFilename());
            //            wsTool.getId(
            //                    Workspace.getCurrentWorkspace() + File.separatorChar
            //                            + fPkg.getDefaultPath(), OpenFile
            //                            .openFpdFile(Workspace.getCurrentWorkspace()
            //                                    + File.separatorChar
            //                                    + fPkg.getDefaultPath()
            //                                    + File.separatorChar
            //                                    + fPkg.getFarFile().getRelativeFilename()));
            pkgList.add(pkgId);
        }
        return pkgList;
    }

    /**
     * 
     */
    public List<PlatformIdentification> getPlatformList() throws Exception, IOException, XmlException {
        //
        // PlatformIdentification set.
        //
        List<PlatformIdentification> plfList = new ArrayList<PlatformIdentification>();
        Iterator plfItem = this.fPlfList.iterator();
        while (plfItem.hasNext()) {
            FarPlatformItem fpfItem = (FarPlatformItem) plfItem.next();
            File file = new File(Workspace.getCurrentWorkspace() + File.separatorChar
                                 + fpfItem.getFarFile().getRelativeFilename());
            //
            // Set platformIdentificaiton's path as absolutly path (include
            // workspace and FPD relatively path)
            //
            PlatformIdentification plfId = new PlatformIdentification(fpfItem.getFarFile().getRelativeFilename(),
                                                                      fpfItem.getGuidValue(), fpfItem.getVersion(),
                                                                      file.getPath());

            //                (PlatformIdentification) wsTool
            //                    .getId(file.getParent(), OpenFile.openFpdFile(Workspace
            //                            .getCurrentWorkspace()
            //                            + File.separatorChar
            //                            + fpfItem.getFarFile().getRelativeFilename()));
            plfList.add(plfId);
        }
        return plfList;
    }

    public List<FarFileItem> getPlatformContents(PlatformIdentification platformId) {
        List<FarFileItem> result = new ArrayList<FarFileItem>();
        Iterator<FarPlatformItem> iter = this.fPlfList.iterator();

        while (iter.hasNext()) {
            FarPlatformItem item = iter.next();
            if (item.isIdentityPlf(platformId)) {
                FarFileItem farFileItem = item.getFarFile();
                farFileItem.setDefaultPath(farFileItem.getRelativeFilename());
                farFileItem.setRelativeFilename(Tools.getFileNameOnly(farFileItem.getRelativeFilename()));
                result.add(farFileItem);
                break;
            }
        }
        return result;
    }

    public List<FarFileItem> getPackageContents(PackageIdentification packageId) {
        List<FarFileItem> farFileList = new ArrayList<FarFileItem>();
        Iterator pkgItem = this.fPkgList.iterator();
        while (pkgItem.hasNext()) {
            FarPackage pkg = (FarPackage) pkgItem.next();
            if (pkg.isIdentityPkg(packageId)) {
                //
                // Add spd far file to list.
                //
                farFileList.add(pkg.getFarFile());
                //
                // Add all files in contents to list.
                //
                farFileList.addAll(pkg.getContentList());
                //
                // Add all farfiles in <FarPlatformList> to list.
                //
                //                List<FarPlatformItem> plfList = pkg.getFarPlatformList();
                //                Iterator plfItem = plfList.iterator();
                //                while (plfItem.hasNext()) {
                //                    farFileList.add(((FarPlatformItem) plfItem.next())
                //                            .getFarFile());
                //                }

                Iterator<FarFileItem> ffIter = farFileList.iterator();
                while (ffIter.hasNext()) {
                    FarFileItem ffItem = ffIter.next();
                    ffItem.setDefaultPath(pkg.getDefaultPath() + File.separatorChar + ffItem.getRelativeFilename());
                }
                break;
            }
        }

        return farFileList;
    }

    /**
     * 
     * @param pkgId
     * @return String: return string represent jar file entry; 
     */
    public String[] getPackgeSpd(PackageIdentification pkgId) {
        Iterator pkgItem = this.fPkgList.iterator();
        String[] entryStr = new String[2];
        while (pkgItem.hasNext()) {
            FarPackage pkg = (FarPackage) pkgItem.next();
            if (pkg.isIdentityPkg(pkgId)) {
                entryStr[0] = pkg.getFarFile().getRelativeFilename();
                entryStr[1] = pkg.getDefaultPath();
                return entryStr;
            }
        }
        return null;
    }

    public List<FarFileItem> getPackageContents() {
        //
        // In this farFilelist,all FarFileItem's relativeFileName should be
        // set as absolutely path.
        //
        List<FarFileItem> farFileList = new ArrayList<FarFileItem>();
        Iterator pkgItem = this.fPkgList.iterator();
        FarFileItem ffItem = null;

        while (pkgItem.hasNext()) {
            FarPackage pkg = (FarPackage) pkgItem.next();

            //
            // Add spd far file to list.
            //
            ffItem = pkg.getFarFile();
            //
            // Set farFileItem relativeFileName = absolutePath + file Name.
            //
            farFileList.add(new FarFileItem(pkg.getDefaultPath() + File.separatorChar + ffItem.getRelativeFilename(),
                                            ffItem.getMd5Value()));
            //
            // Add all files in contents to list.
            //
            Iterator contentsItem = pkg.getContentList().iterator();
            while (contentsItem.hasNext()) {
                ffItem = (FarFileItem) contentsItem.next();
                farFileList.add(new FarFileItem(pkg.getDefaultPath() + File.separator + ffItem.getRelativeFilename(),
                                                ffItem.getMd5Value()));
            }
            //
            // Add all farfiles in <FarPlatformList> to list.
            //
            List<FarPlatformItem> plfList = pkg.getFarPlatformList();
            Iterator plfItem = plfList.iterator();
            while (plfItem.hasNext()) {
                ffItem = ((FarPlatformItem) plfItem.next()).getFarFile();
                farFileList.add(new FarFileItem(pkg.getDefaultPath() + File.separator + ffItem.getRelativeFilename(),
                                                ffItem.getMd5Value()));
            }
        }
        return farFileList;
    }

    public String getPackageDefaultPath(PackageIdentification packageId) {
        Iterator pkgItr = this.fPkgList.iterator();
        while (pkgItr.hasNext()) {
            FarPackage farPackage = (FarPackage) pkgItr.next();
            if (farPackage.isIdentityPkg(packageId)) {
                return farPackage.getDefaultPath();
            }
        }
        return null;
    }

    //    public void setPackageInstallPath(PackageIdentification packageId, String path) {
    //        Iterator<FarPackage> pkgItr = this.fPkgList.iterator();
    //        while (pkgItr.hasNext()) {
    //            FarPackage farPackage = pkgItr.next();
    //            if (farPackage.isIdentityPkg(packageId)) {
    //                farPackage.setDefaultPath(path);
    //                return ;
    //            }
    //        }
    //    }
    //
    //    public void setPlatformInstallPath(PlatformIdentification platformId, String path) {
    //        Iterator<FarPlatformItem> plfItr = this.fPlfList.iterator();
    //        while (plfItr.hasNext()) {
    //            FarPlatformItem farPlatform = plfItr.next();
    //            if (farPlatform.i.isIdentity(platformId)) {
    //                farPackage.setDefaultPath(path);
    //                return ;
    //            }
    //        }
    //    }

    public List<FarFileItem> getAllFileItem() {
        //
        // The farFileName in this list are all abosulte path.
        //
        List<FarFileItem> ffiList = new ArrayList<FarFileItem>();
        //
        // Add far files in <FarPackageList> to list.
        //
        ffiList = this.getPackageContents();

        //
        // Add far files in <FarPlatformList> to list
        //
        NodeList elementList = this.manifestDoc.getElementsByTagName(farPlatformList);
        for (int i = 0; i < elementList.getLength(); i++) {
            //
            // Get <farPlatform> node list.
            //
            Node item = elementList.item(i);
            NodeList plfElements = item.getChildNodes();
            for (int j = 0; j < plfElements.getLength(); j++) {
                //
                // Get each <FarPlatform> content.
                //
                NodeList plfChildNode = plfElements.item(i).getChildNodes();
                Node tempNode = null;
                for (int t = 0; t < plfChildNode.getLength(); t++) {
                    tempNode = plfChildNode.item(t);
                    //
                    // Get child node value and set to platformIdentification.
                    //
                    if (tempNode.getNodeName().equalsIgnoreCase(farPackage_FarfileName)) {
                        NamedNodeMap farAttr = tempNode.getAttributes();
                        //
                        // Change relative path to absolute one
                        //
                        FarFileItem farFile = new FarFileItem(tempNode.getTextContent(),
                                                              farAttr.getNamedItem(farFileName_Md5sum).getTextContent());
                        ffiList.add(farFile);
                    }
                }
            }
        }
        return ffiList;
    }

    public void hibernateToFile() throws Exception {
        //
        // create manifest root node
        //
        Element rootNode = this.manifestDoc.createElement("FrameworkArchiveManifest");
        this.manifestDoc.appendChild(rootNode);

        //
        // create manifest header node
        //
        Element headerNode = this.manifestDoc.createElement(farHeader);
        rootNode.appendChild(headerNode);
        //
        // Add FarHeader to headerNode.
        //
        Element farName = this.manifestDoc.createElement(farHeader_FarName);
        farName.setTextContent(this.fhInfo.getFarName());
        headerNode.appendChild(farName);

        Element gv = this.manifestDoc.createElement(guidValue);
        gv.setTextContent(this.fhInfo.getGuidValue());
        headerNode.appendChild(gv);

        Element ver = this.manifestDoc.createElement(version);
        ver.setTextContent(this.fhInfo.getVersion());
        headerNode.appendChild(ver);

        Element abstra = this.manifestDoc.createElement(farHeader_Abstract);
        abstra.setTextContent(this.fhInfo.getAbstractStr());
        headerNode.appendChild(abstra);

        Element descr = this.manifestDoc.createElement(farHeader_Description);
        descr.setTextContent(this.fhInfo.getDescription());
        headerNode.appendChild(descr);

        Element copyright = this.manifestDoc.createElement(farHeader_CopyRight);
        copyright.setTextContent(this.fhInfo.getCopyright());
        headerNode.appendChild(copyright);

        Element license = this.manifestDoc.createElement(farHeader_License);
        license.setTextContent(this.fhInfo.getLicense());
        headerNode.appendChild(license);

        Element spec = this.manifestDoc.createElement(farHeader_Specification);
        spec.setTextContent(this.fhInfo.getSpecification());
        System.out.println(this.fhInfo.getSpecification());
        headerNode.appendChild(spec);

        //
        // create manifest FarPackageList node
        //
        Element pkgListNode = this.manifestDoc.createElement(farPackageList);
        rootNode.appendChild(pkgListNode);

        //
        // Save each item in farPackage list to <FarPackage>.
        //
        Iterator pkgItem = this.fPkgList.iterator();
        while (pkgItem.hasNext()) {
            pkgToFarPkgNode(pkgListNode, (FarPackage) pkgItem.next());

        }

        //
        // create manifest FarPlatformList node
        //
        Element plfListNode = this.manifestDoc.createElement(farPlatformList);
        rootNode.appendChild(plfListNode);

        //
        // Save farPakcage list info to <FarPackageList> node
        //
        Iterator plfItem = this.fPlfList.iterator();
        while (plfItem.hasNext()) {
            FarPlatformItem plfIterator = (FarPlatformItem) plfItem.next();
            PlfToPlatformNode(plfListNode, plfIterator);
        }

        //
        // Write the DOM document to the file
        //
        Transformer xformer = TransformerFactory.newInstance().newTransformer();
        xformer.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "2");
        xformer.setOutputProperty(OutputKeys.INDENT, "yes");

        //
        // Prepare the DOM document for writing
        //
        Source source = new DOMSource(this.manifestDoc);
        //
        // Prepare the output file, get the Mainifest file name from <FarHeader>
        // <FarName>.
        //
        this.mfFile = new File(Workspace.getCurrentWorkspace() + File.separatorChar + mfFileName);
        //
        // generate all directory path
        //
        Result result = new StreamResult(this.mfFile);
        xformer.transform(source, result);
        //
        // Close result. Flush file by manual for Jdk1.5.0_04. 
        //
        ((StreamResult) result).getOutputStream().close();
    }

    public void pkgToFarPkgNode(Element parentNode, FarPackage pkgItem) {
        Element pkgNode = this.manifestDoc.createElement(farPackageList_FarPackage);
        //
        // Add <FarFileName>
        //
        ffiToFfNode(pkgNode, pkgItem.getFarFile());
        //
        // Add <GuidValue>
        //
        setStrItemToNode(pkgNode, pkgItem.getGuidValue(), guidValue);
        //
        // Add <Version>
        //
        setStrItemToNode(pkgNode, pkgItem.getVersion(), version);
        //
        // Add <DefaultPath>
        //
        setStrItemToNode(pkgNode, pkgItem.getDefaultPath(), farPackage_DefaultPath);

        //
        // Add <Contents>
        //
        Element contentNode = this.manifestDoc.createElement(contents);
        Iterator iterator = pkgItem.getContentList().iterator();
        while (iterator.hasNext()) {
            ffiToFfNode(contentNode, (FarFileItem) iterator.next());
        }
        pkgNode.appendChild(contentNode);
        parentNode.appendChild(pkgNode);
    }

    public void PlfToPlatformNode(Element parentNode, FarPlatformItem fplItem) {
        Element fplNode = this.manifestDoc.createElement(farPlatformList_FarPlatform);
        //
        // Add <FarFileName>
        //
        ffiToFfNode(fplNode, fplItem.getFarFile());
        //
        // Add <GuidValue>
        //
        setStrItemToNode(fplNode, fplItem.getGuidValue(), guidValue);
        //
        // Add <Version>
        //
        setStrItemToNode(fplNode, fplItem.getVersion(), version);
        //
        // Add to <PlatformList>
        //
        parentNode.appendChild(fplNode);

    }

    public void ffiToFfNode(Element parentNode, FarFileItem ffi) {
        Element farFileName = this.manifestDoc.createElement(farPackage_FarfileName);
        farFileName.setTextContent(ffi.getRelativeFilename());
        System.out.println(farFileName.getTextContent());
        System.out.println(ffi.getRelativeFilename());
        farFileName.setAttribute(farFileName_Md5sum, ffi.getMd5Value());
        parentNode.appendChild(farFileName);
    }

    public void setStrItemToNode(Element parentNode, String strValue, String strName) {
        Element node = this.manifestDoc.createElement(strName);
        node.setTextContent(strValue);
        parentNode.appendChild(node);
    }

    private void parseManifest() {

        //
        // Parse header
        //
        parseMfHeader();
        //
        // parse <farPackageList>
        //
        parseHeaderFarPackageList();

        //
        // parse <farPlatformList>
        //
        NodeList ele = this.manifestDoc.getElementsByTagName(farPlatformList);
        Node plfNode;
        if (ele.getLength() > 0) {
            //
            // Only have one <FarPlatformList> node under manifest root node.
            //
            plfNode = ele.item(0);
            parseFarPlatformList(plfNode, this.fPlfList);
        }
    }

    private void parseMfHeader() {
        Node headerNode;
        NodeList ele = this.manifestDoc.getElementsByTagName(farHeader);
        if (ele.getLength() > 0) {
            //
            // For manifest file only have one <FarHeader>
            //
            headerNode = ele.item(0);
        } else {
            return;
        }
        NodeList childList = headerNode.getChildNodes();
        Node node = null;
        String nodeName = null;
        for (int i = 0; i < childList.getLength(); i++) {
            node = childList.item(i);
            nodeName = node.getNodeName();
            if (nodeName.equalsIgnoreCase(farHeader_FarName)) {
                String nodeValue = node.getTextContent();
                this.fhInfo.setFarName(nodeValue);
            } else if (nodeName.equalsIgnoreCase(guidValue)) {
                this.fhInfo.setGuidValue(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(version)) {
                this.fhInfo.setVersion(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(farHeader_Abstract)) {
                this.fhInfo.setAbstractStr(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(farHeader_Description)) {
                this.fhInfo.setDescription(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(farHeader_CopyRight)) {
                this.fhInfo.setCopyright(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(farHeader_License)) {
                this.fhInfo.setLicense(node.getTextContent());
            } else if (nodeName.equalsIgnoreCase(farHeader_Specification)) {
                this.fhInfo.setSpecification(node.getTextContent());
            }
        }
    }

    public void parseHeaderFarPackageList() {
        Node farPkgNode;
        NodeList ele = this.manifestDoc.getElementsByTagName(farPackageList);
        if (ele.getLength() > 0) {
            //
            // For manifest file only have one <FarHeader>
            //
            farPkgNode = ele.item(0);
        } else {
            return;
        }
        NodeList fpnList = farPkgNode.getChildNodes();
        for (int i = 0; i < fpnList.getLength(); i++) {
            if (fpnList.item(i).getNodeType() == Node.TEXT_NODE) {
                continue;
            }
            FarPackage fpItem = new FarPackage();
            parseFarPackage(fpnList.item(i), fpItem);
            this.fPkgList.add(fpItem);
        }
    }

    public void parseFarPackage(Node farPkgNode, FarPackage fpItem) {
        NodeList childList = farPkgNode.getChildNodes();
        Node childNode;
        String childName;
        for (int i = 0; i < childList.getLength(); i++) {
            childNode = childList.item(i);
            childName = childNode.getNodeName();
            if (childName.equalsIgnoreCase(farPackage_FarfileName)) {
                fpItem.setFarFile(parseFarFile(childNode));
            } else if (childName.equalsIgnoreCase(guidValue)) {
                fpItem.setGuidValue(childNode.getTextContent());
            } else if (childName.equalsIgnoreCase(version)) {
                fpItem.setVersion(childNode.getTextContent());
            } else if (childName.equalsIgnoreCase(farPackage_DefaultPath)) {
                fpItem.setDefaultPath(childNode.getTextContent());
            } else if (childName.equalsIgnoreCase(farPlatformList)) {
                List<FarPlatformItem> plfList = new ArrayList<FarPlatformItem>();
                parseFarPlatformList(childNode, plfList);
                fpItem.setFarPlatformList(plfList);
            } else if (childName.equalsIgnoreCase(contents)) {
                List<FarFileItem> ffList = new ArrayList<FarFileItem>();
                parseContents(childNode, ffList);
                fpItem.setContentList(ffList);
            }
        }
    }

    /**
     * 
     * @param fpfListNode
     * @param plfList
     */
    public void parseFarPlatformList(Node fpfListNode, List<FarPlatformItem> plfList) {
        //
        // Get <FarPlatform> list.
        //
        NodeList child = fpfListNode.getChildNodes();
        Node farPlfNode;
        for (int i = 0; i < child.getLength(); i++) {
            if (child.item(i).getNodeType() == Node.TEXT_NODE) {
                continue;
            }
            farPlfNode = child.item(i);
            plfList.add(parseFarPlatform(farPlfNode));
        }
    }

    /**
     * 
     * @param fpfNode
     * @return
     */
    public FarPlatformItem parseFarPlatform(Node fpfNode) {
        //
        // New FarPlatformItem.
        //
        FarPlatformItem fplItem = new FarPlatformItem();
        //
        // Get <FarPlatform> elements;
        //
        NodeList childList = fpfNode.getChildNodes();
        Node child;
        String nodeName;
        for (int i = 0; i < childList.getLength(); i++) {
            child = childList.item(i);
            nodeName = child.getNodeName();
            if (nodeName.equalsIgnoreCase(farPackage_FarfileName)) {
                fplItem.setFarFile(parseFarFile(child));
            } else if (nodeName.equalsIgnoreCase(guidValue)) {
                fplItem.setGuidValue(child.getTextContent());
            } else if (nodeName.equalsIgnoreCase(version)) {
                fplItem.setVersion(child.getTextContent());
            }
        }

        return fplItem;
    }

    public void parseContents(Node contentsNode, List<FarFileItem> ffList) {
        NodeList contentList = contentsNode.getChildNodes();
        Node contentNode;
        for (int i = 0; i < contentList.getLength(); i++) {
            if (contentList.item(i).getNodeType() == Node.TEXT_NODE) {
                continue;
            }
            contentNode = contentList.item(i);
            //
            // Parse each  <FarFileName>.
            //
            ffList.add(parseFarFile(contentNode));
        }
    }

    public FarFileItem parseFarFile(Node farFileNode) {
        String ffName = farFileNode.getTextContent();
        NamedNodeMap attr = farFileNode.getAttributes();
        FarFileItem ffItem = new FarFileItem(ffName, attr.getNamedItem(farFileName_Md5sum).getTextContent());
        return ffItem;
    }

    public boolean isFilter(File file, Set<String> fileter) {
        Iterator<String> iter = fileter.iterator();
        while (iter.hasNext()) {
            Pattern pattern = Pattern.compile(iter.next());
            Matcher matcher = pattern.matcher(file.getName());

            if (matcher.find()) {
                return true;
            }
        }
        return false;
    }

}
