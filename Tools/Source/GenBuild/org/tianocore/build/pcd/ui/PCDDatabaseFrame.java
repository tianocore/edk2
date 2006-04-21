/** @file
  PCDDatabaseFrame class.

  The class is the frame class for displaying PCD database in tree method.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php
 
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/   
package org.tianocore.build.pcd.ui;

import java.awt.*;
import java.awt.event.*;

import javax.swing.*;
import javax.swing.tree.DefaultMutableTreeNode;

import org.tianocore.build.pcd.action.ActionMessage;
import org.tianocore.build.pcd.entity.MemoryDatabaseManager;
import org.tianocore.build.pcd.entity.Token;
import org.tianocore.build.pcd.entity.UsageInstance;

/**
  The class is the frame class for displaying PCD database in tree method.
**/
public class PCDDatabaseFrame extends JFrame {
    static final long serialVersionUID = -7034897190740068939L;
    ///
    /// Database instance 
    ///
    private MemoryDatabaseManager dbManager;
    ///
    /// The token and module tree
    ///
    private JTree                 databaseTree;

    /**
      Constructure function. 
     
      Create the UI component and display frame.

      @param dbManager databaase manager instance.
    **/
    public PCDDatabaseFrame(MemoryDatabaseManager dbManager) {
        if (dbManager != null) {
            this.dbManager = dbManager;
        }
        //
        // Put the frame into center of desktop.
        //
        setLocation(100, 100);
        initializeComponent();

        setTitle("PCD View Tool");
        pack();
        setVisible(true);
    }

    /**
       Initliaze the UI component in Display frame.
    **/
    public void initializeComponent() {
        JScrollPane scrollPane = new JScrollPane();
        Container contentPane  = getContentPane();

        contentPane.setLayout(new BorderLayout());
        scrollPane.setViewportView(initializeTree());
        contentPane.add(scrollPane);

        addWindowListener(new PCDDatabaseFrameAdapter());
    }

    /**
      Initiliaze the TREE control.
    **/
    public JTree initializeTree() {
        Token[]                tokenArray     = null;
        Token                  token          = null;
        DefaultMutableTreeNode root           = new DefaultMutableTreeNode(dbManager.getLogFileName());
        DefaultMutableTreeNode rootByPCD      = new DefaultMutableTreeNode("By PCD");
        DefaultMutableTreeNode rootByModule   = new DefaultMutableTreeNode("By Module");
        DefaultMutableTreeNode tokenNode      = null;
        DefaultMutableTreeNode usageNode      = null;
        DefaultMutableTreeNode moduleNode     = null;
        java.util.List<String> moduleNames    = null;
        int                    index          = 0; 
        int                    usageIndex     = 0;
        int                    moduleIndex    = 0;
        java.util.List<UsageInstance>    usageArray     = null;
        UsageInstance          usageInstance  = null;

        root.add(rootByPCD);
        //
        // By PCD Node
        //

        tokenArray = dbManager.getRecordArray();
        for (index = 0; index < tokenArray.length; index ++) {
            token = tokenArray[index];
            ActionMessage.debug(this, token.cName);
            tokenNode = new DefaultMutableTreeNode(token.cName);
            tokenNode.add(new DefaultMutableTreeNode(String.format("TOKEN NUMBER: 0x%08x", token.tokenNumber)));
            tokenNode.add(new DefaultMutableTreeNode(String.format("ASSIGNED TOKEN NUMBER: 0x%08x", token.assignedtokenNumber)));
            tokenNode.add(new DefaultMutableTreeNode("TOKEN SPACE NAME: " + token.tokenSpaceName.toString()));
            tokenNode.add(new DefaultMutableTreeNode("ASSIGNED TOKEN SPACE NAME: " + token.assignedtokenSpaceName.toString()));
            tokenNode.add(new DefaultMutableTreeNode("PCD TYPE: " + Token.getStringOfpcdType(token.pcdType)));
            tokenNode.add(new DefaultMutableTreeNode("DATUM TYPE: " +Token.getStringOfdatumType(token.datumType)));
            tokenNode.add(new DefaultMutableTreeNode("DATUM: " + token.datum.toString()));
            tokenNode.add(new DefaultMutableTreeNode("HIIENABLE: " +(token.hiiEnabled?"true":"false")));
            tokenNode.add(new DefaultMutableTreeNode("VARIABLE NAME: " + token.variableName));
            tokenNode.add(new DefaultMutableTreeNode("VARIABLE GUID: " + token.variableGuid.toString()));
            tokenNode.add(new DefaultMutableTreeNode("SKUENABLE: " +(token.skuEnabled?"true":"false")));
            tokenNode.add(new DefaultMutableTreeNode("SKUDATA ARRAY ENABLE: " +(token.skuDataArrayEnabled?"true":"false")));
            tokenNode.add(new DefaultMutableTreeNode(String.format("SKUID: %d", token.skuId)));
            tokenNode.add(new DefaultMutableTreeNode(String.format("MAX SKU COUNT: %d", token.maxSkuCount)));
            tokenNode.add(new DefaultMutableTreeNode("VPDENABLE: " +(token.vpdEnabled?"true":"false")));

            usageNode = new DefaultMutableTreeNode("PRODUCER");
            tokenNode.add(usageNode);

            //
            // Prepare producer's leaf node
            //

            for (usageIndex = 0; usageIndex < token.producers.size(); usageIndex ++) {
                usageNode.add(new DefaultMutableTreeNode(token.producers.get(usageIndex).moduleName));
            }

            //
            // Prepare consumer's leaf node
            //
            usageNode = new DefaultMutableTreeNode("CONSUMER");
            tokenNode.add(usageNode);

            for (usageIndex = 0; usageIndex < token.consumers.size(); usageIndex ++) {
                usageNode.add(new DefaultMutableTreeNode(token.consumers.get(usageIndex).moduleName));
            }

            rootByPCD.add(tokenNode);
        }

        //
        // BY MODULE Node
        //
        root.add(rootByModule);
        moduleNames = dbManager.getAllModuleArray();
        for (moduleIndex = 0; moduleIndex < moduleNames.size(); moduleIndex ++) {
            ActionMessage.debug(this, "ModuleName:" + moduleNames.get(moduleIndex));
        }
        for (moduleIndex = 0; moduleIndex < moduleNames.size(); moduleIndex ++) {
            moduleNode = new DefaultMutableTreeNode(moduleNames.get(moduleIndex));
            usageArray = dbManager.getUsageInstanceArrayByModuleName(moduleNames.get(moduleIndex));
            for (usageIndex = 0; usageIndex < usageArray.size(); usageIndex ++) {
                usageInstance = usageArray.get(usageIndex);
                usageNode = new DefaultMutableTreeNode(usageInstance.parentToken.cName);
                usageNode.add(new DefaultMutableTreeNode("MODULE PCD TYPE: " + Token.getStringOfpcdType(usageInstance.modulePcdType)));
                usageNode.add(new DefaultMutableTreeNode("HELP TEXT: " + usageInstance.helpTextInMSA));
                usageNode.add(new DefaultMutableTreeNode("IS INHERIT: " +(usageInstance.isInherit?"true":"false")));
                usageNode.add(new DefaultMutableTreeNode("USAGE: " + Token.getStringOfUsage(usageInstance.usage)));
                moduleNode.add(usageNode);
            }
            rootByModule.add(moduleNode);
        }

        databaseTree = new JTree(root);
        return databaseTree;
    }
}

/**
  The adatper class for PCDDatabaseFrame. This class instance many windows message 
  callback function.
**/
class PCDDatabaseFrameAdapter  extends WindowAdapter {
    public void windowClosing(WindowEvent e) {
        System.exit(0);
    }
}
