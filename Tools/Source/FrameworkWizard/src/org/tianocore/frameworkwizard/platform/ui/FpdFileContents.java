/** @file
  Java class FpdFileContents is used to parse fpd xml file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.platform.ui;

import java.io.File;
import java.io.IOException;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;

import javax.xml.namespace.QName;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.tianocore.AntTaskDocument;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.DynamicPcdBuildDefinitionsDocument;
import org.tianocore.FlashDefinitionFileDocument;
import org.tianocore.FlashDocument;
import org.tianocore.FrameworkModulesDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.ModuleSurfaceAreaDocument;
import org.tianocore.OptionDocument;
import org.tianocore.OptionsDocument;
import org.tianocore.PcdBuildDefinitionDocument;
import org.tianocore.PcdCodedDocument;
import org.tianocore.PcdDataTypes;
import org.tianocore.PcdDeclarationsDocument;
import org.tianocore.PcdItemTypes;
import org.tianocore.PlatformDefinitionsDocument;
import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.FvImageTypes;
import org.tianocore.FvImagesDocument;
import org.tianocore.LicenseDocument;
import org.tianocore.PlatformHeaderDocument;
import org.tianocore.SkuInfoDocument;
import org.tianocore.UserDefinedAntTasksDocument;
import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;
import org.tianocore.frameworkwizard.platform.ui.global.SurfaceAreaQuery;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;

/**
 This class processes fpd file contents such as add remove xml elements. 
 @since PackageEditor 1.0
**/
public class FpdFileContents {

    static final String xmlNs = "http://www.TianoCore.org/2006/Edk2.0";
    
    private PlatformSurfaceAreaDocument fpdd = null;
    
    private PlatformSurfaceAreaDocument.PlatformSurfaceArea fpdRoot = null;
    
    private PlatformHeaderDocument.PlatformHeader fpdHdr = null;
    
    private PlatformDefinitionsDocument.PlatformDefinitions fpdPlatformDefs = null;
    
    private FlashDocument.Flash fpdFlash = null;
    
    private BuildOptionsDocument.BuildOptions fpdBuildOpts = null;
    
    private FrameworkModulesDocument.FrameworkModules fpdFrameworkModules = null;
    
    private DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions fpdDynPcdBuildDefs = null;
    
    public static HashMap<String, ArrayList<String>> dynPcdMap = null;
    
    /**
     * look through all pcd data in all ModuleSA, create pcd -> ModuleSA mappings.
     */
    public void initDynPcdMap() {
      if (dynPcdMap == null) {
          dynPcdMap = new HashMap<String, ArrayList<String>>();
          List<ModuleSADocument.ModuleSA> l = getfpdFrameworkModules().getModuleSAList();
          if (l == null) {
              return;
          }
          ListIterator<ModuleSADocument.ModuleSA> li = l.listIterator();
          while (li.hasNext()) {
              ModuleSADocument.ModuleSA msa = li.next();
              if (msa.getPcdBuildDefinition() == null || msa.getPcdBuildDefinition().getPcdDataList() == null) {
                  continue;
              }
              String ModuleInfo = msa.getModuleGuid() + " " + msa.getModuleVersion() +
               " " + msa.getPackageGuid() + " " + msa.getPackageVersion();
              List<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData> lp = msa.getPcdBuildDefinition().getPcdDataList();
              ListIterator<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData> lpi = lp.listIterator();
              while (lpi.hasNext()) {
                  PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = lpi.next();
                  String pcdKey = pcdData.getCName() + " " + pcdData.getTokenSpaceGuidCName();
                  if (dynPcdMap.get(pcdKey) == null) {
                      ArrayList<String> al = new ArrayList<String>();
                      al.add(ModuleInfo + " " + pcdData.getItemType().toString());
                      dynPcdMap.put(pcdKey, al);
                  }
                  else{
                      dynPcdMap.get(pcdKey).add(ModuleInfo + " " + pcdData.getItemType().toString());
                  }
              }
          }
      }
    }
    /**
     Constructor to create a new spd file
     **/
    public FpdFileContents() {

        fpdd = PlatformSurfaceAreaDocument.Factory.newInstance();
        fpdRoot = fpdd.addNewPlatformSurfaceArea();

    }

    /**
     Constructor for existing document object
     @param psa
     **/
    public FpdFileContents(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        fpdRoot = fpd;
        fpdHdr = fpdRoot.getPlatformHeader();
        fpdPlatformDefs = fpdRoot.getPlatformDefinitions();
        fpdBuildOpts = fpdRoot.getBuildOptions();
        fpdFrameworkModules = fpdRoot.getFrameworkModules();
        fpdDynPcdBuildDefs = fpdRoot.getDynamicPcdBuildDefinitions();
        fpdFlash = fpdRoot.getFlash();
    }

    /**
     Constructor based on an existing spd file
     
     @param f Existing spd file
     **/
    public FpdFileContents(File f) {
        try {
            fpdd = PlatformSurfaceAreaDocument.Factory.parse(f);
            fpdRoot = fpdd.getPlatformSurfaceArea();
        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }
    
    public DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions getfpdDynPcdBuildDefs() {
        if (fpdDynPcdBuildDefs == null){
            fpdDynPcdBuildDefs = fpdRoot.addNewDynamicPcdBuildDefinitions();
        }
        return fpdDynPcdBuildDefs;
    }
   
    public FrameworkModulesDocument.FrameworkModules getfpdFrameworkModules() {
        if (fpdFrameworkModules == null){
            fpdFrameworkModules = fpdRoot.addNewFrameworkModules();
        }
        return fpdFrameworkModules;
    }
    
    public int getPlatformDefsSkuInfoCount(){
        if (getfpdPlatformDefs().getSkuInfo() == null || getfpdPlatformDefs().getSkuInfo().getUiSkuNameList() == null) {
            return 0;
        }
        return getfpdPlatformDefs().getSkuInfo().getUiSkuNameList().size();
    }
    
    public void getPlatformDefsSkuInfos(String[][] saa){
        if (getfpdPlatformDefs().getSkuInfo() == null || getfpdPlatformDefs().getSkuInfo().getUiSkuNameList() == null) {
            return ;
        }
        
        List<SkuInfoDocument.SkuInfo.UiSkuName> l = getfpdPlatformDefs().getSkuInfo().getUiSkuNameList();
        ListIterator<SkuInfoDocument.SkuInfo.UiSkuName> li = l.listIterator();
        int i = 0;
        while(li.hasNext()) {
            SkuInfoDocument.SkuInfo.UiSkuName sku = li.next();
            saa[i][0] = sku.getSkuID()+"";
            saa[i][1] = sku.getStringValue();
            ++i;
        }
    }
    
    public int getFrameworkModulesCount() {
        if (getfpdFrameworkModules().getModuleSAList() == null){
            return 0;
        }
        return getfpdFrameworkModules().getModuleSAList().size();
    }
    
    public void getFrameworkModulesInfo(String[][] saa) {
        if (getFrameworkModulesCount() == 0){
            return;
        }
        
        ListIterator li = getfpdFrameworkModules().getModuleSAList().listIterator();
        int i = 0;
        while(li.hasNext()) {
            ModuleSADocument.ModuleSA msa = (ModuleSADocument.ModuleSA)li.next();
            saa[i][1] = msa.getModuleGuid();
            saa[i][2] = msa.getModuleVersion();
            
            saa[i][3] = msa.getPackageGuid();
            saa[i][4] = msa.getPackageVersion();
//            saa[i][4] = listToString(msa.getSupArchList());
            ++i;
        }
    }
    
    public ModuleSADocument.ModuleSA getModuleSA(String key) {
        String[] s = key.split(" ");
        if (getfpdFrameworkModules().getModuleSAList() == null) {
            return null;
        }
        ListIterator li = getfpdFrameworkModules().getModuleSAList().listIterator();
        while(li.hasNext()) {
            ModuleSADocument.ModuleSA msa = (ModuleSADocument.ModuleSA)li.next();
            if (msa.getModuleGuid().equals(s[0]) && msa.getModuleVersion().equals(s[1])
                            && msa.getPackageGuid().equals(s[2]) && msa.getPackageVersion().equals(s[3])) {
                
                return msa;
            }
        }
        return null;
    }
    public void removeModuleSA(int i) {
        XmlObject o = getfpdFrameworkModules();
        if (o == null) {
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            //
            // remove pcd from dynPcdMap, if DynamicPcdBuildData exists, remove them too.
            //
            ModuleSADocument.ModuleSA moduleSa = (ModuleSADocument.ModuleSA)cursor.getObject();
            String moduleInfo = moduleSa.getModuleGuid() + " " + moduleSa.getModuleVersion() + " " +
            moduleSa.getPackageGuid()+ " " + moduleSa.getPackageVersion();
            PcdBuildDefinitionDocument.PcdBuildDefinition pcdBuildDef = moduleSa.getPcdBuildDefinition();
            if (pcdBuildDef != null) {
                maintainDynPcdMap(pcdBuildDef, moduleInfo);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    private void maintainDynPcdMap(PcdBuildDefinitionDocument.PcdBuildDefinition o, String moduleInfo) {
        XmlCursor cursor = o.newCursor();
        boolean fromLibInstance = false;
        if (!cursor.toFirstChild()){
            return;
        }
        //
        // deal with first child, same process in the while loop below for siblings.
        //
        PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData)cursor.getObject();
        String pcdKey = pcdData.getCName() + " " + pcdData.getTokenSpaceGuidCName();
        ArrayList<String> al = dynPcdMap.get(pcdKey);
        for(int i = 0; i < al.size(); ++i){
            if (al.get(i).startsWith(moduleInfo)){
                fromLibInstance = true;
                break;
            }
        }
        al.remove(moduleInfo + " " + pcdData.getItemType().toString());
        if (al.size() == 0) {
            dynPcdMap.remove(pcdKey);
        }
        
        if (pcdData.getItemType().toString().equals("DYNAMIC")) {
            if (dynPcdMap.get(pcdKey) == null) {
                removeDynamicPcdBuildData(pcdData.getCName(), pcdData.getTokenSpaceGuidCName());
            }
        }
        if (fromLibInstance){
            cursor.removeXml();
        }
        while(cursor.toNextSibling()) {
            fromLibInstance = false;
            pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData)cursor.getObject();
            //
            // remove each pcd record from dynPcdMap
            //
            pcdKey = pcdData.getCName() + " " + pcdData.getTokenSpaceGuidCName();
            al = dynPcdMap.get(pcdKey);
            for(int i = 0; i < al.size(); ++i){
                if (al.get(i).startsWith(moduleInfo)){
                    fromLibInstance = true;
                    break;
                }
            }
            al.remove(moduleInfo + " " + pcdData.getItemType().toString());
            if (al.size() == 0) {
                dynPcdMap.remove(pcdKey);
            }
            
            if (pcdData.getItemType().toString().equals("DYNAMIC")) {
                //
                // First check whether this is the only consumer of this dyn pcd.
                //
                if (dynPcdMap.get(pcdKey) == null) {
                    //
                    // delete corresponding entry in DynamicPcdBuildData
                    //
                    removeDynamicPcdBuildData(pcdData.getCName(), pcdData.getTokenSpaceGuidCName());
                }
            }
            if (fromLibInstance){
                cursor.removeXml();
            }
        }
    }
    //
    // key for ModuleSA : "ModuleGuid ModuleVer PackageGuid PackageVer"
    //
    public int getPcdDataCount(String key){
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getPcdBuildDefinition() == null || msa.getPcdBuildDefinition().getPcdDataList() == null){
            return 0;
        }
        return msa.getPcdBuildDefinition().getPcdDataList().size();
    }
    
    public void getPcdData(String key, String[][] saa) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getPcdBuildDefinition() == null || msa.getPcdBuildDefinition().getPcdDataList() == null){
            return;
        }
        ListIterator<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData>li = msa.getPcdBuildDefinition().getPcdDataList().listIterator();
        for (int i = 0; i < saa.length; ++i) {
            PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = li.next();
            saa[i][0] = pcdData.getCName();
            saa[i][1] = pcdData.getTokenSpaceGuidCName();
            saa[i][2] = pcdData.getItemType().toString();
            saa[i][3] = pcdData.getToken().toString();
            saa[i][4] = pcdData.getDatumType().toString();
            saa[i][5] = pcdData.getValue();
            
        }
    }
    //
    // key for ModuleSA : "ModuleGuid ModuleVer PackageGuid PackageVer"
    //
    public int getLibraryInstancesCount(String key) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getLibraries() == null || msa.getLibraries().getInstanceList() == null){
            return 0;
        }
        return msa.getLibraries().getInstanceList().size();
    }
    
    public void getLibraryInstances(String key, String[][] saa){
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getLibraries() == null || msa.getLibraries().getInstanceList() == null){
            return ;
        }
        
        ListIterator<LibrariesDocument.Libraries.Instance> li = msa.getLibraries().getInstanceList().listIterator();
        for (int i = 0; i < saa.length; ++i) {
            LibrariesDocument.Libraries.Instance instance = li.next();
            saa[i][1] = instance.getModuleGuid();
            saa[i][2] = instance.getModuleVersion();
            saa[i][3] = instance.getPackageGuid();
            saa[i][4] = instance.getPackageVersion();
        }
    }
    
    public void removeLibraryInstances(String key) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getLibraries() == null){
            return ;
        }
        
        XmlCursor cursor = msa.getLibraries().newCursor();
        cursor.removeXml();
        cursor.dispose();
    }
    
    public void genLibraryInstance(String mg, String mv, String pg, String pv, String key) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null){
            msa = getfpdFrameworkModules().addNewModuleSA();
        }
        LibrariesDocument.Libraries libs = msa.getLibraries();
        if(libs == null){
            libs = msa.addNewLibraries();
        }
        
        LibrariesDocument.Libraries.Instance instance = libs.addNewInstance();
        instance.setModuleGuid(mg);
        instance.setModuleVersion(mv);
        instance.setPackageGuid(pg);
        instance.setPackageVersion(pv);
        
    }
    /**add pcd information of module mi to a ModuleSA. 
     * @param mi
     * @param moduleSa if null, generate a new ModuleSA.
     */
    public void addFrameworkModulesPcdBuildDefs(ModuleIdentification mi, ModuleSADocument.ModuleSA moduleSa){
        //ToDo add Arch filter
        
        try {
            ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)GlobalData.getModuleXmlObject(mi);
            if (msa.getPcdCoded() == null) {
                return;
            }
            if (moduleSa == null) {
                moduleSa = genModuleSA(mi);
            }
            Map<String, XmlObject> m = new HashMap<String, XmlObject>();
            m.put("ModuleSurfaceArea", msa);
            SurfaceAreaQuery.setDoc(m);
            PackageIdentification[] depPkgs = SurfaceAreaQuery.getDependencePkg(null);
            //
            // Implementing InitializePlatformPcdBuildDefinitions
            //
            List<PcdCodedDocument.PcdCoded.PcdEntry> l = msa.getPcdCoded().getPcdEntryList();
            ListIterator li = l.listIterator();
            while(li.hasNext()) {
                PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry)li.next();
                PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = LookupPcdDeclaration(msaPcd, depPkgs);
                if (spdPcd == null) {
                    //
                    // ToDo Error 
                    //
                    break;
                }
                //
                // AddItem to ModuleSA PcdBuildDefinitions
                //
                String defaultVal = msaPcd.getDefaultValue() == null ? spdPcd.getDefaultValue() : msaPcd.getDefaultValue();
                genPcdData(msaPcd.getCName(), spdPcd.getToken(), msaPcd.getTokenSpaceGuidCName(), msaPcd.getPcdItemType().toString(), spdPcd.getDatumType()+"", defaultVal, moduleSa);
            }
            
        }
        catch (Exception e){
            e.printStackTrace();
        }
        
    }
    
    private PcdDeclarationsDocument.PcdDeclarations.PcdEntry LookupPcdDeclaration (PcdCodedDocument.PcdCoded.PcdEntry msaPcd, PackageIdentification[] depPkgs) {
        
        Map<String, XmlObject> m = new HashMap<String, XmlObject>();
        PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = null;
        for (int i = 0; i < depPkgs.length; ++i) {
            m.put("PackageSurfaceArea", GlobalData.getPackageXmlObject(depPkgs[i]));
            SurfaceAreaQuery.setDoc(m);
            XmlObject[] xo = SurfaceAreaQuery.getSpdPcdDeclarations();
            if (xo == null) {
                continue;
            }
            for (int j = 0; j < xo.length; ++j) {
                spdPcd = (PcdDeclarationsDocument.PcdDeclarations.PcdEntry)xo[j];
                if (msaPcd.getTokenSpaceGuidCName() == null) {
                    if (spdPcd.getCName().equals(msaPcd.getCName())) {
                        return spdPcd;
                    }
                }
                else{
                    if (spdPcd.getCName().equals(msaPcd.getCName()) && spdPcd.getTokenSpaceGuidCName().equals(msaPcd.getTokenSpaceGuidCName())) {
                        return spdPcd;
                    }
                }
                
            }
      
        }
        return null;
    }
    
    private ModuleSADocument.ModuleSA genModuleSA (ModuleIdentification mi) {
        PackageIdentification pi = GlobalData.getPackageForModule(mi);
        ModuleSADocument.ModuleSA msa = getfpdFrameworkModules().addNewModuleSA();
        msa.setModuleGuid(mi.getGuid());
        msa.setModuleVersion(mi.getVersion());
        msa.setPackageGuid(pi.getGuid());
        msa.setPackageVersion(pi.getVersion());
        
        return msa;
    }
    
    private void genPcdData (String cName, Object token, String tsGuid, String itemType, String dataType, String defaultVal, ModuleSADocument.ModuleSA moduleSa) {
        if (moduleSa.getPcdBuildDefinition() == null){
            moduleSa.addNewPcdBuildDefinition();
        }
        //
        // constructe pcd to modulesa mapping first.
        // Attention : for any error condition, remove from map this pcd.
        //
        ArrayList<String> pcdConsumer = LookupPlatformPcdData(cName + " " + tsGuid);
        if (pcdConsumer == null) {
            pcdConsumer = new ArrayList<String>();
        }
        String listValue = moduleSa.getModuleGuid() + " " + moduleSa.getModuleVersion() 
        + " " + moduleSa.getPackageGuid() + " " + moduleSa.getPackageVersion() 
        + " " + itemType;
        pcdConsumer.add(listValue);
        dynPcdMap.put(cName + " " + tsGuid, pcdConsumer);
        //
        // Special dynamic type, if this pcd already exists in other ModuleSA
        //
        if (itemType.equals("DYNAMIC")) {
            
            ListIterator li = pcdConsumer.listIterator();
            while(li.hasNext()) {
                String value = li.next().toString();
                String[] valuePart= value.split(" ");
                if (!valuePart[4].equals("DYNAMIC")) {
                    //ToDo error for same pcd, other type than dynamic
                    pcdConsumer.remove(listValue);
                    return;
                }
            }
        }
        else {
            ListIterator li = pcdConsumer.listIterator();
            while(li.hasNext()) {
                String value = li.next().toString();
                String[] valuePart= value.split(" ");
                if (valuePart[4].equals("DYNAMIC")) {
                    //ToDo error for same pcd, other type than non-dynamic
                    pcdConsumer.remove(listValue);
                    return;
                }
            }
        }
        
        PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData fpdPcd = moduleSa.getPcdBuildDefinition().addNewPcdData();
        fpdPcd.setCName(cName);
        fpdPcd.setToken(token);
        fpdPcd.setTokenSpaceGuidCName(tsGuid);
        fpdPcd.setDatumType(PcdDataTypes.Enum.forString(dataType));
        fpdPcd.setItemType(PcdItemTypes.Enum.forString(itemType));
        
        if (itemType.equals("DYNAMIC") || itemType.equals("DYNAMIC_EX")) {
            ArrayList<String> al = LookupDynamicPcdBuildDefinition(cName + " " + tsGuid);
            //
            // if only one module mapped to this pcd, then the one is myself. so no other module mapped.
            // so need to add one dyn pcd.
            //
            if (al.size() == 1) {
                addDynamicPcdBuildData(cName, token, tsGuid, itemType, dataType, defaultVal);
            }
        }
        else {
            if (defaultVal != null){
                fpdPcd.setValue(defaultVal);
            }
            else {
                if (dataType.equals("UINT8") || dataType.equals("UINT16") || dataType.equals("UINT32") || dataType.equals("UINT64")) {
                    fpdPcd.setValue("0");
                }
                if (dataType.equals("BOOLEAN")){
                    fpdPcd.setValue("false");
                }
                if (dataType.equals("VOID*")) {
                    fpdPcd.setValue("");
                }
            }
            if (dataType.equals("UINT8")){
                fpdPcd.setMaxDatumSize(1);
            }
            if (dataType.equals("UINT16")) {
                fpdPcd.setMaxDatumSize(2);
            }
            if (dataType.equals("UINT32")) {
                fpdPcd.setMaxDatumSize(4);
            }
            if (dataType.equals("UINT64")){
                fpdPcd.setMaxDatumSize(8);
            }
            if (dataType.equals("BOOLEAN")){
                fpdPcd.setMaxDatumSize(1);
            }
            if (dataType.equals("VOID*")) {
                int maxSize = setMaxSizeForPointer(fpdPcd.getValue());
                fpdPcd.setMaxDatumSize(maxSize);
            }
        }
    }
    
    private int setMaxSizeForPointer(String datum) {
        if (datum == null) {
            return 0;
        }
        char    ch     = datum.charAt(0);
        int     start, end;
        String  strValue;
        //
        // For void* type PCD, only three datum is support:
        // 1) Unicode: string with start char is "L"
        // 2) Ansci: String  is ""
        // 3) byte array: String start char "{"
        // 
        if (ch == 'L') {
            start       = datum.indexOf('\"');
            end         = datum.lastIndexOf('\"');
            if ((start > end)           || 
                (end   > datum.length())||
                ((start == end) && (datum.length() > 0))) {
                //ToDo Error handling here
            }

            strValue    = datum.substring(start + 1, end);
            return strValue.length() * 2;
        } else if (ch == '\"'){
            start       = datum.indexOf('\"');
            end         = datum.lastIndexOf('\"');
            if ((start > end)           || 
                (end   > datum.length())||
                ((start == end) && (datum.length() > 0))) {
                
            }
            strValue    = datum.substring(start + 1, end);
            return strValue.length();
        } else if (ch =='{') {
            String[]  strValueArray;

            start           = datum.indexOf('{');
            end             = datum.lastIndexOf('}');
            strValue        = datum.substring(start + 1, end);
            strValue        = strValue.trim();
            if (strValue.length() == 0) {
                return 0;
            }
            strValueArray   = strValue.split(",");
            for (int index = 0; index < strValueArray.length; index ++) {
                    Integer value = Integer.decode(strValueArray[index].trim());
                
                if (value > 0xFF) {
//                   "[FPD file error] The datum type of PCD %s in %s is VOID*, "+
//                   "it is byte array in fact. But the element of %s exceed the byte range",
                                                    
                }
            }
            return strValueArray.length;


        } else {
//            "[FPD file error] The datum type of PCD %s in %s is VOID*. For VOID* type, you have three format choise:\n "+
//            "1) UNICODE string: like L\"xxxx\";\r\n"+
//            "2) ANSIC string: like \"xxx\";\r\n"+
//            "3) Byte array: like {0x2, 0x45, 0x23}\r\n"+
//            "but the datum in seems does not following above format!",
              return -1;                             
            
        }
    }
    
    private ArrayList<String> LookupDynamicPcdBuildDefinition(String dynPcdKey) {
        ArrayList<String> al = dynPcdMap.get(dynPcdKey);
        
        return al;
    }
    
    private ArrayList<String> LookupPlatformPcdData(String pcdKey) {
        
        return dynPcdMap.get("pcdKey");
    }
    
    public int getDynamicPcdBuildDataCount() {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null) {
            return 0;
        }
        return getfpdDynPcdBuildDefs().getPcdBuildDataList().size();
    }
    
    public void getDynamicPcdBuildData(String[][] saa) {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null) {
            return ;
        }
        List<DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData> l = getfpdDynPcdBuildDefs().getPcdBuildDataList();
        ListIterator<DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData> li = l.listIterator();
        int i = 0;
        while(li.hasNext()) {
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData dynPcd = li.next();
            saa[i][0] = dynPcd.getCName();
            saa[i][1] = dynPcd.getToken().toString();
            saa[i][2] = dynPcd.getTokenSpaceGuidCName();
            saa[i][3] = dynPcd.getMaxDatumSize()+"";
            saa[i][4] = dynPcd.getDatumType().toString();
            
            ++i;
        }
    }
    
    private void addDynamicPcdBuildData(String cName, Object token, String tsGuid, String itemType, String dataType, String defaultVal) {
        DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData dynPcdData = getfpdDynPcdBuildDefs().addNewPcdBuildData();
        dynPcdData.setItemType(PcdItemTypes.Enum.forString(itemType));
        dynPcdData.setCName(cName);
        dynPcdData.setToken(token);
        dynPcdData.setTokenSpaceGuidCName(tsGuid);
        dynPcdData.setDatumType(PcdDataTypes.Enum.forString(dataType));
        
        BigInteger bigInt = new BigInteger("0");
        DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo skuInfo = dynPcdData.addNewSkuInfo();
        skuInfo.setSkuId(bigInt);
        if (defaultVal != null){
            skuInfo.setValue(defaultVal);
        }
        else {
            if (dataType.equals("UINT8")){
                skuInfo.setValue("0");
            }
            if (dataType.equals("UINT16")) {
                skuInfo.setValue("0");
            }
            if (dataType.equals("UINT32")) {
                skuInfo.setValue("0");
            }
            if (dataType.equals("UINT64")){
                skuInfo.setValue("0");
            }
            if (dataType.equals("BOOLEAN")){
                skuInfo.setValue("false");
            }
            if (dataType.equals("VOID*")) {
                skuInfo.setValue("");
            }
        }
        if (dataType.equals("UINT8")){
            dynPcdData.setMaxDatumSize(1);
        }
        if (dataType.equals("UINT16")) {
            dynPcdData.setMaxDatumSize(2);
        }
        if (dataType.equals("UINT32")) {
            dynPcdData.setMaxDatumSize(4);
        }
        if (dataType.equals("UINT64")){
            dynPcdData.setMaxDatumSize(8);
        }
        if (dataType.equals("BOOLEAN")){
            dynPcdData.setMaxDatumSize(1);
        }
        if (dataType.equals("VOID*")) {
            int maxSize = setMaxSizeForPointer(defaultVal);
            dynPcdData.setMaxDatumSize(maxSize);
        }
    }
    
    private void removeDynamicPcdBuildData(String cName, String tsGuid) {
        XmlObject o = getfpdDynPcdBuildDefs();
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdBuildData = 
                (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            while (!(pcdBuildData.getCName().equals(cName) && pcdBuildData.getTokenSpaceGuidCName().equals(tsGuid))) {
                cursor.toNextSibling();
                pcdBuildData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            }
            
            cursor.removeXml();
        }
        cursor.dispose();
    }
    //
    // Get the Sku Info count of ith dyn pcd element.
    //
    public int getDynamicPcdSkuInfoCount(int i){
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return 0;
        }
        
        int skuInfoCount = 0;
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            if (pcdData.getSkuInfoList() == null) {
                skuInfoCount = 0;
            }
            else {
                skuInfoCount = pcdData.getSkuInfoList().size();
            }
        }
        cursor.dispose();
        return skuInfoCount;
    }
    
    public void getDynamicPcdSkuInfos(int i, String[][] saa){
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            if (pcdData.getSkuInfoList() == null) {
                cursor.dispose();
                return;
            }
            else {
                ListIterator<DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo> li = pcdData.getSkuInfoList().listIterator();
                int k = 0;
                while (li.hasNext()) {
                    DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo skuInfo = li.next();
                    saa[k][0] = skuInfo.getSkuId()+"";
                    saa[k][1] = skuInfo.getVariableName();
                    saa[k][2] = skuInfo.getVariableGuid();
                    saa[k][3] = skuInfo.getVariableOffset();
                    saa[k][4] = skuInfo.getHiiDefaultValue();
                    saa[k][5] = skuInfo.getVpdOffset();
                    saa[k][6] = skuInfo.getValue();
                    ++k;
                }
                
            }
        }
        cursor.dispose();
       
    }
    
    public String getDynamicPcdBuildDataValue(int i){
        String value = null;
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return value;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            if (pcdData.getSkuInfoList() == null) {
                value = null;
            }
            else {
                value = pcdData.getSkuInfoArray(0).getValue();
            }
        }
        cursor.dispose();
        return value;
    }
    
    public String getDynamicPcdBuildDataVpdOffset(int i){
        String vpdOffset = null;
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return vpdOffset;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            if (pcdData.getSkuInfoList() == null) {
                vpdOffset = null;
            }
            else {
                vpdOffset = pcdData.getSkuInfoArray(0).getVpdOffset();
            }
        }
        cursor.dispose();
        return vpdOffset;
    }
    
    public void removeDynamicPcdBuildDataSkuInfo(int i) {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            if (pcdData.getSkuInfoList() == null) {
                cursor.dispose();
                return;
            }
            else {
                QName qSkuInfo = new QName(xmlNs, "SkuInfo");
                cursor.toChild(qSkuInfo);
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    //
    // generate sku info for ith dyn pcd build data.
    //
    public void genDynamicPcdBuildDataSkuInfo(String id, String varName, String varGuid, String varOffset, 
                                              String hiiDefault, String vpdOffset, String value, int i) {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo skuInfo = pcdData.addNewSkuInfo();
            skuInfo.setSkuId(new BigInteger(id));
            if (varName != null){
                skuInfo.setVariableName(varName);
                skuInfo.setVariableGuid(varGuid);
                skuInfo.setVariableOffset(varOffset);
                skuInfo.setHiiDefaultValue(hiiDefault);
            }
            else if (vpdOffset != null){
                skuInfo.setVpdOffset(vpdOffset);
            }
            else{
                skuInfo.setValue(value);
            }
        }
    }
    
    public void updateDynamicPcdBuildDataSkuInfo(String id, String varName, String varGuid, String varOffset, 
                                                 String hiiDefault, String vpdOffset, String value, int i){
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            return;
        }
        
        XmlCursor cursor = getfpdDynPcdBuildDefs().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdData = (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
            ListIterator<DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo> li = pcdData.getSkuInfoList().listIterator();
            while (li.hasNext()) {
                DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData.SkuInfo skuInfo = li.next();
                if (skuInfo.getSkuId().toString().equals(id)){
                    if (varName != null){
                        skuInfo.setVariableName(varName);
                        skuInfo.setVariableGuid(varGuid);
                        skuInfo.setVariableOffset(varOffset);
                        skuInfo.setHiiDefaultValue(hiiDefault);
                    }
                    else if (vpdOffset != null){
                        skuInfo.setVpdOffset(vpdOffset);
                    }
                    else{
                        skuInfo.setValue(value);
                    }
                    break;
                }
            }
        }
    }
    
    public void removePcdDataFromLibraryInstance(String moduleKey, String libInstanceKey){
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(moduleKey);
        //
        //  should better maintain pcd from lib instance only, but maintain all is acceptable now. 
        //
        maintainDynPcdMap(moduleSa.getPcdBuildDefinition(), libInstanceKey);
        
    }
    
    public BuildOptionsDocument.BuildOptions getfpdBuildOpts() {
        if (fpdBuildOpts == null) {
            fpdBuildOpts = fpdRoot.addNewBuildOptions();
        }
        return fpdBuildOpts;
    }
    
    public void genBuildOptionsUserDefAntTask (String id, String fileName, String execOrder) {
        UserDefinedAntTasksDocument.UserDefinedAntTasks udats = getfpdBuildOpts().getUserDefinedAntTasks();
        if (udats == null) {
            udats = getfpdBuildOpts().addNewUserDefinedAntTasks();
        }
        
        AntTaskDocument.AntTask at = udats.addNewAntTask();
        setBuildOptionsUserDefAntTask(id, fileName, execOrder, at);
    }
    
    private void setBuildOptionsUserDefAntTask(String id, String fileName, String execOrder, AntTaskDocument.AntTask at) {
        at.setId(new Integer(id));
        if (fileName != null){
            at.setFilename(fileName);
        }
        if (execOrder != null) {
            at.setAntCmdOptions(execOrder);
        }
    }
    
    public void removeBuildOptionsUserDefAntTask(int i) {
        XmlObject o = getfpdBuildOpts().getUserDefinedAntTasks();
        if (o == null) {
            return;
        }
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsUserDefAntTask(int i, String id, String fileName, String execOrder){
        XmlObject o = getfpdBuildOpts().getUserDefinedAntTasks();
        if (o == null) {
            return;
        }
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            AntTaskDocument.AntTask at = (AntTaskDocument.AntTask)cursor.getObject();
            setBuildOptionsUserDefAntTask(id, fileName, execOrder, at);
        }
        cursor.dispose();
    }
    
    public int getBuildOptionsUserDefAntTaskCount() {
        UserDefinedAntTasksDocument.UserDefinedAntTasks udats = getfpdBuildOpts().getUserDefinedAntTasks();
        if (udats == null || udats.getAntTaskList() == null) {
            return 0;
        }
        
        return udats.getAntTaskList().size();
    }
    
    public void getBuildOptionsUserDefAntTasks(String[][] saa) {
        UserDefinedAntTasksDocument.UserDefinedAntTasks udats = getfpdBuildOpts().getUserDefinedAntTasks();
        if (udats == null || udats.getAntTaskList() == null) {
            return ;
        }
        
        List<AntTaskDocument.AntTask> l = udats.getAntTaskList();
        ListIterator li = l.listIterator();
        int i = 0;
        while (li.hasNext()) {
            AntTaskDocument.AntTask at = (AntTaskDocument.AntTask)li.next();
            saa[i][0] = at.getId() + "";
            saa[i][1] = saa[i][2] = "";
            if (at.getFilename() != null){
                saa[i][1] = at.getFilename();
            }
            if (at.getAntCmdOptions() != null) {
                saa[i][2] = at.getAntCmdOptions();
            }
            ++i;
        }
    }
    public void genBuildOptionsOpt(String buildTargets, String toolChain, String tagName, String toolCmd, String archList, String contents) {
        OptionsDocument.Options opts = getfpdBuildOpts().getOptions();
        if (opts == null) {
            opts = getfpdBuildOpts().addNewOptions();
        }
        OptionDocument.Option opt = opts.addNewOption();
        setBuildOptionsOpt(buildTargets, toolChain, tagName, toolCmd, archList, contents, opt);
    }
    
    private void setBuildOptionsOpt(String buildTargets, String toolChain, String tagName, String toolCmd, String archList, String contents, OptionDocument.Option opt){
        opt.setStringValue(contents);
//        opt.setBuildTargets(buildTargets);
        opt.setToolChainFamily(toolChain);
        opt.setTagName(tagName);
        opt.setToolCode(toolCmd);
        String[] s = archList.split(" ");
        ArrayList<String> al = new ArrayList<String>();
        for (int i = 0; i < s.length; ++i) {
            al.add(s[i]);
        }
        opt.setSupArchList(al);
    }
    
    public void removeBuildOptionsOpt(int i){
    
        XmlObject o = getfpdBuildOpts().getOptions();
        if (o == null) {
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsOpt(int i, String buildTargets, String toolChain, String tagName, String toolCmd, String archList, String contents) {
        XmlObject o = getfpdBuildOpts().getOptions();
        if (o == null) {
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            OptionDocument.Option opt = (OptionDocument.Option)cursor.getObject();
            setBuildOptionsOpt(buildTargets, toolChain, tagName, toolCmd, archList, contents, opt);
        }
        cursor.dispose();
    }
    
    public int getBuildOptionsOptCount(){
        if (getfpdBuildOpts().getOptions() == null || getfpdBuildOpts().getOptions().getOptionList() == null) {
            return 0;
        }
        return getfpdBuildOpts().getOptions().getOptionList().size();
    }
    
    public void getBuildOptionsOpts(String[][] saa) {
        if (getfpdBuildOpts().getOptions() == null || getfpdBuildOpts().getOptions().getOptionList() == null) {
            return ;
        }
        
        List<OptionDocument.Option> lOpt = getfpdBuildOpts().getOptions().getOptionList();
        ListIterator li = lOpt.listIterator();
        int i = 0;
        while(li.hasNext()) {
            OptionDocument.Option opt = (OptionDocument.Option)li.next();
//            saa[i][0] = opt.getBuildTargets();
            saa[i][1] = opt.getToolChainFamily();
            if (opt.getSupArchList() != null){
                Object[] archs = opt.getSupArchList().toArray();
                saa[i][2] = " ";
                for (int j = 0; j < archs.length; ++j){
                    saa[i][2] += archs[j];
                    saa[i][2] += " ";
                }
                saa[i][2] = saa[i][2].trim();
            }
            saa[i][3] = opt.getToolCode();
            saa[i][4] = opt.getTagName();
            saa[i][5] = opt.getStringValue();
             
            ++i;
        }
    }
    
    public PlatformDefinitionsDocument.PlatformDefinitions getfpdPlatformDefs(){
        if (fpdPlatformDefs == null){
            fpdPlatformDefs = fpdRoot.addNewPlatformDefinitions();
        }
        return fpdPlatformDefs;
    }
    
    public FlashDocument.Flash getfpdFlash() {
        if (fpdFlash == null) {
            fpdFlash = fpdRoot.addNewFlash();
        }
        return fpdFlash;
    }
    
    public void genFlashDefinitionFile(String file) {
        FlashDefinitionFileDocument.FlashDefinitionFile fdf = getfpdFlash().getFlashDefinitionFile();
        if (fdf == null) {
            fdf = getfpdFlash().addNewFlashDefinitionFile();
        }
        
        fdf.setStringValue(file);
    }
    
    public String getFlashDefinitionFile() {
        FlashDefinitionFileDocument.FlashDefinitionFile fdf = getfpdFlash().getFlashDefinitionFile();
        if (fdf == null) {
            return "";
        }
        
        return fdf.getStringValue();
    }
    
    
    
    public void genFvImagesNameValue(String name, String value) {
      
        FvImagesDocument.FvImages fi = getfpdFlash().getFvImages();
        if (fi == null) {
            fi = getfpdFlash().addNewFvImages();
        }
        
        FvImagesDocument.FvImages.NameValue nv = fi.addNewNameValue();
        nv.setName(name);
        nv.setValue(value);
    }
    
    public void removeFvImagesNameValue(int i){
     
        XmlObject o = getfpdFlash().getFvImages();
        if (o == null) {
            return;
        }
        
        QName qNameValue = new QName(xmlNs, "NameValue");
        XmlCursor cursor = o.newCursor();
        if (cursor.toChild(qNameValue)) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling(qNameValue);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateFvImagesNameValue(int i, String name, String value){
        
        XmlObject o = getfpdFlash().getFvImages();
        if (o == null) {
            return;
        }
        
        QName qNameValue = new QName(xmlNs, "NameValue");
        XmlCursor cursor = o.newCursor();
        if (cursor.toChild(qNameValue)) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling(qNameValue);
            }
            FvImagesDocument.FvImages.NameValue nv = (FvImagesDocument.FvImages.NameValue)cursor.getObject();
            nv.setName(name);
            nv.setValue(value);
        }
        cursor.dispose();
    }
    
    public int getFvImagesNameValueCount() {
           
        FvImagesDocument.FvImages fi = null;
        if ((fi = getfpdFlash().getFvImages()) == null || fi.getNameValueList() == null) {
            return 0;
        }
        return fi.getNameValueList().size();
    }
    
    public void getFvImagesNameValues(String[][] nv) {
     
        FvImagesDocument.FvImages fi = getfpdFlash().getFvImages();
        if (fi == null){
            return;
        }
        List<FvImagesDocument.FvImages.NameValue> l = fi.getNameValueList();
        int i = 0;
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            FvImagesDocument.FvImages.NameValue e = (FvImagesDocument.FvImages.NameValue) li
                                                                                                                    .next();
            nv[i][0] = e.getName();
            nv[i][1] = e.getValue();
            
            i++;
        }
    }
    
    public void genFvImagesFvImage(String[] names, String types, Map<String, String> options) {
      
        FvImagesDocument.FvImages fis = null;
        if ((fis = getfpdFlash().getFvImages()) == null) {
            fis = getfpdFlash().addNewFvImages();
        }
        
        //
        //gen FvImage with FvImageNames array
        //
        FvImagesDocument.FvImages.FvImage fi = fis.addNewFvImage();
        for (int i = 0; i < names.length; ++i) {
            fi.addFvImageNames(names[i]);
        }
        fi.setType(FvImageTypes.Enum.forString(types));
        if (options != null){
            setFvImagesFvImageFvImageOptions(options, fi);
        }
    }
    
    private void setFvImagesFvImageFvImageOptions(Map<String, String> options, FvImagesDocument.FvImages.FvImage fi){
        FvImagesDocument.FvImages.FvImage.FvImageOptions fio = fi.getFvImageOptions();
        if (fio == null){
            fio = fi.addNewFvImageOptions();
        }
        
        Set<String> key = options.keySet();
        Iterator<String> i = key.iterator();
        while (i.hasNext()) {
            
            FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = fio.addNewNameValue();
            String k = (String)i.next();
            if (k.equals("EFI_ALIGNMENT_CAP")) {
                nv.setName(k);
                nv.setValue("TRUE");
                setFvImageOptionsAlign((String)options.get(k), fio);
                continue;
            }
            nv.setName(k);
            nv.setValue((String)options.get(k));
            
        }
        
    }
    
    private void setFvImageOptionsAlign(String alignValue, FvImagesDocument.FvImages.FvImage.FvImageOptions fio) {
        int numForm = -1;
        if (alignValue.endsWith("K")) {
            alignValue = alignValue.substring(0, alignValue.length()-1);
            numForm = new Integer(alignValue).intValue() * 1024;
        }
        else {
            numForm = new Integer(alignValue).intValue();
        }
        
        FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = null;
        if (numForm / (64*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_64K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_64K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (32*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_32K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_32K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (16*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_16K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_16K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (8*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_8K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_8K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (4*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_4K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_4K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (2*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_2K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_2K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (1*1024) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_1K");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_1K");
            nv.setValue("FALSE");
        }
        
        if (numForm / (512) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_512");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_512");
            nv.setValue("FALSE");
        }
        
        if (numForm / (256) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_256");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_256");
            nv.setValue("FALSE");
        }
        
        if (numForm / (128) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_128");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_128");
            nv.setValue("FALSE");
        }
        
        if (numForm / (64) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_64");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_64");
            nv.setValue("FALSE");
        }
        
        if (numForm / (32) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_32");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_32");
            nv.setValue("FALSE");
        }
        
        if (numForm / (16) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_16");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_16");
            nv.setValue("FALSE");
        }
        
        if (numForm / (8) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_8");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_8");
            nv.setValue("FALSE");
        }
        
        if (numForm / (4) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_4");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_4");
            nv.setValue("FALSE");
        }
        
        if (numForm / (2) >= 1) {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_2");
            nv.setValue("TRUE");
        }
        else {
            nv = fio.addNewNameValue();
            nv.setName("EFI_ALIGNMENT_2");
            nv.setValue("FALSE");
        }
            
    }
    
    public void removeFvImagesFvImage(int i) {
      
        XmlObject o = getfpdFlash().getFvImages();
        if (o == null) {
            return;
        }
        
        QName qFvImage = new QName(xmlNs, "FvImage");
        XmlCursor cursor = o.newCursor();
        if (cursor.toChild(qFvImage)) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling(qFvImage);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateFvImagesFvImage(int i, String[] names, String types, Map<String, String> options){
           
        XmlObject o = getfpdFlash().getFvImages();
        if (o == null) {
            return;
        }
        XmlCursor cursor = o.newCursor();
        QName qFvImage = new QName(xmlNs, "FvImage");
        if (cursor.toChild(qFvImage)) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling(qFvImage);
            }
            FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)cursor.getObject();
            fi.setType(FvImageTypes.Enum.forString(types));
            
            //
            // remove old FvImageNames before adding new ones
            //
            QName qFvImageNames = new QName(xmlNs, "FvImageNames"); 
            cursor.toChild(qFvImageNames);
            cursor.removeXml();
            while (cursor.toNextSibling(qFvImageNames)) {
                cursor.removeXml();
            }
            
            for (int k = 0; k < names.length; ++k) {
                fi.addFvImageNames(names[k]);
            }
            //
            // remove old FvImageOptions before adding new options
            //
            QName qFvImageOptions = new QName(xmlNs, "FvImageOptions");
            cursor.toNextSibling(qFvImageOptions);
            cursor.removeXml();
            
            setFvImagesFvImageFvImageOptions(options, fi);
        }
        cursor.dispose();
    }
    
    public int getFvImagesFvImageCount() {
        
        if (getfpdFlash().getFvImages() == null || getfpdFlash().getFvImages().getFvImageList() == null) {
            return 0;
        }
        return getfpdFlash().getFvImages().getFvImageList().size();
    }
    
    public void getFvImagesFvImages(String[][] saa, ArrayList<LinkedHashMap<String, String>> options) {
    
        if (getfpdFlash().getFvImages() == null) {
            return;
        }
        List<FvImagesDocument.FvImages.FvImage> l = getfpdFlash().getFvImages().getFvImageList();
        if (l == null) {
            return;
        }
        ListIterator li = l.listIterator();
        int i = 0;
        while(li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)li.next();
            //
            // get FvImageNames array, space separated
            //
            List<String> lfn = fi.getFvImageNamesList();
            ListIterator lfni = lfn.listIterator();
            saa[i][0] = " ";
            while (lfni.hasNext()) {
                saa[i][0] += (String)lfni.next();
                saa[i][0] += " ";
            }
            saa[i][0] = saa[i][0].trim();
            
            saa[i][1] = fi.getType()+"";
            
            //
            // get FvImageOptions into Map[i]
            //
            FvImagesDocument.FvImages.FvImage.FvImageOptions fo = fi.getFvImageOptions();
            if (fo == null) {
                ++i;
                continue;
            }
            List<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> lnv = fo.getNameValueList();
            if (lnv == null || lnv.isEmpty()) {
                ++i;
                continue;
            }
            ListIterator lnvi = lnv.listIterator();
            while (lnvi.hasNext()) {
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = (FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue)lnvi.next();
                Map<String, String> m = options.get(i);
                m.put(nv.getName(), nv.getValue());
            }
            
            ++i;
        }
    }
    
    /**
     Get platform header element
     @return PlatformHeaderDocument.PlatformHeader
    **/
    public PlatformHeaderDocument.PlatformHeader getFpdHdr() {
        if (fpdHdr == null) {
            fpdHdr = fpdRoot.addNewPlatformHeader();
        }
        genPlatformDefsSkuInfo("0", "DEFAULT");
        return fpdHdr;
    }
    
    public void genPlatformDefsSkuInfo(String id, String name) {
        SkuInfoDocument.SkuInfo skuInfo = null;
        if (getfpdPlatformDefs().getSkuInfo() == null) {
            skuInfo = getfpdPlatformDefs().addNewSkuInfo();
        }
        skuInfo = getfpdPlatformDefs().getSkuInfo();
        if (skuInfo.getUiSkuNameList() == null || skuInfo.getUiSkuNameList().size() == 0) {
            SkuInfoDocument.SkuInfo.UiSkuName skuName = skuInfo.addNewUiSkuName();
            skuName.setSkuID(new BigInteger("0"));
            skuName.setStringValue("DEFAULT");
        }
        if (id.equals("0")) {
            return;
        }
        SkuInfoDocument.SkuInfo.UiSkuName skuName = skuInfo.addNewUiSkuName();
        skuName.setSkuID(new BigInteger(id));
        skuName.setStringValue(name);
            
        
    }
    public String getFpdHdrPlatformName() {
        return getFpdHdr().getPlatformName();
    }
    
    public String getFpdHdrGuidValue() {
        return getFpdHdr().getGuidValue();
    }
    
    public String getFpdHdrVer() {
        return getFpdHdr().getVersion();
    }
    
    public String getFpdHdrAbs() {
        return getFpdHdr().getAbstract();
    }
    
    public String getFpdHdrDescription() {
        return getFpdHdr().getDescription();
    }
    
    public String getFpdHdrCopyright() {
        return getFpdHdr().getCopyright();
    }
    
    public String getFpdHdrLicense() {
        LicenseDocument.License l = getFpdHdr().getLicense();
        if (l == null) {
            return null;
        }
        return l.getStringValue();
    }
    
    public String getFpdHdrUrl() {
        LicenseDocument.License l = getFpdHdr().getLicense();
        if (l == null) {
            return null;
        }
        return l.getURL();
    }
    
    public String getFpdHdrSpec() {

        return "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION 0x00000052";
//        return getFpdHdr().getSpecification();
    }
    
    public void setFpdHdrPlatformName(String name){
        getFpdHdr().setPlatformName(name);
    }
    
    public void setFpdHdrGuidValue(String guid){
        getFpdHdr().setGuidValue(guid);
    }
    
    public void setFpdHdrVer(String v){
        getFpdHdr().setVersion(v);
    }
    
    public void setFpdHdrAbs(String abs) {
        getFpdHdr().setAbstract(abs);
    }
    
    public void setFpdHdrDescription(String desc){
        getFpdHdr().setDescription(desc);
    }
    
    public void setFpdHdrCopyright(String cr) {
        getFpdHdr().setCopyright(cr);
    }
    
    public void setFpdHdrLicense(String license){
        LicenseDocument.License l = getFpdHdr().getLicense();
        if (l == null) {
            getFpdHdr().addNewLicense().setStringValue(license);
        }
        else {
            l.setStringValue(license);
        }
    }

    public void setFpdHdrUrl(String url){
        LicenseDocument.License l = getFpdHdr().getLicense();
        
        l.setURL(url);
        
    }
    
    public void setFpdHdrSpec(String s){
        s = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION 0x00000052";
        getFpdHdr().setSpecification(s);
    }
    /**
    Save the processed xml contents to file
    
    @param fpdFile The file to save xml contents
    @throws IOException Exceptions during file operation
    **/
    public void saveAs(File fpdFile) throws IOException {

       XmlOptions options = new XmlOptions();

       options.setCharacterEncoding("UTF-8");
       options.setSavePrettyPrint();
       options.setSavePrettyPrintIndent(2);
       try {
           fpdd.save(fpdFile, options);
       } catch (IOException e) {
           e.printStackTrace();
       }

    }
    
    private String listToString(List<String> l) {
        if (l == null) {
            return null;
        }
        String s = " ";
        ListIterator li = l.listIterator();
        while(li.hasNext()) {
            s += li.next();
            s += " ";
        }
        return s.trim();
    }
}
