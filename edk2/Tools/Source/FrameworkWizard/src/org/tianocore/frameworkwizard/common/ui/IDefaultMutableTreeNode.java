/** @file
 
 
 The file is used to override DefaultMutableTreeNode to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.frameworkwizard.common.ui;

import javax.swing.tree.DefaultMutableTreeNode;

import org.tianocore.frameworkwizard.common.Identifications.Identification;

/**
 The class is used to override DefaultMutableTreeNode to provides customized interfaces
 It extends DefaultMutableTreeNode
 

 
 **/
public class IDefaultMutableTreeNode extends DefaultMutableTreeNode {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -1947340717458069548L;

    //
    // Static final definitions for all kinds of node
    //
    public static final int MSA_HEADER = 100;

    public static final int MSA_LIBRARYCLASSDEFINITIONS = 101;

    public static final int MSA_PACKAGEDEPENDENCIES = 102;

    public static final int MSA_SOURCEFILES = 103;

    public static final int MSA_PROTOCOLS = 104;

    public static final int MSA_EVENTS = 105;

    public static final int MSA_HOBS = 106;

    public static final int MSA_PPIS = 107;

    public static final int MSA_VARIABLES = 108;

    public static final int MSA_BOOTMODES = 109;

    public static final int MSA_SYSTEMTABLES = 110;

    public static final int MSA_DATAHUBS = 111;

    public static final int MSA_HIIPACKAGES = 112;

    public static final int MSA_GUIDS = 113;

    public static final int MSA_EXTERNS = 114;

    public static final int MSA_PCDS = 115;

    public static final int MSA_BUILDOPTIONS = 117;

    public static final int MSA_USEREXTENSIONS = 118;

    public static final int MSA_MODULEDEFINITIONS = 119;

    public static final int SPD_HEADER = 200;

    public static final int SPD_LIBRARYCLASSDECLARATIONS = 201;

    public static final int SPD_MSAFILES = 202;

    public static final int SPD_PACKAGEHEADERS = 203;

    public static final int SPD_GUIDDECLARATIONS = 204;

    public static final int SPD_PROTOCOLDECLARATIONS = 205;

    public static final int SPD_PPIDECLARATIONS = 206;

    public static final int SPD_PCDDECLARATIONS = 207;

    public static final int SPD_PACKAGEDEFINITIONS = 208;

    public static final int SPD_INDUSTRYSTDINCLUDES = 209;

    public static final int FPD_PLATFORMHEADER = 300;

    public static final int FPD_FLASH = 301;

    public static final int FPD_FRAMEWORKMODULES = 302;

    public static final int FPD_PCDDYNAMICBUILDDECLARATIONS = 303;

    public static final int FPD_BUILDOPTIONS = 304;

    public static final int FPD_PLATFORMDEFINITIONS = 305;

    public static final int WORKSPACE = 0;

    public static final int MODULE_DESCRIPTION = 1;

    public static final int PACKAGE_DESCRIPTION = 2;

    public static final int PLATFORM_DESCRIPTION = 3;

    public static final int MODULE = 4;

    public static final int PACKAGE = 5;

    public static final int PLATFORM = 6;

    public static final int MODULE_PACKAGE = 7;

    public static final int MODULE_PACKAGE_LIBRARY = 8;

    public static final int MODULE_PACKAGE_MODULE = 9;

    //
    //Static final definitions for operation
    //
    public static final int OPERATION_NULL = 0;

    public static final int OPERATION_ADD = 1;

    public static final int OPERATION_UPDATE = 2;

    public static final int OPERATION_DELETE = 4;

    public static final int OPERATION_ADD_UPDATE = 3;

    public static final int OPERATION_ADD_DELETE = 5;

    public static final int OPERATION_UPDATE_DELETE = 6;

    public static final int OPERATION_ADD_UPDATE_DELETE = 7;

    //
    //Define 4 node attributes
    //
    private int category = 0;

    private String nodeName = "";

    private boolean isOpening = false;

    private Identification id = null;
    
    private IDefaultMutableTreeNode belongNode = null;

    /**
     This is the default constructor
     
     **/
    public IDefaultMutableTreeNode() {
        super();
    }

    /**
     This is the overrided constructor
     Init clase members with input data
     
     @param strNodeName The name of node
     @param intCategory The category of node
     @param bolIsOpened to identify if the node is opening or not
     @param identification The Identification of node
     
     **/
    public IDefaultMutableTreeNode(String strNodeName, int intCategory, boolean bolIsOpening,
                                   Identification identification, IDefaultMutableTreeNode idmtBelongNode) {
        super(strNodeName);
        this.nodeName = strNodeName;
        this.category = intCategory;
        this.isOpening = bolIsOpening;
        this.id = identification;
        this.belongNode = idmtBelongNode;
    }

    /**
     Get category of node 
     
     @return The category of node
     
     **/
    public int getCategory() {
        return category;
    }

    /**
     Set category of node
     
     @param category The input data of node category
     
     **/
    public void setCategory(int category) {
        this.category = category;
    }

    /**
     Get name of node
     
     @return The name of node
     
     **/
    public String getNodeName() {
        return nodeName;
    }

    /**
     Set name of node
     
     @param nodeName The input data of node name
     
     **/
    public void setNodeName(String nodeName) {
        this.nodeName = nodeName;
    }

    /**
     Get identification of node
     
     @return
     
     **/
    public Identification getId() {
        return id;
    }

    /**
     Set identification of node
     
     @param id
     
     **/
    public void setId(Identification id) {
        this.id = id;
    }

    /**
     get isOpening of node
     
     @return
     
     **/
    public boolean isOpening() {
        return isOpening;
    }

    /**
     Set isOpening of node
     
     @param isOpening
     
     **/
    public void setOpening(boolean isOpening) {
        this.isOpening = isOpening;
    }
    
    public String toString() {
        return this.nodeName;
    }

    public IDefaultMutableTreeNode getBelongNode() {
        return belongNode;
    }

    public void setBelongNode(IDefaultMutableTreeNode belongNode) {
        this.belongNode = belongNode;
    }
}
