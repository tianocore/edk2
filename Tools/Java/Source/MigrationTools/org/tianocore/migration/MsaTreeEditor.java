package org.tianocore.migration;

import java.awt.GridBagLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;

import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JMenuItem;
import javax.swing.JPanel;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JTree;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class MsaTreeEditor extends JPanel {
	/**
	 * Define class Serial Version UID
	 */
	private static final long serialVersionUID = 3169905938472150649L;

	MsaTreeEditor() throws Exception {
		rootNode = new DefaultMutableTreeNode("Root Node");
		treeModel = new DefaultTreeModel(rootNode);

		tree = new JTree(treeModel);
		tree.setEditable(true);
		tree.getSelectionModel().setSelectionMode(
				TreeSelectionModel.SINGLE_TREE_SELECTION);
		tree.setShowsRootHandles(false);
		tree.addMouseListener(mouseadapter);

		JScrollPane scrollPane = new JScrollPane(tree);
		// scrollPane.setSize(800, 600);
		add(scrollPane);

		popupmenu = new JPopupMenu();
		menuitemadd = new JMenuItem("Add Node");
		menuitemdel = new JMenuItem("Delete Node");
		menuitemedit = new JMenuItem("Edit Node");
		popupmenu.add(menuitemadd);
		popupmenu.add(menuitemdel);
		popupmenu.add(menuitemedit);
		menuitemadd.addActionListener(actionListener);
		menuitemdel.addActionListener(actionListener);
		menuitemedit.addActionListener(actionListener);

		genDomTree(MigrationTool.ui.getFilepath("Select a msa file",
				JFileChooser.FILES_AND_DIRECTORIES));
	}

	// private ModuleSurfaceAreaDocument msadoc;

	private JTree tree;

	private DefaultMutableTreeNode rootNode;

	private DefaultTreeModel treeModel;

	private JMenuItem menuitemadd, menuitemdel, menuitemedit;

	private JPopupMenu popupmenu;

	private MouseAdapter mouseadapter = new MouseAdapter() {
		public void mouseReleased(MouseEvent me) {
			if (me.getClickCount() == 1
					&& SwingUtilities.isRightMouseButton(me)) {
				tree.setSelectionPath(tree.getPathForLocation(me.getX(), me
						.getY()));
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
			} else if (ae.getSource() == menuitemedit) {
				editNode();
			}
		}
	};

	private void editNode() {
		DefaultMutableTreeNode node = (DefaultMutableTreeNode) (tree
				.getSelectionPath().getLastPathComponent());
		Element element = (Element) node.getUserObject();
		System.out.println(element.getTextContent());
	}

	private void delNode() {
		treeModel.removeNodeFromParent((DefaultMutableTreeNode) (tree
				.getSelectionPath().getLastPathComponent()));
	}

	private void addNode() {
		addNode((DefaultMutableTreeNode) (tree.getSelectionPath()
				.getLastPathComponent()), MigrationTool.ui
				.getInput("Input Node Name"));
	}

	private DefaultMutableTreeNode addNode(DefaultMutableTreeNode parentNode,
			Object child) {
		DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(child);
		treeModel.insertNodeInto(childNode, parentNode, parentNode
				.getChildCount());
		tree.scrollPathToVisible(new TreePath(childNode.getPath()));
		return childNode;
	}

	private final void handleNode(Node node, DefaultMutableTreeNode parentNode) {
		DefaultMutableTreeNode curNode = null;
		if (node.getNodeType() == Node.ELEMENT_NODE) {
			System.out.println("elem");
			curNode = addNode(parentNode, node);
		} else if (node.getNodeType() == Node.DOCUMENT_NODE) {
			System.out.println("doc");
			curNode = addNode(parentNode, "MsaDocum"); // can Docum be with
														// Root Node?
		}

		NodeList nodelist = node.getChildNodes();
		for (int i = 0; i < nodelist.getLength(); i++) {
			handleNode(nodelist.item(i), curNode);
		}
	}

	private final void genDomTree(String filename) throws Exception {
		DocumentBuilder builder = DocumentBuilderFactory.newInstance()
				.newDocumentBuilder();
		Document document = builder.parse(filename);
		handleNode(document, rootNode);
	}

	public static final void init() throws Exception {
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());

		JFrame frame = new JFrame("MsaTreeEditor");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		MsaTreeEditor mte = new MsaTreeEditor();
		mte.setLayout(new GridBagLayout());
		mte.setOpaque(true);
		frame.setContentPane(mte);

		frame.pack();
		frame.setVisible(true);
	}
}