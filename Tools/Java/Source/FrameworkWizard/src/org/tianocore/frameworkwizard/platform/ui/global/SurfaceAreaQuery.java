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
package org.tianocore.frameworkwizard.platform.ui.global;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Stack;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.BuildTargetList;
import org.tianocore.LibraryClassDocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.FilenameDocument.Filename;

import org.tianocore.frameworkwizard.common.GlobalData;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;

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

			String query = queryDeclaration
					+ normalizeQueryString(xPath, "$this/" + keys[i]);
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
	public static XmlObject[] get(String rootName, String[] xPath) {
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
		XmlObject[] returns;

		if (arch == null || arch.equals("")) {
			xPath = new String[] { "/Filename" };
		} else {
			xPath = new String[] { "/Filename[not(@SupArchList) or @SupArchList='"
					+ arch + "']" };
		}

		returns = get("SourceFiles", xPath);

		if (returns == null || returns.length == 0) {
			return null;
		}

		Filename[] sourceFileNames = (Filename[]) returns;
		String[][] outputString = new String[sourceFileNames.length][2];
		for (int i = 0; i < sourceFileNames.length; i++) {
			outputString[i][0] = sourceFileNames[i].getToolCode();
			outputString[i][1] = sourceFileNames[i].getStringValue();
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
		String[] xPath = new String[] { "/PlatformDefinitions/OutputDirectory" };

		XmlObject[] returns = get("FrameworkPlatformDescription", xPath);
		if (returns != null && returns.length > 0) {
			// String TBD
		}

		return null;
	}

	public static String getFpdIntermediateDirectories() {
		String[] xPath = new String[] { "/PlatformDefinitions/IntermediateDirectories" };

		XmlObject[] returns = get("FrameworkPlatformDescription", xPath);
		if (returns != null && returns.length > 0) {
			// TBD
		}
		return "UNIFIED";
	}

	public static String getBuildTarget() {
		String[] xPath = new String[] { "/PlatformDefinitions/BuildTargets" };

		XmlObject[] returns = get("FrameworkPlatformDescription", xPath);
		if (returns != null && returns.length > 0) {
			return ((BuildTargetList) returns[0]).getStringValue();
		}

		return null;
	}

	/**
	 * Retrieve <xxxHeader>/ModuleType
	 * 
	 * @returns The module type name if elements are found at the known xpath
	 * @returns null if nothing is there
	 */
	public static String getModuleType(ModuleIdentification mi) {
		ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = WorkspaceProfile.getModuleXmlObject(mi);

		return msa.getMsaHeader().getModuleType()+"";
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

	public static PackageIdentification[] getDependencePkg(String arch, ModuleIdentification mi){
		
		String packageGuid = null;
		String packageVersion = null;

        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) WorkspaceProfile.getModuleXmlObject(mi);
        if (msa.getPackageDependencies() == null) {
            return new PackageIdentification[0];
        }
        int size = msa.getPackageDependencies().getPackageList().size();
        XmlObject[] returns = new XmlObject[size];
        for (int i = 0; i < size; ++i) {
            returns[i] = msa.getPackageDependencies().getPackageList().get(i);
        }

		PackageIdentification[] packageIdList = new PackageIdentification[returns.length];
		for (int i = 0; i < returns.length; i++) {
			PackageDependenciesDocument.PackageDependencies.Package item = (PackageDependenciesDocument.PackageDependencies.Package) returns[i];
			packageGuid = item.getPackageGuid();
			packageVersion = item.getPackageVersion();

            Iterator<PackageIdentification> ispi = GlobalData.vPackageList.iterator();
            String ver = "";
            while(ispi.hasNext()) {
                PackageIdentification pi = ispi.next();
                if (packageVersion != null) {
                    if (pi.getGuid().equalsIgnoreCase(packageGuid) && pi.getVersion().equals(packageVersion)) {
                        packageIdList[i] = pi;
                        break;
                    } 
                }
                else {
                    if (pi.getGuid().equalsIgnoreCase(packageGuid)) {
                        if (pi.getVersion() != null && pi.getVersion().compareTo(ver) > 0){
                            ver = pi.getVersion();
                            packageIdList[i] = pi;
                        }
                        else if (packageIdList[i] == null){
                            packageIdList[i] = pi;
                        }
                    }
                }
                
            }
		}
		return packageIdList;
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
	public static Vector<LibraryClassDescriptor> getLibraryClasses(String usage, ModuleIdentification mi){
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)WorkspaceProfile.getModuleXmlObject(mi);
        Vector<LibraryClassDescriptor> libraryClassInfo = new Vector<LibraryClassDescriptor>();
        if (msa.getLibraryClassDefinitions() == null) {
            return libraryClassInfo;
        }
        
        int size = msa.getLibraryClassDefinitions().getLibraryClassList().size();
		
		for (int i = 0; i < size; i++) {
            LibraryClassDocument.LibraryClass libClass = msa.getLibraryClassDefinitions().getLibraryClassList().get(i);
            if (usage.equals(libClass.getUsage().toString())) {

                libraryClassInfo.add(new LibraryClassDescriptor(libClass.getKeyword(), libClass.getSupArchList()+"", libClass.getSupModuleList()+""));
            }
		}
        
		return libraryClassInfo;
	}

    public static XmlObject[] getSpdPcdDeclarations(PackageIdentification pi) {
        XmlObject[] returns = null;
        PackageSurfaceAreaDocument.PackageSurfaceArea psa = WorkspaceProfile.getPackageXmlObject(pi);
        if (psa.getPcdDeclarations() != null && psa.getPcdDeclarations().getPcdEntryList() != null) {
            int size = psa.getPcdDeclarations().getPcdEntryList().size();
            returns = new XmlObject[size];
            for (int i = 0; i < size; ++i) {
                returns[i] = psa.getPcdDeclarations().getPcdEntryList().get(i);
            }
        }
     
        return returns;
    }

}

