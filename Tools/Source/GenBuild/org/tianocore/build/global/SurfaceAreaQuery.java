/** @file
 This file is for surface area information retrieval.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.build.global;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlString;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.CNameType;
import org.tianocore.ExternsDocument;
import org.tianocore.FileNameConvention;
import org.tianocore.FvImagesDocument;
import org.tianocore.GuidDeclarationsDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryClassDeclarationsDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.ModuleDefinitionsDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.ModuleSaBuildOptionsDocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaFilesDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.OptionDocument;
import org.tianocore.PPIsDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.PackageHeadersDocument;
import org.tianocore.PcdCodedDocument;
import org.tianocore.PlatformDefinitionsDocument;
import org.tianocore.PlatformHeaderDocument;
import org.tianocore.PpiDeclarationsDocument;
import org.tianocore.ProtocolDeclarationsDocument;
import org.tianocore.Sentence;
import org.tianocore.SpdHeaderDocument;
import org.tianocore.UserExtensionsDocument;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.ProtocolsDocument.Protocols.Protocol;
import org.tianocore.ProtocolsDocument.Protocols.ProtocolNotify;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.toolchain.ToolChainInfo;
import org.tianocore.logger.EdkLog;
import org.w3c.dom.Node;

/**
 * SurfaceAreaQuery class is used to query Surface Area information from msa,
 * mbd, spd and fpd files.
 * 
 * This class should not instantiated. All the public interfaces is static.
 * 
 * @since GenBuild 1.0
 */
public class SurfaceAreaQuery {

    public static String prefix = "http://www.TianoCore.org/2006/Edk2.0";

    // /
    // / Contains name/value pairs of Surface Area document object. The name is
    // / always the top level element name.
    // /
    private static Map<String, XmlObject> map = null;

    // /
    // / mapStack is used to do nested query
    // /
    private static Stack<Map<String, XmlObject>> mapStack = new Stack<Map<String, XmlObject>>();

    // /
    // / prefix of name space
    // /
    private static String nsPrefix = "sans";

    // /
    // / xmlbeans needs a name space for each Xpath element
    // /
    private static String ns = null;

    // /
    // / keep the namep declaration for xmlbeans Xpath query
    // /
    private static String queryDeclaration = null;

    /**
     * Set a Surface Area document for query later
     * 
     * @param map
     *            A Surface Area document in TopLevelElementName/XmlObject
     *            format.
     */
    public static void setDoc(Map<String, XmlObject> map) {
        ns = prefix;
        queryDeclaration = "declare namespace " + nsPrefix + "='" + ns + "'; ";
        SurfaceAreaQuery.map = map;
    }

    /**
     * Push current used Surface Area document into query stack. The given new
     * document will be used for any immediately followed getXXX() callings,
     * untill pop() is called.
     * 
     * @param newMap
     *            The TopLevelElementName/XmlObject format of a Surface Area
     *            document.
     */
    public static void push(Map<String, XmlObject> newMap) {
        mapStack.push(SurfaceAreaQuery.map);
        SurfaceAreaQuery.map = newMap;
    }

    /**
     * Discard current used Surface Area document and use the top document in
     * stack instead.
     */
    public static void pop() {
        SurfaceAreaQuery.map = mapStack.pop();
    }

    // /
    // / Convert xPath to be namespace qualified, which is necessary for
    // XmlBeans
    // / selectPath(). For example, converting /MsaHeader/ModuleType to
    // / /ns:MsaHeader/ns:ModuleType
    // /
    private static String normalizeQueryString(String[] exp, String from) {
        StringBuffer normQueryString = new StringBuffer(4096);

        int i = 0;
        while (i < exp.length) {
            String newExp = from + exp[i];
            Pattern pattern = Pattern.compile("([^/]*)(/|//)([^/]+)");
            Matcher matcher = pattern.matcher(newExp);

            while (matcher.find()) {
                String starter = newExp.substring(matcher.start(1), matcher
                        .end(1));
                String seperator = newExp.substring(matcher.start(2), matcher
                        .end(2));
                String token = newExp.substring(matcher.start(3), matcher
                        .end(3));

                normQueryString.append(starter);
                normQueryString.append(seperator);
                normQueryString.append(nsPrefix);
                normQueryString.append(":");
                normQueryString.append(token);
            }

            ++i;
            if (i < exp.length) {
                normQueryString.append(" | ");
            }
        }

        return normQueryString.toString();
    }

    /**
     * Search all XML documents stored in "map" for the specified xPath, using
     * relative path (starting with '$this')
     * 
     * @param xPath
     *            xpath query string array
     * @returns An array of XmlObject if elements are found at the specified
     *          xpath
     * @returns NULL if nothing is at the specified xpath
     */
    public static Object[] get(String[] xPath) {
        if (map == null) {
            return null;
        }

        String[] keys = (String[]) map.keySet().toArray(new String[map.size()]);
        List<Object> result = new ArrayList<Object>();
        for (int i = 0; i < keys.length; ++i) {
            XmlObject rootNode = (XmlObject) map.get(keys[i]);
            if (rootNode == null) {
                continue;
            }

            String query = queryDeclaration
                    + normalizeQueryString(xPath, "$this/" + keys[i]);
            XmlObject[] tmp = rootNode.selectPath(query);
            for (int j = 0; j < tmp.length; ++j) {
                result.add((Object)tmp[j]);
            }
        }

        int size = result.size();
        if (size <= 0) {
            return null;
        }

        return (Object[]) result.toArray(new Object[size]);
    }

    /**
     * Search XML documents named by "rootName" for the given xPath, using
     * relative path (starting with '$this')
     * 
     * @param rootName
     *            The top level element name
     * @param xPath
     *            The xpath query string array
     * @returns An array of XmlObject if elements are found at the given xpath
     * @returns NULL if nothing is found at the given xpath
     */
    public static Object[] get(String rootName, String[] xPath) {
        if (map == null) {
            return null;
        }

        XmlObject root = (XmlObject) map.get(rootName);
        if (root == null) {
            return null;
        }

        String query = queryDeclaration
                + normalizeQueryString(xPath, "$this/" + rootName);
        XmlObject[] result = root.selectPath(query);
        if (result.length > 0) {
            return (Object[])result;
        }

        query = queryDeclaration + normalizeQueryString(xPath, "/" + rootName);
        result = root.selectPath(query);
        if (result.length > 0) {
            return (Object[])result;
        }

        return null;
    }

    /**
     * Retrieve SourceFiles/Filename for specified ARCH type
     * 
     * @param arch
     *            architecture name
     * @returns An 2 dimension string array if elements are found at the known
     *          xpath
     * @returns NULL if nothing is found at the known xpath
     */
    public static String[][] getSourceFiles(String arch) {
        String[] xPath;
        Object[] returns;

        xPath = new String[] { "/Filename" };

        returns = get("SourceFiles", xPath);

        if (returns == null || returns.length == 0) {
            return new String[0][0];
        }

        Filename[] sourceFileNames = (Filename[]) returns;
        List<String[]> outputList = new ArrayList<String[]>();
        for (int i = 0; i < sourceFileNames.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = sourceFileNames[i].getSupArchList();
            if (arch == null || arch.equalsIgnoreCase("") || archList == null || archList.contains(arch)) {
                outputList.add(new String[] {sourceFileNames[i].getToolCode(),sourceFileNames[i].getStringValue()});
            }
        }
           
        String[][] outputString = new String[outputList.size()][2];
        for (int index = 0; index < outputList.size(); index++) {
            outputString[index][0] = outputList.get(index)[0];
            outputString[index][1] = outputList.get(index)[1];
        }
        return outputString;
    }

    /**
     * Retrieve /PlatformDefinitions/OutputDirectory from FPD
     * 
     * @returns Directory names array if elements are found at the known xpath
     * @returns Empty if nothing is found at the known xpath
     */
    public static String getFpdOutputDirectory() {
        String[] xPath = new String[] { "/PlatformDefinitions" };

        Object[] returns = get("PlatformSurfaceArea", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        PlatformDefinitionsDocument.PlatformDefinitions item = (PlatformDefinitionsDocument.PlatformDefinitions)returns[0];
        return item.getOutputDirectory();
    }

    public static String getFpdIntermediateDirectories() {
        String[] xPath = new String[] { "/PlatformDefinitions" };

        Object[] returns = get("PlatformSurfaceArea", xPath);
        if (returns == null || returns.length == 0) {
            return "UNIFIED";
        }
        PlatformDefinitionsDocument.PlatformDefinitions item = (PlatformDefinitionsDocument.PlatformDefinitions)returns[0];
        if(item.getIntermediateDirectories() == null) {
            return null;     
        }
        else {
            return item.getIntermediateDirectories().toString();
        }
    }

    public static String getModuleFfsKeyword() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleSaBuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleSaBuildOptionsDocument.ModuleSaBuildOptions item = (ModuleSaBuildOptionsDocument.ModuleSaBuildOptions)returns[0];
        return item.getFfsFormatKey();
    }
    
    public static String getModuleFvBindingKeyword() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleSaBuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleSaBuildOptionsDocument.ModuleSaBuildOptions item = (ModuleSaBuildOptionsDocument.ModuleSaBuildOptions)returns[0];
        return item.getFvBinding();
    }
    
    public static List getModuleSupportedArchs() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleDefinitions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleDefinitionsDocument.ModuleDefinitions item = (ModuleDefinitionsDocument.ModuleDefinitions)returns[0];
        return item.getSupportedArchitectures();
    }
    
    public static BuildOptionsDocument.BuildOptions.Ffs[] getFpdFfs() {
        String[] xPath = new String[] {"/Ffs"};
        
        Object[] returns = get("BuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return new BuildOptionsDocument.BuildOptions.Ffs[0];
        }
        return (BuildOptionsDocument.BuildOptions.Ffs[])returns;
    }
    
    public static String getModuleOutputFileBasename() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleDefinitions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleDefinitionsDocument.ModuleDefinitions item = (ModuleDefinitionsDocument.ModuleDefinitions)returns[0];
        return item.getOutputFileBasename();
    }
    
    /**
     * Retrieve BuildOptions/Option or Arch/Option
     * 
     * @param toolChainFamilyFlag
     *            if true, retrieve options for toolchain family; otherwise for
     *            toolchain
     * 
     * @returns String[][5] name, target, toolchain, arch, coommand of options
     *          if elements are found at the known xpath. String[0][] if dont
     *          find element.
     * 
     * @returns Empty array if nothing is there
     */
    public static String[][] getOptions(String from, String[] xPath, boolean toolChainFamilyFlag) {
        String target = null;
        String toolchain = null;
        String toolchainFamily = null;
        List<String> archList = null;
        String cmd = null;
        String targetName = null;
        String optionName = null;

        Object[] returns = get(from, xPath);
        if (returns == null) {
            return new String[0][5];
        }

        List<String[]> optionList = new ArrayList<String[]>();
        OptionDocument.Option option;

        for (int i = 0; i < returns.length; i++) {
            option = (OptionDocument.Option) returns[i];

            //
            // Get Target, ToolChain(Family), Arch, Cmd, and Option from Option,
            // then
            // put to result[][5] array in above order.
            //
            String[] targetList;
            if (option.getBuildTargets() == null) {
                target = null;
            }
            else {
                target = option.getBuildTargets().toString();
            }
            if (target != null) {
                targetList = target.split(" ");
            } else {
                targetList = new String[1];
                targetList[0] = null;
            }

            if (toolChainFamilyFlag) {
                toolchainFamily = option.getToolChainFamily();
                if (toolchainFamily != null) {
                    toolchain = toolchainFamily.toString();
                } else {
                    toolchain = null;
                }
            } else {
                toolchain = option.getTagName();
            }

            archList = new ArrayList<String>();
            @SuppressWarnings("unchecked")
            List<String> archEnumList = option.getSupArchList();            
            if (archEnumList == null) {
                archList.add(null);
            } else {
                archList.addAll(archEnumList);
                /*
                Iterator it = archEnumList.iterator();
                while (it.hasNext()) {
                    System.out.println(it.next().getClass().getName());
                    SupportedArchitectures.Enum archType = it.next();
                    archList.add(archType.toString());
                }
                */
            }

            cmd = option.getToolCode();

            optionName = option.getStringValue();
            for (int t = 0; t < targetList.length; t++) {
                for (int j = 0; j < archList.size(); j++) {
                    optionList.add(new String[] { targetList[t],
                            toolchain, archList.get(j), cmd, optionName});
                }
            }
        }

        String[][] result = new String[optionList.size()][5];
        for (int i = 0; i < optionList.size(); i++) {
            result[i][0] = optionList.get(i)[0];
            result[i][1] = optionList.get(i)[1];
            result[i][2] = optionList.get(i)[2];
            result[i][3] = optionList.get(i)[3];
            result[i][4] = optionList.get(i)[4];
        }
        return result;
    }

    public static String[][] getModuleBuildOptions(boolean toolChainFamilyFlag) {
        String[] xPath;
        
        if (toolChainFamilyFlag == true) {
            xPath = new String[] {
                    "/Options/Option[not(@ToolChainFamily) and not(@TagName)]",
                    "/Options/Option[@ToolChainFamily]", };
        } else {
            xPath = new String[] {
                    "/Options/Option[not(@ToolChainFamily) and not(@TagName)]",
                    "/Options/Option[@TagName]", };
        }
        return getOptions("ModuleSaBuildOptions", xPath, toolChainFamilyFlag);
    }   
    
    public static String[][] getPlatformBuildOptions(boolean toolChainFamilyFlag) {
        String[] xPath;

        if (toolChainFamilyFlag == true) {
            xPath = new String[] {
                    "/BuildOptions/Options/Option[not(@ToolChainFamily) and not(@TagName)]",
                    "/BuildOptions/Options/Option[@ToolChainFamily]", };
        } else {
            xPath = new String[] {
                    "/BuildOptions/Options/Option[not(@ToolChainFamily) and not(@TagName)]",
                    "/BuildOptions/Options/Option[@TagName]", };
        }

        return getOptions("PlatformSurfaceArea", xPath, toolChainFamilyFlag);
    }

    public static ToolChainInfo getFpdToolChainInfo() {
        String[] xPath = new String[] { "/PlatformDefinitions" };

        Object[] returns = get("PlatformSurfaceArea", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        
        PlatformDefinitionsDocument.PlatformDefinitions item = (PlatformDefinitionsDocument.PlatformDefinitions)returns[0];
        ToolChainInfo toolChainInfo = new ToolChainInfo();
        toolChainInfo.addTargets(item.getBuildTargets().toString());
        toolChainInfo.addArchs(item.getSupportedArchitectures().toString());
        toolChainInfo.addTagnames((String)null);
        return toolChainInfo;
    }

    /**
     * Retrieve <xxxHeader>/ModuleType
     * 
     * @returns The module type name if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String getModuleType() {
        String[] xPath = new String[] { "/ModuleType" };

        Object[] returns = get(xPath);
        if (returns != null && returns.length > 0) {
            ModuleTypeDef type = (ModuleTypeDef) returns[0];
            return type.enumValue().toString();
        }

        return null;
    }

    /**
     * Retrieve PackageDependencies/Package
     * 
     * @param arch
     *            Architecture name
     * 
     * @returns package name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static PackageIdentification[] getDependencePkg(String arch) {
        String[] xPath;
        String packageGuid = null;
        String packageVersion = null;

        
        xPath = new String[] { "/Package" };
        
        Object[] returns = get("PackageDependencies", xPath);
        if (returns == null) {
            return new PackageIdentification[0];
        }

        //
        //  Get packageIdentification 
        // 
        List<PackageIdentification> packageIdList = new ArrayList<PackageIdentification>();
        for (int i = 0; i < returns.length; i++) {
            PackageDependenciesDocument.PackageDependencies.Package item = (PackageDependenciesDocument.PackageDependencies.Package) returns[i];
            @SuppressWarnings("unchecked")
            List<String> archList = item.getSupArchList();
            if (arch == null || archList == null || archList.contains(arch)) {
                packageGuid = item.getPackageGuid();
                packageVersion = item.getPackageVersion();
                packageIdList.add(new PackageIdentification(null, packageGuid,
                    packageVersion));
            }
        }

        //
        //  transfer packageIdentification list to array.
        // 
        PackageIdentification[] packageIdArray = new PackageIdentification[packageIdList.size()];
        for (int i = 0; i < packageIdList.size(); i++) {
            packageIdArray[i] = new PackageIdentification(null, packageIdList.get(i).getGuid(),packageIdList.get(i).getVersion());
        }
        return packageIdArray;
    }

    /**
     * Retrieve LibraryClassDefinitions/LibraryClass for specified usage
     * 
     * @param usage
     *            Library class usage
     * 
     * @returns LibraryClass objects list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public static String[] getLibraryClasses(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] { "/LibraryClass" };
        } else {
            xPath = new String[] { "/LibraryClass[@Usage='" + usage + "']" };
        }

        Object[] returns = get("LibraryClassDefinitions", xPath);
        if (returns == null || returns.length == 0) {
            return new String[0];
        }

        LibraryClassDocument.LibraryClass[] libraryClassList = (LibraryClassDocument.LibraryClass[]) returns;
        String[] libraryClassName = new String[libraryClassList.length];
        for (int i = 0; i < libraryClassList.length; i++) {
            libraryClassName[i] = libraryClassList[i].getKeyword();
        }
        return libraryClassName;
    }

    /**
     * Retrieve ModuleEntryPoint names
     * 
     * @returns ModuleEntryPoint name list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public static String[] getModuleEntryPointArray() {
        String[] xPath = new String[] { "/Extern/ModuleEntryPoint" };

        Object[] returns = get("Externs", xPath);

        if (returns != null && returns.length > 0) {
            String[] entryPoints = new String[returns.length];

            for (int i = 0; i < returns.length; ++i) {
                entryPoints[i] = ((CNameType) returns[i]).getStringValue();
            }

            return entryPoints;
        }

        return null;
    }

    /**
     * retrieve Protocol for specified usage
     * 
     * @param usage
     *            Protocol usage arch Architecture
     * 
     * @returns Protocol String list if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getProtocolArray(String arch, String usage) {
        String[] xPath;
        String usageXpath = "";
        String archXpath = "";

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            archXpath = "/Protocol";
            if (usage != null && !usage.equals("")) {
                usageXpath = "/Protocol[@Usage='" + usage + "']";
                xPath = new String[] { usageXpath, archXpath };
            } else {
                return getProtocolArray(arch);
            }

        }

        Object[] returns = get("Protocols", xPath);
        if (returns == null) {
            return new String[0];
        }
        Protocol[] protocolList = (Protocol[]) returns;

        String[] protocolArray = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            protocolArray[i] = protocolList[i].getProtocolCName();
        }
        return protocolArray;
    }

    /**
     * retrieve Protocol for specified usage
     * 
     * @param arch
     *            Architecture
     * 
     * @returns Protocol String list if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getProtocolArray(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            xPath = new String[] { "/Protocol" };
        }

        Object[] returns = get("Protocols", xPath);
        if (returns == null) {
            return new String[0];
        }
        Protocol[] returnlList = (Protocol[]) returns;

        List<String> protocolList = new ArrayList<String>();
        
        for (int i = 0; i < returns.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = returnlList[i].getSupArchList();
            if (archList == null || archList.contains(arch)){
                protocolList.add(returnlList[i].getProtocolCName());
            }
        }
        String[] protocolArray = new String[protocolList.size()];
        for (int i = 0; i < protocolList.size(); i++) {
            protocolArray[i] = protocolList.get(i);
        }
        return protocolArray;
    }

    /**
     * Retrieve ProtocolNotify for specified usage
     * 
     * @param usage
     *            ProtocolNotify usage
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getProtocolNotifyArray(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            xPath = new String[] { "/ProtocolNotify" };
        }

        Object[] returns = get("Protocols", xPath);
        if (returns == null) {
            return new String[0];
        }

        List<String> protocolNotifyList = new ArrayList<String>();
        
        for (int i = 0; i < returns.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = ((ProtocolNotify) returns[i]).getSupArchList();
            if (archList == null || archList.contains(arch)){
                protocolNotifyList.add(((ProtocolNotify) returns[i]).getProtocolNotifyCName());
            }
            
        }
        String[] protocolNotifyArray = new String[protocolNotifyList.size()];
        for (int i = 0; i < protocolNotifyList.size(); i++) {
            protocolNotifyArray[i] = protocolNotifyList.get(i);
        }
        return protocolNotifyArray;
    }

    /**
     * Retrieve ProtocolNotify for specified usage
     * 
     * @param usage
     *            ProtocolNotify usage
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getProtocolNotifyArray(String arch, String usage) {

        String[] xPath;
        String usageXpath;
        String archXpath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            archXpath = "/ProtocolNotify";
            if (usage != null && !usage.equals("")) {
                usageXpath = "/ProtocolNotify[@Usage='" + arch + "']";
                xPath = new String[] { archXpath, usageXpath };
            } else {
                return getProtocolNotifyArray(arch);
            }
        }

        Object[] returns = get("Protocols", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] protocolNotifyList = new String[returns.length];

        for (int i = 0; i < returns.length; i++) {
            protocolNotifyList[i] = ((ProtocolNotify) returns[i]).getProtocolNotifyCName();
        }
        return protocolNotifyList;
    }

    /**
     * Retrieve ModuleUnloadImage names
     * 
     * @returns ModuleUnloadImage name list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public static String[] getModuleUnloadImageArray() {
        String[] xPath = new String[] { "/Extern/ModuleUnloadImage" };

        Object[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            String[] stringArray = new String[returns.length];
            CNameType[] doc = (CNameType[]) returns;

            for (int i = 0; i < returns.length; ++i) {
                stringArray[i] = doc[i].getStringValue();
            }

            return stringArray;
        }

        return null;
    }

    /**
     * Retrieve Extern
     * 
     * @returns Extern objects list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static ExternsDocument.Externs.Extern[] getExternArray() {
        String[] xPath = new String[] { "/Extern" };

        Object[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            return (ExternsDocument.Externs.Extern[]) returns;
        }

        return null;
    }

    /**
     * Retrieve PpiNotify for specified arch
     * 
     * @param arch
     *            PpiNotify arch
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getPpiNotifyArray(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            xPath = new String[] { "/PpiNotify" };
        }

        Object[] returns = get("PPIs", xPath);
        if (returns == null) {
            return new String[0];
        }

        
        List<String> ppiNotifyList = new ArrayList<String>();
        for (int i = 0; i < returns.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = ((PPIsDocument.PPIs.PpiNotify) returns[i]).getSupArchList();
            if (archList == null || archList.contains(arch)){
                ppiNotifyList.add(((PPIsDocument.PPIs.PpiNotify) returns[i]).getPpiNotifyCName()); 
            }
            
        }
        String[] ppiNotifyArray = new String[ppiNotifyList.size()];
        for (int i = 0; i < ppiNotifyList.size(); i++) {
            ppiNotifyArray[i] = ppiNotifyList.get(i);
        }

        return ppiNotifyArray;
    }

    /**
     * Retrieve PpiNotify for specified usage and arch
     * 
     * @param arch
     *            PpiNotify arch usage PpiNotify usage
     * 
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getPpiNotifyArray(String arch, String usage) {

        String[] xPath;
        String usageXpath;
        String archXpath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            archXpath = "/PpiNotify";
            if (usage != null && !usage.equals("")) {
                usageXpath = "/PpiNotify[@Usage='" + arch + "']";
                xPath = new String[] { archXpath, usageXpath };
            } else {
                return getProtocolNotifyArray(arch);
            }
        }

        Object[] returns = get("PPIs", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] ppiNotifyList = new String[returns.length];

        for (int i = 0; i < returns.length; i++) {
            ppiNotifyList[i] = ((PPIsDocument.PPIs.PpiNotify) returns[i]).getPpiNotifyCName();
        }
        return ppiNotifyList;
    }

    /**
     * Retrieve Ppi for specified arch
     * 
     * @param arch
     *            Ppi arch
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getPpiArray(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            xPath = new String[] { "/Ppi" };
        }

        Object[] returns = get("PPIs", xPath);
        if (returns == null) {
            return new String[0];
        }

        List<String> ppiList = new ArrayList<String>();
        for (int i = 0; i < returns.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = ((PPIsDocument.PPIs.Ppi) returns[i]).getSupArchList();
            if (archList == null || archList.contains(arch)){
                ppiList.add(((PPIsDocument.PPIs.Ppi) returns[i]).getPpiCName());    
            }
            
        }
        String[] ppiArray = new String[ppiList.size()];
        for (int i = 0; i < ppiList.size(); i++) {
            ppiArray[i] = ppiList.get(i);
        }
        return ppiArray;
    }

    /**
     * Retrieve PpiNotify for specified usage and arch
     * 
     * @param arch
     *            PpiNotify arch usage PpiNotify usage
     * 
     * 
     * @returns String[] if elements are found at the known xpath
     * @returns String[0] if nothing is there
     */
    public static String[] getPpiArray(String arch, String usage) {

        String[] xPath;
        String usageXpath;
        String archXpath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            archXpath = "/Ppi";
            if (usage != null && !usage.equals("")) {
                usageXpath = "/Ppi[@Usage='" + arch + "']";
                xPath = new String[] { archXpath, usageXpath };
            } else {
                return getProtocolNotifyArray(arch);
            }
        }

        Object[] returns = get("PPIs", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] ppiList = new String[returns.length];

        for (int i = 0; i < returns.length; i++) {
            ppiList[i] = ((PPIsDocument.PPIs.Ppi) returns[i]).getPpiCName();
        }
        return ppiList;
    }

    /**
     * Retrieve GuidEntry information for specified usage
     * 
     * @param arch
     *            GuidEntry arch
     * 
     * @returns GuidEntry objects list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getGuidEntryArray(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            xPath = new String[] { "/GuidCNames" };
        } else {
            xPath = new String[] { "/GuidCNames" };
        }

        Object[] returns = get("Guids", xPath);
        if (returns == null) {
            return new String[0];
        }

        List<String> guidList = new ArrayList<String>();
        for (int i = 0; i < returns.length; i++) {
            @SuppressWarnings("unchecked")
            List<String> archList = ((GuidsDocument.Guids.GuidCNames) returns[i]).getSupArchList();
            if (archList == null || archList.contains(arch)){
                guidList.add(((GuidsDocument.Guids.GuidCNames) returns[i]).getGuidCName());    
            }
            
        }
        String[] guidArray = new String[guidList.size()];
        for (int i = 0; i < guidList.size(); i++) {
            guidArray[i] = guidList.get(i);
        }
        return guidArray;

    }

    /**
     * Retrieve GuidEntry information for specified usage
     * 
     * @param arch
     *            GuidEntry arch usage GuidEntry usage
     * 
     * @returns GuidEntry objects list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getGuidEntryArray(String arch, String usage) {
        String[] xPath;
        String archXpath;
        String usageXpath;

        if (arch == null || arch.equals("")) {
            return new String[0];
        } else {
            archXpath = "/GuidEntry";
            if (usage != null && !usage.equals("")) {
                usageXpath = "/GuidEntry[@Usage='" + arch + "']";
                xPath = new String[] { archXpath, usageXpath };
            } else {
                return getProtocolNotifyArray(arch);
            }
        }

        Object[] returns = get("Guids", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] guidList = new String[returns.length];

        for (int i = 0; i < returns.length; i++) {
            guidList[i] = ((GuidsDocument.Guids.GuidCNames) returns[i]).getGuidCName();
        }
        return guidList;
    }

    /**
     * Retrieve Library instance information
     * 
     * @param arch
     *            Architecture name
     * @param usage
     *            Library instance usage
     * 
     * @returns library instance name list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public static ModuleIdentification[] getLibraryInstance(String arch) {
        String[] xPath;
        String saGuid = null;
        String saVersion = null;
        String pkgGuid = null;
        String pkgVersion = null;

        if (arch == null || arch.equalsIgnoreCase("")) {
            xPath = new String[] { "/Instance" };
        } else {
            //
            // Since Schema don't have SupArchList now, so the follow Xpath is 
            // equal to "/Instance" and [not(@SupArchList) or @SupArchList= arch]
            // don't have effect.
            //
            xPath = new String[] { "/Instance[not(@SupArchList) or @SupArchList='"
                    + arch + "']" };
        }

        Object[] returns = get("Libraries", xPath);
        if (returns == null || returns.length == 0) {
            return new ModuleIdentification[0];
        }

        ModuleIdentification[] saIdList = new ModuleIdentification[returns.length];
        for (int i = 0; i < returns.length; i++) {
            LibrariesDocument.Libraries.Instance library = (LibrariesDocument.Libraries.Instance) returns[i];
            saGuid = library.getModuleGuid();
            saVersion = library.getModuleVersion();

            pkgGuid = library.getPackageGuid();
            pkgVersion = library.getPackageVersion();

            ModuleIdentification saId = new ModuleIdentification(null, saGuid,
                    saVersion);
            PackageIdentification pkgId = new PackageIdentification(null,
                    pkgGuid, pkgVersion);
            saId.setPackage(pkgId);

            saIdList[i] = saId;

        }
        return saIdList;
    }

    // /
    // / This method is used for retrieving the elements information which has
    // / CName sub-element
    // /
    private static String[] getCNames(String from, String xPath[]) {
        Object[] returns = get(from, xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }

        String[] strings = new String[returns.length];
        for (int i = 0; i < returns.length; ++i) {
            // TBD
             strings[i] = ((CNameType) returns[i]).getStringValue();
        }

        return strings;
    }

    /**
     * Retrive library's constructor name
     * 
     * @returns constructor name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String getLibConstructorName() {
        String[] xPath = new String[] { "/Extern/Constructor" };

        Object[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            CNameType constructor = ((CNameType) returns[0]);
            return constructor.getStringValue();
        }

        return null;
    }

    /**
     * Retrive library's destructor name
     * 
     * @returns destructor name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String getLibDestructorName() {
        String[] xPath = new String[] { "/Extern/Destructor" };

        Object[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            //
            // Only support one Destructor function.
            //
             CNameType destructor = (CNameType) returns[0];
             return destructor.getStringValue();
        }

        return null;
    }

    /**
     * Retrive DriverBinding names
     * 
     * @returns DriverBinding name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getDriverBindingArray() {
        String[] xPath = new String[] { "/Extern/DriverBinding" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive ComponentName names
     * 
     * @returns ComponentName name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getComponentNameArray() {
        String[] xPath = new String[] { "/Extern/ComponentName" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive DriverConfig names
     * 
     * @returns DriverConfig name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getDriverConfigArray() {
        String[] xPath = new String[] { "/Extern/DriverConfig" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive DriverDiag names
     * 
     * @returns DriverDiag name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String[] getDriverDiagArray() {
        String[] xPath = new String[] { "/Extern/DriverDiag" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive SetVirtualAddressMapCallBack names
     * 
     * @returns SetVirtualAddressMapCallBack name list if elements are found at
     *          the known xpath
     * @returns null if nothing is there
     */
    public static String[] getSetVirtualAddressMapCallBackArray() {
        String[] xPath = new String[] { "/Extern/SetVirtualAddressMapCallBack" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive ExitBootServicesCallBack names
     * 
     * @returns ExitBootServicesCallBack name list if elements are found at the
     *          known xpath
     * @returns null if nothing is there
     */
    public static String[] getExitBootServicesCallBackArray() {
        String[] xPath = new String[] { "/Extern/ExitBootServicesCallBack" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrieve module surface area file information
     * 
     * @returns ModuleSA objects list if elements are found at the known xpath
     * @returns Empty ModuleSA list if nothing is there
     */
    public static Map<FpdModuleIdentification, Map<String, XmlObject>> getFpdModules() {
        String[] xPath = new String[] { "/FrameworkModules/ModuleSA" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        String arch = null;
        String fvBinding = null;
        String saGuid = null;
        String saVersion = null;
        String pkgGuid = null;
        String pkgVersion = null;

        Map<FpdModuleIdentification, Map<String, XmlObject>> fpdModuleMap = new LinkedHashMap<FpdModuleIdentification, Map<String, XmlObject>>();

        if (result == null) {
            return fpdModuleMap;
        }

        for (int i = 0; i < result.length; i++) {
            //
            // Get Fpd SA Module element node and add to ObjectMap.
            //
            Map<String, XmlObject> ObjectMap = new HashMap<String, XmlObject>();
            ModuleSADocument.ModuleSA moduleSA = (ModuleSADocument.ModuleSA) result[i];
            if (((ModuleSADocument.ModuleSA) result[i]).getLibraries() != null) {
                ObjectMap.put("Libraries", moduleSA.getLibraries());
            }
            if (((ModuleSADocument.ModuleSA) result[i]).getPcdBuildDefinition() != null) {
                ObjectMap.put("PcdBuildDefinition", moduleSA
                        .getPcdBuildDefinition());
            }
            if (((ModuleSADocument.ModuleSA) result[i])
                    .getModuleSaBuildOptions() != null) {
                ObjectMap.put("ModuleSaBuildOptions", moduleSA
                        .getModuleSaBuildOptions());
            }

            //
            // Get Fpd SA Module attribute and create FpdMoudleIdentification.
            //
            arch = moduleSA.getSupArchList().toString();

            // TBD
            fvBinding = null;
            saVersion = ((ModuleSADocument.ModuleSA) result[i])
                    .getModuleVersion();

            saGuid = moduleSA.getModuleGuid();
            pkgGuid = moduleSA.getPackageGuid();
            pkgVersion = moduleSA.getPackageVersion();

            //
            // Create Module Identification which have class member of package
            // identification.
            //
            PackageIdentification pkgId = new PackageIdentification(null,
                    pkgGuid, pkgVersion);
            ModuleIdentification saId = new ModuleIdentification(null, saGuid,
                    saVersion);

            saId.setPackage(pkgId);

            //
            // Create FpdModule Identification which have class member of module
            // identification
            //
            if (arch != null) {
                String[] archList = arch.split(" ");
                for (int j = 0; j < archList.length; j++) {
                    FpdModuleIdentification fpdSaId = new FpdModuleIdentification(saId,    archList[j]);
        
                    if (fvBinding != null) {
                        fpdSaId.setFvBinding(fvBinding);
                    }
        
                    //
                    // Put element to Map<FpdModuleIdentification, Map<String,
                    // Object>>.
                    //
                    fpdModuleMap.put(fpdSaId, ObjectMap);
                }
            }
        }
        return fpdModuleMap;
    }

    /**
     * Retrieve valid image names
     * 
     * @returns valid iamges name list if elements are found at the known xpath
     * @returns empty list if nothing is there
     */
    public static String[] getFpdValidImageNames() {
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='ImageName']/FvImageNames" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null) {
            return new String[0];
        }

        String[] result = new String[queryResult.length];
        for (int i = 0; i < queryResult.length; i++) {
            result[i] = ((XmlString) queryResult[i]).getStringValue();
        }

        return result;
    }
    
    public static Node getFpdUserExtension() {
        String[] xPath = new String[] { "/UserExtensions[@UserID='TianoCore']" }; 

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)queryResult[0];
        
        return a.getDomNode();
    }

    /**
     * Retrieve FV image option information
     * 
     * @param fvName
     *            FV image name
     * 
     * @returns option name/value list if elements are found at the known xpath
     * @returns empty list if nothing is there
     */
    public static String[][] getFpdOptions(String fvName) {
           String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Options' and ./FvImageNames='"
                      + fvName.toUpperCase() + "']/FvImageOptions" };
           Object[] queryResult = get("PlatformSurfaceArea", xPath);
           if (queryResult == null) {
                 return new String[0][];
           }
           ArrayList<String[]> list = new ArrayList<String[]>();
           for (int i = 0; i < queryResult.length; i++) {
               FvImagesDocument.FvImages.FvImage.FvImageOptions item = (FvImagesDocument.FvImages.FvImage.FvImageOptions) queryResult[i];
               List<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> namevalues = item
               .getNameValueList();
               Iterator iter = namevalues.iterator();
               while (iter.hasNext()) {
                   FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nvItem = (FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue) iter
                   .next();
                   list.add(new String[] { nvItem.getName(), nvItem.getValue() });
               }
           }
          String[][] result = new String[list.size()][2];
          for (int i = 0; i < list.size(); i++) {
              result[i][0] = list.get(i)[0];
              result[i][1] = list.get(i)[1];
          }
          return result;

    }

    public static XmlObject getFpdBuildOptions() {
        String[] xPath = new String[] { "/BuildOptions" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);

        if (queryResult == null || queryResult.length == 0) {
            return null;
        }
        return (XmlObject)queryResult[0];
    }

    public static PlatformIdentification getFpdHeader() {
        String[] xPath = new String[] { "/PlatformHeader" };

        Object[] returns = get("PlatformSurfaceArea", xPath);

        if (returns == null || returns.length == 0) {
            return null;
        }
        PlatformHeaderDocument.PlatformHeader header = (PlatformHeaderDocument.PlatformHeader) returns[0];

        String name = header.getPlatformName();

        String guid = header.getGuidValue();

        String version = header.getVersion();

        return new PlatformIdentification(name, guid, version);
    }

    /**
     * Retrieve FV image attributes information
     * 
     * @param fvName
     *            FV image name
     * 
     * @returns attribute name/value list if elements are found at the known
     *          xpath
     * @returns empty list if nothing is there
     */
    public static String[][] getFpdAttributes(String fvName) {
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Attributes' and ./FvImageNames='"
                         + fvName.toUpperCase() + "']/FvImageOptions" };
        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null) {
             return new String[0][];
        }
        ArrayList<String[]> list = new ArrayList<String[]>();
        for (int i = 0; i < queryResult.length; i++) {
            
            FvImagesDocument.FvImages.FvImage.FvImageOptions item = (FvImagesDocument.FvImages.FvImage.FvImageOptions) queryResult[i];
            List<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> namevalues = item.getNameValueList();
            Iterator iter = namevalues.iterator();
            while (iter.hasNext()) {
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nvItem = (FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue) iter
                 .next();
                 list.add(new String[] { nvItem.getName(), nvItem.getValue() });
            }
        }
       String[][] result = new String[list.size()][2];
       for (int i = 0; i < list.size(); i++) {
             result[i][0] = list.get(i)[0];
             result[i][1] = list.get(i)[1];
       }
       return result;
    }

    /**
     * Retrieve flash definition file name
     * 
     * @returns file name if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String getFlashDefinitionFile() {
        String[] xPath = new String[] { "/PlatformDefinitions/FlashDeviceDefinitions/FlashDefinitionFile" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }

        FileNameConvention filename = (FileNameConvention) queryResult[queryResult.length - 1];
        return filename.getStringValue();
    }

    public static String[][] getFpdGlobalVariable() {
        String[] xPath = new String[] { "/Flash/FvImages/NameValue" };
        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        String[][] result = new String[queryResult.length][2];
        
        for (int i = 0; i < queryResult.length; i++) {
            FvImagesDocument.FvImages.NameValue item = (FvImagesDocument.FvImages.NameValue)queryResult[i];
            result[i][0] = item.getName();
            result[i][1] = item.getValue();
        }
        return result;   
    }
    
    /**
     * Retrieve FV image component options
     * 
     * @param fvName
     *            FV image name
     * 
     * @returns name/value pairs list if elements are found at the known xpath
     * @returns empty list if nothing is there
     */
    public static String[][] getFpdComponents(String fvName) {
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Components' and ./FvImageNames='"+ fvName.toUpperCase() + "']/FvImageOptions" };
        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        ArrayList<String[]> list = new ArrayList<String[]>();
        for (int i = 0; i < queryResult.length; i++) {
        FvImagesDocument.FvImages.FvImage.FvImageOptions item = (FvImagesDocument.FvImages.FvImage.FvImageOptions) queryResult[i];
        List<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> namevalues = item.getNameValueList();
        Iterator iter = namevalues.iterator();
        while (iter.hasNext()) {
            FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nvItem = (FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue) iter
                        .next();
                   list.add(new String[] { nvItem.getName(), nvItem.getValue() });
                 }
                }
         String[][] result = new String[list.size()][2];
                for (int i = 0; i < list.size(); i++) {
               result[i][0] = list.get(i)[0];
             result[i][1] = list.get(i)[1];
        }
            return result;  
    }

    /**
     * Retrieve PCD tokens
     * 
     * @returns CName/ItemType pairs list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public static String[][] getPcdTokenArray() {
        String[] xPath = new String[] { "/PcdData" };

        Object[] returns = get("PCDs", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }

        // PcdCoded.PcdData[] pcds = (PcdCoded.PcdData[]) returns;
        // String[][] result = new String[pcds.length][2];
        // for (int i = 0; i < returns.length; ++i) {
        // if (pcds[i].getItemType() != null) {
        // result[i][1] = pcds[i].getItemType().toString();
        // } else {
        // result[i][1] = null;
        // }
        // result[i][0] = pcds[i].getCName();
        // }

        return null;
    }

    /**
     * Get the PcdToken array from module's surface area document. The array
     * should contains following data:
     * <p>
     * -------------------------------------------------------------------
     * </p>
     * <p>
     * CName | ItemType | TokenspaceName | DefaultValue | Usage | HelpText
     * </p>
     * <p>
     * -------------------------------------------------------------------
     * </p>
     * <p>
     * Note: Until new schema applying, now we can only get CName, ItemType,
     * </p>
     * 
     * @return 2-array table contains all information of PCD token retrieved
     *         from MSA.
     */
    public static Object[][] etModulePCDTokenArray() {
        return null;
        // int index;
        // Object[][] result;
        // PCDs.PcdData[] pcds;
        // String[] xPath = new String[] { "/PcdData" };
        // Object[] returns = get("PCDs", xPath);
        //
        // if ((returns == null) || (returns.length == 0)) {
        // return null;
        // }
        //
        // pcds = (PCDs.PcdData[]) returns;
        // result = new Object[pcds.length][6];
        // for (index = 0; index < pcds.length; index++) {
        // //
        // // Get CName
        // //
        // result[index][0] = pcds[index].getCName();
        // //
        // // Get ItemType: FEATURE_FLAG, FIXED_AT_BUILD, PATCHABLE_IN_MODLE,
        // // DYNAMIC, DYNAMIC_EX
        // //
        // if (pcds[index].getItemType() != null) {
        // result[index][1] = pcds[index].getItemType().toString();
        // } else {
        // result[index][1] = null;
        // }
        //
        // //
        // // BUGBUG: following field can *not* be got from current MSA until
        // // schema changed.
        // //
        // // result [index][2] = pcds[index].getTokenSpaceName();
        // result[index][2] = null;
        // result[index][3] = pcds[index].getDefaultValue();
        // // result [index][4] = pcds[index].getUsage ();
        // result[index][4] = null;
        // // result [index][5] = pcds[index].getHelpText ();
        // result[index][5] = null;
        // }
        // return result;
    }

    /**
     * Retrieve MAS header
     * 
     * @return
     * @return
     */
    public static ModuleIdentification getMsaHeader() {
        String[] xPath = new String[] { "/" };
        Object[] returns = get("MsaHeader", xPath);

        if (returns == null || returns.length == 0) {
            return null;
        }

        MsaHeader msaHeader = (MsaHeader) returns[0];
        //
        // Get BaseName, ModuleType, GuidValue, Version
        // which in MsaHeader.
        //
        String name = msaHeader.getModuleName();
        String moduleType = msaHeader.getModuleType().toString();
        String guid = msaHeader.getGuidValue();
        String version = msaHeader.getVersion();

        ModuleIdentification moduleId = new ModuleIdentification(name, guid,
                version);

        moduleId.setModuleType(moduleType);

        return moduleId;
    }

    /**
     * Retrieve Extern Specification
     * 
     * @param
     * 
     * @return String[] If have specification element in the <extern> String[0]
     *         If no specification element in the <extern>
     * 
     */

    public static String[] getExternSpecificaiton() {
        String[] xPath = new String[] { "/Specification" };

        Object[] queryResult = get("Externs", xPath);
        if (queryResult == null) {
            return new String[0];
        }

        String[] specificationList = new String[queryResult.length];
        for (int i = 0; i < queryResult.length; i++) {
             specificationList[i] = ((Sentence)queryResult[i])
             .getStringValue();
        }
        return specificationList;
    }

    /**
     * Retreive MsaFile which in SPD
     * 
     * @param
     * @return String[][3] The string sequence is ModuleName, ModuleGuid,
     *         ModuleVersion, MsaFile String[0][] If no msafile in SPD
     */
    public static String[] getSpdMsaFile() {
        String[] xPath = new String[] { "/MsaFiles" };

        Object[] returns = get("PackageSurfaceArea", xPath);
        if (returns == null) {
            return new String[0];
        }

        List<String> filenameList = ((MsaFilesDocument.MsaFiles) returns[0])
                .getFilenameList();
        return filenameList.toArray(new String[filenameList.size()]);
    }

    /**
     * Reteive
     */
    public static Map<String, String[]> getSpdLibraryClasses() {
        String[] xPath = new String[] { "/LibraryClassDeclarations/LibraryClass" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - LibraryClass, String[] - LibraryClass Header file.
        //
        Map<String, String[]> libClassHeaderMap = new HashMap<String, String[]>();

        if (returns == null) {
            return libClassHeaderMap;
        }

        for (int i = 0; i < returns.length; i++) {
            LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass library = (LibraryClassDeclarationsDocument.LibraryClassDeclarations.LibraryClass) returns[i];
            libClassHeaderMap.put(library.getName(), new String[] { library
                    .getIncludeHeader() });
        }
        return libClassHeaderMap;
    }

    /**
     * Reteive
     */
    public static Map<String, String> getSpdPackageHeaderFiles() {
        String[] xPath = new String[] { "/PackageHeaders/IncludePkgHeader" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - ModuleType, String - PackageInclude Header file.
        //
        Map<String, String> packageIncludeMap = new HashMap<String, String>();

        if (returns == null) {
            return packageIncludeMap;
        }
//        GlobalData.log.info("" + returns[0].getClass().getName());
        for (int i = 0; i < returns.length; i++) {
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader includeHeader = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader) returns[i];
            packageIncludeMap.put(includeHeader.getModuleType().toString(),
                    includeHeader.getStringValue());
        }
        return packageIncludeMap;
    }

    public static PackageIdentification getSpdHeader() {
        String[] xPath = new String[] { "/SpdHeader" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        if (returns == null || returns.length == 0) {
            return null;
        }

        SpdHeaderDocument.SpdHeader header = (SpdHeaderDocument.SpdHeader) returns[0];

        String name = header.getPackageName();

        String guid = header.getGuidValue();

        String version = header.getVersion();

        return new PackageIdentification(name, guid, version);
    }

    /**
     * Reteive
     */
    public static Map<String, String[]> getSpdGuid() {
        String[] xPath = new String[] { "/GuidDeclarations/Entry" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - GuidName, String[] - C_NAME & GUID value.
        //
        Map<String, String[]> guidDeclMap = new HashMap<String, String[]>();
        if (returns == null) {
            return guidDeclMap;
        }

        for (int i = 0; i < returns.length; i++) {
            GuidDeclarationsDocument.GuidDeclarations.Entry entry = (GuidDeclarationsDocument.GuidDeclarations.Entry) returns[i];
            String[] guidPair = new String[2];
            guidPair[0] = entry.getCName();
            guidPair[1] = entry.getGuidValue();
            guidDeclMap.put(entry.getName(), guidPair);
            EdkLog.log(EdkLog.EDK_VERBOSE, entry.getName());
            EdkLog.log(EdkLog.EDK_VERBOSE, guidPair[0]);
            EdkLog.log(EdkLog.EDK_VERBOSE, guidPair[1]);
        }
        return guidDeclMap;
    }

    /**
     * Reteive
     */
    public static Map<String, String[]> getSpdProtocol() {
        String[] xPath = new String[] { "/ProtocolDeclarations/Entry" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - protocolName, String[] - C_NAME & GUID value.
        //
        Map<String, String[]> protoclMap = new HashMap<String, String[]>();

        if (returns == null) {
            return protoclMap;
        }

        for (int i = 0; i < returns.length; i++) {
            ProtocolDeclarationsDocument.ProtocolDeclarations.Entry entry = (ProtocolDeclarationsDocument.ProtocolDeclarations.Entry) returns[i];
            String[] protocolPair = new String[2];

            protocolPair[0] = entry.getCName();
            protocolPair[1] = entry.getGuidValue();
            protoclMap.put(entry.getName(), protocolPair);
            EdkLog.log(EdkLog.EDK_VERBOSE, entry.getName());
            EdkLog.log(EdkLog.EDK_VERBOSE, protocolPair[0]);
            EdkLog.log(EdkLog.EDK_VERBOSE, protocolPair[1]);
        }
        return protoclMap;
    }

    /**
     * getSpdPpi() Retrieve the SPD PPI Entry
     * 
     * @param
     * @return Map<String, String[2]> if get the PPI entry from SPD. Key - PPI
     *         Name String[0] - PPI CNAME String[1] - PPI Guid Null if no PPI
     *         entry in SPD.
     */
    public static Map<String, String[]> getSpdPpi() {
        String[] xPath = new String[] { "/PpiDeclarations/Entry" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - protocolName, String[] - C_NAME & GUID value.
        //
        Map<String, String[]> ppiMap = new HashMap<String, String[]>();

        if (returns == null) {
            return ppiMap;
        }

        for (int i = 0; i < returns.length; i++) {
            PpiDeclarationsDocument.PpiDeclarations.Entry entry = (PpiDeclarationsDocument.PpiDeclarations.Entry) returns[i];
            String[] ppiPair = new String[2];
            ppiPair[0] = entry.getCName();
            ppiPair[1] = entry.getGuidValue();
            ppiMap.put(entry.getName(), ppiPair);
        }
        return ppiMap;
    }

    /**
     * getToolChainFamily
     * 
     * This function is to retrieve ToolChainFamily attribute of FPD
     * <BuildOptions>
     * 
     * @param
     * @return toolChainFamily If find toolChainFamily attribute in
     *         <BuildOptions> Null If don't have toolChainFamily in
     *         <BuildOptions>.
     */
    public String getToolChainFamily() {
        String toolChainFamily;
        String[] xPath = new String[] { "/BuildOptions" };

        Object[] result = get("PlatformSurfaceArea", xPath);
        if (result == null) {
            return null;
        }
        // toolChainFamily =
        // ((BuildOptionsDocument.BuildOptions)result[0]).getToolChainFamilies();
        // return toolChainFamily;
        return null;
    }

    /**
     * Retrieve module Guid string
     * 
     * @returns GUILD string if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public static String getModuleGuid() {
        String[] xPath = new String[] { "" };

        Object[] returns = get("MsaHeader", xPath);
        if (returns != null && returns.length > 0) {
            String guid = ((MsaHeaderDocument.MsaHeader) returns[0])
                    .getGuidValue();
            return guid;
        }

        return null;
    }

    //
    // For new Pcd
    //
    public static ModuleSADocument.ModuleSA[] getFpdModuleSAs() {
        String[] xPath = new String[] { "/FrameworkModules/ModuleSA" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        if (result != null) {
            return (ModuleSADocument.ModuleSA[]) result;
        }
        return new ModuleSADocument.ModuleSA[0];

    }
    /**
    Get name array of PCD in a module. In one module, token space
    is same, and token name should not be conflicted.
    
    @return String[]
 **/
 public static String[] getModulePcdEntryNameArray() {
     PcdCodedDocument.PcdCoded.PcdEntry[] pcdEntries  = null;
     String[]            results;
     int                 index;
     String[]            xPath       = new String[] {"/PcdEntry"};
     Object[]         returns     = get ("PcdCoded", xPath);
     
     if (returns == null) {
         return new String[0];
     }
     
     pcdEntries = (PcdCodedDocument.PcdCoded.PcdEntry[])returns;
     results    = new String[pcdEntries.length];
     
     for (index = 0; index < pcdEntries.length; index ++) {
         results[index] = pcdEntries[index].getCName();
     }
     return results;
 }
}
