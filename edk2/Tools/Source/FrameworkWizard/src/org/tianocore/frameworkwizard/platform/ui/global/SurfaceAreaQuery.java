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
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Stack;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.xmlbeans.XmlNormalizedString;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlString;
import org.tianocore.BuildTargetList;
import org.tianocore.DataIdDocument;
import org.tianocore.ExternsDocument;
import org.tianocore.FileNameConvention;
//import org.tianocore.FvImageDocument;
import org.tianocore.GuidDeclarationsDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryClassDeclarationsDocument;
import org.tianocore.LibraryClassDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.ModuleTypeDef;
import org.tianocore.MsaFilesDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.PackageDependenciesDocument;
import org.tianocore.PackageHeadersDocument;
import org.tianocore.PpiDeclarationsDocument;
import org.tianocore.ProtocolDeclarationsDocument;
import org.tianocore.SpdHeaderDocument;
import org.tianocore.FilenameDocument.Filename;
import org.tianocore.MsaHeaderDocument.MsaHeader;
import org.tianocore.PlatformHeaderDocument;
import org.tianocore.frameworkwizard.platform.ui.id.FpdModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PlatformIdentification;

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
	 * Retrieve PackageDependencies/Package
	 * 
	 * @param arch
	 *            Architecture name
	 * 
	 * @returns package name list if elements are found at the known xpath
	 * @returns null if nothing is there
	 */

	public static PackageIdentification[] getDependencePkg(String arch, ModuleIdentification mi) throws Exception{
		
		String packageGuid = null;
		String packageVersion = null;

        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) GlobalData.getModuleXmlObject(mi);
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

            Set<PackageIdentification> spi = GlobalData.getPackageList();
            Iterator<PackageIdentification> ispi = spi.iterator();
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
	public static Vector<String> getLibraryClasses(String usage, ModuleIdentification mi) throws Exception{
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)GlobalData.getModuleXmlObject(mi);
        Vector<String> libraryClassName = new Vector<String>();
        if (msa.getLibraryClassDefinitions() == null) {
            return libraryClassName;
        }
        
        int size = msa.getLibraryClassDefinitions().getLibraryClassList().size();
		
		for (int i = 0; i < size; i++) {
            LibraryClassDocument.LibraryClass libClass = msa.getLibraryClassDefinitions().getLibraryClassList().get(i);
            if (usage.equals(libClass.getUsage().toString())) {
                libraryClassName.add(libClass.getKeyword());
            }
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
	 * Retrieve ModuleUnloadImage names
	 * 
	 * @returns ModuleUnloadImage name list if elements are found at the known
	 *          xpath
	 * @returns null if nothing is there
	 */
	public static String[] getModuleUnloadImageArray() {
		String[] xPath = new String[] { "/Extern/ModuleUnloadImage" };

		XmlObject[] returns = get("Externs", xPath);
		if (returns != null && returns.length > 0) {
			String[] stringArray = new String[returns.length];
			XmlNormalizedString[] doc = (XmlNormalizedString[]) returns;

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

		XmlObject[] returns = get("Externs", xPath);
		if (returns != null && returns.length > 0) {
			return (ExternsDocument.Externs.Extern[]) returns;
		}

		return null;
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
			xPath = new String[] { "/Instance[not(@SupArchList) or @SupArchList='"
					+ arch + "']" };
		}

		XmlObject[] returns = get("Libraries", xPath);
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
		XmlObject[] returns = get(from, xPath);
		if (returns == null || returns.length == 0) {
			return null;
		}

		String[] strings = new String[returns.length];
		for (int i = 0; i < returns.length; ++i) {
			// TBD
			// strings[i] = ((CName) returns[i]).getStringValue();
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

		XmlObject[] returns = get("Externs", xPath);
		if (returns != null && returns.length > 0) {
			// CName constructor = (CName) returns[0];
			// return constructor.getStringValue();
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

		XmlObject[] returns = get("Externs", xPath);
		if (returns != null && returns.length > 0) {
			// CName destructor = (CName) returns[0];
			// return destructor.getStringValue();
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
		XmlObject[] result = get("FrameworkPlatformDescription", xPath);
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
			// Get Fpd SA Module element node and add to xmlObjectMap.
			//
			Map<String, XmlObject> xmlObjectMap = new HashMap<String, XmlObject>();
			ModuleSADocument.ModuleSA moduleSA = (ModuleSADocument.ModuleSA) result[i];
			if (((ModuleSADocument.ModuleSA) result[i]).getLibraries() != null) {
				xmlObjectMap.put("Libraries", moduleSA.getLibraries());
			}
			if (((ModuleSADocument.ModuleSA) result[i]).getPcdBuildDefinition() != null) {
				xmlObjectMap.put("PcdBuildDefinition", moduleSA
						.getPcdBuildDefinition());
			}
			if (((ModuleSADocument.ModuleSA) result[i])
					.getModuleSaBuildOptions() != null) {
				xmlObjectMap.put("ModuleSaBuildOptions", moduleSA
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
			FpdModuleIdentification fpdSaId = new FpdModuleIdentification(saId,
					arch);
			if (arch != null) {
				fpdSaId.setArch(arch);
			}
			if (fvBinding != null) {
				fpdSaId.setFvBinding(fvBinding);
			}

			//
			// Put element to Map<FpdModuleIdentification, Map<String,
			// XmlObject>>.
			//
			fpdModuleMap.put(fpdSaId, xmlObjectMap);
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
		String[] xPath = new String[] { "/PlatformDefinitions/FlashDeviceDefinitions/FvImages/FvImage[@Type='ValidImageNames']/FvImageNames" };

		XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
		if (queryResult == null) {
			return new String[0];
		}

		String[] result = new String[queryResult.length];
		for (int i = 0; i < queryResult.length; i++) {
			result[i] = ((XmlString) queryResult[i]).getStringValue();
		}

		return result;
	}

	

	public static XmlObject getFpdBuildOptions() {
		String[] xPath = new String[] { "/BuildOptions" };

		XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);

		if (queryResult == null || queryResult.length == 0) {
			return null;
		}
		return queryResult[0];
	}

	public static PlatformIdentification getFpdHeader() {
		String[] xPath = new String[] { "/PlatformHeader" };

		XmlObject[] returns = get("FrameworkPlatformDescription", xPath);

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
	 * Retrieve flash definition file name
	 * 
	 * @returns file name if elements are found at the known xpath
	 * @returns null if nothing is there
	 */
	public static String getFlashDefinitionFile() {
		String[] xPath = new String[] { "/PlatformDefinitions/FlashDeviceDefinitions/FlashDefinitionFile" };

		XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
		if (queryResult == null || queryResult.length == 0) {
			return null;
		}

		FileNameConvention filename = (FileNameConvention) queryResult[queryResult.length - 1];
		return filename.getStringValue();
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
		String[] xPath = new String[] { "/PlatformDefinitions/FlashDeviceDefinitions/DataRegions/FvDataRegion[@Name='"
				+ fvName.toUpperCase() + "']/DataId" };

		XmlObject[] queryResult = get("FrameworkPlatformDescription", xPath);
		if (queryResult == null) {
			return new String[0][];
		}

		ArrayList<String[]> list = new ArrayList<String[]>();
		for (int i = 0; i < queryResult.length; i++) {
			DataIdDocument.DataId item = (DataIdDocument.DataId) queryResult[i];
			list
					.add(new String[] { item.getStringValue(),
							item.getDataSize() });
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

		XmlObject[] returns = get("PCDs", xPath);
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
	 * Retrieve MSA header
	 * 
	 * @return
	 * @return
	 */
	public static ModuleIdentification getMsaHeader() {
		String[] xPath = new String[] { "/" };
		XmlObject[] returns = get("MsaHeader", xPath);

		if (returns == null || returns.length == 0) {
			return null;
		}

		MsaHeader msaHeader = (MsaHeader) returns[0];
		//
		// Get BaseName, ModuleType, GuidValue, Version
		// which in MsaHeader.
		//
		String name = msaHeader.getModuleName();
        String moduleType = "";
        if (msaHeader.getModuleType() != null) {
            moduleType = msaHeader.getModuleType().toString();
        }
        
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

		XmlObject[] queryResult = get("Externs", xPath);
		if (queryResult == null) {
			return new String[0];
		}

		String[] specificationList = new String[queryResult.length];
		for (int i = 0; i < queryResult.length; i++) {
			// specificationList[i] = ((SpecificationDocument.Specification)
			// queryResult[i])
			// .getStringValue();
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

		XmlObject[] returns = get("PackageSurfaceArea", xPath);
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

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

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

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

		//
		// Create Map, Key - ModuleType, String - PackageInclude Header file.
		//
		Map<String, String> packageIncludeMap = new HashMap<String, String>();

		if (returns == null) {
			return packageIncludeMap;
		}
		GlobalData.log.info("" + returns[0].getClass().getName());
		for (int i = 0; i < returns.length; i++) {
			PackageHeadersDocument.PackageHeaders.IncludePkgHeader includeHeader = (PackageHeadersDocument.PackageHeaders.IncludePkgHeader) returns[i];
			packageIncludeMap.put(includeHeader.getModuleType().toString(),
					includeHeader.getStringValue());
		}
		return packageIncludeMap;
	}

	public static PackageIdentification getSpdHeader() {
		String[] xPath = new String[] { "/SpdHeader" };

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

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

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

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
		}
		return guidDeclMap;
	}

	/**
	 * Reteive
	 */
	public static Map<String, String[]> getSpdProtocol() {
		String[] xPath = new String[] { "/ProtocolDeclarations/Entry" };

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

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

		XmlObject[] returns = get("PackageSurfaceArea", xPath);

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
	 * getModuleSupportedArchs()
	 * 
	 * This function is to Retrieve Archs one module supported.
	 * 
	 * @param
	 * @return supportArch String of supporting archs. null No arch specified in
	 *         <MouduleSupport> element.
	 */
	public static List<String> getModuleSupportedArchs() {
		String[] xPath = new String[] { "/ModuleDefinitions/SupportedArchitectures" };

		XmlObject[] returns = get("ModuleSurfaceArea", xPath);

		if (returns == null) {
			return null;
		}

		return (List<String>)returns[0];
	}

    public static XmlObject[] getSpdPcdDeclarations() {
        String[] xPath = null;
//        if (tsGuid != null){
//            xPath = new String[] { "/PcdDeclarations/PcdEntry[C_Name='" + cName + "' and TokenSpaceGuid='"+ tsGuid + "']" };
//        }
//        else{
//            xPath = new String[] { "/PcdDeclarations/PcdEntry[C_Name='" + cName  + "']" };
//        }            
        xPath = new String[] { "/PcdDeclarations/PcdEntry"};
        XmlObject[] returns = get("PackageSurfaceArea", xPath);
     
        return returns;
    }
    
    public static XmlObject[] getFpdPcdBuildDefinitions(String cName, String tsGuid, String type) {
        String[] xPath = new String[] { "/PcdBuildDefinition/PcdData[C_Name='" + cName + "' and TokenSpaceGuid='"
                                        + tsGuid + "' and DatumType!='" + type + "']" };

        XmlObject[] returns = get("ModuleSA", xPath);

        return returns;
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
		String[] xPath = new String[] { "/BuildOptions" };

		XmlObject[] result = get("FrameworkPlatformDescription", xPath);
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

		XmlObject[] returns = get("MsaHeader", xPath);
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
		XmlObject[] result = get("FrameworkPlatformDescription", xPath);
		if (result != null) {
			return (ModuleSADocument.ModuleSA[]) result;
		}
		return new ModuleSADocument.ModuleSA[0];

	}
}
