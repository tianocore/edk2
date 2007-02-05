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
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.security.MessageDigest;
import java.util.Vector;

import javax.xml.namespace.QName;

import org.apache.tools.ant.BuildException;
import org.apache.tools.ant.Project;
import org.apache.xmlbeans.XmlCursor;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.build.global.GlobalData;
import org.tianocore.build.global.SurfaceAreaQuery;
import org.tianocore.build.id.FpdModuleIdentification;
import org.tianocore.common.definitions.EdkDefinitions;
import org.tianocore.common.logger.EdkLog;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

/** 
  <p><code>FfsProcess</code> is a class to find the corresponding FFS layout. </p>
  
  <p>The FFS Layout is like following: </p>
  
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

    private BuildOptionsDocument.BuildOptions.Ffs ffsXmlObject;

    private Project project = null;
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
    public static final String[][] sectionExt = EdkDefinitions.SectionTypeExtensions;

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
      Find the corresponding FFS layout in <code>FPD</code>. 
      
      @param buildType Current module's component type
      @param project Ant project
      @return whether find the corresponding FFS layout
      @throws BuildException
              If can't find FFS Layout in FPD.
    **/
    public boolean initSections(String buildType, Project project, FpdModuleIdentification fpdModuleId) throws BuildException {
        this.project = project;
        //
        // Try to find Ffs layout from FPD file
        //
        SurfaceAreaQuery saq = new SurfaceAreaQuery(GlobalData.getFpdBuildOptionsMap());
        BuildOptionsDocument.BuildOptions.Ffs[] ffsArray = saq.getFpdFfs();
        for (int i = 0; i < ffsArray.length; i++) {
            if (isMatch(ffsArray[i].getFfsKey(), buildType)) {
                ffsXmlObject = ffsArray[i];
                genDigest();
                return true;
            }
        }
        
        //
        // If FfsFormatKey is not null, report exception and fail build
        // Otherwise report warning message
        //
        if (buildType == null) {
            EdkLog.log(EdkLog.EDK_WARNING, "Warning: this module doesn't specify a FfsFormatKey. ");
        } else {
            throw new BuildException("Can't find the FfsFormatKey [" + buildType + "] attribute in the FPD file!");            
        }

        return false;
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
        if (ffsXmlObject == null) {
            return new String[0];
        }
        Vector<String> sectionList = new Vector<String>();
        XmlCursor cursor = null;

        cursor = ffsXmlObject.newCursor();

        int mode = MODE_NONE;
        Element genffsfileEle = document.createElement("genffsfile");
        genffsfileEle.setAttribute("outputDir", "${BIN_DIR}");
        genffsfileEle.setAttribute("moduleType", "${MODULE_TYPE}");
        genffsfileEle.setAttribute("BaseName", basename);
        genffsfileEle.setAttribute("fileGuid", guid);

        if (cursor.toFirstChild()) {
            do {
                if (cursor.getName().getLocalPart().equalsIgnoreCase("Attribute")) {
                    String name = cursor.getAttributeText(new QName("Name"));
                    String value = cursor.getAttributeText(new QName("Value"));
                    genffsfileEle.setAttribute(changeAttributeName(name), value);
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Section")) {
                    cursor.push();
                    dealSection(mode, document, genffsfileEle, cursor, sectionList);
                    cursor.pop();
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Sections")) {
                    cursor.push();
                    dealSections(mode, document, genffsfileEle, cursor, sectionList);
                    cursor.pop();
                }
            } while (cursor.toNextSibling());
        }
        //
        // Check dependency 
        //
        Element outofdateEle = document.createElement("OnDependency");
        Element sourceEle = document.createElement("sourcefiles");
        Vector<String> sections = new Vector<String>();
        for (int i = 0; i < sectionList.size(); i++) {
            String section = (String) sectionList.get(i);
            if (isSectionType(section)) {
                sections.addElement(section);
            }
            Element pathEle = document.createElement("file");
            pathEle.setAttribute("name", getSectionFile(basename, section));
            sourceEle.appendChild(pathEle);
        }
        //
        // add FFS layout digest into the source file list
        // 
        Element pathEle = document.createElement("file");
        pathEle.setAttribute("name", "${DEST_DIR_OUTPUT}" + File.separator + "ffs.md5");
        sourceEle.appendChild(pathEle);

        String[] result = sections.toArray(new String[sections.size()]);

        outofdateEle.appendChild(sourceEle);
        Element targetEle = document.createElement("targetfiles");
        Element fileEle = document.createElement("file");
        fileEle.setAttribute("name", "${BIN_DIR}" + File.separatorChar + targetFilename);
        targetEle.appendChild(fileEle);
        outofdateEle.appendChild(targetEle);
        Element sequentialEle = document.createElement("sequential");
        sequentialEle.appendChild(genffsfileEle);
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
        String toolName = cursor.getAttributeText(new QName("ToolName"));
        String sectType = cursor.getAttributeText(new QName("SectionType"));
        if (type == null && sectType == null) {
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
        Element toolEle = null;
        if (type.equalsIgnoreCase("COMPRESS") && (toolName == null || toolName.equalsIgnoreCase(""))) {
            mode = MODE_COMPRESS;
            //
            // <gensection sectiontype="EFI_SECTION_COMPRESSION">   
            // 
            ele = doc.createElement("gensection");
            ele.setAttribute("sectionType", "EFI_SECTION_COMPRESSION");
            
        } else {
            mode = MODE_GUID_DEFINED;
            //
            // <gensection sectiontype="EFI_SECTION_GUID_DEFINED">
            // 
            ele = doc.createElement("gensection");
            if (type != null) {
                if (type.equalsIgnoreCase("COMPRESS")) {
                    ele.setAttribute("sectionType", "EFI_SECTION_COMPRESSION");
                }else {
                    ele.setAttribute("sectiontype", "EFI_SECTION_GUID_DEFINED");    
                }
                
            } else {
                ele.setAttribute("sectiontype", sectType);
            }
            //
            // <tool toolName="${OEMTOOLPATH}\toolname"
            // outputPath = "${DEST_DIR_OUTPUT}">
            //
            toolEle = doc.createElement("tool");
            if (toolName == null || toolName.equalsIgnoreCase("")) {
                toolEle.setAttribute("toolName", "${WORKSPACE_DIR}" + File.separatorChar + "Tools" + File.separatorChar + "bin"
                                         + File.separatorChar + "GenCRC32Section");
            }else{
                File toolExe = new File(toolName);
                //
                //  If <Tool> element exist, add sub element under <tool> . 
                // 
                if (toolExe.isAbsolute()) {
                    toolEle.setAttribute("toolName", toolName);
                } else {
                    toolEle.setAttribute("toolName", "${WORKSPACE_DIR}" + File.separatorChar + "Tools" + File.separatorChar + "bin"
                                         + File.separatorChar + toolName);
                }
            }
            
            toolEle.setAttribute("outputPath", "${DEST_DIR_OUTPUT}");
            ele.appendChild(toolEle);
        }
        if (cursor.toFirstChild()) {
            do {
                if (cursor.getName().getLocalPart().equalsIgnoreCase("Section")) {
                    cursor.push();
                    if (toolEle == null) {
                        dealSection(mode, doc, ele, cursor, list);
                    } else {
                        dealSection(mode, doc, toolEle, cursor, list);
                    }
                    
                    cursor.pop();
                } else if (cursor.getName().getLocalPart().equalsIgnoreCase("Sections")) {
                    cursor.push();
                    if (toolEle == null) {
                        dealSections(mode, doc, ele, cursor, list);
                    } else {
                        dealSections(mode, doc, toolEle, cursor, list);
                    }
                    
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
        String alignment = cursor.getAttributeText(new QName("Alignment"));
        
        //
        // Judge if file is specified? Yes, just use the file, else call Build Macro
        // If fileName is null, means without FileNames specify in FPD file
        //
        String fileName = null;
        cursor.push();
        if (cursor.toFirstChild()) {
            do {
                if (cursor.getName().getLocalPart().equalsIgnoreCase("Filenames")) {
                    cursor.push();
                    if (cursor.toFirstChild()) {
                        do {
                            if (cursor.getName().getLocalPart().equalsIgnoreCase("Filename")) {
                                fileName = cursor.getTextValue();
                            }
                        } while (cursor.toNextSibling());
                    }
                    cursor.pop();
                }
            } while (cursor.toNextSibling());
        }

        cursor.pop();
        
        if (fileName == null) {
            list.addElement(type);
        } else {
            list.addElement(fileName);
        }

        if (mode == MODE_GUID_DEFINED) {
            //
            // <input file="${DEST_DIR_OUTPUT}\Bds.pe32"/>
            //
            Element ele = doc.createElement("input");
            if (fileName == null) {
                ele.setAttribute("file", getSectionFile(basename, type));
            } else {
                ele.setAttribute("file", fileName);
            }
            root.appendChild(ele);
        } else {
            //
            // <sectFile fileName= "..."/>
            //
            Element ele = doc.createElement("sectFile");
            if (fileName == null) {
                ele.setAttribute("fileName", getSectionFile(basename, type));
            } else {
                ele.setAttribute("fileName", fileName);
            }
            if (alignment != null) {
                ele.setAttribute("Alignment", alignment);
            }
            root.appendChild(ele);
        }
    }

    /**
      Get the corresponding section file suffix.
       
      @param type Section type
      @return Corresponding section file extension
    **/
    private String getSectionFile(String basename, String type) {
        for (int i = 0; i < sectionExt.length; i++) {
            if (sectionExt[i][0].equalsIgnoreCase(type)) {
                return "${DEST_DIR_OUTPUT}" + File.separatorChar + basename + sectionExt[i][1];
            }
        }
        return type;
    }

    private boolean isSectionType(String type) {
        for (int i = 0; i < sectionExt.length; i++) {
            if (sectionExt[i][0].equalsIgnoreCase(type)) {
                return true;
            }
        }
        return false;
    }

    /**
      Return the ANT script to call GenFfs Tool.
      
      @return ANT script to call GenFfs Tool
    **/
    public Element getFfsNode() {
        return ffsNode;
    }

    private void genDigest() {
        String digestFilePath = project.getProperty("DEST_DIR_OUTPUT");
        if (digestFilePath == null) {
            EdkLog.log(EdkLog.EDK_WARNING, "Warning: cannot get DEST_DIR_OUTPUT!");
            return;
        }

        //
        // use MD5 algorithm
        // 
        MessageDigest md5 = null;
        try {
            md5 = MessageDigest.getInstance("MD5");
            //
            // convert the FFS layout XML DOM tree into string and use it to
            // calculate its MD5 digest value
            // 
            md5.update(ffsXmlObject.xmlText().getBytes());
        } catch (Exception e) {
            EdkLog.log(EdkLog.EDK_WARNING, "Warning: " + e.getMessage());
            return;
        }

        //
        // get the MD5 digest value
        // 
        byte[] digest = md5.digest();

        //
        // put the digest in a file named "ffs.md5" if it doesn't exist, otherwise
        // we will compare the digest in the file with the one just calculated
        // 
        digestFilePath += File.separator + "ffs.md5";
        File digestFile = new File(digestFilePath);
        if (digestFile.exists()) {
            byte[] oldDigest = new byte[digest.length];
            try {
                FileInputStream fIn = new FileInputStream(digestFile);
                fIn.read(oldDigest);
                fIn.close();
            } catch (Exception e) {
                throw new BuildException(e.getMessage());
            }

            boolean noChange = true;
            for (int i = 0; i < oldDigest.length; ++i) {
                if (digest[i] != oldDigest[i]) {
                    noChange = false;
                    break;
                }
            }

            if (noChange) {
                return;
            }
        }

        //
        // update the "ffs.md5" file content with new digest value
        // 
        try {
            FileOutputStream fOut = new FileOutputStream(digestFile);
            fOut.write(digest);
            fOut.close();
        } catch (Exception e) {
            throw new BuildException(e.getMessage());
        }
    }
}
