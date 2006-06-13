/** @file
  OverrideProcess class.
  
  OverrideProcess class is used to override surface area information. 

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
package org.tianocore.build.global;

import java.util.HashMap;
import java.util.Map;

import javax.xml.namespace.QName;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlCursor;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.BootModesDocument;
import org.tianocore.BuildOptionsDocument;
import org.tianocore.DataHubsDocument;
import org.tianocore.EventsDocument;
import org.tianocore.ExternsDocument;
import org.tianocore.FormsetsDocument;
import org.tianocore.GuidsDocument;
import org.tianocore.HobsDocument;
import org.tianocore.IncludesDocument;
import org.tianocore.LibrariesDocument;
import org.tianocore.LibraryClassDefinitionsDocument;
import org.tianocore.MsaHeaderDocument;
import org.tianocore.MsaLibHeaderDocument;
import org.tianocore.PcdCodedDocument;
import org.tianocore.PPIsDocument;
import org.tianocore.ProtocolsDocument;
import org.tianocore.SourceFilesDocument;
import org.tianocore.SystemTablesDocument;
import org.tianocore.VariablesDocument;

/**
  This class is used to override surface area information. For example, MBD can
  overried MSA, Platform can override all information of the module. 
  
  <p>Override will take effect if two element satisfy one of following two condition: </p>
  <ul>
    <li>Element name and its attribute OverrideID equal each other. </li>
    <li>Element is defined as exclusive which mean such element can be
    only appeared in the surface area. </li>
  </ul>
  
  <p>For example, here OutputDirectory element is exclusive: </p>
  
  <pre>
  Low priority Xml Document fragment:
     &lt;Libraries&gt;
       &lt;Arch ArchType="IA32"&gt;
         &lt;Library OverrideID="8888"&gt;EdkPeCoffLoaderLib&lt;/Library&gt;
         &lt;Library OverrideID="8888"&gt;BasePeCoffLib&lt;/Library&gt;
       &lt;/Arch&gt;
     &lt;/Libraries&gt; 
     &lt;BuildOptions&gt;
       &lt;OutputDirectory IntermediateDirectories="MODULE"/&gt;
       &lt;Option&gt;CC_FLAGS = "/NOLOGO", "/C"&lt;/Option&gt;
     &lt;BuildOptions&gt;
 
  High priority Xml Document fragment:
     &lt;Libraries&gt;
       &lt;Arch ArchType="IA32"&gt;
         &lt;Library OverrideID="8888">Nt32PeCoffLoaderLib&lt;/Library&gt;
       &lt;/Arch&gt;
     &lt;/Libraries&gt;
     &lt;BuildOptions&gt;
       &lt;OutputDirectory IntermediateDirectories="UNIFIED"/&gt;
       &lt;Option&gt;LIB_FLAGS = "/NOLOGO"&lt;/Option&gt;
     &lt;BuildOptions&gt;
     
   The result is: 
     &lt;Libraries&gt;
       &lt;Arch ArchType="IA32"&gt;
         &lt;Library OverrideID="8888"&gt;Nt32PeCoffLoaderLib&lt;/Library&gt;
       &lt;/Arch&gt;
     &lt;/Libraries&gt;
     &lt;BuildOptions&gt;
       &lt;OutputDirectory IntermediateDirectories="UNIFIED"/&gt;
       &lt;Option&gt;CC_FLAGS = "/NOLOGO", "/C"&lt;/Option&gt;
       &lt;Option&gt;LIB_FLAGS = "/NOLOGO"&lt;/Option&gt;
     &lt;BuildOptions&gt;
   
  </pre>
  
  <p>Note that using XmlBeans to walk through the whole XML document tree.</p> 
  
  @since GenBuild 1.0
  @see org.apache.xmlbeans.XmlBeans
**/
public class OverrideProcess {

    ///
    /// URI, the namespace of current XML schema
    ///
    public static String prefix = "http://www.TianoCore.org/2006/Edk2.0";

    ///
    /// list of top elements of surface area
    ///
    public static String[] topElements = { "LibraryClassDefinitions",
                    "SourceFiles", "Includes", "Libraries", "Protocols",
                    "Events", "Hobs", "PPIs", "Variables", "BootModes",
                    "SystemTables", "DataHubs", "Formsets", "Guids", "Externs",
                    "PcdCoded", "BuildOptions" };

    ///
    /// list of exclusive elements
    ///
    public static String[] exclusiveElements = {"OutputDirectory"};
    
    /**
      Recursively find out all elements specified with OverrideId attribute
      and exclusive elements in current XML object. 
      
      @param o curent parsing XML object
      @param map Map to list elements specified OverrideID attribute
      @param execlusiveMap Map to list exclusive elements appeared in current XMl object
      @param level the depth in XML document tree
    **/
    private void listOverrideID(XmlObject o, Map<String,Object> map, Map<String,Object> execlusiveMap, int level) {
        XmlCursor cursor = o.newCursor();
        String name = cursor.getName().getLocalPart();
        for (int i = 0 ; i < exclusiveElements.length; i++){
            if (name.equalsIgnoreCase(exclusiveElements[i])){
                execlusiveMap.put(exclusiveElements[i], cursor.getObject());
            }
        }
        String overrideID = cursor.getAttributeText(new QName("OverrideID"));
        if (overrideID != null) {
            map.put(name + ":" + overrideID, cursor.getObject());
        }
        if (cursor.toFirstChild()) {
            do {
                listOverrideID(cursor.getObject(), map, execlusiveMap, level + 1);
            } while (cursor.toNextSibling());
        }
    }

    /**
      This function is used to prepare for overriding with changing data. 
      
      @param map original surface area information 
      @return after normalize surface area information
    **/
    public synchronized static Map<String, XmlObject> deal(Map<String, XmlObject> map) {
        Map<String, XmlObject> newMap = new HashMap<String, XmlObject>();
        if (map.get("MsaHeader") != null) {
            newMap.put("MsaHeader", ((MsaHeaderDocument) map.get("MsaHeader"))
                            .getMsaHeader());
        }
        if (map.get("MsaLibHeader") != null) {
            newMap.put("MsaLibHeader", ((MsaLibHeaderDocument) map
                            .get("MsaLibHeader")).getMsaLibHeader());
        }
        if (map.get("LibraryClassDefinitions") != null) {
            newMap.put("LibraryClassDefinitions",
                            ((LibraryClassDefinitionsDocument) map
                                            .get("LibraryClassDefinitions"))
                                            .getLibraryClassDefinitions());
        }
        if (map.get("SourceFiles") != null) {
            newMap.put("SourceFiles", ((SourceFilesDocument) map
                            .get("SourceFiles")).getSourceFiles());
        }
        if (map.get("Includes") != null) {
            newMap.put("Includes", ((IncludesDocument) map.get("Includes"))
                            .getIncludes());
        }
        if (map.get("Libraries") != null) {
            newMap.put("Libraries", ((LibrariesDocument) map.get("Libraries"))
                            .getLibraries());
        }
        if (map.get("Protocols") != null) {
            newMap.put("Protocols", ((ProtocolsDocument) map.get("Protocols"))
                            .getProtocols());
        }
        if (map.get("Events") != null) {
            newMap.put("Events", ((EventsDocument) map.get("Events"))
                            .getEvents());
        }
        if (map.get("Hobs") != null) {
            newMap.put("Hobs", ((HobsDocument) map.get("Hobs")).getHobs());
        }
        if (map.get("PPIs") != null) {
            newMap.put("PPIs", ((PPIsDocument) map.get("PPIs")).getPPIs());
        }
        if (map.get("Variables") != null) {
            newMap.put("Variables", ((VariablesDocument) map.get("Variables"))
                            .getVariables());
        }
        if (map.get("BootModes") != null) {
            newMap.put("BootModes", ((BootModesDocument) map.get("BootModes"))
                            .getBootModes());
        }
        if (map.get("SystemTables") != null) {
            newMap.put("SystemTables", ((SystemTablesDocument) map
                            .get("SystemTables")).getSystemTables());
        }
        if (map.get("DataHubs") != null) {
            newMap.put("DataHubs", ((DataHubsDocument) map.get("DataHubs"))
                            .getDataHubs());
        }
        if (map.get("Formsets") != null) {
            newMap.put("Formsets", ((FormsetsDocument) map.get("Formsets"))
                            .getFormsets());
        }
        if (map.get("Guids") != null) {
            newMap.put("Guids", ((GuidsDocument) map.get("Guids")).getGuids());
        }
        if (map.get("Externs") != null) {
            newMap.put("Externs", ((ExternsDocument) map.get("Externs"))
                            .getExterns());
        }
        if (map.get("PcdCoded") != null) {
            newMap.put("PcdCoded", ((PcdCodedDocument) map.get("PcdCoded")).getPcdCoded());
        }
        if (map.get("BuildOptions") != null) {
            newMap.put("BuildOptions", ((BuildOptionsDocument) map
                            .get("BuildOptions")).getBuildOptions());
        }
        return newMap;
    }

    /**
      Recursively remove all subelement in Xml Object l (with low priority) 
      based on OverrideID or exclusive elements. 
    
      @param l the XML object to process
      @param map list of elements with OverrideID in high priority XML object
      @param execusiveMap  list of exclusive elements in high priority XML object
    **/
    private void cut(XmlCursor l, Map map, Map execusiveMap) {
        String name = l.getName().getLocalPart();
        if (execusiveMap.containsKey(name)){
            l.removeXml();
            return;
        }
        String overrideID = l.getAttributeText(new QName("OverrideID"));
        if (overrideID != null) {
            if (map.containsKey(name + ":" + overrideID)) {
                l.removeXml();
                return;
            }
        }
        if (l.toFirstChild()) {
            do {
                cut(l, map, execusiveMap);
            } while (l.toNextSibling());
        }
    }

    private XmlObject cloneXmlObject(XmlObject object, boolean deep) throws BuildException {
        XmlObject result = null;
        try {
            result = XmlObject.Factory.parse(object.getDomNode()
                            .cloneNode(deep));
        } catch (Exception ex) {
            throw new BuildException(ex.getMessage());
        }
        return result;
    }

    /**
      Process every item list in h and l.
    
      @param h surface area info with high priority
      @param l surface area info with low priority
      @return surface area after override
    **/
    public Map<String, XmlObject> override(Map<String, XmlObject> h,
                    Map<String, XmlObject> l) {
        Map<String, XmlObject> result = new HashMap<String, XmlObject>();
        result.put("MsaHeader", override(l.get("MsaHeader"), null));
        result.put("MsaLibHeader", override(l.get("MsaLibHeader"), null));
        for (int i = 0; i < topElements.length; i++) {
            result.put(topElements[i], override(h.get(topElements[i]), l
                            .get(topElements[i])));
        }
        return result;
    }

    /**
      Recursively override two Xml Objects.
      
      @param h Xml Object info with high priority
      @param l Xml Object info with low priority
      @return Xml Object after area
    **/
    public XmlObject override(XmlObject h, XmlObject l) {
        if (l == null && h == null) {
            return null;
        }
        if (h == null) {
            return cloneXmlObject(l, true);
        }
        if (l == null) {
            return cloneXmlObject(h, true);
        }
        XmlCursor hc = h.newCursor();
        if (h.getClass() != l.getClass()) {
            System.out
                            .println("Error: Two XmlObject does not with compliant format.");
            return null;
        }
        if (!hc.toFirstChild()) {
            return cloneXmlObject(l, true);
        }

        XmlCursor result = cloneXmlObject(h, true).newCursor();
        XmlCursor lcursor = cloneXmlObject(l, true).newCursor();
        result.push();
        result.toNextToken();
        result.insertNamespace("", prefix);
        result.toFirstChild();
        //
        // found out all element specified a OverrideID
        //
        Map<String,Object> hmap = new HashMap<String,Object>();
        Map<String,Object> execlusiveMap = new HashMap<String,Object>();
        listOverrideID(h, hmap, execlusiveMap, 0);
        lcursor.toNextToken();
        lcursor.push();
        //
        // for every direct subelement of l, cut all element satisfied with
        // override rule
        //
        if (lcursor.toFirstChild()) {
            do {
                cut(lcursor, hmap, execlusiveMap);
            } while (lcursor.toNextSibling());
        }
        lcursor.pop();
        if (lcursor.toFirstChild()) {
            do {
                lcursor.copyXml(result);
                result.insertChars("\n");
            } while (lcursor.toNextSibling());
        }
        result.pop();
        return result.getObject();
    }
}