/** @file
  SurfaceAreaParser class.
  
  SurfaceAreaParser class is used to parse module surface area include both 
  driver and library. 

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.build.global;

import java.io.File;
import java.util.HashMap;
import java.util.Map;

import org.apache.tools.ant.BuildException;
import org.apache.xmlbeans.XmlObject;
import org.tianocore.LibraryModuleBuildDescriptionDocument;
import org.tianocore.LibraryModuleSurfaceAreaDocument;
import org.tianocore.ModuleBuildDescriptionDocument;
import org.tianocore.ModuleSurfaceAreaDocument;

/**
  This class is used to parse module surface area (MSA & MBD) include both 
  driver and library. 

  @since GenBuild 1.0
**/
public class SurfaceAreaParser {

    /**
      Using XmlBeans to parse and valid surface area file. 
    
      @param surfaceAreaFile the surface area file to parse
      @return top level elements and its value mapping information
      @throws BuildException
              If surface area is not well-formed or invalid
    **/
    public Map<String, XmlObject> parseFile(File surfaceAreaFile) throws BuildException {
        Map<String, XmlObject> map = new HashMap<String, XmlObject>();
        try {
            XmlObject sadoc = XmlObject.Factory.parse(surfaceAreaFile);
            // Validate File if they obey XML Schema
            
            if ( ! sadoc.validate()){
                throw new BuildException("Surface Area file [" + surfaceAreaFile.getPath() + "] is invalid.");
            }
            if (sadoc instanceof ModuleSurfaceAreaDocument){
                parseFile((ModuleSurfaceAreaDocument) sadoc, map);
            }
            else if(sadoc instanceof ModuleBuildDescriptionDocument){
                parseFile((ModuleBuildDescriptionDocument) sadoc, map);
            }
            else if(sadoc instanceof LibraryModuleSurfaceAreaDocument){
                parseFile((LibraryModuleSurfaceAreaDocument) sadoc, map);
            }
            else if(sadoc instanceof LibraryModuleBuildDescriptionDocument){
                parseFile((LibraryModuleBuildDescriptionDocument) sadoc, map);
            }
        }
        catch (Exception ex){
            throw new BuildException(ex.getMessage());
        }
        return map;
    }
    
    
    /**
      Parse MSA.
    
      @param doc top level surface area XML document
      @param msaMap the map to store the result
    **/
    private void parseFile(ModuleSurfaceAreaDocument doc, Map<String, XmlObject> msaMap) {
        msaMap.put("MsaHeader", doc.getModuleSurfaceArea().getMsaHeader());
        msaMap.put("LibraryClassDefinitions", doc.getModuleSurfaceArea()
                        .getLibraryClassDefinitions());
        msaMap.put("SourceFiles", doc.getModuleSurfaceArea().getSourceFiles());
        msaMap.put("Includes", doc.getModuleSurfaceArea().getIncludes());
        msaMap.put("Protocols", doc.getModuleSurfaceArea().getProtocols());

        msaMap.put("Events", doc.getModuleSurfaceArea().getEvents());
        msaMap.put("Hobs", doc.getModuleSurfaceArea().getHobs());
        msaMap.put("PPIs", doc.getModuleSurfaceArea().getPPIs());
        msaMap.put("Variables", doc.getModuleSurfaceArea().getVariables());
        msaMap.put("BootModes", doc.getModuleSurfaceArea().getBootModes());

        msaMap
                        .put("SystemTables", doc.getModuleSurfaceArea()
                                        .getSystemTables());
        msaMap.put("DataHubs", doc.getModuleSurfaceArea().getDataHubs());
        msaMap.put("Formsets", doc.getModuleSurfaceArea().getFormsets());
        msaMap.put("Guids", doc.getModuleSurfaceArea().getGuids());
        msaMap.put("Externs", doc.getModuleSurfaceArea().getExterns());

        msaMap.put("PCDs", doc.getModuleSurfaceArea().getPCDs());
        msaMap
                        .put("BuildOptions", doc.getModuleSurfaceArea()
                                        .getBuildOptions());
    }

    /**
      Parse MBD.
  
      @param doc top level surface area XML document
      @param msaMap the map to store the result
    **/
    private void parseFile(ModuleBuildDescriptionDocument doc, Map<String, XmlObject> mbdMap) {
        mbdMap.put("MbdHeader", doc.getModuleBuildDescription().getMbdHeader());
        mbdMap.put("Libraries", doc.getModuleBuildDescription().getLibraries());
        mbdMap.put("SourceFiles", doc.getModuleBuildDescription()
                        .getSourceFiles());
        mbdMap.put("Includes", doc.getModuleBuildDescription().getIncludes());
        mbdMap.put("Protocols", doc.getModuleBuildDescription().getProtocols());

        mbdMap.put("Events", doc.getModuleBuildDescription().getEvents());
        mbdMap.put("Hobs", doc.getModuleBuildDescription().getHobs());
        mbdMap.put("PPIs", doc.getModuleBuildDescription().getPPIs());
        mbdMap.put("Variables", doc.getModuleBuildDescription().getVariables());
        mbdMap.put("BootModes", doc.getModuleBuildDescription().getBootModes());

        mbdMap.put("SystemTables", doc.getModuleBuildDescription()
                        .getSystemTables());
        mbdMap.put("DataHubs", doc.getModuleBuildDescription().getDataHubs());
        mbdMap.put("Formsets", doc.getModuleBuildDescription().getFormsets());
        mbdMap.put("Guids", doc.getModuleBuildDescription().getGuids());
        mbdMap.put("Externs", doc.getModuleBuildDescription().getExterns());

        mbdMap.put("PCDs", doc.getModuleBuildDescription().getPCDs());
        mbdMap.put("BuildOptions", doc.getModuleBuildDescription()
                        .getBuildOptions());
    }
    /**
      Parse Library MSA.

      @param doc top level surface area XML document
      @param msaMap the map to store the result
    **/
    private void parseFile(LibraryModuleSurfaceAreaDocument doc, Map<String, XmlObject> msaMap) {
        msaMap.put("MsaLibHeader", doc.getLibraryModuleSurfaceArea()
                        .getMsaLibHeader());
        msaMap.put("LibraryClassDefinitions", doc.getLibraryModuleSurfaceArea()
                        .getLibraryClassDefinitions());
        msaMap.put("SourceFiles", doc.getLibraryModuleSurfaceArea()
                        .getSourceFiles());
        msaMap.put("Includes", doc.getLibraryModuleSurfaceArea().getIncludes());
        msaMap.put("Protocols", doc.getLibraryModuleSurfaceArea()
                        .getProtocols());

        msaMap.put("Events", doc.getLibraryModuleSurfaceArea().getEvents());
        msaMap.put("Hobs", doc.getLibraryModuleSurfaceArea().getHobs());
        msaMap.put("PPIs", doc.getLibraryModuleSurfaceArea().getPPIs());
        msaMap.put("Variables", doc.getLibraryModuleSurfaceArea()
                        .getVariables());
        msaMap.put("BootModes", doc.getLibraryModuleSurfaceArea()
                        .getBootModes());

        msaMap.put("SystemTables", doc.getLibraryModuleSurfaceArea()
                        .getSystemTables());
        msaMap.put("DataHubs", doc.getLibraryModuleSurfaceArea().getDataHubs());
        msaMap.put("Formsets", doc.getLibraryModuleSurfaceArea().getFormsets());
        msaMap.put("Guids", doc.getLibraryModuleSurfaceArea().getGuids());
        msaMap.put("Externs", doc.getLibraryModuleSurfaceArea().getExterns());

        msaMap.put("PCDs", doc.getLibraryModuleSurfaceArea().getPCDs());
        msaMap.put("BuildOptions", doc.getLibraryModuleSurfaceArea()
                        .getBuildOptions());
    }

    /**
      Parse Library MBD.

      @param doc top level surface area XML document
      @param msaMap the map to store the result
    **/
    private void parseFile(LibraryModuleBuildDescriptionDocument doc, Map<String, XmlObject> mbdMap) {
        mbdMap.put("MbdLibHeader", doc.getLibraryModuleBuildDescription()
                        .getMbdLibHeader());
        mbdMap.put("Libraries", doc.getLibraryModuleBuildDescription()
                        .getLibraries());
        mbdMap.put("SourceFiles", doc.getLibraryModuleBuildDescription()
                        .getSourceFiles());
        mbdMap.put("Includes", doc.getLibraryModuleBuildDescription()
                        .getIncludes());
        mbdMap.put("Protocols", doc.getLibraryModuleBuildDescription()
                        .getProtocols());

        mbdMap
                        .put("Events", doc.getLibraryModuleBuildDescription()
                                        .getEvents());
        mbdMap.put("Hobs", doc.getLibraryModuleBuildDescription().getHobs());
        mbdMap.put("PPIs", doc.getLibraryModuleBuildDescription().getPPIs());
        mbdMap.put("Variables", doc.getLibraryModuleBuildDescription()
                        .getVariables());
        mbdMap.put("BootModes", doc.getLibraryModuleBuildDescription()
                        .getBootModes());

        mbdMap.put("SystemTables", doc.getLibraryModuleBuildDescription()
                        .getSystemTables());
        mbdMap.put("DataHubs", doc.getLibraryModuleBuildDescription()
                        .getDataHubs());
        mbdMap.put("Formsets", doc.getLibraryModuleBuildDescription()
                        .getFormsets());
        mbdMap.put("Guids", doc.getLibraryModuleBuildDescription().getGuids());
        mbdMap.put("Externs", doc.getLibraryModuleBuildDescription()
                        .getExterns());

        mbdMap.put("PCDs", doc.getLibraryModuleBuildDescription().getPCDs());
        mbdMap.put("BuildOptions", doc.getLibraryModuleBuildDescription()
                        .getBuildOptions());
    }
}
