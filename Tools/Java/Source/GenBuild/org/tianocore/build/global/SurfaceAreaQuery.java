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
import org.tianocore.*;
import org.tianocore.ExternsDocument.Externs.Extern;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.ModuleDefinitionsDocument.ModuleDefinitions;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.ProtocolsDocument.Protocols.Protocol;
import org.tianocore.ProtocolsDocument.Protocols.ProtocolNotify;
import org.tianocore.build.autogen.CommonDefinition;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.id.PlatformIdentification;
import org.tianocore.build.toolchain.ToolChainInfo;
import org.tianocore.common.definitions.EdkDefinitions;
import org.tianocore.common.exception.EdkException;
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

    public String prefix = "http://www.TianoCore.org/2006/Edk2.0";

    //
    // Contains name/value pairs of Surface Area document object. The name is
    // always the top level element name.
    //
    private Map<String, XmlObject> map = null;

    //
    // mapStack is used to do nested query
    //
    private Stack<Map<String, XmlObject>> mapStack = new Stack<Map<String, XmlObject>>();

    //
    // prefix of name space
    //
    private String nsPrefix = "sans";

    //
    // xmlbeans needs a name space for each Xpath element
    //
    private String ns = null;

    //
    // keep the namep declaration for xmlbeans Xpath query
    //
    private String queryDeclaration = null;    
    private StringBuffer normQueryString = new StringBuffer(4096);
    private Pattern xPathPattern = Pattern.compile("([^/]*)(/|//)([^/]+)");

    /**
     * Set a Surface Area document for query later
     *
     * @param map
     *            A Surface Area document in TopLevelElementName/XmlObject
     *            format.
     */
    public SurfaceAreaQuery(Map<String, XmlObject> map) {
        ns = prefix;
        queryDeclaration = "declare namespace " + nsPrefix + "='" + ns + "'; ";
        this.map = map;
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
    public void push(Map<String, XmlObject> newMap) {
        mapStack.push(this.map);
        this.map = newMap;
    }

    /**
     * Discard current used Surface Area document and use the top document in
     * stack instead.
     */
    public void pop() {
        this.map = mapStack.pop();
    }

    // /
    // / Convert xPath to be namespace qualified, which is necessary for
    // XmlBeans
    // / selectPath(). For example, converting /MsaHeader/ModuleType to
    // / /ns:MsaHeader/ns:ModuleType
    // /
    private String normalizeQueryString(String[] exp, String from) {
        normQueryString.setLength(0);

        int i = 0;
        while (i < exp.length) {
            String newExp = from + exp[i];
            Matcher matcher = xPathPattern.matcher(newExp);

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
    public Object[] get(String[] xPath) {
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
    public Object[] get(String rootName, String[] xPath) {
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
    public String[][] getSourceFiles(String arch) {
        String[] xPath;
        Object[] returns;

        xPath = new String[] { "/Filename" };

        returns = get("SourceFiles", xPath);

        if (returns == null || returns.length == 0) {
            return new String[0][3];
        }

        Filename[] sourceFileNames = (Filename[]) returns;
        List<String[]> outputList = new ArrayList<String[]>();
        for (int i = 0; i < sourceFileNames.length; i++) {
            List archList = sourceFileNames[i].getSupArchList();
            if (arch == null || arch.trim().equalsIgnoreCase("") || archList == null || contains(archList, arch)) {
                outputList.add(new String[] {sourceFileNames[i].getToolCode(), sourceFileNames[i].getStringValue(), sourceFileNames[i].getToolChainFamily()});
            }
        }

        String[][] outputString = new String[outputList.size()][3];
        for (int index = 0; index < outputList.size(); index++) {
            //
            // ToolCode (FileType)
            //
            outputString[index][0] = outputList.get(index)[0];
            //
            // File name (relative to MODULE_DIR)
            //
            outputString[index][1] = outputList.get(index)[1];
            //
            // Tool chain family
            //
            outputString[index][2] = outputList.get(index)[2];
        }
        return outputString;
    }

    /**
     * Retrieve /PlatformDefinitions/OutputDirectory from FPD
     *
     * @returns Directory names array if elements are found at the known xpath
     * @returns Empty if nothing is found at the known xpath
     */
    public String getFpdOutputDirectory() {
        String[] xPath = new String[] { "/PlatformDefinitions" };

        Object[] returns = get("PlatformSurfaceArea", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        PlatformDefinitionsDocument.PlatformDefinitions item = (PlatformDefinitionsDocument.PlatformDefinitions)returns[0];
        return item.getOutputDirectory();
    }

    public String getFpdIntermediateDirectories() {
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

    public String getModuleFfsKeyword() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleSaBuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleSaBuildOptionsDocument.ModuleSaBuildOptions item = (ModuleSaBuildOptionsDocument.ModuleSaBuildOptions)returns[0];
        return item.getFfsFormatKey();
    }

    public String getModuleFvBindingKeyword() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleSaBuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleSaBuildOptionsDocument.ModuleSaBuildOptions item = (ModuleSaBuildOptionsDocument.ModuleSaBuildOptions)returns[0];
        return item.getFvBinding();
    }

    public List getModuleSupportedArchs() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleDefinitions", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        ModuleDefinitionsDocument.ModuleDefinitions item = (ModuleDefinitionsDocument.ModuleDefinitions)returns[0];
        return item.getSupportedArchitectures();
    }

    public BuildOptionsDocument.BuildOptions.Ffs[] getFpdFfs() {
        String[] xPath = new String[] {"/Ffs"};

        Object[] returns = get("BuildOptions", xPath);
        if (returns == null || returns.length == 0) {
            return new BuildOptionsDocument.BuildOptions.Ffs[0];
        }
        return (BuildOptionsDocument.BuildOptions.Ffs[])returns;
    }

    public String getModuleOutputFileBasename() {
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
    public String[][] getOptions(String from, String[] xPath, boolean toolChainFamilyFlag) {
        String target = null;
        String toolchain = null;
        String toolchainFamily = null;
        List<String> archList = null;
        String cmd = null;
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
            List archEnumList = option.getSupArchList();
            if (archEnumList == null) {
                archList.add(null);
            } else {
                //archList.addAll(archEnumList);
                Iterator it = archEnumList.iterator();
                while (it.hasNext()) {
                    String archType = (String)it.next();
                    archList.add(archType);
                }
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

    public String[][] getModuleBuildOptions(boolean toolChainFamilyFlag) {
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

    public String[][] getPlatformBuildOptions(boolean toolChainFamilyFlag) {
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
    
    public String[][] getMsaBuildOptions(boolean toolChainFamilyFlag) {
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

        return getOptions("ModuleBuildOptions", xPath, toolChainFamilyFlag);
    }

    public ToolChainInfo getFpdToolChainInfo() {
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
    public String getModuleType() {
        String[] xPath = new String[] { "/ModuleType" };

        Object[] returns = get(xPath);
        if (returns != null && returns.length > 0) {
            ModuleTypeDef type = (ModuleTypeDef) returns[0];
            return type.enumValue().toString();
        }

        return null;
    }

    /**
     * Retrieve <ModuleDefinitions>/<BinaryModule>
     *
     * @returns The module type name if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public boolean getBinaryModule() {
        String[] xPath = new String[] { "/" };

        Object[] returns = get("ModuleDefinitions", xPath);
        if (returns != null && returns.length > 0) {
            ModuleDefinitionsDocument.ModuleDefinitions def = (ModuleDefinitionsDocument.ModuleDefinitions)returns[0];
            return def.getBinaryModule();
        }

        return false;
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
    public PackageIdentification[] getDependencePkg(String arch) throws EdkException {
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
            List archList = item.getSupArchList();
            if (arch == null || archList == null || contains(archList, arch)) {
                packageGuid = item.getPackageGuid();
                packageVersion = item.getPackageVersion();
                PackageIdentification pkgId = new PackageIdentification(null, packageGuid, packageVersion);
                GlobalData.refreshPackageIdentification(pkgId);
                packageIdList.add(pkgId);
            }
        }

        return packageIdList.toArray(new PackageIdentification[packageIdList.size()]);
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
    public String[] getLibraryClasses(String usage, String arch, String moduleType) {
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
        List<String> libraryClassName = new ArrayList<String>();
        for (int i = 0; i < libraryClassList.length; i++) {
			List archList = libraryClassList[i].getSupArchList();
            List moduleTypeList = libraryClassList[i].getSupModuleList();
			if ((arch == null || contains(archList, arch))
                && (moduleType == null || contains(moduleTypeList, moduleType))) {
                libraryClassName.add(libraryClassList[i].getKeyword());
			}
        }

        String[] libraryArray = new String[libraryClassName.size()];
        libraryClassName.toArray(libraryArray);
        return libraryArray;
    }

    /**
     * Retrieve ModuleEntryPoint names
     *
     * @returns ModuleEntryPoint name list if elements are found at the known
     *          xpath
     * @returns null if nothing is there
     */
    public String[] getModuleEntryPointArray() {
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
    public String[] getProtocolArray(String arch, String usage) {
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
    public String[] getProtocolArray(String arch) {
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
            List archList = returnlList[i].getSupArchList();
            if (archList == null || contains(archList, arch)){
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
    public String[] getProtocolNotifyArray(String arch) {
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
            List archList = ((ProtocolNotify) returns[i]).getSupArchList();
            if (archList == null || contains(archList, arch)){
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
    public String[] getProtocolNotifyArray(String arch, String usage) {

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
    public String[] getModuleUnloadImageArray() {
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
    public ExternsDocument.Externs.Extern[] getExternArray() {
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
    public String[] getPpiNotifyArray(String arch) {
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
            List archList = ((PPIsDocument.PPIs.PpiNotify) returns[i]).getSupArchList();
            if (archList == null || contains(archList, arch)){
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
    public String[] getPpiNotifyArray(String arch, String usage) {

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
    public String[] getPpiArray(String arch) {
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
            List archList = ((PPIsDocument.PPIs.Ppi) returns[i]).getSupArchList();
            if (archList == null || contains(archList, arch)){
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
    public String[] getPpiArray(String arch, String usage) {

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
    public String[] getGuidEntryArray(String arch) {
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
            List archList = ((GuidsDocument.Guids.GuidCNames) returns[i]).getSupArchList();
            if (archList == null || contains(archList, arch)){
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
    public String[] getGuidEntryArray(String arch, String usage) {
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
                return getGuidEntryArray(arch);
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

    public String[] getEventCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{
                "/CreateEvents/EventTypes[@EventGuidCName]",
                "/SignalEvents/EventTypes[@EventGuidCName]",
            };
        } else {
            xPath = new String[]{
                "/CreateEvents/EventTypes[@EventGuidCName and not(@SupArchList)]",
                "/SignalEvents/EventTypes[@EventGuidCName and not(@SupArchList)]",
                "/CreateEvents/EventTypes[@EventGuidCName and contains(@SupArchList,'" + arch + "')]",
                "/SignalEvents/EventTypes[@EventGuidCName and contains(@SupArchList,'" + arch + "')]",
            };
        }

        Object[] returns = get("Events", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            if (returns[i] instanceof EventsDocument.Events.CreateEvents.EventTypes) {
                cnameList[i] = ((EventsDocument.Events.CreateEvents.EventTypes) returns[i]).getEventGuidCName();
            } else {
                cnameList[i] = ((EventsDocument.Events.SignalEvents.EventTypes) returns[i]).getEventGuidCName();
            }
        }
        return cnameList;
    }

    public String[] getHobCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{"/HobTypes[@HobGuidCName]"};
        } else {
            xPath = new String[]{
                "/HobTypes[@HobGuidCName and not(@SupArchList)]",
                "/HobTypes[@HobGuidCName and contains(@SupArchList,'" + arch + "')]",
            };
        }

        Object[] returns = get("Hobs", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            cnameList[i] = ((HobsDocument.Hobs.HobTypes) returns[i]).getHobGuidCName();
        }
        return cnameList;
    }

    public String[] getVariableCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{"/Variable"};
        } else {
            xPath = new String[]{"/Variable[not(@SupArchList) or contains(@SupArchList,'" + arch + "')]"};
        }

        Object[] returns = get("Variables", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            cnameList[i] = ((VariablesDocument.Variables.Variable) returns[i]).getGuidCName();
        }
        return cnameList;
    }

    public String[] getSystemTableCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{"/SystemTableCNames"};
        } else {
            xPath = new String[]{
                "/SystemTableCNames[not(@SupArchList) or contains(@SupArchList,'" + arch + "')]"
            };
        }

        Object[] returns = get("SystemTables", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            cnameList[i] = ((SystemTablesDocument.SystemTables.SystemTableCNames) returns[i]).getSystemTableCName();
        }
        return cnameList;
    }

    public String[] getDataHubCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{"/DataHubRecord"};
        } else {
            xPath = new String[]{"/DataHubRecord[not(@SupArchList) or contains(@SupArchList,'" + arch + "')]"};
        }

        Object[] returns = get("DataHubs", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            cnameList[i] = ((DataHubsDocument.DataHubs.DataHubRecord) returns[i]).getDataHubCName();
        }
        return cnameList;
    }

    public String[] getHiiPackageCNameArray(String arch) {
        String[] xPath = null;

        if (arch == null || arch.equals("")) {
            xPath = new String[]{"/HiiPackage"};
        } else {
            xPath = new String[]{"/HiiPackage[not(@SupArchList) or contains(@SupArchList,'" + arch + "')]"};
        }

        Object[] returns = get("HiiPackages", xPath);
        if (returns == null) {
            return new String[0];
        }

        String[] cnameList = new String[returns.length];
        for (int i = 0; i < returns.length; i++) {
            cnameList[i] = ((HiiPackagesDocument.HiiPackages.HiiPackage) returns[i]).getHiiCName();
        }
        return cnameList;
    }

    public String[] getCNameArray(String arch) {
        List<String> cnameList = new ArrayList<String>(100);
        String[] result = null;
        // 
        // "/Guids/GuidCNames/GuidCName",
        // 
        result = getGuidEntryArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/Protocols/Protocol/ProtocolCName",
        // 
        result = getProtocolArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        //
        //  "/Protocols/ProtocolNotify/ProtocolNotifyCName",
        // 
        result = getProtocolNotifyArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/Events/CreateEvents/EventTypes[@EventGuidCName]",
        // "/Events/SignalEvents/EventTypes[@EventGuidCName]",
        // 
        result = getEventCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        //
        // "/Hobs/HobTypes[@HobGuidCName]",
        // 
        result = getHobCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/PPIs/Ppi/PpiCName",
        //
        result = getPpiArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/PPIs/PpiNotify/PpiNotifyCName",
        // 
        result = getPpiNotifyArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/Variables/Variable/GuidC_Name",
        // 
        result = getVariableCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        // 
        // "/SystemTables/SystemTableCNames/SystemTableCName",
        // 
        result = getSystemTableCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        //
        //  "/DataHubs/DataHubRecord/DataHubCName",
        // 
        result = getDataHubCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }
        //
        // "/HiiPackages/HiiPackage/HiiCName",
        // 
        result = getHiiPackageCNameArray(arch);
        for (int i = 0; i < result.length; ++i) {
            cnameList.add(result[i]);
        }

        return cnameList.toArray(new String[cnameList.size()]);
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
    public ModuleIdentification[] getLibraryInstance(String arch) throws EdkException {
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
            GlobalData.refreshPackageIdentification(pkgId);
            saId.setPackage(pkgId);
            GlobalData.refreshModuleIdentification(saId);

            saIdList[i] = saId;

        }
        return saIdList;
    }

    // /
    // / This method is used for retrieving the elements information which has
    // / CName sub-element
    // /
    private String[] getCNames(String from, String xPath[]) {
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
    public String getLibConstructorName() {
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
    public String getLibDestructorName() {
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
    public String[] getDriverBindingArray() {
        String[] xPath = new String[] { "/Extern/DriverBinding" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive ComponentName names
     *
     * @returns ComponentName name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public String[] getComponentNameArray() {
        String[] xPath = new String[] { "/Extern/ComponentName" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive DriverConfig names
     *
     * @returns DriverConfig name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public String[] getDriverConfigArray() {
        String[] xPath = new String[] { "/Extern/DriverConfig" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive DriverDiag names
     *
     * @returns DriverDiag name list if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public String[] getDriverDiagArray() {
        String[] xPath = new String[] { "/Extern/DriverDiag" };
        return getCNames("Externs", xPath);
    }

    /**
     * Retrive DriverBinding, ComponentName, DriverConfig,
     * DriverDiag group array
     * 
     * @returns DriverBinding group name list if elements are found
     *        at the known xpath
     * @returns null if nothing is there
     */
	public String[][] getExternProtocolGroup() {
		String[] xPath = new String[] {"/Extern"};
		Object[] returns = get("Externs",xPath);

        if (returns == null) {
			return new String[0][4];
		}
		List<Extern> externList = new ArrayList<Extern>();
		for (int i = 0; i < returns.length; i++) {
			org.tianocore.ExternsDocument.Externs.Extern extern = (org.tianocore.ExternsDocument.Externs.Extern)returns[i];
			if (extern.getDriverBinding() != null) {
				externList.add(extern);
			}
		}

		String[][] externGroup = new String[externList.size()][4];
		for (int i = 0; i < externList.size(); i++) {
            String driverBindingStr = externList.get(i).getDriverBinding();
			if ( driverBindingStr != null){
				externGroup[i][0] = driverBindingStr;
			} else {
				externGroup[i][0] = null;
			}

			String componentNameStr = externList.get(i).getComponentName();
			if (componentNameStr != null) {
				externGroup[i][1] = componentNameStr;
			} else {
				externGroup[i][1] = null;
			}

			String driverConfigStr = externList.get(i).getDriverConfig();
			if (driverConfigStr != null) {
				externGroup[i][2] = driverConfigStr;
			} else {
				externGroup[i][2] = null;
			}

			String driverDiagStr = externList.get(i).getDriverDiag();
			if (driverDiagStr != null) {
			    externGroup[i][3] = driverDiagStr;
			} else {
				externGroup[i][3] = null;
			}
		}
		return externGroup;
	}
    
    /**
     * Retrive SetVirtualAddressMapCallBack names
     *
     * @returns SetVirtualAddressMapCallBack name list if elements are found at
     *          the known xpath
     * @returns null if nothing is there
     */
    public String[] getSetVirtualAddressMapCallBackArray() {
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
    public String[] getExitBootServicesCallBackArray() {
        String[] xPath = new String[] { "/Extern/ExitBootServicesCallBack" };
        return getCNames("Externs", xPath);
    }

    /**
      Judge whether current driver is PEI_PCD_DRIVER or DXE_PCD_DRIVER or
      NOT_PCD_DRIVER.
      
      @return CommonDefinition.PCD_DRIVER_TYPE  the type of current driver
    **/
    public CommonDefinition.PCD_DRIVER_TYPE getPcdDriverType() {
        String[] xPath   = new String[] {"/PcdIsDriver"};
        Object[] results = get ("Externs", xPath);

        if (results != null && results.length != 0) {
            PcdDriverTypes type     = (PcdDriverTypes) results[0];
            String         typeStr  = type.enumValue().toString();
            if (typeStr.equals(CommonDefinition.PCD_DRIVER_TYPE.PEI_PCD_DRIVER.toString())) {
                return CommonDefinition.PCD_DRIVER_TYPE.PEI_PCD_DRIVER;
            } else if (typeStr.equals(CommonDefinition.PCD_DRIVER_TYPE.DXE_PCD_DRIVER.toString())) {
                return CommonDefinition.PCD_DRIVER_TYPE.DXE_PCD_DRIVER;
            }
            return CommonDefinition.PCD_DRIVER_TYPE.UNKNOWN_PCD_DRIVER;
        }

        return CommonDefinition.PCD_DRIVER_TYPE.NOT_PCD_DRIVER;
    }

    /**
     * Retrieve module surface area file information
     *
     * @returns ModuleSA objects list if elements are found at the known xpath
     * @returns Empty ModuleSA list if nothing is there
     */
    public Map<FpdModuleIdentification, Map<String, XmlObject>> getFpdModules() throws EdkException {
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
                ObjectMap.put("PcdBuildDefinition", moduleSA.getPcdBuildDefinition());
            }
            if (((ModuleSADocument.ModuleSA) result[i]).getModuleSaBuildOptions() != null) {
                ObjectMap.put("ModuleSaBuildOptions", moduleSA.getModuleSaBuildOptions());
            }

            //
            // Get Fpd SA Module attribute and create FpdMoudleIdentification.
            //
            if (moduleSA.isSetSupArchList()) {
                arch = moduleSA.getSupArchList().toString();
            } else {
                arch = null;
            }

            // TBD
            fvBinding = null;
            saVersion = ((ModuleSADocument.ModuleSA) result[i]).getModuleVersion();

            saGuid = moduleSA.getModuleGuid();
            pkgGuid = moduleSA.getPackageGuid();
            pkgVersion = moduleSA.getPackageVersion();

            //
            // Create Module Identification which have class member of package
            // identification.
            //
            PackageIdentification pkgId = new PackageIdentification(null, pkgGuid, pkgVersion);
            GlobalData.refreshPackageIdentification(pkgId);
            
            ModuleIdentification saId = new ModuleIdentification(null, saGuid, saVersion);
            saId.setPackage(pkgId);
            GlobalData.refreshModuleIdentification(saId);
            


            //
            // Create FpdModule Identification which have class member of module
            // identification
            //
            String[] archList = new String[0];
            if (arch == null || arch.trim().length() == 0) {
                archList = GlobalData.getToolChainInfo().getArchs();
            } else {
                archList = arch.split(" ");
            }
            for (int j = 0; j < archList.length; j++) {
                FpdModuleIdentification fpdSaId = new FpdModuleIdentification(saId, archList[j]);

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
        return fpdModuleMap;
    }

    /**
     * Retrieve valid image names
     *
     * @returns valid iamges name list if elements are found at the known xpath
     * @returns empty list if nothing is there
     */
    public String[] getFpdValidImageNames() {
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

    public Node getFpdUserExtensionPreBuild() {
        String[] xPath = new String[] { "/UserExtensions[@UserID='TianoCore' and @Identifier='0']" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)queryResult[0];

        return a.getDomNode();
    }

    public Node getFpdUserExtensionPostBuild() {
        String[] xPath = new String[] { "/UserExtensions[@UserID='TianoCore' and @Identifier='1']" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)queryResult[0];

        return a.getDomNode();
    }

    public Node[] getFpdUserExtensions() {
        String[] xPath = new String[] { "/UserExtensions[@UserID='TianoCore' and not(@Identifier='1') and not(@Identifier='0')]" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return new Node[0];
        }

        Node[] nodeList = new Node[queryResult.length];
        for (int i = 0; i < queryResult.length; ++i) {
            UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)queryResult[i];
            nodeList[i] = a.getDomNode();
        }

        return nodeList;
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
    public String[][] getFpdOptions(String fvName) {
           String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Options' and ./FvImageNames='"
                      + fvName + "']/FvImageOptions" };
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

    public XmlObject getFpdBuildOptions() {
        String[] xPath = new String[] { "/BuildOptions" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);

        if (queryResult == null || queryResult.length == 0) {
            return null;
        }
        return (XmlObject)queryResult[0];
    }

    public PlatformIdentification getFpdHeader() {
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
    public String[][] getFpdAttributes(String fvName) {
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Attributes' and ./FvImageNames='"
                         + fvName + "']/FvImageOptions" };
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
    public String getFlashDefinitionFile() {
        String[] xPath = new String[] { "/PlatformDefinitions/FlashDeviceDefinitions/FlashDefinitionFile" };

        Object[] queryResult = get("PlatformSurfaceArea", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }

        FileNameConvention filename = (FileNameConvention) queryResult[queryResult.length - 1];
        return filename.getStringValue();
    }

    public String[][] getFpdGlobalVariable() {
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
    public String[][] getFpdComponents(String fvName) {
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='Components' and ./FvImageNames='"+ fvName + "']/FvImageOptions" };
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
    public String[][] getPcdTokenArray() {
        String[] xPath = new String[] { "/PcdData" };

        Object[] returns = get("PCDs", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }

        return null;
    }

    /**
     * Retrieve MAS header
     *
     * @return
     * @return
     */
    public ModuleIdentification getMsaHeader() {
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

    public String[] getExternSpecificaiton() {
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
    public String[] getSpdMsaFile() {
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
    public Map<String, String[]> getSpdLibraryClasses() {
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
    public Map<String, String> getSpdPackageHeaderFiles() {
        String[] xPath = new String[] { "/PackageHeaders/IncludePkgHeader" };

        Object[] returns = get("PackageSurfaceArea", xPath);

        //
        // Create Map, Key - ModuleType, String - PackageInclude Header file.
        //
        Map<String, String> packageIncludeMap = new HashMap<String, String>();

        if (returns == null) {
            return packageIncludeMap;
        }

        for (int i = 0; i < returns.length; i++) {
            PackageHeadersDocument.PackageHeaders.IncludePkgHeader includeHeader = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader) returns[i];
            packageIncludeMap.put(includeHeader.getModuleType().toString(),
                    includeHeader.getStringValue());
        }
        return packageIncludeMap;
    }

    public PackageIdentification getSpdHeader() {
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
    public Map<String, String[]> getSpdGuid() {
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
            guidDeclMap.put(entry.getCName(), guidPair);
        }
        return guidDeclMap;
    }

    /**
     * Reteive
     */
    public Map<String, String[]> getSpdProtocol() {
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
            protoclMap.put(entry.getCName(), protocolPair);
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
    public Map<String, String[]> getSpdPpi() {
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
            ppiMap.put(entry.getCName(), ppiPair);
        }
        return ppiMap;
    }

    /**
     * Retrieve module Guid string
     *
     * @returns GUILD string if elements are found at the known xpath
     * @returns null if nothing is there
     */
    public String getModuleGuid() {
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
    public ModuleSADocument.ModuleSA[] getFpdModuleSAs() {
        String[] xPath = new String[] { "/FrameworkModules/ModuleSA" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        if (result != null) {
            return (ModuleSADocument.ModuleSA[]) result;
        }
        return new ModuleSADocument.ModuleSA[0];

    }
 
    /**
       Get name array who contains all PCDs in a module according to specified arch.
       
       @param arch          The specified architecture type.
       
       @return String[]     return all PCDs name into array, if no any PCD used by
                            this module, a String[0] array is returned.
    **/
    public String[] getModulePcdEntryNameArray(String arch) {
        PcdCodedDocument.PcdCoded.PcdEntry[] pcdEntries  = null;
        java.util.List                       archList    = null;
        java.util.List<String>               results     = new java.util.ArrayList<String> ();
        int                                  index;
        String[]                             xPath       = new String[] {"/PcdEntry"};
        Object[]                             returns     = get ("PcdCoded", xPath);

        if (returns == null) {
            return new String[0];
        }

        pcdEntries  = (PcdCodedDocument.PcdCoded.PcdEntry[])returns;

        for (index = 0; index < pcdEntries.length; index ++) {
            archList        = pcdEntries[index].getSupArchList();
            //
            // If the ArchList is specified in MSA for this PCD, need check
            // current arch whether can support by this PCD.
            // 
            if (archList != null) {
                if (archList.contains(arch)) {
                    results.add(new String(pcdEntries[index].getCName()));
                }
            } else {
                //
                // If no ArchList is specificied in MSA for this PCD, that means
                // this PCD support all architectures.
                // 
                results.add(new String(pcdEntries[index].getCName()));
            }
        }

        if (results.size() == 0) {
            return new String[0];
        }

        String[]    retArray = new String[results.size()];
        results.toArray(retArray);

        return retArray;        
    }

    /**
     Search in a List for a given string

     @return boolean
     **/
    public boolean contains(List list, String str) {
		if (list == null || list.size()== 0) {
			return true;
		}

        return list.contains(str);
    }

	public boolean isHaveTianoR8FlashMap(){
        String[]            xPath       = new String[] {"/"};
        Object[]         returns     = get ("Externs", xPath);

        if (returns == null) {
            return false;
        }

		ExternsDocument.Externs ext = (ExternsDocument.Externs)returns[0];

		if (ext.getTianoR8FlashMapH()){
			return true;
	    }else {
			return false;
		}
	}
    
    public Node getPeiApriori(String fvName) {
        String[] xPath = new String[] { "/BuildOptions/UserExtensions[@UserID='APRIORI' and @Identifier='0' and ./FvName='" + fvName + "']" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        
        if (result == null || result.length == 0) {
            return null;
        }
        
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)result[0];
        
        return a.getDomNode();
    }
    
    public Node getDxeApriori(String fvName) {
        String[] xPath = new String[] { "/BuildOptions/UserExtensions[@UserID='APRIORI' and @Identifier='1' and ./FvName='" + fvName + "']" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        
        if (result == null || result.length == 0) {
            return null;
        }
        
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)result[0];
        
        return a.getDomNode();
    }
    
    public Node getFpdModuleSequence(String fvName) {
        String[] xPath = new String[] { "/BuildOptions/UserExtensions[@UserID='IMAGES' and @Identifier='1' and ./FvName='" + fvName + "']" };
        Object[] result = get("PlatformSurfaceArea", xPath);
        
        if (result == null || result.length == 0) {
            return null;
        }
        
        UserExtensionsDocument.UserExtensions a =  (UserExtensionsDocument.UserExtensions)result[0];
        
        return a.getDomNode();
    }

    /**
     Get the value of PCD by PCD cName

     @return PcdValue         String of PcdComponentName
	         null         If don't find ComponentName Pcd
    **/
    public String getPcdValueBycName(String cName){
        String[] xPath = new String[] { "/PcdData" };
        Object[] returns = get("PcdBuildDefinition", xPath);
	if (returns == null || returns.length == 0) {
	    return  null;
	} 
	for (int i = 0; i < returns.length; i++) {
            PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData)returns[i];
	    if (pcdData.getCName().equalsIgnoreCase(cName)){
	        return pcdData.getValue();
            }
        }
        return null;
    }
}
