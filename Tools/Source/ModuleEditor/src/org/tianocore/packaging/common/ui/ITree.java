/** @file
 
 The file is used to override JTree to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/

package org.tianocore.packaging.common.ui;

import javax.swing.JTree;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;

/**
 The class is used to override JTree to provides customized interfaces 
 It extends JTree
 
 @since ModuleEditor 1.0

 **/
public class ITree extends JTree {
    ///
    /// Define class Serial Version UID
    ///
    private static final long serialVersionUID = -7907086164518295327L;
    
    //
    // Define class members
    //
    DefaultTreeModel treeModel = null;

    /**
     This is the default constructor
     
     **/
    public ITree() {
        super();
    }

    /**
     This is the overrided constructor
     Init class members with input data
     
     @param iDmtRoot The root node of the tree
     
     **/
    public ITree(IDefaultMutableTreeNode iDmtRoot) {
        super(iDmtRoot);
    }

    /**
     Get category of selected node
     
     @return The category of selected node
     
     **/
    public int getSelectCategory() {
        int intCategory = 0;
        TreePath path = this.getSelectionPath();
        IDefaultMutableTreeNode node = (IDefaultMutableTreeNode) path.getLastPathComponent();
        intCategory = node.getCategory();
        return intCategory;
    }

    /**
     Get operation of selected node
     
     @return The operation of selected node
     
     **/
    public int getSelectOperation() {
        int intOperation = 0;
        TreePath path = this.getSelectionPath();
        IDefaultMutableTreeNode node = (IDefaultMutableTreeNode) path.getLastPathComponent();
        intOperation = node.getOperation();
        return intOperation;
    }

    /**
     Get selectLoaction of selected node
     
     @return The selectLoaction of selected node
     
     **/
    public int getSelectLoaction() {
        int intLocation = 0;
        TreePath path = this.getSelectionPath();
        IDefaultMutableTreeNode node = (IDefaultMutableTreeNode) path.getLastPathComponent();
        intLocation = node.getLocation();
        return intLocation;
    }

    /**
     Main class, reserved for test
     
     @param args
     
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub
    }

    /**
     Add input node as child node for current selected node
     
     @param strNewNode The name of the node which need be added
     
     **/
    public void addNode(String strNewNode) {
        DefaultMutableTreeNode parentNode = null;
        DefaultMutableTreeNode newNode = new DefaultMutableTreeNode(strNewNode);
        newNode.setAllowsChildren(true);
        TreePath parentPath = this.getSelectionPath();

        /**
         * Get parent node of new node
         */
        parentNode = (DefaultMutableTreeNode) (parentPath.getLastPathComponent());

        /**
         * Insert new node
         */
        treeModel.insertNodeInto(newNode, parentNode, parentNode.getChildCount());
        this.scrollPathToVisible(new TreePath(newNode.getPath()));
    }

    /**
     Add input node as child node for current selected node
     
     @param newNode The node need be added
     
     **/
    public void addNode(IDefaultMutableTreeNode newNode) {
        IDefaultMutableTreeNode parentNode = null;
        newNode.setAllowsChildren(true);
        TreePath parentPath = this.getSelectionPath();
        parentNode = (IDefaultMutableTreeNode) (parentPath.getLastPathComponent());
        treeModel.insertNodeInto(newNode, parentNode, parentNode.getChildCount());
        this.scrollPathToVisible(new TreePath(newNode.getPath()));
    }

    /**
     Remove current selectd node
     
     **/
    public void removeNode() {
        TreePath treepath = this.getSelectionPath();
        if (treepath != null) {
            DefaultMutableTreeNode selectionNode = (DefaultMutableTreeNode) treepath.getLastPathComponent();
            TreeNode parent = (TreeNode) selectionNode.getParent();
            if (parent != null) {
                treeModel.removeNodeFromParent(selectionNode);
            }
        }
    }

    /**
     Remove all node on a same level
     
     **/
    public void removeNodeOnSameLevel() {
        TreePath treepath = this.getSelectionPath();
        IDefaultMutableTreeNode parentNode = (IDefaultMutableTreeNode) treepath.getLastPathComponent();
        parentNode.removeAllChildren();
        treeModel.reload();
    }

    /**
     Remove the input node by name
     
     @param strRemovedNode
     
     **/
    public void removeNode(String strRemovedNode) {
        TreePath treepath = this.getSelectionPath();
        if (treepath != null) {
            DefaultMutableTreeNode selectionNode = (DefaultMutableTreeNode) treepath.getLastPathComponent();
            TreeNode parent = (TreeNode) selectionNode.getParent();
            if (parent != null) {
                treeModel.removeNodeFromParent(selectionNode);
            }
        }
    }

    /**
     Remove all nodes of the tree
     
     **/
    public void removeAllNode() {
        DefaultMutableTreeNode rootNode = (DefaultMutableTreeNode) treeModel.getRoot();
        rootNode.removeAllChildren();
        treeModel.reload();
    }
}
