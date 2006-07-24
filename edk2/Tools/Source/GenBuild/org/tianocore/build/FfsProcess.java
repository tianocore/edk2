/** @file
  File is FfsProcess class which is used to get the corresponding FFS layout
  information for driver module. 
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Vector;

import javax.xml.namespace.QName;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/** 
  <p><code>FfsProcess</code> is a class to find the corresponding FFS layout. </p>
  
  <p>Property <code>COMMON_FILE</code> specified which file to search. The element
  in <code>COMMON_FILE</code> is like following: </p>
  
  <pre>
    &lt;Ffs type="APPLICATION"&gt;
      &lt;Attribute Name="FFS_FILETYPE" Value="EFI_FV_FILETYPE_APPLICATION" /&gt;
      &lt;Attribute Name="FFS_ATTRIB_CHECKSUM" Value="TRUE" /&gt;
      &lt;Sections EncapsulationType="Compress"&gt;
        &lt;Sections EncapsulationType="Guid-Defined"&gt;
          &lt;Section SectionType="EFI_SECTION_PE32" /&gt; 
          &lt;Section SectionType="EFI_SECTION_USER_INTERFACE" /&gt;
          &lt;Section SectionType="EFI_SECTION_VERSION" /&gt; 
        &lt;/Sections&gt;
      &lt;/Sections&gt;
    &lt;/Ffs&gt;
  </pre>
 
  @since GenBuild 1.0
**/
public class FfsProcess {

    ///
    /// Xml Document Node for corresponding FFS layout
    ///
    private Node ffs;
    
    private BuildOptionsDocument.BuildOptions.Ffs ffsXmlObject;

    ///
    /// ANT script to call GenFfs
    ///
    private Element ffsNode = null;

    ///
    /// Module base name
    ///
    private String basename;

    ///
    /// Sections type: normal
    ///
    private static int MODE_NONE = 0;

    ///
    /// Sections type: compress
    ///
    private static int MODE_COMPRESS = 1;

    ///
    /// Sections type: guid-define
    ///
    private static int MODE_GUID_DEFINED = 2;

    ///
    /// mapping from section type to section output file extension
    ///
    public static final String[][] sectionExt = { { "EFI_SECTION_FREEFORM_SUBTYPE_GUID", ".sec" },
                                                 { "EFI_SECTION_VERSION", ".ver" },
                                                 { "EFI_SECTION_USER_INTERFACE", ".ui" },
                                                 { "EFI_SECTION_DXE_DEPEX", ".dpx" },
                                                 { "EFI_SECTION_PEI_DEPEX", ".dpx" }, 
                                                 { "EFI_SECTION_PE32", ".pe32" },
                                                 { "EFI_SECTION_PIC", ".pic" }, 
                                                 { "EFI_SECTION_TE", ".tes" },
                                                 { "EFI_SECTION_RAW", ".sec" }, 
                                                 { "EFI_SECTION_COMPRESSION", ".sec" },
                                                 { "EFI_SECTION_GUID_DEFINED", ".sec" },
                                                 { "EFI_SECTION_COMPATIBILITY16", ".sec" },
                                                 { "EFI_SECTION_FIRMWARE_VOLUME_IMAGE", ".sec" } };

    /**
      search in the type, if componentType is listed in type, return true; 
      otherwise return false.
      
      @param type a list supported component type separated by comma
      @param componentType current module component type
      @return whether componentType is one of type 
    **/
    private boolean isMatch(String type, String componentType) {
        String[] items = type.split("[ \t]*,[ \t]*");
        for (int i = 0; i < items.length; i++) {
            if (items[i].equalsIgnoreCase(componentType)) {
                return true;
            }
        }
        return false;
    }

    /**
      Find the corresponding FFS layout in <code>COMMON_FILE</code> if it
      does not specify in module's surface area. 
      
      @param buildType Current module's component type
      @param project Ant project
      @return whether find the corresponding FFS layout
      @throws BuildException
              If specified COMMON_FILE XML file is not valide.
    **/
    public boolean initSections(String buildType, Project project, FpdModuleIdentification fpdModuleId) throws BuildException {
        //
        // Firstly, try to find in ModuleSA
        //
//        BuildOptionsDocument.BuildOptions.Ffs[] ffsArray = SurfaceAreaQuery.getModuleFfs();
//        for (int i = 0; i < ffsArray.length; i++) {
//            if (isMatch(ffsArray[i].getFfsKey(), buildType)) {
//                ffsXmlObject = ffsArray[i];
//                return true;
//            }
//        }
        
        //
        // secondly, try to sections defined in PLATFORM level
        //
        SurfaceAreaQuery.push(GlobalData.getFpdBuildOptions());
        BuildOptionsDocument.BuildOptions.Ffs[] ffsArray = SurfaceAreaQuery.getFpdFfs();
        SurfaceAreaQuery.pop();
        for (int i = 0; i < ffsArray.length; i++) {
            if (isMatch(ffsArray[i].getFfsKey(), buildType)) {
                ffsXmlObject = ffsArray[i];
                return true;
            }
        }
        
        //
        // If FfsFormatKey is not null, report exception and fail build
        // Otherwise report warning message
        //
        if (buildType == null) {
            System.out.println("Warning: this module doesn't specify a FfsFormatKey. ");
            }
        else {
            throw new BuildException("Can't find FfsFormatKey [" + buildType + "] in FPD file. ");            
        }

        if (ffs == null) {
            return false;
        } else {
            return true;
        }
    }
    
    /**
      Recursive parse the FFS layout. Find out all section type here used. 
      
      @param document BaseName_build.xml Xml document
      @param basename Module's base name
      @param guid Module's GUID
      @param targetFilename Module's final file name (GUID-BaseName.APP)
      @return List of section type
    **/
    public String[] getGenSectionElements(Document document, String basename, String guid, String targetFilename) {
        this.basename = basename;
        if (ffs == null && ffsXmlObject == null) {
            return new String[0];
        }
        Vector<String> sectionList = new Vector<String>();
        XmlCursor cursor = null;
        try {
            if (ffsXmlObject == null) {
                cursor = XmlObject.Factory.parse(ffs).newCursor();
            }
            else {
                cursor = ffsXmlObject.newCursor();
            }
        } catch (Exception e) {
            return null;
        }
        int mode = MODE_NONE;
        Element root = document.createElement("genffsfile");
        root.setAttribute("outputDir", "${BIN_DIR}");
        root.setAttribute("moduleType", "${MODULE_TYPE}");
        root.setAttribute("BaseName", basename);
        root.setAttribute("fileGuid", guid);
        if (ffsXmlObject == null) {
            cursor.toFirstChild();
        }
        if (cursor.toFirstChild()) {
            do {
                if (cursor.getName().getLocalPart().equalsIgnoreCase("Attribute")) {
                    String name = cursor.getAttributeText(new QName("Name"));
                    String value = cursor.getAttributeText(new QName("Value"));
                    root.setAttribute(changeAttributeName(name), value);
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Section")) {
                    cursor.push();
                    dealSection(mode, document, root, cursor, sectionList);
                    cursor.pop();
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Sections")) {
                    cursor.push();
                    dealSections(mode, document, root, cursor, sectionList);
                    cursor.pop();
                }
            } while (cursor.toNextSibling());
        }
        //
        // Check dependency 
        //
        Element outofdateEle = document.createElement("OnDependency");
        Element sourceEle = document.createElement("sourcefiles");
        String[] result = new String[sectionList.size()];
        for (int i = 0; i < sectionList.size(); i++) {
            result[i] = (String) sectionList.get(i);
            Element pathEle = document.createElement("file");
            pathEle.setAttribute("name", "${DEST_DIR_OUTPUT}" + File.separatorChar + basename
                                         + getSectionExt(result[i]));
            sourceEle.appendChild(pathEle);
        }
        outofdateEle.appendChild(sourceEle);
        Element targetEle = document.createElement("targetfiles");
        Element fileEle = document.createElement("file");
        fileEle.setAttribute("name", "${BIN_DIR}" + File.separatorChar + targetFilename);
        targetEle.appendChild(fileEle);
        outofdateEle.appendChild(targetEle);
        Element sequentialEle = document.createElement("sequential");
        sequentialEle.appendChild(root);
        outofdateEle.appendChild(sequentialEle);
        ffsNode = outofdateEle;
        return result;
    }

    /**
      Change the attribute name. For example: 
      
      <pre>
          Before change: FFS_ATTRIB_CHECKSUM 
          After  change: ffsATTRIBCHECKSUM
      </pre>
      
      @param name Original attribute name
      @return Changed attribute name
    **/
    private String changeAttributeName(String name) {
        String[] strs = name.split("_");
        String str = strs[0].toLowerCase();
        for (int j = 1; j < strs.length; j++) {
            str += strs[j];
        }
        return str;
    }

    /**
      Recursively deal with Sections. If sections does not specify a type, then omit it.
      
      @param mode Current node mode (MODE_NONE | MODE_COMPREE | MODE_GUID_DEFINED)
      @param doc Xml Document
      @param root Root Node
      @param cursor Current FFS layout cursor
      @param list List of section type here used
    **/
    private void dealSections(int mode, Document doc, Element root, XmlCursor cursor, Vector<String> list) {
        String type = cursor.getAttributeText(new QName("EncapsulationType"));
        if (type == null) {
            if (cursor.toFirstChild()) {
                do {
                    if (cursor.getName().getLocalPart().equalsIgnoreCase("Section")) {
                        cursor.push();
                        dealSection(mode, doc, root, cursor, list);
                        cursor.pop();
                    } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Sections")) {
                        cursor.push();
                        dealSections(mode, doc, root, cursor, list);
                        cursor.pop();
                    }
                } while (cursor.toNextSibling());
            }
            return;
        }
        Element ele;
        if (type.equalsIgnoreCase("COMPRESS")) {
            mode = MODE_COMPRESS;
            //
            // <compress compressName = "dummy">
            //
            ele = doc.createElement("compress");
            ele.setAttribute("compressName", "dummy");
        } else {
            mode = MODE_GUID_DEFINED;
            //
            // <tool toolName="${OEMTOOLPATH}\toolname"
            // outputPath = "${DEST_DIR_OUTPUT}">
            //
            ele = doc.createElement("tool");
            ele.setAttribute("toolName", "${WORKSPACE_DIR}" + File.separatorChar + "Tools" + File.separatorChar + "bin"
                                         + File.separatorChar + "GenCRC32Section");
            ele.setAttribute("outputPath", "${DEST_DIR_OUTPUT}");
        }
        if (cursor.toFirstChild()) {
            do {
                if (cursor.getName().getLocalPart().equalsIgnoreCase("Section")) {
                    cursor.push();
                    dealSection(mode, doc, ele, cursor, list);
                    cursor.pop();
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Sections")) {
                    cursor.push();
                    dealSections(mode, doc, ele, cursor, list);
                    cursor.pop();
                }
            } while (cursor.toNextSibling());
        }
        root.appendChild(ele);
    }
    
    /**
      Recursively deal with section.
      
      @param mode Current node mode (MODE_NONE | MODE_COMPREE | MODE_GUID_DEFINED)
      @param doc Xml Document
      @param root Root Node
      @param cursor Current FFS layout cursor
      @param list List of section type here used
    **/
    private void dealSection(int mode, Document doc, Element root, XmlCursor cursor, Vector<String> list) {
        String type = cursor.getAttributeText(new QName("SectionType"));
        list.addElement(type);
        if (mode == MODE_GUID_DEFINED) {
            //
            // <input file="${DEST_DIR_OUTPUT}\Bds.pe32"/>
            //
            Element ele = doc.createElement("input");
            ele.setAttribute("file", "${DEST_DIR_OUTPUT}" + File.separatorChar + basename + getSectionExt(type));
            root.appendChild(ele);
        } else {
            //
            // <sectFile fileName= "..."/>
            //
            Element ele = doc.createElement("sectFile");
            ele.setAttribute("fileName", "${DEST_DIR_OUTPUT}" + File.separatorChar + basename + getSectionExt(type));
            root.appendChild(ele);
        }
    }

    /**
      Get the corresponding section file suffix.
       
      @param type Section type
      @return Corresponding section file extension
    **/
    private String getSectionExt(String type) {
        for (int i = 0; i < sectionExt.length; i++) {
            if (sectionExt[i][0].equalsIgnoreCase(type)) {
                return sectionExt[i][1];
            }
        }
        return ".sec";
    }

    /**
      Return the ANT script to call GenFfs Tool.
      
      @return ANT script to call GenFfs Tool
    **/
    public Element getFfsNode() {
        return ffsNode;
    }
}
