package org.tianocore.migration;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.tree.*;

import org.tianocore.ModuleSurfaceAreaDocument;

public class MsaTreeEditor extends JPanel {
	/**
	 *  Define class Serial Version UID
	 */
	private static final long serialVersionUID = 3169905938472150649L;
/*
	MsaTreeEditor(ModuleInfo m, UI u, ModuleSurfaceAreaDocument md) {
		mi = m;
		ui = u;
		msadoc = md;
		
		//rootNode = msadoc.getDomNode();
        rootNode = new DefaultMutableTreeNode("Root Node");
        treeModel = new DefaultTreeModel(rootNode);

        tree = new JTree(treeModel);
        tree.setEditable(true);
        tree.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
        tree.setShowsRootHandles(false);
        tree.addMouseListener(mouseadapter);

        JScrollPane scrollPane = new JScrollPane(tree);
        add(scrollPane);
        
        popupmenu = new JPopupMenu();
        menuitemadd = new JMenuItem("addNode");
        menuitemdel = new JMenuItem("deleteNode");
        popupmenu.add(menuitemadd);
        popupmenu.add(menuitemdel);
        menuitemadd.addActionListener(actionListener);
        menuitemdel.addActionListener(actionListener);
        
        addNode(rootNode, "1st");
        addNode(rootNode, "2nd");
	}
*/
	MsaTreeEditor(ModuleInfo m, UI u) {
		mi = m;
		ui = u;
		
        rootNode = new DefaultMutableTreeNode("Root Node");
        treeModel = new DefaultTreeModel(rootNode);

        tree = new JTree(treeModel);
        tree.setEditable(true);
        tree.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
        tree.setShowsRootHandles(false);
        tree.addMouseListener(mouseadapter);

        JScrollPane scrollPane = new JScrollPane(tree);
        add(scrollPane);
        
        popupmenu = new JPopupMenu();
        menuitemadd = new JMenuItem("addNode");
        menuitemdel = new JMenuItem("deleteNode");
        popupmenu.add(menuitemadd);
        popupmenu.add(menuitemdel);
        menuitemadd.addActionListener(actionListener);
        menuitemdel.addActionListener(actionListener);
        
        addNode(rootNode, "1st");
        addNode(rootNode, "2nd");
	}
	
	private ModuleInfo mi;
	private UI ui;
	//private ModuleSurfaceAreaDocument msadoc;
	
	private JTree tree;
	private DefaultMutableTreeNode rootNode;
	private DefaultTreeModel treeModel;
	private JMenuItem menuitemadd, menuitemdel;
	
	private JPopupMenu popupmenu;
	private MouseAdapter mouseadapter = new MouseAdapter() {
		public void mouseReleased(MouseEvent me) {
			if (me.getClickCount() == 1 && SwingUtilities.isRightMouseButton(me)) {
				tree.setSelectionPath(tree.getPathForLocation(me.getX(), me.getY()));
				popupmenu.show(tree, me.getX(), me.getY());
			}
		}
	};
	private ActionListener actionListener = new ActionListener() {
		public void actionPerformed(ActionEvent ae) {
			if (ae.getSource() == menuitemadd) {
				addNode();
			} else if (ae.getSource() == menuitemdel) {
				delNode();
			}
		}
	};
	
	private void delNode() {
		treeModel.removeNodeFromParent((DefaultMutableTreeNode)(tree.getSelectionPath().getLastPathComponent()));
	}
	
	private void addNode() {
		addNode((DefaultMutableTreeNode)(tree.getSelectionPath().getLastPathComponent()), MigrationTool.ui.getInput("Input Node Name"));
	}
	
	private void addNode(DefaultMutableTreeNode parentNode, Object child) {
        DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(child);
        treeModel.insertNodeInto(childNode, parentNode, parentNode.getChildCount());
        tree.scrollPathToVisible(new TreePath(childNode.getPath()));
	}
	/*
	public static void init(ModuleInfo mi, UI ui, ModuleSurfaceAreaDocument msadoc) throws Exception {
		init(mi, ui);
	}
	*/
	public static void init(ModuleInfo mi, UI ui) throws Exception {
    	UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());

		JFrame frame = new JFrame("MsaTreeEditor");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		MsaTreeEditor mte = new MsaTreeEditor(mi, ui);
		mte.setLayout(new GridBagLayout());
		mte.setOpaque(true);
        frame.setContentPane(mte);

		frame.pack();
		frame.setVisible(true);
	}
}