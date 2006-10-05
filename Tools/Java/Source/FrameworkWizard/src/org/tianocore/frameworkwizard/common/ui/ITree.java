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

package org.tianocore.frameworkwizard.common.ui;

import javax.swing.JTree;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import javax.swing.tree.TreePath;

import org.tianocore.frameworkwizard.common.Identifications.Identification;

/**
 The class is used to override JTree to provides customized interfaces 
 It extends JTree

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
        treeModel = (DefaultTreeModel) this.getModel();
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
     Add input node as child node for current selected node
     
     @param newNode The node need be added
     
     **/
    public void addNode(IDefaultMutableTreeNode parentNode, IDefaultMutableTreeNode newNode) {
        treeModel.insertNodeInto(newNode, parentNode, parentNode.getChildCount());
        this.scrollPathToVisible(new TreePath(newNode.getPath()));
    }

    /**
     Remove the selected node
     
     @param strRemovedNode
     
     **/
    public void removeSelectedNode() {
        TreePath treePath = this.getSelectionPath();
        removeNodeByPath(treePath);
    }

    /**
     Remove the node by tree path
     
     @param strRemovedNode
     
     **/
    public void removeNodeByPath(TreePath treePath) {
        if (treePath != null) {
            DefaultMutableTreeNode selectionNode = (DefaultMutableTreeNode) treePath.getLastPathComponent();
            TreeNode parent = (TreeNode) selectionNode.getParent();
            if (parent != null) {
                treeModel.removeNodeFromParent(selectionNode);
            }
        }
    }
    
    /**
     Return a node by input tree path
    
     @param treePath
     @return
    
    **/
    public IDefaultMutableTreeNode getNodeByPath(TreePath treePath) {
        if (treePath != null) {
            IDefaultMutableTreeNode selectionNode = (IDefaultMutableTreeNode) treePath.getLastPathComponent();
            return selectionNode;
        }
        return null;
    }

    /**
     Remove all child nodes under current node
     
     **/
    public void removeNodeChildrenByPath(TreePath treePath) {
        if (treePath != null) {
            DefaultMutableTreeNode currentNode = (DefaultMutableTreeNode) treePath.getLastPathComponent();
            for (int index = currentNode.getChildCount() - 1; index > -1; index--) {
                treeModel.removeNodeFromParent((DefaultMutableTreeNode) currentNode.getChildAt(index));
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

    public IDefaultMutableTreeNode getSelectNode() {
        TreePath treepath = this.getSelectionPath();
        IDefaultMutableTreeNode selectionNode = null;
        if (treepath != null) {
            selectionNode = (IDefaultMutableTreeNode) treepath.getLastPathComponent();
        }
        return selectionNode;
    }

    public IDefaultMutableTreeNode getNodeById(IDefaultMutableTreeNode node, Identification id) {
        for (int index = 0; index < node.getChildCount(); index++) {
            IDefaultMutableTreeNode iNode = (IDefaultMutableTreeNode) node.getChildAt(index);
            if (iNode.getId().equals(id)) {
                return iNode;
            }
        }
        return null;
    }

    public IDefaultMutableTreeNode getNodeById(IDefaultMutableTreeNode node, Identification id, int category) {
        for (int index = 0; index < node.getChildCount(); index++) {
            IDefaultMutableTreeNode iNode = (IDefaultMutableTreeNode) node.getChildAt(index);
            if (iNode.getId().equals(id) && iNode.getCategory() == category) {
                return iNode;
            }
            IDefaultMutableTreeNode childNode = getNodeById(iNode, id, category);
            if (childNode != null) {
                return childNode;
            }
        }
        return null;
    }

    public TreePath getPathOfNode(IDefaultMutableTreeNode node) {
        if (node != null) {
            TreePath treePath = new TreePath(treeModel.getPathToRoot(node));
            return treePath;
        }
        return null;
    }
}
