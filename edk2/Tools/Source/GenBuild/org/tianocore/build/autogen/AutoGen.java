/** @file
 AutoGen class.

 This class is to generate Autogen.h and Autogen.c according to module surface area
 or library surface area.

 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php

 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.build.autogen;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.GuidsDocument;
import org.tianocore.LibraryClassDocument.LibraryClass;
import org.tianocore.PPIsDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.build.exception.*;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.Spd;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.ModuleIdentification;
import org.tianocore.build.id.PackageIdentification;
import org.tianocore.build.pcd.action.PCDAutoGenAction;

import org.tianocore.common.logger.EdkLog;

/**
 * This class is to generate Autogen.h and Autogen.c according to module surface
 * area or library surface area.
 */
public class AutoGen {
    ///
    /// The output path of Autogen.h and Autogen.c
    ///
    private String outputPath;

    ///
    /// The name of FV directory
    ///
    private String fvDir;

    ///
    /// The base name of module or library.
    ///
    private ModuleIdentification moduleId;

    ///
    /// The build architecture
    ///
    private String arch;

    ///
    /// PcdAutogen instance which is used to manage how to generate the PCD
    /// information.
    ///
    private PCDAutoGenAction myPcdAutogen;

    ///
    /// the one of type : NOT_PCD_DRIVER, PEI_PCD_DRIVER, DXE_PCD_DRIVER
    /// 
    private CommonDefinition.PCD_DRIVER_TYPE pcdDriverType;

    ///
    /// The protocl list which records in module or library surface area and
    /// it's dependence on library instance surface area.
    ///
    private Set<String> mProtocolList = new HashSet<String>();

    ///
    /// The Ppi list which recorded in module or library surface area and its
    /// dependency on library instance surface area.
    ///
    private Set<String> mPpiList = new HashSet<String>();

    ///
    /// The Guid list which recoreded in module or library surface area and it's
    /// dependence on library instance surface area.
    ///
    private Set<String> mGuidList = new HashSet<String>();

    //
    // The dependence package list which recoreded in module or library surface
    // area and it's dependence on library instance surface are.
    //
    private List<PackageIdentification> mDepPkgList = new LinkedList<PackageIdentification>();

    //
    //  For non library module, add its library instance's construct and destructor to
    //  list.
    //
    private List<String> libConstructList = new ArrayList<String>();
    private List<String> libDestructList = new ArrayList<String>();

    //
    // List to store SetVirtalAddressMapCallBack, ExitBootServiceCallBack
    //
    private List<String> setVirtalAddList = new ArrayList<String>();
    private List<String> exitBootServiceList = new ArrayList<String>();


    /**
     * Construct function
     *
     * This function mainly initialize some member variable.
     *
     * @param outputPath
     *            Output path of AutoGen file.
     * @param baseName
     *            Module base name.
     * @param arch
     *            Target architecture.
     */
    public AutoGen(String fvDir, String outputPath, ModuleIdentification moduleId, String arch) {
        this.outputPath = outputPath;
        this.moduleId = moduleId;
        this.arch = arch;
        this.fvDir = fvDir;

    }

    /**
     * saveFile function
     *
     * This function save the content in stringBuffer to file.
     *
     * @param fileName
     *            The name of file.
     * @param fileBuffer
     *            The content of AutoGen file in buffer.
     * @return "true" successful, "false" failed.
     */
    private boolean saveFile(String fileName, StringBuffer fileBuffer) {
        try {
            File autoGenH = new File(fileName);

            //
            // if the file exists, compare their content
            //
            if (autoGenH.exists()) {
                FileReader fIn = new FileReader(autoGenH);
                char[] oldFileBuffer = new char[(int) autoGenH.length()];
                fIn.read(oldFileBuffer, 0, (int) autoGenH.length());
                fIn.close();

                //
                // if we got the same file, don't re-generate it to prevent
                // sources depending on it from re-building
                //
                if (fileBuffer.toString().compareTo(new String(oldFileBuffer)) == 0) {
                    return true;
                }
            }
            FileWriter fOut = new FileWriter(autoGenH);
            fOut.write(fileBuffer.toString());
            fOut.close();
        } catch (Exception e) {
            return false;
        }
        return true;
    }

    /**
     * genAutogen function
     *
     * This function call libGenAutoGen or moduleGenAutogen function, which
     * dependence on generate library autogen or module autogen.
     *
     * @throws BuildException
     *             Failed to creat AutoGen.c & AutoGen.h.
     */
    public void genAutogen() throws BuildException {
        try {
            //
            // If outputPath do not exist, create it.
            //
            File path = new File(outputPath);
            path.mkdirs();

            //
            // Check current is library or not, then call the corresponding
            // function.
            //
            if (this.moduleId.isLibrary()) {
                libGenAutogen();
            } else {
                moduleGenAutogen();
            }

        } catch (Exception e) {
            throw new BuildException(
                                    "Failed to create AutoGen.c & AutoGen.h!\n"
                                    + e.getMessage());
        }
    }

    /**
     * moduleGenAutogen function
     *
     * This function generates AutoGen.c & AutoGen.h for module.
     *
     * @throws BuildException
     *             Faile to create module AutoGen.c & AutoGen.h.
     */
    void moduleGenAutogen() throws BuildException {

        try {
            collectLibInstanceInfo();
            moduleGenAutogenC();
            moduleGenAutogenH();
        } catch (Exception e) {
            throw new BuildException(
                                    "Faile to create module AutoGen.c & AutoGen.h!\n"
                                    + e.getMessage());
        }
    }

    /**
     * libGenAutogen function
     *
     * This function generates AutoGen.c & AutoGen.h for library.
     *
     * @throws BuildException
     *             Faile to create library AutoGen.c & AutoGen.h
     */
    void libGenAutogen() throws BuildException {
        try {
            libGenAutogenC();
            libGenAutogenH();
        } catch (Exception e) {
            throw new BuildException(
                                    "Failed to create library AutoGen.c & AutoGen.h!\n"
                                    + e.getMessage());
        }
    }

    /**
     * moduleGenAutogenH
     *
     * This function generates AutoGen.h for module.
     *
     * @throws BuildException
     *             Failed to generate AutoGen.h.
     */
    void moduleGenAutogenH() throws AutoGenException {

        Set<String> libClassIncludeH;
        String moduleType;
        // List<String> headerFileList;
        List<String> headerFileList;
        Iterator item;
        StringBuffer fileBuffer = new StringBuffer(8192);

        //
        // Write Autogen.h header notation
        //
        fileBuffer.append(CommonDefinition.autogenHNotation);

        //
        // Add #ifndef ${BaseName}_AUTOGENH
        // #def ${BseeName}_AUTOGENH
        //
        fileBuffer.append("#ifndef    " + "_AUTOGENH_" + this.moduleId.getGuid().replaceAll("-", "_") +"\r\n");
        fileBuffer.append("#define    " + "_AUTOGENH_" + this.moduleId.getGuid().replaceAll("-", "_") +"\r\n\r\n");

        //
        // Write the specification version and release version at the begine
        // of autogen.h file.
        // Note: the specification version and release version should
        // be got from module surface area instead of hard code by it's
        // moduleType.
        //
        moduleType = SurfaceAreaQuery.getModuleType();

        //
        // Add "extern int __make_me_compile_correctly;" at begin of
        // AutoGen.h.
        //
        fileBuffer.append(CommonDefinition.autoGenHbegin);

        //
        // Put EFI_SPECIFICATION_VERSION, and EDK_RELEASE_VERSION.
        //
        String[] specList = SurfaceAreaQuery.getExternSpecificaiton();
        for (int i = 0; i < specList.length; i++) {
            fileBuffer.append(CommonDefinition.marcDefineStr + specList[i]
                              + "\r\n");
        }
        //
        // Write consumed package's mdouleInfo related .h file to autogen.h
        //
        // PackageIdentification[] consumedPkgIdList = SurfaceAreaQuery
        // .getDependencePkg(this.arch);
        PackageIdentification[] consumedPkgIdList = SurfaceAreaQuery
                                                    .getDependencePkg(this.arch);
        if (consumedPkgIdList != null) {
            headerFileList = depPkgToAutogenH(consumedPkgIdList, moduleType);
            item = headerFileList.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }

        //
        // Write library class's related *.h file to autogen.h.
        //
        String[] libClassList = SurfaceAreaQuery
                                .getLibraryClasses(CommonDefinition.AlwaysConsumed,this.arch);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            item = libClassIncludeH.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }

        libClassList = SurfaceAreaQuery
                       .getLibraryClasses(CommonDefinition.AlwaysProduced, this.arch);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            item = libClassIncludeH.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }
        fileBuffer.append("\r\n");

        //
        //  If is TianoR8FlashMap, copy {Fv_DIR}/FlashMap.h to
        // {DEST_DIR_DRBUG}/FlashMap.h
        //
        if (SurfaceAreaQuery.isHaveTianoR8FlashMap()) {
            fileBuffer.append(CommonDefinition.include);
            fileBuffer.append("  <");
            fileBuffer.append(CommonDefinition.tianoR8FlashMapH + ">\r\n");
            copyFlashMapHToDebugDir();
        }

        // Write PCD autogen information to AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.OutputH());
        }

        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");

        //
        // Save string buffer content in AutoGen.h.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.h !!!");
        }
    }

    /**
     * moduleGenAutogenC
     *
     * This function generates AutoGen.c for module.
     *
     * @throws BuildException
     *             Failed to generate AutoGen.c.
     */
    void moduleGenAutogenC() throws AutoGenException {

        StringBuffer fileBuffer = new StringBuffer(8192);
        //
        // Write Autogen.c header notation
        //
        fileBuffer.append(CommonDefinition.autogenCNotation);

        //
        // Write #include <AutoGen.h> at beginning of AutoGen.c
        //
        fileBuffer.append(CommonDefinition.includeAutogenH);

        //
        // Get the native MSA file infomation. Since before call autogen,
        // the MSA native <Externs> information were overrided. So before
        // process <Externs> it should be set the DOC as the Native MSA info.
        //
        Map<String, XmlObject> doc = GlobalData.getNativeMsa(this.moduleId);
        SurfaceAreaQuery.push(doc);
        //
        // Write <Extern>
        // DriverBinding/ComponentName/DriverConfiguration/DriverDialog
        // to AutoGen.c
        //

        ExternsDriverBindingToAutoGenC(fileBuffer);

        //
        // Write DriverExitBootServicesEvent/DriverSetVirtualAddressMapEvent
        // to Autogen.c
        //
        ExternCallBackToAutoGenC(fileBuffer);

        //
        // Write EntryPoint to autgoGen.c
        //
        String[] entryPointList = SurfaceAreaQuery.getModuleEntryPointArray();
        EntryPointToAutoGen(CommonDefinition.remDupString(entryPointList), fileBuffer);

        pcdDriverType = SurfaceAreaQuery.getPcdDriverType();

        //
        // Restore the DOC which include the FPD module info.
        //
        SurfaceAreaQuery.pop();

        //
        // Write Guid to autogen.c
        //
        String guid = CommonDefinition.formatGuidName(SurfaceAreaQuery
                                                      .getModuleGuid());

        fileBuffer
        .append("GLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID gEfiCallerIdGuid = {");
        if (guid == null) {
            throw new AutoGenException("Guid value must set!\n");
        }

        //
        // Formate Guid as ANSI c form.Example:
        // {0xd2b2b828, 0x826, 0x48a7,{0xb3, 0xdf, 0x98, 0x3c, 0x0, 0x60, 0x24,
        // 0xf0}}
        //

        fileBuffer.append(guid);
        fileBuffer.append("};\r\n");

        //
        // Generate library instance consumed protocol, guid, ppi, pcd list.
        // Save those to this.protocolList, this.ppiList, this.pcdList,
        // this.guidList. Write Consumed library constructor and desconstuct to
        // autogen.c
        //
        LibInstanceToAutogenC(fileBuffer);

        //
        // Get module dependent Package identification.
        //
        PackageIdentification[] packages = SurfaceAreaQuery.getDependencePkg(this.arch);
        for (int i = 0; i < packages.length; i++) {
            if (!this.mDepPkgList.contains(packages[i])) {
                this.mDepPkgList.add(packages[i]);
            }

        }

        //
        // Write consumed ppi, guid, protocol to autogen.c
        //
        ProtocolGuidToAutogenC(fileBuffer);
        PpiGuidToAutogenC(fileBuffer);
        GuidGuidToAutogenC(fileBuffer);

        //
        // Call pcd autogen.
        //
        this.myPcdAutogen = new PCDAutoGenAction(moduleId, 
                                                 arch, 
                                                 false, 
                                                 null,
                                                 pcdDriverType);
        try {
            this.myPcdAutogen.execute();
        } catch (Exception exp) {
            throw new PcdAutogenException (exp.getMessage());
        }

        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.OutputC());
        }

        if (!saveFile(outputPath + File.separatorChar + "AutoGen.c", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.c !!!");
        }

    }

    /**
     * libGenAutogenH
     *
     * This function generates AutoGen.h for library.
     *
     * @throws BuildException
     *             Failed to generate AutoGen.c.
     */
    void libGenAutogenH() throws AutoGenException {

        Set<String> libClassIncludeH;
        String moduleType;
        List<String> headerFileList;
        Iterator item;
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.h header notation
        //
        fileBuffer.append(CommonDefinition.autogenHNotation);

        //
        // Add #ifndef ${BaseName}_AUTOGENH
        // #def ${BseeName}_AUTOGENH
        //
        fileBuffer.append("#ifndef    " + "_AUTOGENH_" + this.moduleId.getGuid().replaceAll("-", "_") + "\r\n");
        fileBuffer.append("#define    " + "_AUTOGENH_" + this.moduleId.getGuid().replaceAll("-", "_") + "\r\n\r\n");

        //
        // Write EFI_SPECIFICATION_VERSION and EDK_RELEASE_VERSION
        // to autogen.h file.
        // Note: the specification version and release version should
        // be get from module surface area instead of hard code.
        //
        fileBuffer.append(CommonDefinition.autoGenHbegin);
        String[] specList = SurfaceAreaQuery.getExternSpecificaiton();
        for (int i = 0; i < specList.length; i++) {
            fileBuffer.append(CommonDefinition.marcDefineStr + specList[i]
                              + "\r\n");
        }
        // fileBuffer.append(CommonDefinition.autoGenHLine1);
        // fileBuffer.append(CommonDefinition.autoGenHLine2);

        //
        // Write consumed package's mdouleInfo related *.h file to autogen.h.
        //
        moduleType = SurfaceAreaQuery.getModuleType();
        PackageIdentification[] cosumedPkglist = SurfaceAreaQuery
                                                 .getDependencePkg(this.arch);
        headerFileList = depPkgToAutogenH(cosumedPkglist, moduleType);
        item = headerFileList.iterator();
        while (item.hasNext()) {
            fileBuffer.append(item.next().toString());
        }
        //
        // Write library class's related *.h file to autogen.h
        //
        String[] libClassList = SurfaceAreaQuery
                                .getLibraryClasses(CommonDefinition.AlwaysConsumed, this.arch);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            item = libClassIncludeH.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }

        libClassList = SurfaceAreaQuery
                       .getLibraryClasses(CommonDefinition.AlwaysProduced, this.arch);
        if (libClassList != null) {
            libClassIncludeH = LibraryClassToAutogenH(libClassList);
            item = libClassIncludeH.iterator();
            while (item.hasNext()) {
                fileBuffer.append(item.next().toString());
            }
        }
        fileBuffer.append("\r\n");

        //
        //  If is TianoR8FlashMap, copy {Fv_DIR}/FlashMap.h to
        // {DEST_DIR_DRBUG}/FlashMap.h
        //
        if (SurfaceAreaQuery.isHaveTianoR8FlashMap()) {
            fileBuffer.append(CommonDefinition.include);
            fileBuffer.append("  <");
            fileBuffer.append(CommonDefinition.tianoR8FlashMapH + ">\r\n");
            copyFlashMapHToDebugDir();
        }

        //
        // Write PCD information to library AutoGen.h.
        //
        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.OutputH());
        }

        //
        // Append the #endif at AutoGen.h
        //
        fileBuffer.append("#endif\r\n");

        //
        // Save content of string buffer to AutoGen.h file.
        //
        if (!saveFile(outputPath + File.separatorChar + "AutoGen.h", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.h !!!");
        }
    }

    /**
     * libGenAutogenC
     *
     * This function generates AutoGen.h for library.
     *
     * @throws BuildException
     *             Failed to generate AutoGen.c.
     */
    void libGenAutogenC() throws BuildException, PcdAutogenException {
        StringBuffer fileBuffer = new StringBuffer(10240);

        //
        // Write Autogen.c header notation
        //
        fileBuffer.append(CommonDefinition.autogenCNotation);

        fileBuffer.append(CommonDefinition.autoGenCLine1);
        fileBuffer.append("\r\n");

        //
        // Call pcd autogen.
        //
        this.myPcdAutogen = new PCDAutoGenAction(moduleId,
                                                 arch,
                                                 true,
                                                 SurfaceAreaQuery.getModulePcdEntryNameArray(),
                                                 pcdDriverType);
        try {
            this.myPcdAutogen.execute();
        } catch (Exception e) {
            throw new PcdAutogenException(e.getMessage());
        }

        if (this.myPcdAutogen != null) {
            fileBuffer.append("\r\n");
            fileBuffer.append(this.myPcdAutogen.OutputC());
        }

        if (!saveFile(outputPath + File.separatorChar + "AutoGen.c", fileBuffer)) {
            throw new BuildException("Failed to generate AutoGen.c !!!");
        }
    }

    /**
     * LibraryClassToAutogenH
     *
     * This function returns *.h files declared by library classes which are
     * consumed or produced by current build module or library.
     *
     * @param libClassList
     *            List of library class which consumed or produce by current
     *            build module or library.
     * @return includeStrList List of *.h file.
     */
    Set<String> LibraryClassToAutogenH(String[] libClassList)
    throws AutoGenException {
        Set<String> includStrList = new LinkedHashSet<String>();
        String includerName[];
        String str = "";

        //
        // Get include file from GlobalData's SPDTable according to
        // library class name.
        //

        for (int i = 0; i < libClassList.length; i++) {
            includerName = GlobalData.getLibraryClassHeaderFiles(
                                                                SurfaceAreaQuery.getDependencePkg(this.arch),
                                                                libClassList[i]);
            if (includerName == null) {
                throw new AutoGenException("Can not find library class ["
                                           + libClassList[i] + "] declaration in any SPD package. ");
            }
            for (int j = 0; j < includerName.length; j++) {
                String includeNameStr = includerName[j];
                if (includeNameStr != null) {
                    str = CommonDefinition.include + " " + "<";
                    str = str + includeNameStr + ">\r\n";
                    includStrList.add(str);
                    includeNameStr = null;
                }
            }
        }
        return includStrList;
    }

    /**
     * IncludesToAutogenH
     *
     * This function add include file in AutoGen.h file.
     *
     * @param packageNameList
     *            List of module depended package.
     * @param moduleType
     *            Module type.
     * @return
     */
    List<String> depPkgToAutogenH(PackageIdentification[] packageNameList,
                                  String moduleType) throws AutoGenException {

        List<String> includeStrList = new LinkedList<String>();
        String pkgHeader;
        String includeStr = "";

        //
        // Get include file from moduleInfo file
        //
        for (int i = 0; i < packageNameList.length; i++) {
            pkgHeader = GlobalData.getPackageHeaderFiles(packageNameList[i],
                                                         moduleType);
            if (pkgHeader == null) {
                throw new AutoGenException("Can not find package ["
                                           + packageNameList[i]
                                           + "] declaration in any SPD package. ");
            } else if (!pkgHeader.equalsIgnoreCase("")) {
                includeStr = CommonDefinition.include + " <" + pkgHeader
                             + ">\r\n";
                includeStrList.add(includeStr);
            }
        }

        return includeStrList;
    }

    /**
     * EntryPointToAutoGen
     *
     * This function convert <ModuleEntryPoint> & <ModuleUnloadImage>
     * information in mas to AutoGen.c
     *
     * @param entryPointList
     *            List of entry point.
     * @param fileBuffer
     *            String buffer fo AutoGen.c.
     * @throws Exception
     */
    void EntryPointToAutoGen(String[] entryPointList, StringBuffer fileBuffer)
    throws BuildException {

        String typeStr = SurfaceAreaQuery.getModuleType();

        //
        // The parameters and return value of entryPoint is difference
        // for difference module type.
        //
        switch (CommonDefinition.getModuleType(typeStr)) {
        
        case CommonDefinition.ModuleTypePeiCore:
            if (entryPointList == null ||entryPointList.length != 1 ) {
                throw new BuildException(
                                        "Module type = 'PEI_CORE', can have only one module entry point!");
            } else {
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (\r\n");
                fileBuffer
                .append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                fileBuffer
                .append("  IN VOID                        *OldCoreData\r\n");
                fileBuffer.append("  );\r\n\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer
                .append("  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,\r\n");
                fileBuffer
                .append("  IN VOID                        *OldCoreData\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (PeiStartupDescriptor, OldCoreData);\r\n");
                fileBuffer.append("}\r\n\r\n");
            }
            break;

        case CommonDefinition.ModuleTypeDxeCore:
            fileBuffer.append("const UINT32 _gUefiDriverRevision = 0;\r\n");
            if (entryPointList == null || entryPointList.length != 1) {
                throw new BuildException(
                                        "Module type = 'DXE_CORE', can have only one module entry point!");
            } else {

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (\n");
                fileBuffer.append("  IN VOID  *HobStart\r\n");
                fileBuffer.append("  );\r\n\r\n");

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN VOID  *HobStart\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (HobStart);\r\n");
                fileBuffer.append("}\r\n\r\n");
            }
            break;

        case CommonDefinition.ModuleTypePeim:
            int entryPointCount = 0;
            fileBuffer
            .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT32 _gPeimRevision = 0;\r\n");
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
                fileBuffer.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n\r\n");
                break;
            }
            for (int i = 0; i < entryPointList.length; i++) {
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(entryPointList[i]);
                fileBuffer.append(" (\r\n");
                fileBuffer
                .append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
                fileBuffer
                .append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
                fileBuffer.append("  );\r\n");
                entryPointCount++;

            }

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleEntryPointList (\r\n");
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER  *FfsHeader,\r\n");
            fileBuffer.append("  IN EFI_PEI_SERVICES     **PeiServices\r\n");
            fileBuffer.append("  )\r\n\r\n");
            fileBuffer.append("{\r\n");
            if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("  EFI_STATUS  CombinedStatus;\r\n\r\n");
                fileBuffer.append("  CombinedStatus = EFI_LOAD_ERROR;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (!entryPointList[i].equals("")) {
                        fileBuffer.append("  Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
                        fileBuffer
                        .append("  if (!EFI_ERROR (Status) || EFI_ERROR (CombinedStatus)) {\r\n");
                        fileBuffer.append("    CombinedStatus = Status;\r\n");
                        fileBuffer.append("  }\r\n\r\n");
                    } else {
                        break;
                    }
                }
                fileBuffer.append("  return CombinedStatus;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeSmmDriver:
            entryPointCount = 0;
            //
            // If entryPoint is null, create an empty ProcessModuleEntryPointList
            // function.
            //
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED  const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n\r\n");

            } else {
                for (int i = 0; i < entryPointList.length; i++) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                }
                fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED  const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                fileBuffer
                .append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
                fileBuffer
                .append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n\r\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");


                for (int i = 0; i < entryPointList.length; i++) {
                    fileBuffer
                    .append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                    fileBuffer.append("    ExitDriver (");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                    fileBuffer.append("    ASSERT (FALSE);\r\n");
                    fileBuffer.append("  }\r\n");

                }
                fileBuffer.append("  return mDriverEntryPointStatus;\r\n");
                fileBuffer.append("}\r\n\r\n");

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ExitDriver (\r\n");
                fileBuffer.append("  IN EFI_STATUS  Status\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer
                .append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
                fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
                fileBuffer.append("  }\r\n");
                fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
                fileBuffer.append("  ASSERT (FALSE);\r\n");
                fileBuffer.append("}\r\n\r\n");

            }


            //
            // Add "ModuleUnloadImage" for DxeSmmDriver module type;
            //
            entryPointList = SurfaceAreaQuery.getModuleUnloadImageArray();
            entryPointList = CommonDefinition.remDupString(entryPointList);
            entryPointCount = 0;

            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer
                    .append("  IN EFI_HANDLE        ImageHandle\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                }
            }

            fileBuffer
            .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n\r\n");

            fileBuffer.append("EFI_STATUS\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (i == 0) {
                        fileBuffer.append("     Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                    } else {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    }
                }
                fileBuffer.append("  return Status;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            entryPointCount = 0;
            fileBuffer.append("const UINT32 _gUefiDriverRevision = 0;\r\n");
            //
            // If entry point is null, create a empty ProcessModuleEntryPointList function.
            //
            if (entryPointList == null || entryPointList.length == 0) {
                fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverEntryPointCount = 0;\r\n");
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
                fileBuffer.append("}\r\n");

            } else {
                for (int i = 0; i < entryPointList.length; i++) {

                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                    fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                }

                fileBuffer
                .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverEntryPointCount = ");
                fileBuffer.append(Integer.toString(entryPointCount));
                fileBuffer.append(";\r\n");
                if (entryPointCount > 1) {
                    fileBuffer
                    .append("static BASE_LIBRARY_JUMP_BUFFER  mJumpContext;\r\n");
                    fileBuffer
                    .append("static EFI_STATUS  mDriverEntryPointStatus = EFI_LOAD_ERROR;\r\n");
                }
                fileBuffer.append("\n");

                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ProcessModuleEntryPointList (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");

                if (entryPointCount == 1) {
                    fileBuffer.append("  return (");
                    fileBuffer.append(entryPointList[0]);
                    fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                } else {
                    for (int i = 0; i < entryPointList.length; i++) {
                        if (!entryPointList[i].equals("")) {
                            fileBuffer
                            .append("  if (SetJump (&mJumpContext) == 0) {\r\n");
                            fileBuffer.append("    ExitDriver (");
                            fileBuffer.append(entryPointList[i]);
                            fileBuffer.append(" (ImageHandle, SystemTable));\r\n");
                            fileBuffer.append("    ASSERT (FALSE);\r\n");
                            fileBuffer.append("  }\r\n");
                        } else {
                            break;
                        }
                    }
                    fileBuffer.append("  return mDriverEntryPointStatus;\r\n");
                }
                fileBuffer.append("}\r\n\r\n");

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append("ExitDriver (\r\n");
                fileBuffer.append("  IN EFI_STATUS  Status\r\n");
                fileBuffer.append("  )\r\n\r\n");
                fileBuffer.append("{\r\n");
                if (entryPointCount <= 1) {
                    fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                    fileBuffer
                    .append("    ProcessLibraryDestructorList (gImageHandle, gST);\r\n");
                    fileBuffer.append("  }\r\n");
                    fileBuffer
                    .append("  gBS->Exit (gImageHandle, Status, 0, NULL);\r\n");
                } else {
                    fileBuffer
                    .append("  if (!EFI_ERROR (Status) || EFI_ERROR (mDriverEntryPointStatus)) {\r\n");
                    fileBuffer.append("    mDriverEntryPointStatus = Status;\r\n");
                    fileBuffer.append("  }\r\n");
                    fileBuffer.append("  LongJump (&mJumpContext, (UINTN)-1);\r\n");
                    fileBuffer.append("  ASSERT (FALSE);\r\n");
                }
                fileBuffer.append("}\r\n\r\n");

            }

            //
            // Add ModuleUnloadImage for DxeDriver and UefiDriver module type.
            //
            entryPointList = SurfaceAreaQuery.getModuleUnloadImageArray();
            //
            // Remover duplicate unload entry point.
            //
            entryPointList = CommonDefinition.remDupString(entryPointList);
            entryPointCount = 0;
            if (entryPointList != null) {
                for (int i = 0; i < entryPointList.length; i++) {
                    fileBuffer.append("EFI_STATUS\r\n");
                    fileBuffer.append("EFIAPI\r\n");
                    fileBuffer.append(entryPointList[i]);
                    fileBuffer.append(" (\r\n");
                    fileBuffer
                    .append("  IN EFI_HANDLE        ImageHandle\r\n");
                    fileBuffer.append("  );\r\n");
                    entryPointCount++;
                }
            }

            fileBuffer
            .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverUnloadImageCount = ");
            fileBuffer.append(Integer.toString(entryPointCount));
            fileBuffer.append(";\r\n\r\n");

            fileBuffer.append("EFI_STATUS\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessModuleUnloadList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE  ImageHandle\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");

            if (entryPointCount == 0) {
                fileBuffer.append("  return EFI_SUCCESS;\r\n");
            } else if (entryPointCount == 1) {
                fileBuffer.append("  return ");
                fileBuffer.append(entryPointList[0]);
                fileBuffer.append("(ImageHandle);\r\n");
            } else {
                fileBuffer.append("  EFI_STATUS  Status;\r\n\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n\r\n");
                for (int i = 0; i < entryPointList.length; i++) {
                    if (i == 0) {
                        fileBuffer.append("  Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                    } else {
                        fileBuffer.append("  if (EFI_ERROR (Status)) {\r\n");
                        fileBuffer.append("    ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  } else {\r\n");
                        fileBuffer.append("    Status = ");
                        fileBuffer.append(entryPointList[i]);
                        fileBuffer.append("(ImageHandle);\r\n");
                        fileBuffer.append("  }\r\n");
                    }
                }
                fileBuffer.append("  return Status;\r\n");
            }
            fileBuffer.append("}\r\n\r\n");
            break;
        }
    }

    /**
     * PpiGuidToAutogenc
     *
     * This function gets GUIDs from SPD file accrodeing to <PPIs> information
     * and write those GUIDs to AutoGen.c.
     *
     * @param fileBuffer
     *            String Buffer for Autogen.c file.
     * @throws BuildException
     *             Guid must set value!
     */
    void PpiGuidToAutogenC(StringBuffer fileBuffer) throws AutoGenException {
        String[] cNameGuid = null;

        //
        // Get the all PPI adn PPI Notify from MSA file,
        // then add those PPI ,and PPI Notify name to list.
        //

        String[] ppiList = SurfaceAreaQuery.getPpiArray(this.arch);
        for (int i = 0; i < ppiList.length; i++) {
            this.mPpiList.add(ppiList[i]);
        }

        String[] ppiNotifyList = SurfaceAreaQuery.getPpiNotifyArray(this.arch);
        for (int i = 0; i < ppiNotifyList.length; i++) {
            this.mPpiList.add(ppiNotifyList[i]);
        }

        //
        // Find CNAME and GUID from dependence SPD file and write to Autogen.c
        //
        Iterator ppiIterator = this.mPpiList.iterator();
        String ppiKeyWord = null;
        while (ppiIterator.hasNext()) {
            ppiKeyWord = ppiIterator.next().toString();
            cNameGuid = GlobalData.getPpiGuid(this.mDepPkgList, ppiKeyWord);
            if (cNameGuid != null) {
                fileBuffer
                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                fileBuffer.append(cNameGuid[0]);
                fileBuffer.append(" =     { ");
                fileBuffer.append(CommonDefinition.formatGuidName(cNameGuid[1]));
                fileBuffer.append(" } ;");
            } else {
                //
                // If can't find Ppi GUID declaration in every package
                //
                throw new AutoGenException("Can not find Ppi GUID ["
                                           + ppiKeyWord + "] declaration in any SPD package!");
            }
        }
    }

    /**
     * ProtocolGuidToAutogenc
     *
     * This function gets GUIDs from SPD file accrodeing to <Protocol>
     * information and write those GUIDs to AutoGen.c.
     *
     * @param fileBuffer
     *            String Buffer for Autogen.c file.
     * @throws BuildException
     *             Protocol name must set.
     */
    void ProtocolGuidToAutogenC(StringBuffer fileBuffer) throws BuildException {
        String[] cNameGuid = null;

        String[] protocolList = SurfaceAreaQuery.getProtocolArray(this.arch);

        //
        // Add result to Autogen global list.
        //
        for (int i = 0; i < protocolList.length; i++) {
            this.mProtocolList.add(protocolList[i]);
        }

        String[] protocolNotifyList = SurfaceAreaQuery
                                      .getProtocolNotifyArray(this.arch);

        for (int i = 0; i < protocolNotifyList.length; i++) {
            this.mProtocolList.add(protocolNotifyList[i]);
        }

        //
        // Get the NAME and GUID from dependence SPD and write to Autogen.c
        //
        Iterator protocolIterator = this.mProtocolList.iterator();
        String protocolKeyWord = null;


        while (protocolIterator.hasNext()) {
            protocolKeyWord = protocolIterator.next().toString();
            cNameGuid = GlobalData.getProtocolGuid(this.mDepPkgList, protocolKeyWord);
            if (cNameGuid != null) {
                fileBuffer
                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                fileBuffer.append(cNameGuid[0]);
                fileBuffer.append(" =     { ");
                fileBuffer.append(CommonDefinition.formatGuidName(cNameGuid[1]));
                fileBuffer.append(" } ;");
            } else {
                //
                // If can't find protocol GUID declaration in every package
                //
                throw new BuildException("Can not find protocol Guid ["
                                         + protocolKeyWord + "] declaration in any SPD package!");
            }
        }
    }

    /**
     * GuidGuidToAutogenc
     *
     * This function gets GUIDs from SPD file accrodeing to <Guids> information
     * and write those GUIDs to AutoGen.c.
     *
     * @param fileBuffer
     *            String Buffer for Autogen.c file.
     *
     */
    void GuidGuidToAutogenC(StringBuffer fileBuffer) throws AutoGenException {
        String[] cNameGuid = null;
        String guidKeyWord = null;

        String[] guidList = SurfaceAreaQuery.getGuidEntryArray(this.arch);

        for (int i = 0; i < guidList.length; i++) {
            this.mGuidList.add(guidList[i]);
        }


        Iterator guidIterator = this.mGuidList.iterator();
        while (guidIterator.hasNext()) {
            guidKeyWord = guidIterator.next().toString();
            cNameGuid = GlobalData.getGuid(this.mDepPkgList, guidKeyWord);

            if (cNameGuid != null) {
                fileBuffer
                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED EFI_GUID ");
                fileBuffer.append(cNameGuid[0]);
                fileBuffer.append(" =     { ");
                fileBuffer.append(CommonDefinition.formatGuidName(cNameGuid[1]));
                fileBuffer.append("} ;");
            } else {
                //
                // If can't find GUID declaration in every package
                //
                throw new AutoGenException("Can not find Guid [" + guidKeyWord
                                           + "] declaration in any SPD package. ");
            }

        }
    }

    /**
     * LibInstanceToAutogenC
     *
     * This function adds dependent library instance to autogen.c,which
     * includeing library's constructor, destructor, and library dependent ppi,
     * protocol, guid, pcd information.
     *
     * @param fileBuffer
     *            String buffer for AutoGen.c
     * @throws BuildException
     */
    void LibInstanceToAutogenC(StringBuffer fileBuffer) throws BuildException {
        try {
            String moduleType = this.moduleId.getModuleType();
            //
            // Add library constructor to AutoGen.c
            //
            LibConstructorToAutogenC(libConstructList, moduleType,
                                     fileBuffer/* autogenC */);
            //
            // Add library destructor to AutoGen.c
            //
            LibDestructorToAutogenC(libDestructList, moduleType, fileBuffer/* autogenC */);
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }

    /**
     * LibConstructorToAutogenc
     *
     * This function writes library constructor list to AutoGen.c. The library
     * constructor's parameter and return value depend on module type.
     *
     * @param libInstanceList
     *            List of library construct name.
     * @param moduleType
     *            Module type.
     * @param fileBuffer
     *            String buffer for AutoGen.c
     * @throws Exception
     */
    void LibConstructorToAutogenC(List<String> libInstanceList,
                                  String moduleType, StringBuffer fileBuffer) throws Exception {
        boolean isFirst = true;

        //
        // The library constructor's parameter and return value depend on
        // module type.
        //
        for (int i = 0; i < libInstanceList.size(); i++) {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("RETURN_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  VOID\r\n");
                fileBuffer.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer
                .append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                fileBuffer
                .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                fileBuffer.append("  );\r\n");
                break;

            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  );\r\n");
                break;
            }
        }

        //
        // Add ProcessLibraryConstructorList in AutoGen.c
        //
        fileBuffer.append("VOID\r\n");
        fileBuffer.append("EFIAPI\r\n");
        fileBuffer.append("ProcessLibraryConstructorList (\r\n");
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeBase:
            fileBuffer.append("  VOID\r\n");
            break;

        case CommonDefinition.ModuleTypePeiCore:
        case CommonDefinition.ModuleTypePeim:
            fileBuffer.append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
            fileBuffer
            .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
            break;

        case CommonDefinition.ModuleTypeDxeCore:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
            break;
        }

        fileBuffer.append("  )\r\n");
        fileBuffer.append("{\r\n");
        //
        // If no constructor function, return EFI_SUCCESS.
        //
        //if (libInstanceList.size() == 0){
        //   fileBuffer.append("  return EFI_SUCCESS;\r\n");
        //}
        for (int i = 0; i < libInstanceList.size(); i++) {
            if (isFirst) {
                fileBuffer.append("  EFI_STATUS  Status;\r\n");
                fileBuffer.append("  Status = EFI_SUCCESS;\r\n");
                fileBuffer.append("\r\n");
                isFirst = false;
            }
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append("();\r\n");
                fileBuffer.append("  VOID\r\n");
                break;
            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (FfsHeader, PeiServices);\r\n");
                break;
            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (ImageHandle, SystemTable);\r\n");
                break;
            default:
                EdkLog.log(EdkLog.EDK_INFO,"Autogen doesn't know how to deal with module type - " + moduleType + "!");
            }
            fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
        }
        fileBuffer.append("}\r\n");
    }

    /**
     * LibDestructorToAutogenc
     *
     * This function writes library destructor list to AutoGen.c. The library
     * destructor's parameter and return value depend on module type.
     *
     * @param libInstanceList
     *            List of library destructor name.
     * @param moduleType
     *            Module type.
     * @param fileBuffer
     *            String buffer for AutoGen.c
     * @throws Exception
     */
    void LibDestructorToAutogenC(List<String> libInstanceList,
                                 String moduleType, StringBuffer fileBuffer) throws Exception {
        boolean isFirst = true;
        for (int i = 0; i < libInstanceList.size(); i++) {
            switch (CommonDefinition.getModuleType(moduleType)) {
            case CommonDefinition.ModuleTypeBase:
                fileBuffer.append("RETURN_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  VOID\r\n");
                fileBuffer.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypePeiCore:
            case CommonDefinition.ModuleTypePeim:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer
                .append("  IN EFI_FFS_FILE_HEADER       *FfsHeader,\r\n");
                fileBuffer
                .append("  IN EFI_PEI_SERVICES          **PeiServices\r\n");
                fileBuffer.append("  );\r\n");
                break;
            case CommonDefinition.ModuleTypeDxeCore:
            case CommonDefinition.ModuleTypeDxeDriver:
            case CommonDefinition.ModuleTypeDxeRuntimeDriver:
            case CommonDefinition.ModuleTypeDxeSmmDriver:
            case CommonDefinition.ModuleTypeDxeSalDriver:
            case CommonDefinition.ModuleTypeUefiDriver:
            case CommonDefinition.ModuleTypeUefiApplication:
                fileBuffer.append("EFI_STATUS\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
                fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
                fileBuffer.append("  );\r\n");
                break;
            }
        }

        //
        // Write ProcessLibraryDestructor list to autogen.c
        //
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeBase:
        case CommonDefinition.ModuleTypePeiCore:
        case CommonDefinition.ModuleTypePeim:
            break;
        case CommonDefinition.ModuleTypeDxeCore:
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSmmDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            fileBuffer.append("VOID\r\n");
            fileBuffer.append("EFIAPI\r\n");
            fileBuffer.append("ProcessLibraryDestructorList (\r\n");
            fileBuffer.append("  IN EFI_HANDLE        ImageHandle,\r\n");
            fileBuffer.append("  IN EFI_SYSTEM_TABLE  *SystemTable\r\n");
            fileBuffer.append("  )\r\n");
            fileBuffer.append("{\r\n");
            //
            // If no library destructor function, return EFI_SUCCESS.
            //

            for (int i = 0; i < libInstanceList.size(); i++) {
                if (isFirst) {
                    fileBuffer.append("  EFI_STATUS  Status;\r\n");
                    fileBuffer.append("  Status = EFI_SUCCESS;\r\n");
                    fileBuffer.append("\r\n");
                    isFirst = false;
                }
                fileBuffer.append("  Status = ");
                fileBuffer.append(libInstanceList.get(i));
                fileBuffer.append("(ImageHandle, SystemTable);\r\n");
                fileBuffer.append("  ASSERT_EFI_ERROR (Status);\r\n");
            }
            fileBuffer.append("}\r\n");
            break;
        }
    }

    /**
     * ExternsDriverBindingToAutoGenC
     *
     * This function is to write DRIVER_BINDING, COMPONENT_NAME,
     * DRIVER_CONFIGURATION, DRIVER_DIAGNOSTIC in AutoGen.c.
     *
     * @param fileBuffer
     *            String buffer for AutoGen.c
     */
    void ExternsDriverBindingToAutoGenC(StringBuffer fileBuffer)
    throws BuildException {

        //
        // Check what <extern> contains. And the number of following elements
        // under <extern> should be same. 1. DRIVER_BINDING 2. COMPONENT_NAME
        // 3.DRIVER_CONFIGURATION 4. DRIVER_DIAGNOSTIC
        //

        String[] drvBindList = SurfaceAreaQuery.getDriverBindingArray();

        //
        // If component name protocol,component configuration protocol,
        // component diagnostic protocol is not null or empty, check
        // if every one have the same number of the driver binding protocol.
        //
        if (drvBindList == null || drvBindList.length == 0) {
            return;
        }

        String[] compNamList = SurfaceAreaQuery.getComponentNameArray();
        String[] compConfList = SurfaceAreaQuery.getDriverConfigArray();
        String[] compDiagList = SurfaceAreaQuery.getDriverDiagArray();

        int BitMask = 0;

        //
        // Write driver binding protocol extern to autogen.c
        //
        for (int i = 0; i < drvBindList.length; i++) {
            fileBuffer.append("extern EFI_DRIVER_BINDING_PROTOCOL ");
            fileBuffer.append(drvBindList[i]);
            fileBuffer.append(";\r\n");
        }

        //
        // Write component name protocol extern to autogen.c
        //
        if (compNamList != null && compNamList.length != 0) {
            if (drvBindList.length != compNamList.length) {
                throw new BuildException(
                                        "Different number of Driver Binding and Component Name protocols!");
            }

            BitMask |= 0x01;
            for (int i = 0; i < compNamList.length; i++) {
                fileBuffer.append("extern EFI_COMPONENT_NAME_PROTOCOL ");
                fileBuffer.append(compNamList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver configration protocol extern to autogen.c
        //
        if (compConfList != null && compConfList.length != 0) {
            if (drvBindList.length != compConfList.length) {
                throw new BuildException(
                                        "Different number of Driver Binding and Driver Configuration protocols!");
            }

            BitMask |= 0x02;
            for (int i = 0; i < compConfList.length; i++) {
                fileBuffer.append("extern EFI_DRIVER_CONFIGURATION_PROTOCOL ");
                fileBuffer.append(compConfList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver dignastic protocol extern to autogen.c
        //
        if (compDiagList != null && compDiagList.length != 0) {
            if (drvBindList.length != compDiagList.length) {
                throw new BuildException(
                                        "Different number of Driver Binding and Driver Diagnosis protocols!");
            }

            BitMask |= 0x04;
            for (int i = 0; i < compDiagList.length; i++) {
                fileBuffer.append("extern EFI_DRIVER_DIAGNOSTICS_PROTOCOL ");
                fileBuffer.append(compDiagList[i]);
                fileBuffer.append(";\r\n");
            }
        }

        //
        // Write driver module protocol bitmask.
        //
        fileBuffer
        .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINT8  _gDriverModelProtocolBitmask = ");
        fileBuffer.append(Integer.toString(BitMask));
        fileBuffer.append(";\r\n");

        //
        // Write driver module protocol list entry
        //
        fileBuffer
        .append("GLOBAL_REMOVE_IF_UNREFERENCED const UINTN  _gDriverModelProtocolListEntries = ");

        fileBuffer.append(Integer.toString(drvBindList.length));
        fileBuffer.append(";\r\n");

        //
        // Write drive module protocol list to autogen.c
        //
        fileBuffer
        .append("GLOBAL_REMOVE_IF_UNREFERENCED const EFI_DRIVER_MODEL_PROTOCOL_LIST  _gDriverModelProtocolList[] = {");
        for (int i = 0; i < drvBindList.length; i++) {
            if (i != 0) {
                fileBuffer.append(",");
            }
            fileBuffer.append("\r\n {\r\n");
            fileBuffer.append("  &");
            fileBuffer.append(drvBindList[i]);
            fileBuffer.append(", \r\n");

            if (compNamList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compNamList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            if (compConfList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compConfList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }

            if (compDiagList != null) {
                fileBuffer.append("  &");
                fileBuffer.append(compDiagList[i]);
                fileBuffer.append(", \r\n");
            } else {
                fileBuffer.append("  NULL, \r\n");
            }
            fileBuffer.append("  }");
        }
        fileBuffer.append("\r\n};\r\n");
    }

    /**
     * ExternCallBackToAutoGenC
     *
     * This function adds <SetVirtualAddressMapCallBack> and
     * <ExitBootServicesCallBack> infomation to AutoGen.c
     *
     * @param fileBuffer
     *            String buffer for AutoGen.c
     * @throws BuildException
     */
    void ExternCallBackToAutoGenC(StringBuffer fileBuffer)
    throws BuildException {
        //
        // Collect module's <SetVirtualAddressMapCallBack> and
        // <ExitBootServiceCallBack> and add to setVirtualAddList
        //  exitBootServiceList.
        //
        String[] setVirtuals = SurfaceAreaQuery.getSetVirtualAddressMapCallBackArray();
        String[] exitBoots = SurfaceAreaQuery.getExitBootServicesCallBackArray();
        if (setVirtuals != null) {
            for (int j = 0; j < setVirtuals.length; j++) {
                this.setVirtalAddList.add(setVirtuals[j]);
            }
        }
        if (exitBoots != null) {
            for (int k = 0; k < exitBoots.length; k++) {
                this.exitBootServiceList.add(exitBoots[k]);
            }
        }
        //
        //  Add c code in autogen.c which relate to <SetVirtualAddressMapCallBack>
        //  and <ExitBootServicesCallBack>
        //
        String moduleType = this.moduleId.getModuleType();
        boolean UefiOrDxeModule = false;
        int Count = 0;
        int i;
        switch (CommonDefinition.getModuleType(moduleType)) {
        case CommonDefinition.ModuleTypeDxeDriver:
        case CommonDefinition.ModuleTypeDxeRuntimeDriver:
        case CommonDefinition.ModuleTypeDxeSalDriver:
        case CommonDefinition.ModuleTypeUefiDriver:
        case CommonDefinition.ModuleTypeUefiApplication:
            //
            // Entry point lib for these module types needs to know the count
            // of entryPoint.
            //
            UefiOrDxeModule = true;
            fileBuffer
            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverSetVirtualAddressMapEventCount = ");

            //
            // If the list is not valid or has no entries set count to zero else
            // set count to the number of valid entries
            //
            Count = 0;
            if (this.setVirtalAddList != null) {
                for (i = 0; i < this.setVirtalAddList.size(); i++) {
                    if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                        break;
                    }
                }
                Count = i;
            }

            fileBuffer.append(Integer.toString(Count));
            fileBuffer.append(";\r\n\r\n");
            break;
        default:
            break;
        }

        if (this.setVirtalAddList == null || this.setVirtalAddList.size() == 0) {
            if (UefiOrDxeModule) {
                //
                // No data so make a NULL list
                //
                fileBuffer
                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {\r\n");
                fileBuffer.append("  NULL\r\n");
                fileBuffer.append("};\r\n\r\n");
            }
        } else {
            //
            // Write SetVirtualAddressMap function definition.
            //
            for (i = 0; i < this.setVirtalAddList.size(); i++) {
                if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                    break;
                }
                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(this.setVirtalAddList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_EVENT  Event,\r\n");
                fileBuffer.append("  IN VOID       *Context\r\n");
                fileBuffer.append("  );\r\n\r\n");
            }

            //
            // Write SetVirtualAddressMap entry point array.
            //
            fileBuffer
            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[] = {");
            for (i = 0; i < this.setVirtalAddList.size(); i++) {
                if (this.setVirtalAddList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }

                fileBuffer.append(this.setVirtalAddList.get(i));
            }
            //
            //  If module is not DXE_DRIVER, DXE_RUNTIME_DIRVER, UEFI_DRIVER
            //  UEFI_APPLICATION and DXE_SAL_DRIVER add the NULL at the end of
            //  _gDriverSetVirtualAddressMapEvent list.
            //
            if (!UefiOrDxeModule) {
                fileBuffer.append(",\r\n  NULL");
            }
            fileBuffer.append("\r\n};\r\n\r\n");
        }

        if (UefiOrDxeModule) {
            //
            // Entry point lib for these module types needs to know the count.
            //
            fileBuffer
            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED  const UINTN _gDriverExitBootServicesEventCount = ");

            //
            // If the list is not valid or has no entries set count to zero else
            // set count to the number of valid entries.
            //
            Count = 0;
            if (this.exitBootServiceList != null) {
                for (i = 0; i < this.exitBootServiceList.size(); i++) {
                    if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                        break;
                    }
                }
                Count = i;
            }
            fileBuffer.append(Integer.toString(Count));
            fileBuffer.append(";\r\n\r\n");
        }

        if (this.exitBootServiceList == null || this.exitBootServiceList.size() == 0) {
            if (UefiOrDxeModule) {
                //
                // No data so make a NULL list.
                //
                fileBuffer
                .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {\r\n");
                fileBuffer.append("  NULL\r\n");
                fileBuffer.append("};\r\n\r\n");
            }
        } else {
            //
            // Write DriverExitBootServices function definition.
            //
            for (i = 0; i < this.exitBootServiceList.size(); i++) {
                if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                fileBuffer.append("VOID\r\n");
                fileBuffer.append("EFIAPI\r\n");
                fileBuffer.append(this.exitBootServiceList.get(i));
                fileBuffer.append(" (\r\n");
                fileBuffer.append("  IN EFI_EVENT  Event,\r\n");
                fileBuffer.append("  IN VOID       *Context\r\n");
                fileBuffer.append("  );\r\n\r\n");
            }

            //
            // Write DriverExitBootServices entry point array.
            //
            fileBuffer
            .append("\r\nGLOBAL_REMOVE_IF_UNREFERENCED const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[] = {");
            for (i = 0; i < this.exitBootServiceList.size(); i++) {
                if (this.exitBootServiceList.get(i).equalsIgnoreCase("")) {
                    break;
                }

                if (i == 0) {
                    fileBuffer.append("\r\n  ");
                } else {
                    fileBuffer.append(",\r\n  ");
                }
                fileBuffer.append(this.exitBootServiceList.get(i));
            }
            if (!UefiOrDxeModule) {
                fileBuffer.append(",\r\n  NULL");
            }
            fileBuffer.append("\r\n};\r\n\r\n");
        }

    }

    private void copyFlashMapHToDebugDir() throws  AutoGenException{

        File inFile = new File(fvDir + File.separatorChar + CommonDefinition.flashMapH);
        int size = (int)inFile.length();
        byte[] buffer = new byte[size];
        File outFile = new File (this.outputPath + File.separatorChar + CommonDefinition.tianoR8FlashMapH);
        //
        //  If TianoR8FlashMap.h existed and the flashMap.h don't change,
        //  do nothing.
        //
        if ((!outFile.exists()) ||(inFile.lastModified() - outFile.lastModified()) >= 0) {
            try {
                if (inFile.exists()) {
                    FileInputStream fis = new FileInputStream (inFile);
                    fis.read(buffer);
                    FileOutputStream fos = new FileOutputStream(outFile);
                    fos.write(buffer);
                    fis.close();
                    fos.close();
                } else {
                    throw new AutoGenException("The file, flashMap.h doesn't exist!");
                }
            } catch (Exception e) {
                throw new AutoGenException(e.getMessage());
            }
        }
    }

    /**
    *This function first order the library instances, then collect
    *library instance 's PPI, Protocol, GUID,
    *SetVirtalAddressMapCallBack, ExitBootServiceCallBack, and
    *Destructor, Constructor.
    *
    **/
    private void collectLibInstanceInfo(){
        int index;

        String moduleType = SurfaceAreaQuery.getModuleType();
        String libConstructName = null;
        String libDestructName = null;
        String[] setVirtuals = null;
        String[] exitBoots = null;

        ModuleIdentification[] libraryIdList = SurfaceAreaQuery
                                               .getLibraryInstance(this.arch);
        try {
            if (libraryIdList != null) {
                //
                // Reorder library instance sequence.
                //
                AutogenLibOrder libOrder = new AutogenLibOrder(libraryIdList,
                                                               this.arch);
                List<ModuleIdentification> orderList = libOrder
                                                       .orderLibInstance();

                if (orderList != null) {
                    //
                    // Process library instance one by one.
                    //
                    for (int i = 0; i < orderList.size(); i++) {

                        //
                        // Get library instance basename.
                        //
                        ModuleIdentification libInstanceId = orderList.get(i);

                        //
                        // Get override map
                        //

                        Map<String, XmlObject> libDoc = GlobalData.getDoc(
                                                                         libInstanceId, this.arch);
                        SurfaceAreaQuery.push(libDoc);
                        //
                        // Get <PPis>, <Protocols>, <Guids> list of this library
                        // instance.
                        //
                        String[] ppiList = SurfaceAreaQuery.getPpiArray(this.arch);
                        String[] ppiNotifyList = SurfaceAreaQuery
                                                 .getPpiNotifyArray(this.arch);
                        String[] protocolList = SurfaceAreaQuery
                                                .getProtocolArray(this.arch);
                        String[] protocolNotifyList = SurfaceAreaQuery
                                                      .getProtocolNotifyArray(this.arch);
                        String[] guidList = SurfaceAreaQuery
                                            .getGuidEntryArray(this.arch);
                        PackageIdentification[] pkgList = SurfaceAreaQuery.getDependencePkg(this.arch);

                        //
                        // Add those ppi, protocol, guid in global ppi,
                        // protocol, guid
                        // list.
                        //
                        for (index = 0; index < ppiList.length; index++) {
                            this.mPpiList.add(ppiList[index]);
                        }

                        for (index = 0; index < ppiNotifyList.length; index++) {
                            this.mPpiList.add(ppiNotifyList[index]);
                        }

                        for (index = 0; index < protocolList.length; index++) {
                            this.mProtocolList.add(protocolList[index]);
                        }

                        for (index = 0; index < protocolNotifyList.length; index++) {
                            this.mProtocolList.add(protocolNotifyList[index]);
                        }

                        for (index = 0; index < guidList.length; index++) {
                            this.mGuidList.add(guidList[index]);
                        }
                        for (index = 0; index < pkgList.length; index++) {
                            if (!this.mDepPkgList.contains(pkgList[index])) {
                                this.mDepPkgList.add(pkgList[index]);
                            }
                        }

                        //
                        // If not yet parse this library instance's constructor
                        // element,parse it.
                        //
                        libConstructName = SurfaceAreaQuery
                                           .getLibConstructorName();
                        libDestructName = SurfaceAreaQuery
                                          .getLibDestructorName();

                        //
                        // Collect SetVirtualAddressMapCallBack and
                        // ExitBootServiceCallBack.
                        //
                        setVirtuals = SurfaceAreaQuery.getSetVirtualAddressMapCallBackArray();
                        exitBoots = SurfaceAreaQuery.getExitBootServicesCallBackArray();
                        if (setVirtuals != null) {
                            for (int j = 0; j < setVirtuals.length; j++) {
                                this.setVirtalAddList.add(setVirtuals[j]);
                            }
                        }
                        if (exitBoots != null) {
                            for (int k = 0; k < exitBoots.length; k++) {
                                this.exitBootServiceList.add(exitBoots[k]);
                            }
                        }
                        SurfaceAreaQuery.pop();
                        //
                        // Add dependent library instance constructor function.
                        //
                        if (libConstructName != null) {
                            this.libConstructList.add(libConstructName);
                        }
                        //
                        // Add dependent library instance destructor fuction.
                        //
                        if (libDestructName != null) {
                            this.libDestructList.add(libDestructName);
                        }
                    }
                }

            }

        } catch (Exception e) {
            System.out.println(e.getMessage());
            System.out.println("Collect library instance failed!");
        }
    }
}
