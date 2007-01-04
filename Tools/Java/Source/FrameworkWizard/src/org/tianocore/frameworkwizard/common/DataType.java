/** @file
 
 The file is used to define all used final variables
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/
package org.tianocore.frameworkwizard.common;

/**
 The class is used to define all used final variables

 **/
public class DataType {

    /**
     
     @param args
     
     **/
    public static void main(String[] args) {

    }

    //
    // Define all return types
    //
    public static final int RETURN_TYPE_OK = 1;

    public static final int RETURN_TYPE_CANCEL = 2;

    public static final int RETURN_TYPE_NEXT = 3;

    public static final int RETURN_TYPE_BACK = 4;

    public static final int RETURN_TYPE_MODULE_SURFACE_AREA = 11;

    public static final int RETURN_TYPE_PACKAGE_SURFACE_AREA = 12;

    public static final int RETURN_TYPE_PLATFORM_SURFACE_AREA = 13;

    public static final int RETURN_TYPE_BUILD_XML = 14;

    public static final int RETURN_TYPE_WORKSPACE = 15;

    public static final int RETURN_TYPE_TEXT = 16;

    public static final int RETURN_TYPE_FAR_SURFACE_AREA = 17;
    
    
    //
    // Define all used final variables
    //
    public static final String DOS_LINE_SEPARATOR = "\r\n";

    public static final String UNIX_LINE_SEPARATOR = "\n";
    
    public static final String LINE_SEPARATOR = UNIX_LINE_SEPARATOR;
    
    public static final String HTML_LINE_SEPARATOR = "<br>";

    public static final String EMPTY_SELECT_ITEM = "----";

    public static final String DOS_FILE_SEPARATOR = "\\";

    public static final String UNIX_FILE_SEPARATOR = "/";

    //
    // Define xml files ext
    //
    public static final String COPY_OF = "Copy of ";

    public static final String FILE_EXT_SEPARATOR = ".";

    public static final String WORKSPACE = "Workspace";

    public static final String MODULE_SURFACE_AREA = "Module Surface Area Description";

    public static final String MODULE_SURFACE_AREA_EXT = "msa";

    public static final String MODULE_SURFACE_AREA_EXT_DESCRIPTION = MODULE_SURFACE_AREA + " (*."
                                                                     + MODULE_SURFACE_AREA_EXT + ")";

    public static final String PACKAGE_SURFACE_AREA = "Package Surface Area Description";

    public static final String PACKAGE_SURFACE_AREA_EXT = "spd";

    public static final String PACKAGE_SURFACE_AREA_EXT_DESCRIPTION = PACKAGE_SURFACE_AREA + " (*."
                                                                      + PACKAGE_SURFACE_AREA_EXT + ")";

    public static final String PLATFORM_SURFACE_AREA = "Platform Surface Area Description";

    public static final String PLATFORM_SURFACE_AREA_EXT = "fpd";

    public static final String PLATFORM_SURFACE_AREA_EXT_DESCRIPTION = PLATFORM_SURFACE_AREA + " (*."
                                                                       + PLATFORM_SURFACE_AREA_EXT + ")";

    public static final String ANT_BUILD_FILE = "ANT Build File";

    public static final String ANT_BUILD_FILE_EXT = "xml";

    public static final String ANT_BUILD_FILE_EXT_DESCRIPTION = ANT_BUILD_FILE + " (*." + ANT_BUILD_FILE_EXT + ")";

    public static final String TEXT_FILE = "Text File";

    public static final String TEXT_FILE_EXT = "txt";

    public static final String TEXT_FILE_EXT_DESCRIPTION = TEXT_FILE + " (*." + TEXT_FILE_EXT + ")";

    public static final String FAR_SURFACE_AREA = "Framework Archive";
    
    public static final String FAR_SURFACE_AREA_EXT = "far";
    
    public static final String FAR_SURFACE_AREA_EXT_DESCRIPTION = FAR_SURFACE_AREA + " (*." + FAR_SURFACE_AREA_EXT + ")";
    //
    // Define file separator for current OS
    //
    public static String FILE_SEPARATOR = System.getProperty("file.separator");

    //
    // Defien all used frame, container component's sizes
    //
    public static final int MAIN_FRAME_PREFERRED_SIZE_WIDTH = 800;

    public static final int MAIN_FRAME_PREFERRED_SIZE_HEIGHT = 600;

    public static final int MAIN_FRAME_MAX_SIZE_WIDTH = 1920;

    public static final int MAIN_FRAME_MAX_SIZE_HEIGHT = 1200;

    public static final int MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_WIDTH = 790;

    public static final int MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_HEIGHT = 545;

    public static final int MAIN_FRAME_WIDTH_SPACING = MAIN_FRAME_PREFERRED_SIZE_WIDTH
                                                       - MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_WIDTH;

    public static final int MAIN_FRAME_HEIGHT_SPACING = MAIN_FRAME_PREFERRED_SIZE_HEIGHT
                                                        - MAIN_FRAME_SPLIT_PANEL_PREFERRED_SIZE_HEIGHT;

    public static final int MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_WIDTH = 273;

    public static final int MAIN_FRAME_TREE_PANEL_PREFERRED_SIZE_HEIGHT = 545;

    public static final int MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_WIDTH = 515;

    public static final int MAIN_FRAME_EDITOR_PANEL_PREFERRED_SIZE_HEIGHT = 545;

    public static final int MAIN_FRAME_EDITOR_PANEL_LOCATION_X = 260;

    public static final int MAIN_FRAME_EDITOR_PANEL_LOCATION_Y = 1;

    public static final int SPACE_TO_RIGHT_FOR_GENERATE_BUTTON = 10;

    public static final int SPACE_TO_RIGHT_FOR_OK_BUTTON = 10;

    public static final int SPACE_TO_RIGHT_FOR_ADD_BUTTON = 200;

    public static final int SPACE_TO_RIGHT_FOR_REMOVE_BUTTON = 105;

    public static final int SPACE_TO_RIGHT_FOR_UPDATE_BUTTON = 10;

    public static final int SPACE_TO_BOTTOM_FOR_ADD_BUTTON = 30;

    public static final int SPACE_TO_BOTTOM_FOR_REMOVE_BUTTON = 30;

    public static final int SPACE_TO_BOTTOM_FOR_UPDATE_BUTTON = 30;
    
    public static final int SPACE_TO_RIGHT_FOR_CLOSE_BUTTON = 240;
    
    public static final int SPACE_TO_BOTTOM_FOR_CLOSE_BUTTON = 25;

    public static final int SPACE_TO_RIGHT_FOR_PROTOCOL_NOTIFY = 25;

    public static final int LEFT_WIDTH = 160;

    public static final int LEFT_HEIGHT = 20;

    public static final int RIGHT_WIDTH = 320;

    public static final int RIGHT_HEIGHT = 20;

    public static final int RIGHT_MULTIPLE_HEIGHT = 4 * RIGHT_HEIGHT;

    public static final int BUTTON_GEN_WIDTH = 65;

    public static final int BUTTON_GEN_HEIGHT = 20;

    public static final int BUTTON_BROWSE_WIDTH = 65;

    public static final int BUTTON_BROWSE_HEIGHT = 20;

    public static final int BUTTON_ADD_WIDTH = 80;

    public static final int BUTTON_ADD_HEIGHT = 20;

    public static final int BUTTON_UPDATE_WIDTH = 80;

    public static final int BUTTON_UPDATE_HEIGHT = 20;

    public static final int BUTTON_REMOVE_WIDTH = 80;

    public static final int BUTTON_REMOVE_HEIGHT = 20;

    public static final int SCROLLBAR_WIDTH = 18;

    public static final int SCROLLBAR_HEIGHT = 18;

    //
    // Common Help Text
    // First defined here
    // Will be replaced by resource file later
    //
    public static final String SUP_ARCH_LIST_HELP_TEXT = "<html>Selecting a checkbox is a restriction of only the selected architectures;<br>If none of boxes are selected, all architectures are supported.<html>";

    //
    // Project name and version
    //
    public static final String PROJECT_NAME = "Framework Wizard";

    public static final String PROJECT_VERSION = "1.0";

    //
    // Sort Type
    //
    public static final int SORT_TYPE_ASCENDING = 1;

    public static final int SORT_TYPE_DESCENDING = 2;

    //
    // Module Type
    //
    public static final String MODULE_TYPE_LIBRARY = "Library";

    public static final String MODULE_TYPE_MODULE = "Module";
    
    //
    // Hex String Header
    //
    public static final String HEX_STRING_HEADER = "0x";
    
    //
    // The String of Boolean
    //
    public static final String TRUE = "True";
    
    public static final String FALSE = "False";
    
    //
    // The String for USAGE type
    //
    public final static String USAGE_TYPE_ALWAYS_CONSUMED = "ALWAYS_CONSUMED";
    
    public final static String USAGE_TYPE_SOMETIMES_CONSUMED = "SOMETIMES_CONSUMED";
    
    public final static String USAGE_TYPE_ALWAYS_PRODUCED = "ALWAYS_PRODUCED";
    
    public final static String USAGE_TYPE_SOMETIMES_PRODUCED = "SOMETIMES_PRODUCED";
    
    public final static String USAGE_TYPE_PRIVATE = "PRIVATE";
    
    public final static String USAGE_TYPE_TO_START = "TO_START";
    
    public final static String USAGE_TYPE_BY_START = "BY_START";
    
    //
    // The String for PCD type
    //
    public final static String PCD_ITEM_TYPE_FEATURE_FLAG = "FEATURE_FLAG";
    
    public final static String PCD_ITEM_TYPE_FIXED_AT_BUILD = "FIXED_AT_BUILD";
    
    public final static String PCD_ITEM_TYPE_PATCHABLE_IN_MODULE = "PATCHABLE_IN_MODULE";
    
    public final static String PCD_ITEM_TYPE_DYNAMIC = "DYNAMIC";
    
    public final static String PCD_ITEM_TYPE_DYNAMIC_EX = "DYNAMIC_EX";
    
    //
    // The String for PPI type
    //
    public final static String PPI_TYPE_PPI = "Ppi";
    
    public final static String PPI_TYPE_PPI_NOTIFY = "Ppi Notify";
    
    //
    // The String for Protocol type
    //
    public final static String PROTOCOL_TYPE_PROTOCOL = "Protocol";
    
    public final static String PROTOCOL_TYPE_PROTOCOL_NOTIFY = "Protocol Notify";
    
    //
    // The default file name for guids.xref file
    //
    public final static String GUIDS_XREF_FILE_NAME = "guids.xref";
}
