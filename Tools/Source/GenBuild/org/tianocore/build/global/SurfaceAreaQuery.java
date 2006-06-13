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
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlNormalizedString;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlString;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.CName;
import org.tianocore.ExternsDocument;
import org.tianocore.FfsDocument;
import org.tianocore.FileNameConvention;
import org.tianocore.FrameworkComponentTypes;
import org.tianocore.FvImageOptionsDocument;
import org.tianocore.GuidDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.LibraryUsage;
import org.tianocore.ModuleSADocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.NameValueDocument;
import org.tianocore.OutputDirectoryDocument;
import org.tianocore.PPIsDocument;
import org.tianocore.PackageNameDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.PcdCodedDocument.PcdCoded;

/**
  SurfaceAreaQuery class is used to query Surface Area information from msa, mbd,
  spd and fpd files. 
  
  This class should not instantiated. All the public interfaces is static.
  
  @since GenBuild 1.0
 **/
public class SurfaceAreaQuery {
    /// 
    /// Contains name/value pairs of Surface Area document object. The name is
    /// always the top level element name.
    ///  
    private static Map<String, XmlObject> map = null;
    
    ///
    /// mapStack is used to do nested query
    ///
    private static Stack< Map<String, XmlObject> > mapStack = new Stack< Map<String, XmlObject> >();
    
    /// 
    /// prefix of name space
    /// 
    private static String nsPrefix = "sans";
    
    ///
    /// xmlbeans needs a name space for each Xpath element 
    ///
    private static String ns = null;
    
    ///
    /// keep the namep declaration for xmlbeans Xpath query
    ///
    private static String queryDeclaration = null;

    /**
     Set a Surface Area document for query later
     
     @param     map     A Surface Area document in TopLevelElementName/XmlObject format.
     **/
    public static void setDoc(Map<String, XmlObject> map) {
        ns = OverrideProcess.prefix;
        queryDeclaration = "declare namespace " + nsPrefix + "='" + ns + "'; ";
        SurfaceAreaQuery.map = map;
    }

    /**
     Push current used Surface Area document into query stack. The given new 
     document  will be used for any immediately followed getXXX() callings, 
     untill pop() is called.
     
     @param     newMap  The TopLevelElementName/XmlObject format of a Surface Area document.
     **/
    public static void push(Map<String, XmlObject> newMap) {
        mapStack.push(SurfaceAreaQuery.map);
        SurfaceAreaQuery.map = newMap;
    }
    
    /**
     Discard current used Surface Area document and use the top document in stack
     instead.
     **/
    public static void pop() {
        SurfaceAreaQuery.map = mapStack.pop();
    }
    
    ///
    /// Convert xPath to be namespace qualified, which is necessary for XmlBeans
    /// selectPath(). For example, converting /MsaHeader/ModuleType to
    /// /ns:MsaHeader/ns:ModuleType
    ///     
    private static String normalizeQueryString(String[] exp, String from) {
        StringBuffer normQueryString = new StringBuffer(4096);

        int i = 0;
        while (i < exp.length) {
            String newExp = from + exp[i];
            Pattern pattern = Pattern.compile("([^/]*)(/|//)([^/]+)");
            Matcher matcher = pattern.matcher(newExp);

            while (matcher.find()) {
                String starter = newExp.substring(matcher.start(1), matcher.end(1));
                String seperator = newExp.substring(matcher.start(2), matcher.end(2));
                String token = newExp.substring(matcher.start(3), matcher.end(3));
                
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
      Search all XML documents stored in "map" for the specified xPath, using
      relative path (starting with '$this')
     
     @param     xPath   xpath query string array
     @returns   An array of XmlObject   if elements are found at the specified xpath
     @returns   NULL                    if nothing is at the specified xpath
     **/
    public static XmlObject[] get(String[] xPath) {
        if (map == null) {
            return null;
        }
        
        String[] keys = (String[]) map.keySet().toArray(new String[map.size()]);
        List<XmlObject> result = new ArrayList<XmlObject>();
        for (int i = 0; i < keys.length; ++i) {
            XmlObject rootNode = (XmlObject) map.get(keys[i]);
            if (rootNode == null) {
                continue;
            }
            
            String query = queryDeclaration + normalizeQueryString(xPath, "$this/" + keys[i]);
            XmlObject[] tmp = rootNode.selectPath(query);
            for (int j = 0; j < tmp.length; ++j) {
                result.add(tmp[j]);
            }
        }
        
        int size = result.size();
        if (size <= 0) {
            return null;
        }
        
        return (XmlObject[]) result.toArray(new XmlObject[size]);
    }

    /**
     Search XML documents named by "rootName" for the given xPath, using
     relative path (starting with '$this')
     
     @param     rootName    The top level element name 
     @param     xPath       The xpath query string array
     @returns   An array of XmlObject   if elements are found at the given xpath
     @returns   NULL                    if nothing is found at the given xpath
     **/
    public static XmlObject[] get(String rootName, String[] xPath) {
        if (map == null) {
            return null;
        }
        
        XmlObject root = (XmlObject) map.get(rootName);
        if (root == null) {
            return null;
        }

        String query = queryDeclaration + normalizeQueryString(xPath, "$this/" + rootName);
        XmlObject[] result = root.selectPath(query);
        if (result.length > 0) {
            return result;
        }

        query = queryDeclaration + normalizeQueryString(xPath, "/" + rootName);
        result = root.selectPath(query);
        if (result.length > 0) {
            return result;
        }

        return null;
    }

    /**
     Retrieve SourceFiles/Filename for specified ARCH type
      
     @param     arch        architecture name
     @returns   An array of XmlObject   if elements are found at the known xpath
     @returns   NULL                    if nothing is found at the known xpath
     **/
    public static XmlObject[] getSourceFiles(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            xPath = new String[] {
                "/Filename",
                "/Arch/Filename"
                };
        } else {
            xPath = new String[] {
                "/Filename[not(@ArchType) or @ArchType='ALL' or @ArchType='" + arch + "']",
                "/Arch[@ArchType='ALL' or @ArchType='" + arch + "']/Filename"
                };
        }

        return get("SourceFiles", xPath);
    }

    /**
     Retrieve BuildOptions/Ffs

     @returns   FfsDocument.Ffs object  if elements are found at the known xpath
     @returns   NULL                    if nothing is found at the known xpath
     **/
    public static FfsDocument.Ffs getFfs() {
        String[] xPath = new String[] { "/Ffs" };

        XmlObject[] returns = get("BuildOptions", xPath);
        if (returns != null && returns.length > 0) {
            return (FfsDocument.Ffs) returns[0];
        }

        return null;
    }

    /**
     Retrieve BuildOptions/OutputDirectory

     @returns   Directory names array   if elements are found at the known xpath
     @returns   Empty                   if nothing is found at the known xpath
     **/
    public static String[] getOutputDirectory() {
        String[] xPath = new String[] { "/OutputDirectory" };

        XmlObject[] returns = get("BuildOptions", xPath);
        if (returns != null && returns.length > 0) {
            String[] dirString = new String[2];

            OutputDirectoryDocument.OutputDirectory[] dir = (OutputDirectoryDocument.OutputDirectory[]) returns;
            dirString[0] = dir[0].getIntermediateDirectories().toString();
            dirString[1] = dir[0].getStringValue();

            return dirString;
        }

        return new String[] { "UNIFIED", null };
    }

    /**
     Retrieve BuildOptions/Option or Arch/Option

     @param     arch    architecture name

     @returns   name/value pairs of options if elements are found at the known xpath
     @returns   Empty array                 if nothing is there
     **/
    public static String[][] getOptions(String arch){
        String[] xPath;

        if (arch == null || arch.equals("")) {
            xPath = new String[] {
                "/Option",
                "/Arch/Option"
                };
        } else {
            xPath = new String[] {
                "/Option",
                "/Arch[@ArchType='ALL' or @ArchType='" + arch + "']/Option"
                };
        }

        XmlObject[] returns = get("BuildOptions", xPath);
        if (returns == null){
            return new String[0][2];
        }
        
        String[][] result = new String[returns.length][2];
        for (int i = 0; i < returns.length; i ++){
            String str;
            String name = null;
            String value = null;

            if (returns[i] instanceof BuildOptionsDocument.BuildOptions.Option) {
                BuildOptionsDocument.BuildOptions.Option option = (BuildOptionsDocument.BuildOptions.Option)returns[i];
                str = option.getStringValue();
            } else if (returns[i] instanceof BuildOptionsDocument.BuildOptions.Arch.Option) {
                BuildOptionsDocument.BuildOptions.Arch.Option archOption = (BuildOptionsDocument.BuildOptions.Arch.Option)returns[i];
                str = archOption.getStringValue();
            } else {
                continue;
            }
            
            int equalIndex = str.indexOf('=');
            if ( equalIndex > 0) {
                name = str.substring(0, equalIndex).trim();
                value = str.substring(equalIndex + 1).trim();
                // TBD remove some forbidden name: BASE_NAME, ARCH and so on
                if (name.length() == 0){
                    name = null;
                }
            }
            result[i][0] = name;
            result[i][1] = value;
        }

        return result;
    }
    
    /**
     Retrieve <xxxHeader>/ModuleType

     @returns   The module type name    if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getModuleType() {
        String[] xPath = new String[] { "/ModuleType" };

        XmlObject[] returns = get(xPath);
        if (returns != null && returns.length > 0) {
            ModuleTypeDef type = (ModuleTypeDef) returns[0];
            return type.enumValue().toString();
        }

        return null;
    }

    /**
     Retrieve <xxxHeader>/ComponentType
     
     @returns   The component type name if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getComponentType() {
        String[] xPath = new String[] { "/ComponentType" };

        XmlObject[] returns = get(xPath);
        if (returns != null && returns.length > 0) {
            FrameworkComponentTypes type = (FrameworkComponentTypes) returns[0];
            return type.enumValue().toString();
        }

        return null;
    }

    /**
     Retrieve Includes/PackageName

     @param     arch    Architecture name

     @returns   package name list       if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static List<String> getIncludePackageName(String arch) {
        String[] xPath;

        if (arch == null || arch.equals("")) {
            xPath = new String[] {
                "/PackageName",
                "/Arch/PackageName"
                };
        } else {
            xPath = new String[] {
                "/PackageName",
                "/Arch[@ArchType='ALL' or @ArchType='" + arch + "']/PackageName"
                };
        }
        
        XmlObject[] returns = get("Includes", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }

        List<String> packageNames = new ArrayList<String>();
        PackageNameDocument.PackageName[] nameObj = (PackageNameDocument.PackageName[])returns;
        for (int i = 0; i < returns.length; ++i) {
            packageNames.add(nameObj[i].getStringValue());
        }
        
        return packageNames;
    }

    /**
     Retrieve LibraryClassDefinitions/LibraryClass for specified usage

     @param     usage   Library class usage

     @returns   LibraryClass objects list   if elements are found at the known xpath
     @returns   null                        if nothing is there
     **/
    public static LibraryClassDocument.LibraryClass[] getLibraryClassArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] {"/LibraryClass"};
        } else {
            xPath = new String[] {"/LibraryClass[@Usage='" + usage + "']"};
        }

        XmlObject[] returns = get("LibraryClassDefinitions", xPath);
        if (returns != null && returns.length > 0) {
            return (LibraryClassDocument.LibraryClass[]) returns;
        }

        return null;
    }

    /**
     Retrieve ModuleEntryPoint names

     @returns   ModuleEntryPoint name list  if elements are found at the known xpath
     @returns   null                        if nothing is there
     **/
    public static String[] getModuleEntryPointArray() {
        String[] xPath = new String[] { "/Extern/ModuleEntryPoint" };

        XmlObject[] returns = get("Externs", xPath);

        if (returns != null && returns.length > 0) {
            String[] entryPoints = new String[returns.length];

            for (int i = 0; i < returns.length; ++i) {
                entryPoints[i] = ((XmlNormalizedString) returns[i])
                                .getStringValue();
            }

            return entryPoints;
        }

        return null;
    }

    /**
     Retrieve module Guid string
 
     @returns   GUILD string            if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getModuleGuid() {
        String[] xPath = new String[] { "/Guid" };

        XmlObject[] returns = get(xPath);
        if (returns != null && returns.length > 0) {
            GuidDocument.Guid guid = (GuidDocument.Guid) returns[0];
            return guid.getStringValue();
        }

        return null;
    }

    /**
     retrieve Protocol for specified usage

     @param     usage   Protocol usage

     @returns   Protocol objects list   if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static ProtocolsDocument.Protocols.Protocol[] getProtocolArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] {"/Protocol"};
        } else {
            xPath = new String[] {"/Protocol[@Usage='" + usage + "']"};
        }

        XmlObject[] returns = get("Protocols", xPath);
        if (returns != null && returns.length > 0) {
            return (ProtocolsDocument.Protocols.Protocol[]) returns;
        }

        return null;
    }

    /**
     Retrieve ProtocolNotify for specified usage
    
     @param     usage   ProtocolNotify usage

     @returns   ProtocolNotify objects list     if elements are found at the known xpath
     @returns   null                            if nothing is there
     **/
    public static ProtocolsDocument.Protocols.ProtocolNotify[] getProtocolNotifyArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] {"/ProtocolNotify"};
        } else {
            xPath = new String[] {"/ProtocolNotify[@Usage='" + usage + "']"};
        }

        XmlObject[] returns = get("Protocols", xPath);
        if (returns != null && returns.length > 0) {
            return (ProtocolsDocument.Protocols.ProtocolNotify[]) returns;
        }

        return null;
    }

    /**
     Retrieve ModuleUnloadImage names

     @returns   ModuleUnloadImage name list     if elements are found at the known xpath
     @returns   null                            if nothing is there
     **/
    public static String[] getModuleUnloadImageArray() {
        String[] xPath = new String[] { "/Extern/ModuleUnloadImage" };

        XmlObject[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            String[] stringArray = new String[returns.length];
            XmlNormalizedString[] doc = (XmlNormalizedString[])returns;

            for (int i = 0; i < returns.length; ++i) {
                stringArray[i] = doc[i].getStringValue();
            }

            return stringArray;
        }

        return null;
    }

    /**
     Retrieve Extern

     @returns   Extern objects list     if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static ExternsDocument.Externs.Extern[] getExternArray() {
        String[] xPath = new String[] { "/Extern" };

        XmlObject[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            return (ExternsDocument.Externs.Extern[]) returns;
        }

        return null;
    }

    /**
     Retrieve Ppi information

     @param     usage   Ppi usage

     @returns   Ppi objects list        if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static PPIsDocument.PPIs.Ppi[] getPpiArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] { "/Ppi" };
        } else {
            xPath = new String[] { "/Ppi[@Usage='" + usage + "']" };
        }

        XmlObject[] returns = get("PPIs", xPath);
        if (returns != null && returns.length > 0) {
            return (PPIsDocument.PPIs.Ppi[])returns;
        }

        return null;
    }

    /**
     Retrive PpiNotify information
    
     @param usage

     @returns   PpiNotify objects list  if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static PPIsDocument.PPIs.PpiNotify[] getPpiNotifyArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] { "/PpiNotify" };
        } else {
            xPath = new String[] { "/PpiNotify[@Usage='" + usage + "']" };
        }

        XmlObject[] returns = get("PPIs", xPath);
        if (returns != null && returns.length > 0) {
            return (PPIsDocument.PPIs.PpiNotify[])returns;
        }

        return null;
    }

    /**
     Retrieve GuidEntry information for specified usage

     @param     usage   GuidEntry usage

     @returns   GuidEntry objects list  if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static GuidsDocument.Guids.GuidEntry[] getGuidEntryArray(String usage) {
        String[] xPath;

        if (usage == null || usage.equals("")) {
            xPath = new String[] { "/GuidEntry" };
        } else {
            xPath = new String[] { "/GuidEntry[@Usage='" + usage + "']" };
        }

        XmlObject[] returns = get("Guids", xPath);
        if (returns != null && returns.length > 0) {
            return (GuidsDocument.Guids.GuidEntry[])returns;
        }

        return null;
    }

    /**
     Retrieve Library instance information

     @param     arch    Architecture name
     @param     usage   Library instance usage

     @returns   library instance name list  if elements are found at the known xpath
     @returns   null                        if nothing is there
     **/
    public static List<String> getLibraryInstance(String arch, String usage) {
        String[] xPath;
        String   archAttribute = "";
        String   usageAttribute = "";

        if ((arch != null) || (!arch.equals(""))) {
            archAttribute = "[@ArchType='ALL' or @ArchType='" + arch + "']";
        }
        
        if ((usage != null) || (!usage.equals(""))) {
            // if no Usage attribute specified, default to ALWAYS_CONSUMED
            if (usage.equals(LibraryUsage.ALWAYS_CONSUMED.toString())) {
                usageAttribute = "[not(@Usage) or @Usage='" + usage + "']";
            } else {
                usageAttribute = "[@Usage='" + usage + "']";
            }
        }
        
        xPath = new String[] {
                "/Library" + usageAttribute,
                "/Arch" + archAttribute + "/Library" + usageAttribute
                };

        XmlObject[] returns = get("Libraries", xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        
        List<String> instances = new ArrayList<String>();
        for (int i = 0; i < returns.length; ++i) {
            if (returns[i] instanceof LibrariesDocument.Libraries.Library) {
                LibrariesDocument.Libraries.Library lib = (LibrariesDocument.Libraries.Library)returns[i];
                instances.add(lib.getStringValue());
            } else if (returns[i] instanceof LibrariesDocument.Libraries.Arch.Library) {
                LibrariesDocument.Libraries.Arch.Library lib = (LibrariesDocument.Libraries.Arch.Library)returns[i];
                instances.add(lib.getStringValue());
            }
        }

        return instances;
    }

    ///
    /// This method is used for retrieving the elements information which has 
    /// CName sub-element
    ///
    private static String[] getCNames(String from, String xPath[]) {
        XmlObject[] returns = get(from, xPath);
        if (returns == null || returns.length == 0) {
            return null;
        }
        
        String[] strings = new String[returns.length];
        for (int i = 0; i < returns.length; ++i) {
            strings[i] = ((CName)returns[i]).getStringValue(); 
        }
        
        return strings;            
    }
    
    /**
     Retrive library's constructor name

     @returns   constructor name list   if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getLibConstructorName() {
        String[] xPath = new String[] {"/Extern/Constructor"};

        XmlObject[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            CName constructor = (CName)returns[0]; 
            return constructor.getStringValue();            
        }

        return null;
    }

    /**
     Retrive library's destructor name

     @returns   destructor name list    if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getLibDestructorName() {
        String[] xPath = new String[] {"/Extern/Destructor"};

        XmlObject[] returns = get("Externs", xPath);
        if (returns != null && returns.length > 0) {
            CName destructor = (CName)returns[0]; 
            return destructor.getStringValue();            
        }

        return null;
    }
    
    /**
     Retrive DriverBinding names

     @returns   DriverBinding name list if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getDriverBindingArray() {
        String[] xPath = new String[] {"/Extern/DriverBinding"};
        return getCNames("Externs", xPath);
    }
    
    /**
     Retrive ComponentName names

     @returns   ComponentName name list if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getComponentNameArray() {
        String[] xPath = new String[] {"/Extern/ComponentName"};
        return getCNames("Externs", xPath);
    }
    
    /**
     Retrive DriverConfig names

     @returns   DriverConfig name list  if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getDriverConfigArray() {
        String[] xPath = new String[] {"/Extern/DriverConfig"};
        return getCNames("Externs", xPath);
    }
    
    /**
     Retrive DriverDiag names

     @returns   DriverDiag name list    if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getDriverDiagArray() {
        String[] xPath = new String[] {"/Extern/DriverDiag"};
        return getCNames("Externs", xPath);
    }

    /**
     Retrive SetVirtualAddressMapCallBack names

     @returns   SetVirtualAddressMapCallBack name list  
                                        if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getSetVirtualAddressMapCallBackArray() {
        String[] xPath = new String[] {"/Extern/SetVirtualAddressMapCallBack"};
        return getCNames("Externs", xPath);
    }
    
    /**
     Retrive ExitBootServicesCallBack names

     @returns   ExitBootServicesCallBack name list  
                                        if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String[] getExitBootServicesCallBackArray() {
        String[] xPath = new String[] {"/Extern/ExitBootServicesCallBack"};
        return getCNames("Externs", xPath);
    }

    /**
     Retrieve module surface area file information

     @returns   ModuleSA objects list   if elements are found at the known xpath
     @returns   Empty ModuleSA list     if nothing is there
     **/
    public static ModuleSADocument.ModuleSA[] getFpdModules() {
        String[] xPath = new String[] { "/TianoImage/*/ModuleSA" };

        XmlObject[] result = get("FrameworkPlatformDescription", xPath);
        if (result == null) {
            return new ModuleSADocument.ModuleSA[0];
        }

        return (ModuleSADocument.ModuleSA[]) result;
    }

    /**
     Retrieve variables for FV images

     @returns   name/value list        if elements are found at the known xpath
     @returns   empty list             if nothing is there
     **/
    public static String[][] getFpdGlobalVariable() {
        String[] xPath = new String[] { "/Flash/FvImages/NameValue" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        String[][] result = new String[queryResult.length][2];
        for (int i = 0; i < queryResult.length; i++){
            result[i][0] = ((NameValueDocument.NameValue)queryResult[i]).getName();
            result[i][1] = ((NameValueDocument.NameValue)queryResult[i]).getValue();
        }

        return result;
    }
    
    /**
     Retrieve valid image names

     @returns   valid iamges name list  if elements are found at the known xpath
     @returns   empty list              if nothing is there
     **/
    public static String[] getFpdValidImageNames(){
        String[] xPath = new String[] { "/Flash/FvImages/FvImage[@Type='ValidImageNames']/FvImageNames" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null) {
            return new String[0];
        }

        String[] result = new String[queryResult.length];
        for (int i = 0; i < queryResult.length; i++){
            result[i] = ((XmlString)queryResult[i]).getStringValue();
        }

        return result;
    }

    /**
     Retrieve FV image option information

     @param fvName  FV image name

     @returns   option name/value list if elements are found at the known xpath
     @returns   empty list             if nothing is there
     **/
    public static String[][] getFpdOptions(String fvName){
        String[] xPath = new String[] {"/Flash/FvImages/FvImageName[@Name='" + fvName.toUpperCase() + "']/FvImageOptions/NameValue" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        String[][] result = new String[queryResult.length][2];
        for (int i = 0; i < queryResult.length; i++){
            result[i][0] = ((NameValueDocument.NameValue)queryResult[i]).getName();
            result[i][1] = ((NameValueDocument.NameValue)queryResult[i]).getValue();
        }

        return result;
    }
    
    /**
     Retrieve FV image attributes information

     @param     fvName  FV image name

     @returns   attribute name/value list   if elements are found at the known xpath
     @returns   empty list                  if nothing is there
     **/
    public static String[][] getFpdAttributes(String fvName){
        String[] xPath = new String[] {"/Flash/FvImages/FvImage[@Type='Attributes' and ./FvImageNames='" + fvName.toUpperCase() + "']/FvImageOptions" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        ArrayList<String[]> list = new ArrayList<String[]>();
        for (int i = 0 ; i < queryResult.length; i++){
            FvImageOptionsDocument.FvImageOptions item = (FvImageOptionsDocument.FvImageOptions)queryResult[i];

            List<NameValueDocument.NameValue> namevalues = item.getNameValueList();
            Iterator iter = namevalues.iterator();
            while (iter.hasNext()) {
                NameValueDocument.NameValue nvItem = (NameValueDocument.NameValue)iter.next();
                list.add(new String[]{nvItem.getName(), nvItem.getValue()});
            }

            List<String> enables = item.getEnableList();
            iter = enables.iterator();
            while (iter.hasNext()) {
                String enableItem = (String)iter.next();
                list.add(new String[]{enableItem, "TRUE"});
            }

            List<String> disables = item.getDisableList();
            iter = disables.iterator();
            while (iter.hasNext()) {
                String disableItem = (String)iter.next();
                list.add(new String[]{disableItem, "FALSE"});
            }
        }

        String[][] result = new String[list.size()][2];
        for (int i = 0; i < list.size(); i++){
            result[i][0] = list.get(i)[0];
            result[i][1] = list.get(i)[1];
        }

        return result;
    }
    
    /**
     Retrieve flash definition file name

     @returns   file name               if elements are found at the known xpath
     @returns   null                    if nothing is there
     **/
    public static String getFlashDefinitionFile(){
        String[] xPath = new String[] {"/Flash/FlashDefinitionFile" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null || queryResult.length == 0) {
            return null;
        }

        FileNameConvention filename = (FileNameConvention)queryResult[queryResult.length - 1];
        return filename.getStringValue();
    }
    
    /**
     Retrieve FV image component options

     @param     fvName  FV image name

     @returns   name/value pairs list   if elements are found at the known xpath
     @returns   empty list              if nothing is there
     **/
    public static String[][] getFpdComponents(String fvName){
        String[] xPath = new String[] {"/Flash/FvImages/FvImage[@Type='Components' and ./FvImageNames='" + fvName.toUpperCase() + "']/FvImageOptions" };

        XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
        if (queryResult == null) {
            return new String[0][];
        }

        ArrayList<String[]> list = new ArrayList<String[]>();
        for (int i = 0 ; i < queryResult.length; i++){
            FvImageOptionsDocument.FvImageOptions item = (FvImageOptionsDocument.FvImageOptions)queryResult[i];

            List<NameValueDocument.NameValue> namevalues = item.getNameValueList();
            Iterator iter = namevalues.iterator();
            while (iter.hasNext()) {
                NameValueDocument.NameValue nvItem = (NameValueDocument.NameValue)iter.next();
                list.add(new String[]{nvItem.getName(), nvItem.getValue()});
            }

            List<String> enables = item.getEnableList();
            iter = enables.iterator();
            while (iter.hasNext()) {
                String enableItem = (String)iter.next();
                list.add(new String[]{enableItem, "TRUE"});
            }

            List<String> disables = item.getDisableList();
            iter = disables.iterator();
            while (iter.hasNext()) {
                String disableItem = (String)iter.next();
                list.add(new String[]{disableItem, "FALSE"});
            }
        }

        String[][] result = new String[list.size()][2];
        for (int i = 0; i < list.size(); i++){
            result[i][0] = list.get(i)[0];
            result[i][1] = list.get(i)[1];
        }

        return result;
    }
    
    /**
       Get name array of PCD in a module. In one module, token space
       is same, and token name should not be conflicted.
       
       @return String[]
    **/
    public static String[] getModulePcdEntryNameArray() {
        PcdCoded.PcdEntry[] pcdEntries  = null;
        String[]            results;
        int                 index;
        String[]            xPath       = new String[] {"/PcdEntry"};
        XmlObject[]         returns     = get ("PcdCoded", xPath);
        if (returns == null) {
            return null;
        }

        pcdEntries = (PcdCoded.PcdEntry[])returns;
        results    = new String[pcdEntries.length];
        
        for (index = 0; index < pcdEntries.length; index ++) {
            results[index] = pcdEntries[index].getCName();
        }
        return results;
    }
}
