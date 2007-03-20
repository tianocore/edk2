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
import java.util.Vector;

import javax.xml.namespace.QName;

import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.apache.xmlbeans.XmlOptions;
import org.tianocore.AntTaskDocument;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.DynamicPcdBuildDefinitionsDocument;
import org.tianocore.EfiSectionType;
import org.tianocore.FlashDefinitionFileDocument;
import org.tianocore.FlashDocument;
import org.tianocore.FrameworkModulesDocument;
import org.tianocore.IntermediateOutputType;
import org.tianocore.LibrariesDocument;
import org.tianocore.ModuleSADocument;
import org.tianocore.ModuleSaBuildOptionsDocument;
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
import org.tianocore.UserExtensionsDocument;
import org.tianocore.LibrariesDocument.Libraries.Instance;
import org.tianocore.frameworkwizard.platform.ui.global.WorkspaceProfile;
import org.tianocore.frameworkwizard.platform.ui.global.SurfaceAreaQuery;
import org.tianocore.frameworkwizard.module.Identifications.ModuleIdentification;
import org.tianocore.frameworkwizard.packaging.PackageIdentification;
import org.tianocore.frameworkwizard.workspace.Workspace;

/**
 This class processes fpd file contents such as add remove xml elements. 
 @since PackageEditor 1.0
**/
public class FpdFileContents {

    static final String xmlNs = "http://www.TianoCore.org/2006/Edk2.0";
    static final String regExpNewLineAndSpaces = "((\n)|(\r\n)|(\r)|(\u0085)|(\u2028)|(\u2029))(\\s)*";
    
    private PlatformSurfaceAreaDocument fpdd = null;
    
    private PlatformSurfaceAreaDocument.PlatformSurfaceArea fpdRoot = null;
    
    private PlatformHeaderDocument.PlatformHeader fpdHdr = null;
    
    private PlatformDefinitionsDocument.PlatformDefinitions fpdPlatformDefs = null;
    
    private FlashDocument.Flash fpdFlash = null;
    
    private BuildOptionsDocument.BuildOptions fpdBuildOpts = null;
    
    private FrameworkModulesDocument.FrameworkModules fpdFrameworkModules = null;
    
    private DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions fpdDynPcdBuildDefs = null;
    
    private HashMap<String, ArrayList<String>> dynPcdMap = null;
    
    private HashMap<String, String> defaultPcdValue = new HashMap<String, String>();
    
    private String itemType (String pcdInfo) {
        
        return pcdInfo.substring(pcdInfo.lastIndexOf(" ") + 1);
    }
    
    /**
     * look through all pcd data in all ModuleSA, create pcd -> ModuleSA mappings.
     */
    public void initDynPcdMap() {
      if (dynPcdMap == null) {
          dynPcdMap = new HashMap<String, ArrayList<String>>();
          List<ModuleSADocument.ModuleSA> l = getfpdFrameworkModules().getModuleSAList();
          if (l == null) {
              removeElement(getfpdFrameworkModules());
              fpdFrameworkModules = null;
              return;
          }
          ListIterator<ModuleSADocument.ModuleSA> li = l.listIterator();
          while (li.hasNext()) {
              ModuleSADocument.ModuleSA moduleSa = li.next();
              if (moduleSa.getPcdBuildDefinition() == null || moduleSa.getPcdBuildDefinition().getPcdDataList() == null) {
                  continue;
              }
              String ModuleInfo = moduleSa.getModuleGuid().toLowerCase() + " " + moduleSa.getModuleVersion() +
               " " + moduleSa.getPackageGuid().toLowerCase() + " " + moduleSa.getPackageVersion() + " " + listToString(moduleSa.getSupArchList());
              List<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData> lp = moduleSa.getPcdBuildDefinition().getPcdDataList();
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
    
    public ArrayList<String> getDynPcdMapValue(String key) {
        return dynPcdMap.get(key);
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
    
    public void getFrameworkModuleSAByFvBinding (String fvName, Vector<String[]> vGuid) {
        if (getFrameworkModulesCount() == 0){
            return;
        }
        
        ListIterator li = getfpdFrameworkModules().getModuleSAList().listIterator();
        while(li.hasNext()) {
            ModuleSADocument.ModuleSA moduleSa = (ModuleSADocument.ModuleSA)li.next();
            if (moduleSa.getModuleSaBuildOptions() == null) {
                continue;
            }
            String fvBinding = moduleSa.getModuleSaBuildOptions().getFvBinding();
            if (fvBinding == null) {
                continue;
            }
            
            String[] fvNames = fvBinding.split(" ");
            for (int i = 0; i < fvNames.length; ++i) {
                //
                // BugBug : underscore "_" should not be replaced!!! 
                // But Fv name FVMAIN from fdf file not consist with FV_MAIN in fpd file.
                //
                if (fvNames[i].equals(fvName) || fvNames[i].replaceAll("_", "").equals(fvName)) {
                    String[] sa = new String[] {moduleSa.getModuleGuid(), moduleSa.getModuleVersion(),
                                                moduleSa.getPackageGuid(), moduleSa.getPackageVersion(), 
                                                listToString(moduleSa.getSupArchList())};
                    vGuid.add(sa);
                    break;
                }
            }
        }
    }
    
    public int getFrameworkModulesCount() {
        if (getfpdFrameworkModules().getModuleSAList() == null || getfpdFrameworkModules().getModuleSAList().size() == 0){
            removeElement(getfpdFrameworkModules());
            fpdFrameworkModules = null;
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
            ModuleSADocument.ModuleSA moduleSa = (ModuleSADocument.ModuleSA)li.next();
            saa[i][0] = moduleSa.getModuleGuid();
            saa[i][1] = moduleSa.getModuleVersion();
            
            saa[i][2] = moduleSa.getPackageGuid();
            saa[i][3] = moduleSa.getPackageVersion();
            saa[i][4] = listToString(moduleSa.getSupArchList());
            ++i;
        }
    }
    
    public void getFrameworkModuleInfo(int i, String[] sa) {
        ModuleSADocument.ModuleSA msa = getModuleSA(i);
        if (msa == null) {
            return;
        }
        sa[0] = msa.getModuleGuid();
        sa[1] = msa.getModuleVersion();
        sa[2] = msa.getPackageGuid();
        sa[3] = msa.getPackageVersion();
        sa[4] = listToString(msa.getSupArchList());
    }
    
    public ModuleSADocument.ModuleSA getModuleSA(String key) {
        
        if (getfpdFrameworkModules().getModuleSAList() == null || getfpdFrameworkModules().getModuleSAList().size() == 0) {
            removeElement(getfpdFrameworkModules());
            fpdFrameworkModules = null;
            return null;
        }
        String[] s = key.split(" ");
        String archsInKey = "";
        if (s.length > 4) {
            for (int i = 4; i < s.length; ++i) {
                archsInKey += s[i];
                archsInKey += " ";
            }
            archsInKey = archsInKey.trim();
        }
        
        ListIterator li = getfpdFrameworkModules().getModuleSAList().listIterator();
        while(li.hasNext()) {
            ModuleSADocument.ModuleSA moduleSa = (ModuleSADocument.ModuleSA)li.next();
            if (moduleSa.getModuleGuid().equalsIgnoreCase(s[0]) && moduleSa.getPackageGuid().equalsIgnoreCase(s[2])) {
                if (moduleSa.getModuleVersion() != null) {
                    if (!moduleSa.getModuleVersion().equals(s[1])) {
                        continue;
                    }
                }
                if (moduleSa.getPackageVersion() != null) {
                    if (!moduleSa.getPackageVersion().equals(s[3])) {
                        continue;
                    }
                }
                //ToDo add arch check .
                if (moduleSa.getSupArchList() != null) {
                    if (listToString(moduleSa.getSupArchList()).equals(archsInKey)) {
                        return moduleSa;
                    }
                }
                else {
                    if (archsInKey.length() == 0) {
                        return moduleSa;
                    }
                }
            }
        }
        return null;
    }
    
    private ModuleSADocument.ModuleSA getModuleSA(int i) {
        ModuleSADocument.ModuleSA moduleSa = null;
        if (fpdRoot.getFrameworkModules() == null) {
            return null;
        }
        XmlCursor cursor = fpdRoot.getFrameworkModules().newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            moduleSa = (ModuleSADocument.ModuleSA)cursor.getObject();
        }
        cursor.dispose();
        return moduleSa;
    }
    
    public void removeModuleSA(int i) {
        XmlObject o = fpdRoot.getFrameworkModules();
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
            moduleSa.getPackageGuid()+ " " + moduleSa.getPackageVersion() + " " + listToString(moduleSa.getSupArchList());
            PcdBuildDefinitionDocument.PcdBuildDefinition pcdBuildDef = moduleSa.getPcdBuildDefinition();
            if (pcdBuildDef != null && pcdBuildDef.getPcdDataList() != null) {
                ListIterator<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData> li = pcdBuildDef.getPcdDataList().listIterator();
                while(li.hasNext()) {
                    PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = li.next();
                    maintainDynPcdMap(pcdData.getCName() + " " + pcdData.getTokenSpaceGuidCName(), moduleInfo);
                }
            }
            
            cursor.push();
            while (cursor.hasPrevToken()) {
                cursor.toPrevToken();
                if (!cursor.isText()) {
                    break;
                }
                if (cursor.getObject() == null) {
                    break;
                }
                String s = cursor.getTextValue();
                if (s.matches(regExpNewLineAndSpaces)) {
                    continue;
                }
            }

            if (cursor.isComment()) {
                cursor.removeXml();
            }
            
            cursor.pop();
            cursor.removeXml();
            if (getFrameworkModulesCount() == 0) {
                cursor.dispose();
                removeElement(getfpdFrameworkModules());
                fpdFrameworkModules = null;
                return;
            }
        }
        cursor.dispose();
    }
    
    public boolean adjustPcd (String seqModuleSa, Vector<String> vExceptions) throws Exception {
        boolean dataModified = false;
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(seqModuleSa);
        int pcdCount = getPcdDataCount(seqModuleSa);
        String[][] saaModuleSaPcd = new String[pcdCount][7];
        getPcdData(seqModuleSa, saaModuleSaPcd);
        String mg = moduleSa.getModuleGuid().toLowerCase();
        String mv = moduleSa.getModuleVersion();
        String pg = moduleSa.getPackageGuid().toLowerCase();
        String pv = moduleSa.getPackageVersion();
        String arch = listToString(moduleSa.getSupArchList());
        //
        // delete pcd in ModuleSA but not in MSA files any longer.
        //
        String moduleKey = mg + " " + mv + " " + pg + " " + pv + " " + arch;
        int libCount = getLibraryInstancesCount(moduleKey);
        String[][] saaLib = new String[libCount][5];
        getLibraryInstances(moduleKey, saaLib);
        ModuleIdentification mi = WorkspaceProfile.getModuleId(moduleKey);
        if (mi == null) {
            vExceptions.add("Module " + mg + " does NOT exist in workspace.");
            throw new Exception ("Module does NOT exist in workspace.");
        }
        Vector<ModuleIdentification> vMi = new Vector<ModuleIdentification>();
        //
        // create vector for module & library instance MIs.
        //
        vMi.add(mi);
        for (int j = 0; j < saaLib.length; ++j) {
            String libKey = saaLib[j][1] + " " + saaLib[j][2] + " " + saaLib[j][3] + " " + saaLib[j][4];
            ModuleIdentification libMi = WorkspaceProfile.getModuleId(libKey);
            if (libMi != null) {
                vMi.add(libMi);
            }
        }
        
    nextPcd:for (int i = 0; i < saaModuleSaPcd.length; ++i) {

                for (int j = 0; j < vMi.size(); ++j) {
                    ModuleIdentification nextMi = vMi.get(j);
                    if (nextMi == null) {
                        continue;
                    }
                    if (WorkspaceProfile.pcdInMsa(saaModuleSaPcd[i][0], saaModuleSaPcd[i][1], arch, nextMi)) {
                        continue nextPcd;
                    }
                }
                removePcdData(seqModuleSa, saaModuleSaPcd[i][0], saaModuleSaPcd[i][1]);
                dataModified = true;
            }
        //
        // add new Pcd from MSA file to ModuleSA.
        //
           for (int i = 0; i < vMi.size(); ++i) {
                ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) WorkspaceProfile
                                                                                                                          .getModuleXmlObject(vMi
                                                                                                                                                 .get(i));
                if (msa.getPcdCoded() == null || msa.getPcdCoded().getPcdEntryList() == null) {
                    continue;
                }
                ListIterator li = msa.getPcdCoded().getPcdEntryList().listIterator();
     msaPcdIter:while (li.hasNext()) {
                    PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry) li.next();
                    ArrayList<String> al = getDynPcdMapValue(msaPcd.getCName() + " " + msaPcd.getTokenSpaceGuidCName());
                    if (al != null) {
                        for (int j = 0; j < al.size(); ++j) {
                            if (al.get(j).startsWith(moduleKey)) {
                                continue msaPcdIter;
                            }
                        }
                    }
                    // Check sup arch conformance for the new PCD
                    if (msaPcd.getSupArchList() != null) {
                    	String newPcdArch = msaPcd.getSupArchList().toString();
                    	if (!newPcdArch.toLowerCase().contains(arch.toLowerCase())) {
                    		continue;
                    	}
                    }
                    
                    PackageIdentification[] depPkgs = SurfaceAreaQuery.getDependencePkg(null, vMi.get(i));
                    PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = LookupPcdDeclaration(msaPcd, depPkgs);
                    if (spdPcd == null) {
                        //
                        // ToDo Error 
                        //
                        String errorMessage = "No Declaration for PCD Entry " + msaPcd.getCName() + " in Module "
                        + mi.getName();
                        if (i != 0) {
                            errorMessage += " Library Instance " + vMi.get(i).getName(); 
                        }
                        vExceptions.add(errorMessage);
                        throw new PcdDeclNotFound(errorMessage);
                    }
                    //
                    // AddItem to ModuleSA PcdBuildDefinitions
                    //
                    String defaultVal = msaPcd.getDefaultValue() == null ? spdPcd.getDefaultValue()
                                                                        : msaPcd.getDefaultValue();

                    genPcdData(msaPcd.getCName(), spdPcd.getToken(), msaPcd.getTokenSpaceGuidCName(),
                               msaPcd.getPcdItemType().toString(), spdPcd.getDatumType() + "", defaultVal, moduleSa, spdPcd);
                    dataModified = true;
                 }

            }
        
        return dataModified;
    }
    
    private synchronized void maintainDynPcdMap(String pcdKey, String moduleInfo) {
        
        ArrayList<String> al = dynPcdMap.get(pcdKey);
        if (al == null) {
            return;
        }
        String[] s = moduleInfo.split(" ");
        for(int i = 0; i < al.size(); ++i){
            String consumer = al.get(i);
            if (consumer.contains(s[0].toLowerCase()) && consumer.contains(s[2].toLowerCase())){
                String[] consumerPart = consumer.split(" ");
                if (!consumerPart[4].equals(s[4])) {
                    continue;
                }
                al.remove(consumer);
                break;
            }
        }

        if (al.size() == 0) {
            defaultPcdValue.remove(pcdKey);
            dynPcdMap.remove(pcdKey);
            String[] s1 = pcdKey.split(" ");
            removeDynamicPcdBuildData(s1[0], s1[1]);
        }
        
    }
    //
    // key for ModuleSA : "ModuleGuid ModuleVer PackageGuid PackageVer Arch"
    //
    public int getPcdDataCount (String key){
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        
        if (msa == null || msa.getPcdBuildDefinition() == null || msa.getPcdBuildDefinition().getPcdDataList() == null){
            return 0;
        }
        return msa.getPcdBuildDefinition().getPcdDataList().size();
        
    }
    
    public void getPcdData (String key, String[][] saa) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        
        if (msa == null || msa.getPcdBuildDefinition() == null || msa.getPcdBuildDefinition().getPcdDataList() == null){
            return;
        }
        ListIterator<PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData>li = msa.getPcdBuildDefinition().getPcdDataList().listIterator();
        for (int k = 0; k < saa.length; ++k) {
            PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = li.next();
            saa[k][0] = pcdData.getCName();
            saa[k][1] = pcdData.getTokenSpaceGuidCName();
            saa[k][2] = pcdData.getItemType()+"";
            saa[k][3] = pcdData.getToken().toString();
            saa[k][4] = pcdData.getMaxDatumSize()+"";
            saa[k][5] = pcdData.getDatumType()+"";
            saa[k][6] = pcdData.getValue();
            
        }
    }
    
    public void removePcdData (String key, String cName, String tsGuid) {
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(key);
        if (moduleSa == null || moduleSa.getPcdBuildDefinition() == null){
            return;
        }
        
        String mg = moduleSa.getModuleGuid();
        String mv = moduleSa.getModuleVersion();
        String pg = moduleSa.getPackageGuid();
        String pv = moduleSa.getPackageVersion();
        String arch = listToString(moduleSa.getSupArchList());
        String moduleKey = mg + " " + mv + " " + pg + " " + pv + " " + arch;
        
        XmlCursor cursor = moduleSa.getPcdBuildDefinition().newCursor();
        if (cursor.toFirstChild()){
            
            do {
                PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData)cursor.getObject();
                if (pcdData.getCName().equals(cName) && pcdData.getTokenSpaceGuidCName().equals(tsGuid)) {
                    maintainDynPcdMap(cName + " " + tsGuid, moduleKey);
                    if (getPcdDataCount(key) == 1) {
                        cursor.toParent();
                    }
                    cursor.removeXml();
                    break;
                }
            }
            while(cursor.toNextSibling());
            
        }
        cursor.dispose();
    }
    
    public void updatePcdData (String key, String cName, String tsGuid, String itemType, String maxSize, String value){
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(key);
        if (moduleSa == null || moduleSa.getPcdBuildDefinition() == null){
            return;
        }
        
        XmlCursor cursor = moduleSa.getPcdBuildDefinition().newCursor();
        if (cursor.toFirstChild()){
            do {
                PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData)cursor.getObject();
                if (pcdData.getCName().equals(cName) && pcdData.getTokenSpaceGuidCName().equals(tsGuid)) {
                    //
                    // change item type while not updating dynPcdData????
                    //
                    if (itemType != null) {
                        pcdData.setItemType(PcdItemTypes.Enum.forString(itemType));
                    }
                    
                    if(pcdData.getDatumType().equals("VOID*") && maxSize != null) {
                        pcdData.setMaxDatumSize(new Integer(maxSize));
                    }
                    //
                    // if value input is null, keep old value untouched.
                    //
                    if (value != null) {
                        pcdData.setValue(value);
                        defaultPcdValue.put(cName + " " + tsGuid, value);
                    }
                    
                    break;
                }
            }
            while(cursor.toNextSibling());
        }
        cursor.dispose();
    }
    
    /**Get original Pcd info from MSA & SPD files.
     * @param mi ModuleIdentification from which MSA & SPD come
     * @param cName PCD cName
     * @param sa Results: HelpText, Original item type.
     * @return
     */
    public boolean getPcdBuildDataInfo(ModuleIdentification mi, String cName, String tsGuid, String[] sa, Vector<String> validPcdTypes) throws Exception{
       
        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) WorkspaceProfile
                                                                                                                        .getModuleXmlObject(mi);
        if (msa.getPcdCoded() == null) {
            return false;
        }

        PackageIdentification[] depPkgs = SurfaceAreaQuery.getDependencePkg(null, mi);
        //
        // First look through MSA pcd entries.
        //
        List<PcdCodedDocument.PcdCoded.PcdEntry> l = msa.getPcdCoded().getPcdEntryList();
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry) li.next();
            if (!msaPcd.getCName().equals(cName)) {
                continue;
            }
            if (!msaPcd.getTokenSpaceGuidCName().equals(tsGuid)) {
                continue;
            }
            PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = LookupPcdDeclaration(msaPcd, depPkgs);
            if (spdPcd == null || spdPcd.getValidUsage() == null) {
                //
                // ToDo Error 
                //
                throw new PcdDeclNotFound(mi.getName() + " " + msaPcd.getCName());
            }
            //
            // Get Pcd help text and original item type.
            //
            sa[0] = spdPcd.getHelpText() + msaPcd.getHelpText();
            sa[1] = msaPcd.getPcdItemType() + "";
            sa[2] = msa.getModuleDefinitions().getBinaryModule() + "";
            ListIterator iter = spdPcd.getValidUsage().listIterator();
            while (iter.hasNext()) {
                String usage = iter.next().toString();
                validPcdTypes.add(usage);
            }
            return true;
        }

        return false;
    }
    
    private boolean multiSourcePcd (String cName, String tsGuidCName, String moduleKey) {
        int libCount = getLibraryInstancesCount(moduleKey);
        String[][] saaLib = new String[libCount][5];
        getLibraryInstances(moduleKey, saaLib);
        ModuleIdentification mi = WorkspaceProfile.getModuleId(moduleKey);
        Vector<ModuleIdentification> vMi = new Vector<ModuleIdentification>();
        //
        // create vector for module & library instance MIs.
        //
        vMi.add(mi);
        for (int j = 0; j < saaLib.length; ++j) {
            String libKey = saaLib[j][1] + " " + saaLib[j][2] + " " + saaLib[j][3] + " " + saaLib[j][4];
            ModuleIdentification libMi = WorkspaceProfile.getModuleId(libKey);
            vMi.add(libMi);
        }
        
        int pcdSourceCount = 0;
        for (int i = 0; i < vMi.size(); ++i) {
            if (WorkspaceProfile.pcdInMsa(cName, tsGuidCName, null, vMi.get(i))) {
                pcdSourceCount++;
            }
        }
        
        if (pcdSourceCount < 2) {
            return false;
        }
        else {
            return true;
        }
        
    }
    
    /**Remove PCDBuildDefinition entries from ModuleSA
     * @param moduleKey identifier of ModuleSA.
     * @param consumer where these entries come from.
     */
    public void removePcdData(String moduleKey, ModuleIdentification consumer) {
        
            ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea)WorkspaceProfile.getModuleXmlObject(consumer);
            if (msa.getPcdCoded() == null) {
                return;
            }
            
            List<PcdCodedDocument.PcdCoded.PcdEntry> l = msa.getPcdCoded().getPcdEntryList();
            ListIterator li = l.listIterator();
            
            while(li.hasNext()) {
                PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry)li.next();
                ModuleSADocument.ModuleSA moduleSA = getModuleSA(moduleKey);
                if (moduleSA.getPcdBuildDefinition() != null) {
                    XmlCursor cursor = moduleSA.getPcdBuildDefinition().newCursor();
                    cursor.push();
                    if (cursor.toFirstChild()) {
                        do {
                            PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData pcdData = (PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData) cursor
                                                                                                                                                          .getObject();
                            String cName = msaPcd.getCName();
                            String tsGuidCName = msaPcd.getTokenSpaceGuidCName();
                            if (cName.equals(pcdData.getCName())
                                && tsGuidCName.equals(pcdData.getTokenSpaceGuidCName()) && !multiSourcePcd(cName, tsGuidCName, moduleKey)) {

                                maintainDynPcdMap(pcdData.getCName() + " " + pcdData.getTokenSpaceGuidCName(),
                                                  moduleKey);
                                cursor.removeXml();
                                break;
                            }
                        } while (cursor.toNextSibling());
                    }
                    
                    cursor.pop();
                    if (moduleSA.getPcdBuildDefinition().getPcdDataList().size() == 0) {
                        cursor.removeXml();
                    }
                    cursor.dispose();
                }
            }
        
    }
    //
    // key for ModuleSA : "ModuleGuid ModuleVer PackageGuid PackageVer Arch"
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
    
    public boolean instanceExistsInModuleSA (String key, String mg, String mv, String pg, String pv) {
        int count = 0;
        if ((count = getLibraryInstancesCount(key)) > 0) {
            String[][] saa = new String[count][5];
            getLibraryInstances (key, saa);
            for (int i = 0; i < count; ++i) {
                if (mg.equalsIgnoreCase(saa[i][1]) && pg.equalsIgnoreCase(saa[i][3])) {
                    boolean modVerMatch = false;
                    boolean pkgVerMatch = false;
                    if ((mv.equals("null") || saa[i][2] == null)) {
                        modVerMatch = true;
                    }
                    if (pv.equals("null") || saa[i][4] == null) {
                        pkgVerMatch = true;
                    }
                    if (modVerMatch && pkgVerMatch) {
                    return true;
                }
                    else {
                        if (mv.equals(saa[i][2]) && pv.equals(saa[i][4])) {
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }
    
    public void removeLibraryInstance(String key, String instanceKey) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null || msa.getLibraries() == null){
            return ;
        }
        
        String[] instanceInfo = instanceKey.split(" ");
        XmlCursor cursor = msa.getLibraries().newCursor();
        if (cursor.toFirstChild()) {
            do {
                Instance libIns = (Instance)cursor.getObject();
                if (libIns.getModuleGuid().equalsIgnoreCase(instanceInfo[0]) && libIns.getPackageGuid().equalsIgnoreCase(instanceInfo[2])) {
                    break;
            }
            }while (cursor.toNextSibling());
            cursor.push();
            while (cursor.hasPrevToken()) {
                cursor.toPrevToken();
                if (!cursor.isText()) {
                    break;
                }
                String s = cursor.getTextValue();
                if (s.matches(regExpNewLineAndSpaces)) {
                    continue;
                }
            }
            
            if (cursor.isComment()) {
                cursor.removeXml();
            }
            cursor.pop();
            cursor.removeXml();
            if (getLibraryInstancesCount(key) == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        
        cursor.dispose();
    }
    
    public void genLibraryInstance(ModuleIdentification libMi, String key) {
        ModuleSADocument.ModuleSA msa = getModuleSA(key);
        if (msa == null){
            msa = getfpdFrameworkModules().addNewModuleSA();
        }
        LibrariesDocument.Libraries libs = msa.getLibraries();
        if(libs == null){
            libs = msa.addNewLibraries();
        }
        
        String mn = libMi.getName();
        String mg = libMi.getGuid();
        String mv = libMi.getVersion();
        String pn = libMi.getPackageId().getName();
        String pg = libMi.getPackageId().getGuid();
        String pv = libMi.getPackageId().getVersion();
        LibrariesDocument.Libraries.Instance instance = libs.addNewInstance();
        XmlCursor cursor = instance.newCursor();
        try{
            String comment = "Pkg: " + pn + " Mod: " + mn 
                + " Path: " + libMi.getPath().substring(Workspace.getCurrentWorkspace().length() + 1);
            cursor.insertComment(comment);
        }
        catch (Exception e){
            e.printStackTrace();
        }
        finally {
            cursor.dispose();
        }
        
        instance.setModuleGuid(mg);
        instance.setModuleVersion(mv);
        instance.setPackageGuid(pg);
        instance.setPackageVersion(pv);
        
    }
    
    public String getFvBinding(String moduleKey){
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(moduleKey);
        return getFvBinding (moduleSa);
    }
    
    public String getFvBinding (ModuleSADocument.ModuleSA moduleSa) {
        if (moduleSa == null || moduleSa.getModuleSaBuildOptions() == null) {
            return null;
        }
        return moduleSa.getModuleSaBuildOptions().getFvBinding();
    }
    
    public void setFvBinding(ModuleSADocument.ModuleSA moduleSa, String fvBinding) {
        if (moduleSa == null ) {
            return;
        }
        if (fvBinding == null || fvBinding.length() == 0) {
            if(moduleSa.getModuleSaBuildOptions() != null){
                moduleSa.getModuleSaBuildOptions().unsetFvBinding();
            }
        }
        else {
            if(moduleSa.getModuleSaBuildOptions() == null){
                moduleSa.addNewModuleSaBuildOptions().setFvBinding(fvBinding);
                return;
            }
            moduleSa.getModuleSaBuildOptions().setFvBinding(fvBinding);
        }
    }
    
    public void setFvBinding(String moduleKey, String fvBinding){
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(moduleKey);
        setFvBinding (moduleSa, fvBinding);
    }
    
    private int fvBindingForModuleSA (ModuleSADocument.ModuleSA moduleSa, String fvName) {
        if (moduleSa == null || moduleSa.getModuleSaBuildOptions() == null || moduleSa.getModuleSaBuildOptions().getFvBinding() == null) {
            return -1;
        }
        
        String fvNameList = moduleSa.getModuleSaBuildOptions().getFvBinding();
        String[] fvNamesArray = fvNameList.split(" ");
        int occursAt = -1;
        for (int i = 0; i < fvNamesArray.length; ++i) {
            if (fvNamesArray[i].equals(fvName)) {
                occursAt = i;
                break;
            }
        }
        return occursAt;
    }
    
    public void removeFvBinding (ModuleSADocument.ModuleSA moduleSa, String fvName) {
        if (moduleSa == null || moduleSa.getModuleSaBuildOptions() == null || moduleSa.getModuleSaBuildOptions().getFvBinding() == null) {
            return;
        }
        
        String fvNameList = moduleSa.getModuleSaBuildOptions().getFvBinding();
        String[] fvNamesArray = fvNameList.split(" ");
        int occursAt = -1;
        for (int i = 0; i < fvNamesArray.length; ++i) {
            if (fvNamesArray[i].equals(fvName)) {
                occursAt = i;
                break;
            }
        }
        // jump over where the input fvName occurs in the original Fv list.
        if (occursAt != -1) {
            String newFvNameList = " ";
            for (int i = 0; i < fvNamesArray.length; ++i) {
                if (i == occursAt) {
                    continue;
                }
                newFvNameList += fvNamesArray[i];
            }
            setFvBinding (moduleSa, newFvNameList.trim());
        }

    }
    
    /**
     * @param fvName The FV name that to be removed from FvBinding List.
     */
    public void removeFvBindingAll (String fvName) {
        if (getfpdFrameworkModules().getModuleSAList() == null || getfpdFrameworkModules().getModuleSAList().size() == 0){
            removeElement(getfpdFrameworkModules());
            fpdFrameworkModules = null;
            return;
        }
        
        Iterator<ModuleSADocument.ModuleSA> li = getfpdFrameworkModules().getModuleSAList().iterator();
        while (li.hasNext()) {
            ModuleSADocument.ModuleSA moduleSa = li.next();
            removeFvBinding (moduleSa, fvName); 
        }
    }
    
    public void appendFvBindingAll (String fvName) {
        if (getfpdFrameworkModules().getModuleSAList() == null || getfpdFrameworkModules().getModuleSAList().size() == 0){
            removeElement(getfpdFrameworkModules());
            fpdFrameworkModules = null;
            return;
        }
        
        Iterator<ModuleSADocument.ModuleSA> li = getfpdFrameworkModules().getModuleSAList().iterator();
        while (li.hasNext()) {
            ModuleSADocument.ModuleSA moduleSa = li.next();
            appendFvBinding (moduleSa, fvName); 
        }
    }
    
    public void appendFvBindingFor (String oldFvName, String newFvName) {
        if (getfpdFrameworkModules().getModuleSAList() == null || getfpdFrameworkModules().getModuleSAList().size() == 0){
            removeElement(getfpdFrameworkModules());
            fpdFrameworkModules = null;
            return;
        }
        
        Iterator<ModuleSADocument.ModuleSA> li = getfpdFrameworkModules().getModuleSAList().iterator();
        while (li.hasNext()) {
            ModuleSADocument.ModuleSA moduleSa = li.next();
            String fvBinding = getFvBinding (moduleSa);
            if (fvBinding != null && fvBindingForModuleSA (moduleSa, oldFvName) >= 0) {
                appendFvBinding (moduleSa, newFvName); 
            }
        }
    }
    
    public void appendFvBinding (String moduleKey, String fvName) {
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(moduleKey);
        appendFvBinding (moduleSa, fvName);
    }
    
    public void appendFvBinding (ModuleSADocument.ModuleSA moduleSa, String fvName) {
        if (moduleSa == null) {
            return;
        }
        
        if (moduleSa.getModuleSaBuildOptions() == null || moduleSa.getModuleSaBuildOptions().getFvBinding() == null) {
            setFvBinding(moduleSa, fvName);
            return;
        }
        
        String fvNameList = moduleSa.getModuleSaBuildOptions().getFvBinding();
        String newFvNameList = fvNameList + " " + fvName;
        setFvBinding (moduleSa, newFvNameList.trim());
    }
    
    public void updateFvBindingInModuleSA (String moduleKey, String fvName) {
       
        appendFvBinding (moduleKey, fvName);
    }
    
    public String getFfsFileNameGuid(String moduleKey){
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(moduleKey);
        if (moduleSa == null || moduleSa.getModuleSaBuildOptions() == null) {
            return null;
        }
        return moduleSa.getModuleSaBuildOptions().getFfsFileNameGuid();
    }
    
    public void setFfsFileNameGuid(String moduleKey, String fileGuid){
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa == null ) {
            return;
        }
        if(msa.getModuleSaBuildOptions() == null){
            msa.addNewModuleSaBuildOptions();
            
        }
        ModuleSaBuildOptionsDocument.ModuleSaBuildOptions msaBuildOpts= msa.getModuleSaBuildOptions();
        if (fileGuid != null) {
            msaBuildOpts.setFfsFileNameGuid(fileGuid);
        }
        else{
            XmlCursor cursor = msaBuildOpts.newCursor();
            if (cursor.toChild(xmlNs, "FfsFileNameGuid")) {
                cursor.removeXml();
            }
            cursor.dispose();
        }
        
    }
    
    public String getFfsFormatKey(String moduleKey){
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa == null || msa.getModuleSaBuildOptions() == null) {
            return null;
        }
        return msa.getModuleSaBuildOptions().getFfsFormatKey();
    }
    
    public void setFfsFormatKey(String moduleKey, String ffsKey){
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa == null ) {
            return;
        }
        if(msa.getModuleSaBuildOptions() == null){
            msa.addNewModuleSaBuildOptions().setFfsFormatKey(ffsKey);
            return;
        }
        msa.getModuleSaBuildOptions().setFfsFormatKey(ffsKey);
    }
    
    public void setModuleSAForceDebug(int i, boolean dbgEnable) {
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(i);
        moduleSa.setForceDebug(dbgEnable);
    }
    
    public boolean getModuleSAForceDebug (int i) {
        ModuleSADocument.ModuleSA moduleSa = getModuleSA(i);
        if (moduleSa.getForceDebug() == true) {
            return true;
        }
        return false;
    }
    
    public void getModuleSAOptions(String moduleKey, String[][] saa) {
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa == null || msa.getModuleSaBuildOptions() == null || msa.getModuleSaBuildOptions().getOptions() == null
                        || msa.getModuleSaBuildOptions().getOptions().getOptionList() == null) {
            return ;
        }
        
        List<OptionDocument.Option> lOpt = msa.getModuleSaBuildOptions().getOptions().getOptionList();
        ListIterator li = lOpt.listIterator();
        int i = 0;
        while(li.hasNext()) {
            OptionDocument.Option opt = (OptionDocument.Option)li.next();
            if (opt.getBuildTargets() != null) {
                saa[i][0] = listToString(opt.getBuildTargets());
            }
            saa[i][1] = opt.getToolChainFamily();
            saa[i][2] = opt.getTagName();
            saa[i][3] = opt.getToolCode();
            
            if (opt.getSupArchList() != null){
                saa[i][4] = listToString(opt.getSupArchList());
            }
            else {
                saa[i][4] = "";
            }
            
            saa[i][5] = opt.getStringValue();
             
            ++i;
        }
    }
    
    public int getModuleSAOptionsCount(String moduleKey){
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa == null || msa.getModuleSaBuildOptions() == null || msa.getModuleSaBuildOptions().getOptions() == null
                        || msa.getModuleSaBuildOptions().getOptions().getOptionList() == null) {
            return 0;
        }
        return msa.getModuleSaBuildOptions().getOptions().getOptionList().size();
    }
    
    public void genModuleSAOptionsOpt(String moduleKey, Vector<Object> buildTargets, String toolChain, String tagName, String toolCmd, Vector<Object> archList, String contents) {
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa.getModuleSaBuildOptions() == null) {
            msa.addNewModuleSaBuildOptions();
        }
        if (msa.getModuleSaBuildOptions().getOptions() == null){
            msa.getModuleSaBuildOptions().addNewOptions();
        }
        OptionDocument.Option opt = msa.getModuleSaBuildOptions().getOptions().addNewOption();
        setBuildOptionsOpt(buildTargets, toolChain, tagName, toolCmd, archList, contents, opt);
    }
    
    public void removeModuleSAOptionsOpt(String moduleKey, int i) {
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa.getModuleSaBuildOptions() == null || msa.getModuleSaBuildOptions().getOptions() == null) {
            return ;
        }
        OptionsDocument.Options opts = msa.getModuleSaBuildOptions().getOptions();
        XmlCursor cursor = opts.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j){
                cursor.toNextSibling();
            }
            cursor.removeXml();
            if (getModuleSAOptionsCount(moduleKey) == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    
    public void updateModuleSAOptionsOpt(String moduleKey, int i, Vector<Object> buildTargets, String toolChain, String tagName, String toolCmd, Vector<Object> archList, String contents) {
        ModuleSADocument.ModuleSA msa = getModuleSA(moduleKey);
        if (msa.getModuleSaBuildOptions() == null || msa.getModuleSaBuildOptions().getOptions() == null) {
            return ;
        }
        OptionsDocument.Options opts = msa.getModuleSaBuildOptions().getOptions();
        XmlCursor cursor = opts.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j){
                cursor.toNextSibling();
            }
            OptionDocument.Option opt = (OptionDocument.Option)cursor.getObject();
            setBuildOptionsOpt(buildTargets, toolChain, tagName, toolCmd, archList, contents, opt);
        }
        cursor.dispose();
    }
    
    /**add pcd information of module mi to a ModuleSA. 
     * @param mi
     * @param moduleSa if null, generate a new ModuleSA.
     */
    public void addFrameworkModulesPcdBuildDefs(ModuleIdentification mi, String arch, ModuleSADocument.ModuleSA moduleSa) throws Exception {
        //ToDo add Arch filter
        
        if (moduleSa == null) {
            moduleSa = genModuleSA(mi, arch);
        }

        ModuleSurfaceAreaDocument.ModuleSurfaceArea msa = (ModuleSurfaceAreaDocument.ModuleSurfaceArea) WorkspaceProfile
                                                                                                                        .getModuleXmlObject(mi);
        if (msa.getPcdCoded() == null) {
            return;
        }

        PackageIdentification[] depPkgs = SurfaceAreaQuery.getDependencePkg(null, mi);
        //
        // Implementing InitializePlatformPcdBuildDefinitions
        //
        List<PcdCodedDocument.PcdCoded.PcdEntry> l = msa.getPcdCoded().getPcdEntryList();
        ListIterator li = l.listIterator();
        while (li.hasNext()) {
            PcdCodedDocument.PcdCoded.PcdEntry msaPcd = (PcdCodedDocument.PcdCoded.PcdEntry) li.next();
            if (msaPcd.getSupArchList() != null) {
            	if (!msaPcd.getSupArchList().toString().toLowerCase().contains(arch.toLowerCase())) {
            		continue;
            	}
            }
            PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = LookupPcdDeclaration(msaPcd, depPkgs);
            if (spdPcd == null) {
                //
                // ToDo Error 
                //
                throw new PcdDeclNotFound("No Declaration for PCD Entry " + msaPcd.getCName() + "\n used by Module "
                                          + mi.getName() + " or its Library Instances.");
            }
            //
            // AddItem to ModuleSA PcdBuildDefinitions
            //
            String defaultVal = msaPcd.getDefaultValue() == null ? spdPcd.getDefaultValue() : msaPcd.getDefaultValue();

            genPcdData(msaPcd.getCName(), spdPcd.getToken(), msaPcd.getTokenSpaceGuidCName(), msaPcd.getPcdItemType()
                                                                                                    .toString(),
                       spdPcd.getDatumType() + "", defaultVal, moduleSa, spdPcd);
        }
        
    }
    
    private PcdDeclarationsDocument.PcdDeclarations.PcdEntry LookupPcdDeclaration (PcdCodedDocument.PcdCoded.PcdEntry msaPcd, PackageIdentification[] depPkgs) {
        
        PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd = null;
        for (int i = 0; i < depPkgs.length; ++i) {

            XmlObject[] xo = SurfaceAreaQuery.getSpdPcdDeclarations(depPkgs[i]);
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
    
    private ModuleSADocument.ModuleSA genModuleSA (ModuleIdentification mi, String arch) {
        PackageIdentification pi = WorkspaceProfile.getPackageForModule(mi);
        ModuleSADocument.ModuleSA msa = getfpdFrameworkModules().addNewModuleSA();
        XmlCursor cursor = msa.newCursor();
        try{
            String comment = "Mod: " + mi.getName() + " Type: " + SurfaceAreaQuery.getModuleType(mi) + " Path: "
                            + mi.getPath().substring(Workspace.getCurrentWorkspace().length() + 1);
            cursor.insertComment(comment);
        }
        catch(Exception e){
            e.printStackTrace();
        }
        finally { 
            cursor.dispose();
        }
        msa.setModuleGuid(mi.getGuid());
        msa.setModuleVersion(mi.getVersion());
        msa.setPackageGuid(pi.getGuid());
        msa.setPackageVersion(pi.getVersion());
        if (arch != null) {
            Vector<String> v = new Vector<String>();
            v.add(arch);
            msa.setSupArchList(v); 
        }
        
        return msa;
    }
    
    private String chooseDefaultPcdType (List validPcdTypes) {
        String choosedType = "";
        if (validPcdTypes.contains("FIXED_AT_BUILD")) {
            choosedType = "FIXED_AT_BUILD";
        }
        else if (validPcdTypes.contains("DYNAMIC")) {
            choosedType = "DYNAMIC";
        }
        else if (validPcdTypes.contains("PATCHABLE_IN_MODULE")) {
            choosedType = "PATCHABLE_IN_MODULE";
        }
        else if (validPcdTypes.contains("DYNAMIC_EX")) {
            choosedType = "DYNAMIC_EX";
        }
        return choosedType;
    }
    
    private synchronized void genPcdData (String cName, Object token, String tsGuid, String itemType, String dataType, String defaultVal, 
                             ModuleSADocument.ModuleSA moduleSa, PcdDeclarationsDocument.PcdDeclarations.PcdEntry spdPcd) 
    throws PcdItemTypeConflictException, PcdValueMalFormed{
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
        //
        // Check whether this PCD has already added to ModuleSA, if so, just return.
        //
        String moduleInfo = moduleSa.getModuleGuid().toLowerCase() + " " + moduleSa.getModuleVersion() 
        + " " + moduleSa.getPackageGuid().toLowerCase() + " " + moduleSa.getPackageVersion() + " " + listToString(moduleSa.getSupArchList());
        for (int i = 0; i < pcdConsumer.size(); ++i) {
            String pcdInfo = pcdConsumer.get(i);
            if (moduleInfo.equals(pcdInfo.substring(0, pcdInfo.lastIndexOf(" ")))){
                return;
            }
        }
        // if pcd type from MSA file is Dynamic
        // we must choose one default type from SPD file for it.
        //
        List validPcdTypes = spdPcd.getValidUsage();
        //
        // Using existing Pcd type, if this pcd already exists in other ModuleSA
        //
        if (pcdConsumer.size() > 0) {
            //
            // platform should only contain one type for each pcd.
            //
            String existingItemType = itemType (pcdConsumer.get(0));
            for (int i = 1; i < pcdConsumer.size(); ++i) {
                if (!existingItemType.equals(itemType(pcdConsumer.get(i)))) {
                    throw new PcdItemTypeConflictException (cName, pcdConsumer.get(0), pcdConsumer.get(i));
                }
            }
            
            if (itemType.equals("DYNAMIC")) {
                if (!validPcdTypes.contains(existingItemType)) {
                    throw new PcdItemTypeConflictException(cName, pcdConsumer.get(0));
                }
                itemType = existingItemType;
            }
            else {
                if (!itemType.equals(existingItemType)) {
                    throw new PcdItemTypeConflictException(cName, pcdConsumer.get(0));
                }
            }
        }
        //
        // if this is the first occurence of this pcd. 
        //
        else {
            if (itemType.equals("DYNAMIC")) {
                itemType = chooseDefaultPcdType (validPcdTypes);
            }
        }
        String listValue = moduleInfo + " " + itemType;
        pcdConsumer.add(listValue);
        dynPcdMap.put(cName + " " + tsGuid, pcdConsumer);
        
        PcdBuildDefinitionDocument.PcdBuildDefinition.PcdData fpdPcd = moduleSa.getPcdBuildDefinition().addNewPcdData();
        fpdPcd.setCName(cName);
        fpdPcd.setToken(token);
        fpdPcd.setTokenSpaceGuidCName(tsGuid);
        fpdPcd.setDatumType(PcdDataTypes.Enum.forString(dataType));
        fpdPcd.setItemType(PcdItemTypes.Enum.forString(itemType));
        
        if (defaultVal != null && defaultVal.length() > 0){
            fpdPcd.setValue(defaultVal);
        }
        else {
            if (dataType.equals("UINT8") || dataType.equals("UINT16") || dataType.equals("UINT32") || dataType.equals("UINT64")) {
                fpdPcd.setValue("0");
            }
            if (dataType.equals("BOOLEAN")){
                fpdPcd.setValue("FALSE");
            }
            if (dataType.equals("VOID*")) {
                fpdPcd.setValue("L\"\"");
            }
        }
        //
        // Using existing pcd value, if this pcd already exists in other moduleSa.
        //
        if (defaultPcdValue.get(cName + " " + tsGuid) == null) {
            defaultPcdValue.put(cName + " " + tsGuid, fpdPcd.getValue());
        }
        else {
            fpdPcd.setValue(defaultPcdValue.get(cName + " " + tsGuid));
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
        
    }
    
    public int setMaxSizeForPointer(String datum) throws PcdValueMalFormed{
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
                throw new PcdValueMalFormed (datum);
            }

            strValue    = datum.substring(start + 1, end);
            return strValue.length() * 2;
        } else if (ch == '\"'){
            start       = datum.indexOf('\"');
            end         = datum.lastIndexOf('\"');
            if ((start > end)           || 
                (end   > datum.length())||
                ((start == end) && (datum.length() > 0))) {
                throw new PcdValueMalFormed (datum);
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
//                   "it must be a byte array. But the element of %s exceed the byte range",
                    throw new PcdValueMalFormed (datum);                               
                }
            }
            return strValueArray.length;


        } else {
//            "[FPD file error] The datum type of PCD %s in %s is VOID*. For VOID* type, you have three format choise:\n "+
//            "1) UNICODE string: like L\"xxxx\";\r\n"+
//            "2) ANSIC string: like \"xxx\";\r\n"+
//            "3) Byte array: like {0x2, 0x45, 0x23}\r\n"+
//            "but the datum in seems does not following above format!",
            throw new PcdValueMalFormed (datum);
            
        }
    }
    
    private ArrayList<String> LookupDynamicPcdBuildDefinition(String dynPcdKey) {
        ArrayList<String> al = dynPcdMap.get(dynPcdKey);
        
        return al;
    }
    
    private ArrayList<String> LookupPlatformPcdData(String pcdKey) {
        
        return dynPcdMap.get(pcdKey);
    }
    
    public int getDynamicPcdBuildDataCount() {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
            return 0;
        }
        return getfpdDynPcdBuildDefs().getPcdBuildDataList().size();
    }
    
    public void getDynamicPcdBuildData(String[][] saa) {
        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            saa[i][4] = dynPcd.getDatumType()+"";
            
            ++i;
        }
    }
    
    public void addDynamicPcdBuildData(String cName, Object token, String tsGuid, String itemType, String dataType, String defaultVal) 
    throws PcdValueMalFormed{
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
    
    public void removeDynamicPcdBuildData(String cName, String tsGuid) {
        XmlObject o = fpdRoot.getDynamicPcdBuildDefinitions();
        if (o == null) {
            return;
        }
        
        XmlCursor cursor = o.newCursor();
        if (cursor.toFirstChild()) {
            do {
                DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData pcdBuildData = 
                    (DynamicPcdBuildDefinitionsDocument.DynamicPcdBuildDefinitions.PcdBuildData)cursor.getObject();
                if (pcdBuildData.getCName().equals(cName) && pcdBuildData.getTokenSpaceGuidCName().equals(tsGuid)) {
                    
                    if (getDynamicPcdBuildDataCount() == 1) {
                        cursor.dispose();
                        removeElement(o);
                        fpdDynPcdBuildDefs = null;
                        return;
                    }
                    cursor.removeXml();
                    cursor.dispose();
                    return;
                }
            }
            while (cursor.toNextSibling());
        }
        cursor.dispose();
    }
    //
    // Get the Sku Info count of ith dyn pcd element.
    //
    public int getDynamicPcdSkuInfoCount(int i){
        if (fpdRoot.getDynamicPcdBuildDefinitions() == null || fpdRoot.getDynamicPcdBuildDefinitions().getPcdBuildDataList() == null 
                        || fpdRoot.getDynamicPcdBuildDefinitions().getPcdBuildDataList().size() == 0) {
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
            	pcdData.getSkuInfoList().clear();
//                QName qSkuInfo = new QName(xmlNs, "SkuInfo");
//                cursor.toChild(qSkuInfo);
//                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    //
    // generate sku info for ith dyn pcd build data.
    //
    public void genDynamicPcdBuildDataSkuInfo(String id, String varName, String varGuid, String varOffset, 
                                              String hiiDefault, String vpdOffset, String value, int i) {
//        if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null || getfpdDynPcdBuildDefs().getPcdBuildDataList().size() == 0) {
//            return;
//        }
        
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
            removeElement(getfpdDynPcdBuildDefs());
            fpdDynPcdBuildDefs = null;
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
    
    public BuildOptionsDocument.BuildOptions getfpdBuildOpts() {
        if (fpdBuildOpts == null) {
            fpdBuildOpts = fpdRoot.addNewBuildOptions();
        }
        return fpdBuildOpts;
    }
    
    public void genBuildOptionsUserExtensions(String fvName, String userId, String id, String outputFileName, Vector<String[]> includeModules) {
        QName elementFvName = new QName (xmlNs, "FvName");
        QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
        QName elementInfFileName = new QName(xmlNs, "InfFileName");
        QName elementModule = new QName(xmlNs, "Module");
        
        UserExtensionsDocument.UserExtensions userExts = getfpdBuildOpts().addNewUserExtensions();
        userExts.setUserID(userId);
        userExts.setIdentifier(id);
        XmlCursor cursor = userExts.newCursor();
        cursor.toEndToken();
        
        cursor.beginElement(elementFvName);
        cursor.insertChars(fvName);
        cursor.toNextToken();
        
        cursor.beginElement(elementInfFileName);
        cursor.insertChars(outputFileName);
        cursor.toNextToken();
        
        cursor.beginElement(elementIncludeModules);
        for (int i = 0; i < includeModules.size(); ++i) {
            cursor.beginElement(elementModule);
            cursor.insertAttributeWithValue("ModuleGuid", includeModules.get(i)[0]);
            if (!includeModules.get(i)[1].equals("null") && includeModules.get(i)[1].length() != 0) {
                cursor.insertAttributeWithValue("ModuleVersion", includeModules.get(i)[1]);
            }
            cursor.insertAttributeWithValue("PackageGuid", includeModules.get(i)[2]);
            if (!includeModules.get(i)[3].equals("null") && includeModules.get(i)[3].length() != 0) {
                cursor.insertAttributeWithValue("PackageVersion", includeModules.get(i)[3]);
            }
            
            cursor.insertAttributeWithValue("Arch", includeModules.get(i)[4]);
            cursor.toEndToken();
            cursor.toNextToken();
        }
        cursor.dispose();
    }
    
    public int getUserExtsIncModCount (String fvName, String userId, String id) {
        if (getfpdBuildOpts().getUserExtensionsList() == null) {
            return -1;
        }

        ListIterator<UserExtensionsDocument.UserExtensions> li = getfpdBuildOpts().getUserExtensionsList().listIterator();
        QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
        while (li.hasNext()) {
            UserExtensionsDocument.UserExtensions ues = li.next();
            if (!ues.getUserID().equals(userId)) {
                continue;
            }
            if (ues.getIdentifier() == null || !ues.getIdentifier().equals(id)) {
                continue;
            }
            XmlCursor cursor = ues.newCursor();
            cursor.toFirstChild();
            String elementName = cursor.getTextValue();
            if (elementName.equals(fvName)) {
                cursor.toNextSibling(elementIncludeModules);
                if (cursor.toFirstChild()) {
                    int i = 1;
                    for (i = 1; cursor.toNextSibling(); ++i);
                    cursor.dispose();
                    return i;
                }
                cursor.dispose();
                return 0;
            }
            cursor.dispose();
        }
        return -1;
    }
    
    public void getUserExtsIncMods(String fvName, String userId, String id, String[][] saa) {
        if (getfpdBuildOpts().getUserExtensionsList() == null) {
            return;
        }

        XmlCursor cursor = getfpdBuildOpts().newCursor();
        QName elementUserExts = new QName (xmlNs, "UserExtensions");
        QName attribUserId = new QName ("UserID");
        QName attribId = new QName ("Identifier");
        QName elementFvName = new QName (xmlNs, "FvName");
        QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
        QName attribModuleGuid = new QName("ModuleGuid");
        QName attribModuleVersion = new QName("ModuleVersion");
        QName attribPackageGuid = new QName("PackageGuid");
        QName attribPackageVersion = new QName("PackageVersion");
        QName attribArch = new QName("Arch");
        
        if (cursor.toChild(elementUserExts)) {
            do {
                cursor.push();
                if (cursor.getAttributeText(attribUserId).equals(userId) && cursor.getAttributeText(attribId).equals(id)) {
                    cursor.toChild(elementFvName);
                    String elementName = cursor.getTextValue();
                    if (elementName.equals(fvName)) {
                        cursor.toNextSibling(elementIncludeModules);
                        if (cursor.toFirstChild()) {
                            int i = 0;
                            do {
                                saa[i][0] = cursor.getAttributeText(attribModuleGuid);
                                saa[i][1] = cursor.getAttributeText(attribModuleVersion);
                                saa[i][2] = cursor.getAttributeText(attribPackageGuid);
                                saa[i][3] = cursor.getAttributeText(attribPackageVersion);
                                saa[i][4] = cursor.getAttributeText(attribArch);
                                ++i;
                            }while (cursor.toNextSibling());
                        }
                        break;
                    }
                }
                cursor.pop();
            }while (cursor.toNextSibling(elementUserExts));
        }
        cursor.dispose();
        
    }
    
    public void updateBuildOptionsUserExtensions (String oldFvName, String newFvName) {
        if (getfpdBuildOpts().getUserExtensionsList() == null) {
            return;
        }
        ListIterator<UserExtensionsDocument.UserExtensions> li = getfpdBuildOpts().getUserExtensionsList().listIterator();
        while (li.hasNext()) {
            UserExtensionsDocument.UserExtensions ues = li.next();
            if (!ues.getUserID().equals("IMAGES")) {
                continue;
            }
            XmlCursor cursor = ues.newCursor();
            cursor.toFirstChild();
            String elementName = cursor.getTextValue();
            if (elementName.equals(oldFvName)) {
                cursor.setTextValue(newFvName);
            }
            cursor.dispose();
        }
        
    }
    
    public void removeBuildOptionsUserExtensions (String fvName, String userId, String id) {
        if (getfpdBuildOpts().getUserExtensionsList() == null) {
            return;
        }

        ListIterator<UserExtensionsDocument.UserExtensions> li = getfpdBuildOpts().getUserExtensionsList().listIterator();
        while (li.hasNext()) {
            UserExtensionsDocument.UserExtensions ues = li.next();
            if (!ues.getUserID().equals(userId)) {
                continue;
            }
            if (ues.getIdentifier()== null || !ues.getIdentifier().equals(id)) {
                continue;
            }
            XmlCursor cursor = ues.newCursor();
            cursor.toFirstChild();
            String elementName = cursor.getTextValue();
            if (elementName.equals(fvName)) {
                cursor.toParent();
                cursor.removeXml();
                cursor.dispose();
                return;
            }
            cursor.dispose();
        }
        
    }
    
    private boolean versionEqual (String v1, String v2) {
        
        if ((v1 == null || v1.length() == 0 || v1.equalsIgnoreCase("null")) 
                        && (v2 == null || v2.length() == 0 || v2.equalsIgnoreCase("null"))) {
            return true;
        }
        
        if (v1 != null && v1.equals(v2)) {
            return true;
        }
        
        return false;
    }
    
    public boolean moduleInBuildOptionsUserExtensions (String fvName, String userId, String id, String moduleGuid, String moduleVersion, String packageGuid, String packageVersion, String arch) {
        boolean inList = false;
        if (getUserExtsIncModCount(fvName, userId, id) > 0) {
            XmlCursor cursor = getfpdBuildOpts().newCursor();
            QName elementUserExts = new QName (xmlNs, "UserExtensions");
            QName attribUserId = new QName ("UserID");
            QName attribId = new QName ("Identifier");
            QName elementFvName = new QName (xmlNs, "FvName");
            QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
            QName attribModuleGuid = new QName("ModuleGuid");
            QName attribModuleVersion = new QName("ModuleVersion");
            QName attribPackageGuid = new QName("PackageGuid");
            QName attribPackageVersion = new QName("PackageVersion");
            QName attribArch = new QName("Arch");
            
            if (cursor.toChild(elementUserExts)) {
                do {
                    cursor.push();
                    if (cursor.getAttributeText(attribUserId).equals(userId) && cursor.getAttributeText(attribId).equals(id)) {
                        cursor.toChild(elementFvName);
                        String elementName = cursor.getTextValue();
                        if (elementName.equals(fvName)) {
                            cursor.toNextSibling(elementIncludeModules);
                            if (cursor.toFirstChild()) {
                                
                                do {
                                    String mg = cursor.getAttributeText(attribModuleGuid);
                                    String mv = cursor.getAttributeText(attribModuleVersion);
                                    String pg = cursor.getAttributeText(attribPackageGuid);
                                    String pv = cursor.getAttributeText(attribPackageVersion);
                                    String ar = cursor.getAttributeText(attribArch);
                                    if (!moduleGuid.equalsIgnoreCase(mg)) {
                                        continue;
                                    }
                                    if (!packageGuid.equalsIgnoreCase(pg)) {
                                        continue;
                                    }
                                    if (!arch.equalsIgnoreCase(ar)) {
                                        continue;
                                    }
                                    if (!versionEqual(moduleVersion, mv)) {
                                        continue;
                                    }
                                    if (!versionEqual(packageVersion, pv)) {
                                        continue;
                                    }
                                    inList = true;
                                    break;
                                }while (cursor.toNextSibling());
                            }
                            break;
                        }
                    }
                    cursor.pop();
                }while (cursor.toNextSibling(elementUserExts));
            }
            cursor.dispose();
        }
        return inList;
    }
    
    public void removeModuleInBuildOptionsUserExtensions (String fvName, String userId, String id, String moduleGuid, String moduleVersion, String packageGuid, String packageVersion, String arch) {
        //
        // if there is only one module before remove operation, the whole user extension should be removed.
        //
        int moduleAmount = getUserExtsIncModCount(fvName, userId, id);
        if (moduleAmount == 1) {
            removeBuildOptionsUserExtensions(fvName, userId, id);
            return;
        }
        
        if (moduleAmount > 1) {
            XmlCursor cursor = getfpdBuildOpts().newCursor();
            QName elementUserExts = new QName (xmlNs, "UserExtensions");
            QName attribUserId = new QName ("UserID");
            QName attribId = new QName ("Identifier");
            QName elementFvName = new QName (xmlNs, "FvName");
            QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
            QName attribModuleGuid = new QName("ModuleGuid");
            QName attribModuleVersion = new QName("ModuleVersion");
            QName attribPackageGuid = new QName("PackageGuid");
            QName attribPackageVersion = new QName("PackageVersion");
            QName attribArch = new QName("Arch");
            
            if (cursor.toChild(elementUserExts)) {
                do {
                    cursor.push();
                    if (cursor.getAttributeText(attribUserId).equals(userId) && cursor.getAttributeText(attribId).equals(id)) {
                        cursor.toChild(elementFvName);
                        String elementName = cursor.getTextValue();
                        if (elementName.equals(fvName)) {
                            cursor.toNextSibling(elementIncludeModules);
                            if (cursor.toFirstChild()) {
                                
                                do {
                                    String mg = cursor.getAttributeText(attribModuleGuid);
                                    String mv = cursor.getAttributeText(attribModuleVersion);
                                    String pg = cursor.getAttributeText(attribPackageGuid);
                                    String pv = cursor.getAttributeText(attribPackageVersion);
                                    String ar = cursor.getAttributeText(attribArch);
                                    if (!moduleGuid.equalsIgnoreCase(mg)) {
                                        continue;
                                    }
                                    if (!packageGuid.equalsIgnoreCase(pg)) {
                                        continue;
                                    }
                                    if (!arch.equalsIgnoreCase(ar)) {
                                        continue;
                                    }
                                    if (!versionEqual(moduleVersion, mv)) {
                                        continue;
                                    }
                                    if (!versionEqual(packageVersion, pv)) {
                                        continue;
                                    }
                                    cursor.removeXml();
                                }while (cursor.toNextSibling());
                            }
                            break;
                        }
                    }
                    cursor.pop();
                }while (cursor.toNextSibling(elementUserExts));
            }
            cursor.dispose();
        }
    }
    
    public void addModuleIntoBuildOptionsUserExtensions (String fvName, String userId, String id, String moduleGuid, String moduleVersion, String packageGuid, String packageVersion, String arch) {
        if (moduleInBuildOptionsUserExtensions (fvName, userId, id, moduleGuid, moduleVersion, packageGuid, packageVersion, arch)) {
            return;
        }

        ListIterator<UserExtensionsDocument.UserExtensions> li = getfpdBuildOpts().getUserExtensionsList().listIterator();
        QName elementIncludeModules = new QName(xmlNs, "IncludeModules");
        QName elementModule = new QName(xmlNs, "Module");
        while (li.hasNext()) {
            UserExtensionsDocument.UserExtensions ues = li.next();
            if (!ues.getUserID().equals(userId)) {
                continue;
            }
            if (ues.getIdentifier() == null || !ues.getIdentifier().equals(id)) {
                continue;
            }
            XmlCursor cursor = ues.newCursor();
            cursor.toFirstChild();
            String elementName = cursor.getTextValue();
            if (elementName.equals(fvName)) {
                cursor.toNextSibling(elementIncludeModules);
                cursor.toLastChild();
                cursor.toEndToken();
                cursor.toNextToken();
                cursor.beginElement(elementModule);
                cursor.insertAttributeWithValue("ModuleGuid", moduleGuid);
                if (!moduleVersion.equals("null") && moduleVersion.length() != 0) {
                    cursor.insertAttributeWithValue("ModuleVersion", moduleVersion);
                }
                cursor.insertAttributeWithValue("PackageGuid", packageGuid);
                if (!packageVersion.equals("null") && packageVersion.length() != 0) {
                    cursor.insertAttributeWithValue("PackageVersion", packageVersion);
                }
                
                cursor.insertAttributeWithValue("Arch", arch);
                cursor.dispose();
                return;
            }
            cursor.dispose();
        }
        
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
        XmlCursor cursor = at.newCursor();
        if (fileName != null){
            at.setFilename(fileName);
        }
        else if (cursor.toChild(xmlNs, "Filename")) {
            cursor.removeXml();
        }
        if (execOrder != null) {
            at.setAntCmdOptions(execOrder);
        }
        else if (cursor.toChild(xmlNs, "AntCmdOptions")) {
            cursor.removeXml();
        }
        cursor.dispose();
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
            if (getBuildOptionsUserDefAntTaskCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
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
    public void genBuildOptionsOpt(Vector<Object> buildTargets, String toolChain, String tagName, String toolCmd, Vector<Object> archList, String contents) {
        OptionsDocument.Options opts = getfpdBuildOpts().getOptions();
        if (opts == null) {
            opts = getfpdBuildOpts().addNewOptions();
        }
        OptionDocument.Option opt = opts.addNewOption();
        setBuildOptionsOpt(buildTargets, toolChain, tagName, toolCmd, archList, contents, opt);
    }
    
    private void setBuildOptionsOpt(Vector<Object> buildTargets, String toolChain, String tagName, String toolCmd, Vector<Object> archList, String contents, OptionDocument.Option opt){
        opt.setStringValue(contents);
        if (buildTargets != null) {
            opt.setBuildTargets(buildTargets);
        }
        else {
            if (opt.isSetBuildTargets()) {
                opt.unsetBuildTargets();
            }
        }
        
        if (toolChain != null && toolChain.length() > 0) {
            opt.setToolChainFamily(toolChain);
        }
        else {
            if (opt.isSetToolChainFamily()) {
                opt.unsetToolChainFamily();
            }
        }
        
        if (tagName != null && tagName.length() > 0) {
            opt.setTagName(tagName);
        }
        else {
            if (opt.isSetTagName()) {
                opt.unsetTagName();
            }
        }
        
        if (toolCmd != null && toolCmd.length() > 0) {
            opt.setToolCode(toolCmd);
        }
        else {
            if (opt.isSetToolCode()) {
                opt.unsetToolCode();
            }
        }
        
        
        if (archList != null) {
            opt.setSupArchList(archList);
        }
        else {
            if (opt.isSetSupArchList()) {
                opt.unsetSupArchList();
            }
        }
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
            if (getBuildOptionsOptCount() == 0) {
                cursor.toParent();
                cursor.removeXml();
            }
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsOpt(int i, Vector<Object> buildTargets, String toolChain, String tagName, String toolCmd, Vector<Object> archList, String contents) {
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
            if (opt.getBuildTargets() != null) {
                saa[i][0] = listToString(opt.getBuildTargets());
            }
            saa[i][1] = opt.getToolChainFamily();
            if (opt.getSupArchList() != null){
                saa[i][2] = listToString(opt.getSupArchList());

            }
            saa[i][3] = opt.getToolCode();
            saa[i][4] = opt.getTagName();
            saa[i][5] = opt.getStringValue();
             
            ++i;
        }
    }
    
    public void genBuildOptionsFfs(String ffsKey, String type) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getfpdBuildOpts().addNewFfs();
        ffs.setFfsKey(ffsKey);
        if (type != null) {
            ffs.addNewSections().setEncapsulationType(type);
        }
    }
    
    public void updateBuildOptionsFfsSectionsType(int i, String type) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        ffs.getSections().setEncapsulationType(type);
    }
    
    public void genBuildOptionsFfsAttribute(int i, String name, String value) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Attribute attrib = ffs.addNewAttribute();
        attrib.setName(name);
        attrib.setValue(value);
    }
    
    /**update jth attribute of ith ffs.
     * @param i
     * @param j
     */
    public void updateBuildOptionsFfsAttribute(int i, int j, String name, String value){
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        XmlCursor cursor = ffs.newCursor();
        QName qAttrib = new QName(xmlNs, "Attribute");
        if (cursor.toChild(qAttrib)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qAttrib);
            }
            BuildOptionsDocument.BuildOptions.Ffs.Attribute attrib = (BuildOptionsDocument.BuildOptions.Ffs.Attribute)cursor.getObject();
            attrib.setName(name);
            attrib.setValue(value);
        }
        cursor.dispose();
    }
    
    public void removeBuildOptionsFfsAttribute(int i, int j){
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        XmlCursor cursor = ffs.newCursor();
        QName qAttrib = new QName(xmlNs, "Attribute");
        if (cursor.toChild(qAttrib)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qAttrib);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void genBuildOptionsFfsSectionsSection(int i, String sectionType) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        if (ffs == null) {
            return;
        }
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        
        if (sections == null){
            sections = ffs.addNewSections();
        }
        sections.addNewSection().setSectionType(EfiSectionType.Enum.forString(sectionType));
    }
    
    public void removeBuildOptionsFfsSectionsSection(int i, int j) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSection = new QName(xmlNs, "Section");
        if (cursor.toChild(qSection)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSection);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsFfsSectionsSection(int i, int j, String type){
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSection = new QName(xmlNs, "Section");
        if (cursor.toChild(qSection)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSection);
            }
            BuildOptionsDocument.BuildOptions.Ffs.Sections.Section section = (BuildOptionsDocument.BuildOptions.Ffs.Sections.Section)cursor.getObject();
            section.setSectionType(EfiSectionType.Enum.forString(type));
        }
        cursor.dispose();
    } 
    
    public void genBuildOptionsFfsSectionsSections(int i, String encapType) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        if (ffs == null) {
            return;
        }
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        
        if (sections == null){
            sections = ffs.addNewSections();
        }
        BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2 sections2 = sections.addNewSections();
        sections2.setEncapsulationType(encapType);
        sections2.addNewSection().setSectionType(EfiSectionType.Enum.forString("EFI_SECTION_PE32"));
    }
    
    public void removeBuildOptionsFfsSectionsSections(int i, int j) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSections);
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsFfsSectionsSections(int i, int j, String type) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)) {
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSections);
            }
            BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2 sections2 = (BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2)cursor.getObject();
            sections2.setEncapsulationType(type);
        }
        cursor.dispose();
    }
    
    public void genBuildOptionsFfsSectionsSectionsSection(int i, int j, String type) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        if (ffs == null) {
            return;
        }
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)){
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSections);
            }
            BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2 sections2 = (BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2)cursor.getObject();
            sections2.addNewSection().setSectionType(EfiSectionType.Enum.forString(type));
        }
        cursor.dispose();
    }
    
    public void removeBuildOptionsFfsSectionsSectionsSection(int i, int j, int k) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)) {
            for (int l = 0; l < j; ++l) {
                cursor.toNextSibling(qSections);
            }
            if (cursor.toFirstChild()) {
                int m = 0;
                for (; m < k; ++m) {
                    cursor.toNextSibling();
                }
                cursor.removeXml();
                if (m == 0) {
                    cursor.toParent();
                    cursor.removeXml();
                }
            }
        }
        cursor.dispose();
    }
    
    public void updateBuildOptionsFfsSectionsSectionsSection(int i, int j, int k, String type) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        if (sections == null) {
            return;
        }
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)) {
            for (int l = 0; l < j; ++l) {
                cursor.toNextSibling(qSections);
            }
            if (cursor.toFirstChild()) {
                for (int m = 0; m < k; ++m) {
                    cursor.toNextSibling();
                }
                BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2.Section section = (BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2.Section)cursor.getObject();
                section.setSectionType(EfiSectionType.Enum.forString(type));
            }
        }
        cursor.dispose();
    }
    
    public void getBuildOptionsFfsSectionsSectionsSection(int i, int j, ArrayList<String> al) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        if (ffs == null) {
            return;
        }
        BuildOptionsDocument.BuildOptions.Ffs.Sections sections = ffs.getSections();
        XmlCursor cursor = sections.newCursor();
        QName qSections = new QName(xmlNs, "Sections");
        if (cursor.toChild(qSections)){
            for (int k = 0; k < j; ++k) {
                cursor.toNextSibling(qSections);
            }
            BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2 sections2 = (BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2)cursor.getObject();
            if (sections2.getSectionList() == null){
                cursor.dispose();
                return;
            }
            ListIterator<BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2.Section> li = sections2.getSectionList().listIterator();
            while(li.hasNext()) {
                BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2.Section section = li.next();
//                if (section.isSetSectionType()) {
                    al.add(section.getSectionType()+"");
//                }
                
            }
        }
        cursor.dispose();
        
    }
    
    public int getBuildOptionsFfsCount(){
        if (getfpdBuildOpts().getFfsList() == null) {
            return 0;
        }
        return getfpdBuildOpts().getFfsList().size();
    }
    
    public void getBuildOptionsFfsKey(String[][] saa) {
        if (getfpdBuildOpts().getFfsList() == null) {
            return;
        }
        ListIterator<BuildOptionsDocument.BuildOptions.Ffs> li = getfpdBuildOpts().getFfsList().listIterator();
        int i = 0;
        while(li.hasNext()){
            BuildOptionsDocument.BuildOptions.Ffs ffs = li.next();
            saa[i][0] = ffs.getFfsKey();
            ++i;
        }
    }
    
    public void updateBuildOptionsFfsKey(int i, String key) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        ffs.setFfsKey(key);
    }
    
    /**Get ith FFS key and contents.
     * @param saa
     */
    public void getBuildOptionsFfs(int i, String[] sa, LinkedHashMap<String, String> ffsAttribMap, ArrayList<String> firstLevelSections, ArrayList<String> firstLevelSection) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
     
        if (ffs != null) {
         
            sa[0] = ffs.getFfsKey();
            if (ffs.getSections() != null) {
                if(ffs.getSections().getEncapsulationType() != null){
                    sa[1] = ffs.getSections().getEncapsulationType();
                }
                if (ffs.getSections().getSectionList() != null){
                    ListIterator<BuildOptionsDocument.BuildOptions.Ffs.Sections.Section> li = ffs.getSections().getSectionList().listIterator();
                    while (li.hasNext()) {
                        firstLevelSection.add(li.next().getSectionType()+"");
                    }
                }
                if (ffs.getSections().getSectionsList() != null) {
                    ListIterator<BuildOptionsDocument.BuildOptions.Ffs.Sections.Sections2> li = ffs.getSections().getSectionsList().listIterator();
                    while(li.hasNext()) {
                        firstLevelSections.add(li.next().getEncapsulationType());
                    }
                }
            }
            if (ffs.getAttributeList() != null) {
                ListIterator<BuildOptionsDocument.BuildOptions.Ffs.Attribute> li = ffs.getAttributeList().listIterator();
                while(li.hasNext()) {
                    BuildOptionsDocument.BuildOptions.Ffs.Attribute attrib = li.next();
                    ffsAttribMap.put(attrib.getName(), attrib.getValue());
                }
                
            }
        }

        
    }
    
    private BuildOptionsDocument.BuildOptions.Ffs getFfs(int i) {
        XmlObject o = getfpdBuildOpts();
        BuildOptionsDocument.BuildOptions.Ffs ffs = null;
        
        XmlCursor cursor = o.newCursor();
        QName qFfs = new QName(xmlNs, "Ffs");
        if (cursor.toChild(qFfs)) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling(qFfs);
            }
            ffs = (BuildOptionsDocument.BuildOptions.Ffs)cursor.getObject();
        }
        cursor.dispose();
        return ffs;
    }
    
    public void removeBuildOptionsFfs(int i) {
        BuildOptionsDocument.BuildOptions.Ffs ffs = getFfs(i);
        if (ffs == null){
            return;
        }
        
        XmlCursor cursor = ffs.newCursor();
        cursor.removeXml();
        cursor.dispose();
    }
    
    
    
    public PlatformDefinitionsDocument.PlatformDefinitions getfpdPlatformDefs(){
        if (fpdPlatformDefs == null){
            fpdPlatformDefs = fpdRoot.addNewPlatformDefinitions();
        }
        return fpdPlatformDefs;
    }
    
    public void getPlatformDefsSupportedArchs(Vector<Object> archs){
        if (getfpdPlatformDefs().getSupportedArchitectures() == null) {
            return;
        }
        ListIterator li = getfpdPlatformDefs().getSupportedArchitectures().listIterator();
        while(li.hasNext()) {
            archs.add(li.next());
        }
    }
    
    public void setPlatformDefsSupportedArchs(Vector<Object> archs) {
        if (archs != null) {
            getfpdPlatformDefs().setSupportedArchitectures(archs);
        }
//        else {
//            XmlCursor cursor = getfpdPlatformDefs().newCursor();
//            if (cursor.toChild(xmlNs, "SupportedArchitectures")) {
//                cursor.removeXml();
//            }
//            cursor.dispose();
//        }
    }
    
    public void getPlatformDefsBuildTargets(Vector<Object> targets) {
        if (getfpdPlatformDefs().getBuildTargets() == null) {
            return;
        }
        ListIterator li = getfpdPlatformDefs().getBuildTargets().listIterator();
        while(li.hasNext()) {
            targets.add(li.next());
        }
    }
    
    public void setPlatformDefsBuildTargets(Vector<Object> targets) {
        getfpdPlatformDefs().setBuildTargets(targets);
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
    
    public int getPlatformDefsSkuInfoCount(){
        if (getfpdPlatformDefs().getSkuInfo() == null || getfpdPlatformDefs().getSkuInfo().getUiSkuNameList() == null) {
            return 0;
        }
        return getfpdPlatformDefs().getSkuInfo().getUiSkuNameList().size();
    }
    
    public void getPlatformDefsSkuInfos(String[][] saa){
        if (getfpdPlatformDefs().getSkuInfo() == null || getfpdPlatformDefs().getSkuInfo().getUiSkuNameList() == null) {
            if (getfpdDynPcdBuildDefs().getPcdBuildDataList() == null) {
                removeElement(getfpdDynPcdBuildDefs());
                fpdDynPcdBuildDefs = null;
            }
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
    
    public void removePlatformDefsSkuInfo(int i) {
        SkuInfoDocument.SkuInfo skuInfo = getfpdPlatformDefs().getSkuInfo();
        if (skuInfo == null || i == 0) {
            return ;
        }
        
        XmlCursor cursor = skuInfo.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            cursor.removeXml();
        }
        cursor.dispose();
    }
    
    public void updatePlatformDefsSkuInfo(int i, String id, String name) {
        SkuInfoDocument.SkuInfo skuInfo = getfpdPlatformDefs().getSkuInfo();
        if (skuInfo == null || i == 0) {
            return ;
        }
        
        XmlCursor cursor = skuInfo.newCursor();
        if (cursor.toFirstChild()) {
            for (int j = 0; j < i; ++j) {
                cursor.toNextSibling();
            }
            SkuInfoDocument.SkuInfo.UiSkuName sku = (SkuInfoDocument.SkuInfo.UiSkuName)cursor.getObject();
            sku.setSkuID(new BigInteger(id));
            sku.setStringValue(name);
        }
        cursor.dispose();
    }
    
    public String getPlatformDefsInterDir(){
        if (getfpdPlatformDefs().getIntermediateDirectories() == null) {
            return null;
        }
        return getfpdPlatformDefs().getIntermediateDirectories().toString();
    }
    
    public void setPlatformDefsInterDir(String interDir){
        getfpdPlatformDefs().setIntermediateDirectories(IntermediateOutputType.Enum.forString(interDir));
    }
    
    public String getPlatformDefsOutputDir() {
        return getfpdPlatformDefs().getOutputDirectory();
    }
    
    public void setPlatformDefsOutputDir(String outputDir) {
        if (outputDir != null && outputDir.length() > 0) {
            getfpdPlatformDefs().setOutputDirectory(outputDir);
        }
        else{
            XmlCursor cursor = getfpdPlatformDefs().newCursor();
            if (cursor.toChild(new QName(xmlNs, "OutputDirectory"))) {
                cursor.removeXml();
            }
            cursor.dispose();
        }
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
    
    public void getFvImagesFvImageFvImageNames (Vector<String> vImageNames) {
        FvImagesDocument.FvImages fis = getfpdFlash().getFvImages();
        if (fis == null || fis.getFvImageList() == null) {
            return;
        }
        
        ListIterator<FvImagesDocument.FvImages.FvImage> li = fis.getFvImageList().listIterator();
        while (li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = li.next();
            if (fi.getType().toString().equals("ImageName")) {
                vImageNames.addAll(fi.getFvImageNamesList());
                return;
            }
        }
    }
    
    public void addFvImageFvImageNames (String[] fvNames) {
        FvImagesDocument.FvImages fis = getfpdFlash().getFvImages();
        if (fis == null || fis.getFvImageList() == null) {
            genFvImagesFvImage (fvNames, "ImageName", null);
            return;
        }
        
        ListIterator<FvImagesDocument.FvImages.FvImage> li = fis.getFvImageList().listIterator();
        while (li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = li.next();
            if (fi.getType().toString().equals("ImageName")) {
                addFvImageNamesInFvImage (fi, fvNames);
                return;
            }
        }
        genFvImagesFvImage (fvNames, "ImageName", null);    
    }
    
    public void addFvImageNamesInFvImage (FvImagesDocument.FvImages.FvImage fi, String[] fvNames) {
        
        for (int i = 0; i < fvNames.length; ++i) {
            fi.addFvImageNames(fvNames[i]);
        }
    }
    
    public void addFvImageNamesInFvImage (int i, String[] fvNames) {
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
            addFvImageNamesInFvImage(fi, fvNames);
        }
        cursor.dispose();
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
            
            nv.setName(k);
            nv.setValue((String)options.get(k));
            
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
    
    /**
     * @param oldFvName
     * @param newFvName The New FV Name. If null, remove the old FvImageNames entry.
     */
    public void updateFvImageNameAll (String oldFvName, String newFvName) {
        if (getfpdFlash().getFvImages() == null || getfpdFlash().getFvImages().getFvImageList() == null) {
            return;
        }
        ListIterator<FvImagesDocument.FvImages.FvImage> li = getfpdFlash().getFvImages().getFvImageList().listIterator();
        while (li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = li.next();
            updateFvImageNamesInFvImage (fi, oldFvName, newFvName);
            if (fi.getFvImageNamesList().size() == 0) {
                li.remove();
            }
        }
    }
    
    public void updateFvImageNamesInFvImage (int i, String oldFvName, String newFvName) {
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
            updateFvImageNamesInFvImage (fi, oldFvName, newFvName);
        }
        cursor.dispose();
    }
    /**
     * @param fi
     * @param oldFvName The FV Name to be replaced.
     * @param newFvName The New FV Name. If null, remove the old FvImageNames entry.
     */
    public void updateFvImageNamesInFvImage (FvImagesDocument.FvImages.FvImage fi, String oldFvName, String newFvName) {
        QName qFvImageNames = new QName(xmlNs, "FvImageNames");
        XmlCursor cursor = fi.newCursor();
        
        if (cursor.toChild(qFvImageNames)) {
            do {
                String xmlValue = cursor.getTextValue();
                if (xmlValue.equals(oldFvName)){
                    if (newFvName != null) {
                        cursor.setTextValue(newFvName);
                    }
                    else {
                        cursor.removeXml();
                    }
                }
            }while (cursor.toNextSibling(qFvImageNames));
        }
        
        cursor.dispose();
    }
    
    /**update the Type attribute of ith FvImage with new type.
     * @param i
     * @param type
     */
    public void updateFvImagesFvImageType (int i, String type) {
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
            fi.setType(FvImageTypes.Enum.forString(type));
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
    
    public int getFvImagesFvImageCount(String type) {
        
        if (getfpdFlash().getFvImages() == null || getfpdFlash().getFvImages().getFvImageList() == null) {
            return 0;
        }
        List<FvImagesDocument.FvImages.FvImage> l = getfpdFlash().getFvImages().getFvImageList();
        ListIterator li = l.listIterator();
        int i = 0;
        while(li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)li.next();
            if (!fi.getType().toString().equals(type) && !type.equals("ALL")) {
                continue;
            }
            
            ++i;
        }
        
        return i;
    }
    
    public Vector<FvImagesDocument.FvImages.FvImage> getFvImagesFvImageWithName (String fvName, String type) {
        Vector<FvImagesDocument.FvImages.FvImage> vFvImage = new Vector<FvImagesDocument.FvImages.FvImage>();
        if (getfpdFlash().getFvImages() == null || getfpdFlash().getFvImages().getFvImageList() == null) {
            return vFvImage;
        }
        List<FvImagesDocument.FvImages.FvImage> l = getfpdFlash().getFvImages().getFvImageList();
        ListIterator li = l.listIterator();
        while(li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)li.next();
            if (!fi.getType().toString().equals(type) && !type.equals("ALL")) {
                continue;
            }
            if (fi.getFvImageNamesList().contains(fvName)) {
                vFvImage.add(fi);
            }
        }
        
        return vFvImage;
    }
    /**
     * @param saa
     * @param type "ALL" means all FvImage types: ImageName, Options, Attributes, Components.
     */
    public void getFvImagesFvImages(String[][] saa, String type) {
    
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
            if (!fi.getType().toString().equals(type) && !type.equals("ALL")) {
                continue;
            }
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
            
            ++i;
        }
    }
    
    public void removeFvImageNameValue (int i, String attributeName) {
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
            removeFvImageNameValue (fi, attributeName);
        }
        cursor.dispose();
    }
    /**Remove from fi the attribute pair with attributeName in FvImageOptions.
     * @param fi
     * @param attributeName
     */
    public void removeFvImageNameValue (FvImagesDocument.FvImages.FvImage fi, String attributeName) {
        if (fi.getFvImageOptions() != null && fi.getFvImageOptions().getNameValueList() != null) {
            ListIterator<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> li = fi.getFvImageOptions().getNameValueList().listIterator();
            while (li.hasNext()) {
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = li.next();
                if (nv.getName().equals(attributeName)) {
                    li.remove();
                }
            }
        }
    }
    
    public void removeTypedNamedFvImageNameValue (String fvName, String type, String optName) {
        Vector<FvImagesDocument.FvImages.FvImage> vFvImage = getFvImagesFvImageWithName(fvName, type);
        for (int i = 0; i < vFvImage.size(); ++i) {
            FvImagesDocument.FvImages.FvImage fi = vFvImage.get(i);
            removeFvImageNameValue (fi, optName);
        }
    }
    
    /**Add name-value pair to FvImage element with type.
     * @param fvName FV name to add name-value pair.
     * @param type FvImage attribute.
     * @param name
     * @param value
     */
    public void setTypedNamedFvImageNameValue (String fvName, String type, String name, String value, String newName) {
        boolean fvImageExists = false;
        if (getfpdFlash().getFvImages() != null) {

            List<FvImagesDocument.FvImages.FvImage> l = getfpdFlash().getFvImages().getFvImageList();
            if (l != null) {
                ListIterator li = l.listIterator();
                while (li.hasNext()) {
                    FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage) li.next();
                    if (!fi.getType().toString().equals(type) && !type.equals("ALL")) {
                        continue;
                    }
                    if (!fi.getFvImageNamesList().contains(fvName)) {
                        continue;
                    }
                    fvImageExists = true;
                    setFvImagesFvImageNameValue(fi, name, value, newName);
                }
            }
        }

        if (!fvImageExists) {
            HashMap<String, String> map = new HashMap<String, String>();
            map.put(name, value);
            genFvImagesFvImage(new String[] { fvName }, type, map);
        }
    }
    
    /**Add to all FvImage elements with type, the name-value pair.
     * @param type
     * @param name
     * @param value
     */
    public void setTypedFvImageNameValue (String type, String name, String value) {
        if (getfpdFlash().getFvImages() == null) {
            return;
        }
        List<FvImagesDocument.FvImages.FvImage> l = getfpdFlash().getFvImages().getFvImageList();
        if (l == null) {
            return;
        }
        ListIterator li = l.listIterator();
        while(li.hasNext()) {
            FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)li.next();
            if (!fi.getType().toString().equals(type) && !type.equals("ALL")) {
                continue;
            }
            setFvImagesFvImageNameValue (fi, name, value, null);
        }
        
    }
    
    public void setFvImagesFvImageNameValue (int i, String name, String value) {
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
            setFvImagesFvImageNameValue (fi, name, value, null);
        }
        cursor.dispose();
    }
    
    /**Add to FvImage the name-value pair, or replace old name with newName, or generate new name-value pair if not exists before.
     * @param fi
     * @param name
     * @param value
     * @param newName
     */
    public void setFvImagesFvImageNameValue (FvImagesDocument.FvImages.FvImage fi, String name, String value, String newName) {
        if (fi.getFvImageOptions() == null || fi.getFvImageOptions().getNameValueList() == null) {
            FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = fi.addNewFvImageOptions().addNewNameValue();
            nv.setName(name);
            nv.setValue(value);
            if (newName != null && !newName.equals(name)) {
                nv.setName(newName);
            }
            return;
        }
        
        XmlCursor cursor = fi.getFvImageOptions().newCursor();
        if (cursor.toFirstChild()) {
            do {
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = (FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue)cursor.getObject();
                if (nv.getName().equals(name)) {
                    nv.setValue(value);
                    if (newName != null && !newName.equals(name)) {
                        nv.setName(newName);
                    }
                    cursor.dispose();
                    return;
                }
            }while (cursor.toNextSibling());
        }
        
        FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = fi.getFvImageOptions().addNewNameValue();
        nv.setName(name);
        nv.setValue(value);
        if (newName != null && !newName.equals(name)) {
            nv.setName(newName);
        }
        cursor.dispose();
    }
    
    public void getFvImagesFvImageOptions (String fvName, Map<String, String> m) {
        Vector<FvImagesDocument.FvImages.FvImage> vFvImage = getFvImagesFvImageWithName (fvName, "Options");
        for (int i = 0; i < vFvImage.size(); ++i) {
            FvImagesDocument.FvImages.FvImage fi = vFvImage.get(i);
            if (fi == null || fi.getFvImageOptions() == null || fi.getFvImageOptions().getNameValueList() == null) {
                continue;
            }

            ListIterator<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> li = fi.getFvImageOptions()
                                                                                            .getNameValueList()
                                                                                            .listIterator();
            while (li.hasNext()) {
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = li.next();
                m.put(nv.getName(), nv.getValue());
            }
        }
    }
    
    public int getFvImagePosInFvImages (String fvNameList, String type) {
        XmlObject o = getfpdFlash().getFvImages();
        if (o == null) {
            return -1;
        }
        
        int pos = -1;
        String[] fvNameArray = fvNameList.trim().split(" ");
        Vector<String> vFvNames = new Vector<String>();
        
        
        XmlCursor cursor = o.newCursor();
        QName qFvImage = new QName(xmlNs, "FvImage");
        if (cursor.toChild(qFvImage)) {
            do {
                pos++;
                vFvNames.removeAllElements();
                for (int i = 0; i < fvNameArray.length; ++i) {
                    vFvNames.add(fvNameArray[i]);
                }
                FvImagesDocument.FvImages.FvImage fi = (FvImagesDocument.FvImages.FvImage)cursor.getObject();
                if (!fi.getType().toString().equals(type)) {
                    continue;
                }
                if (fi.getFvImageNamesList() == null || fi.getFvImageNamesList().size() != vFvNames.size()) {
                    continue;
                }
                ListIterator<String> li = fi.getFvImageNamesList().listIterator();
                while (li.hasNext()) {
                    String name = li.next();
                    vFvNames.remove(name);
                }
                if (vFvNames.size() == 0) {
                    cursor.dispose();
                    return pos;
                }
                
            }while (cursor.toNextSibling(qFvImage));
           
        }
        cursor.dispose();
        return -1;
    }
    /**Get FvImage Options for FvImage i
     * @param i the ith FvImage
     */
    public void getFvImagesFvImageOptions(int i, Map<String, String> m) {
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
            if (fi.getFvImageOptions() == null || fi.getFvImageOptions().getNameValueList() == null){
                cursor.dispose();
                return;
            }
            ListIterator<FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue> li = fi.getFvImageOptions().getNameValueList().listIterator();
            while(li.hasNext()){
                FvImagesDocument.FvImages.FvImage.FvImageOptions.NameValue nv = li.next();
                m.put(nv.getName(), nv.getValue());
            }
        }
        cursor.dispose();
    }
    
    /**
     Get platform header element
     @return PlatformHeaderDocument.PlatformHeader
    **/
    public PlatformHeaderDocument.PlatformHeader getFpdHdr() {
        if (fpdHdr == null) {
            fpdHdr = fpdRoot.addNewPlatformHeader();
        }
        
        return fpdHdr;
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

        return "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";
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
        s = "FRAMEWORK_BUILD_PACKAGING_SPECIFICATION   0x00000052";
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
    
    private String listToString(List l) {
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
    
    private void removeElement(XmlObject o) {
        XmlCursor cursor = o.newCursor();
        cursor.removeXml();
        cursor.dispose();
    }
}

class PcdItemTypeConflictException extends Exception {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private String details = null;
    
    PcdItemTypeConflictException (String pcdName, String info) {
        ModuleIdentification mi = WorkspaceProfile.getModuleId(info);
        if (mi != null) {
            details = pcdName + " ItemType Conflicts with " + mi.getName() + "\n in Pkg " + mi.getPackageId().getName();    
        }
        else {
            details = pcdName + " ItemType Conflicts with \n" + info;
        }
    }
    
    PcdItemTypeConflictException (String pcdName, String info1, String info2) {
        ModuleIdentification mi1 = WorkspaceProfile.getModuleId(info1);
        ModuleIdentification mi2 = WorkspaceProfile.getModuleId(info2);
        String moduleInfo1 = "";
        String moduleInfo2 = "";
        if (mi1 != null) {
            moduleInfo1 = mi1.getName() + " in Pkg " + mi1.getPackageId().getName();
        }
        else {
            moduleInfo1 = info1;
        }
        
        if (mi2 != null) {
            moduleInfo2 = mi2.getName() + " in Pkg " + mi2.getPackageId().getName();
        }
        else {
            moduleInfo2 = info2;
        }
        
        details = pcdName + " ItemType Conflicts in \n" + moduleInfo1 + "\n and " + moduleInfo2;
    }
    
    public String getMessage() {
        return details;
    }
}

class PcdDeclNotFound extends Exception {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private String details = null;
    
    PcdDeclNotFound(String info) {
        details = "PcdDeclNotFound: " + info;
    }
    
    public String getMessage() {
        return details;
    }
}

class PcdValueMalFormed extends Exception {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private String details = null;
    
    PcdValueMalFormed(String info) {
        details = "PcdValueMalFormed: " + info;
    }
    
    public String getMessage() {
        return details;
    }
}
