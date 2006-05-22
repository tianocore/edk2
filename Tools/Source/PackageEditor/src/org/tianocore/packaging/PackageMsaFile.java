/** @file
  Java class PackageMsaFile is GUI for create MsaFile elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.util.Vector;

import javax.swing.DefaultListModel;
import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JTextField;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.filechooser.FileFilter;

/**
 GUI for create MsaFile elements of spd file
  
 @since PackageEditor 1.0
**/
public class PackageMsaFile extends JFrame implements ActionListener {
    static JFrame frame;
    
    private DefaultListModel listItem = new DefaultListModel();

    private SpdFileContents sfc = null;

    private JPanel jContentPane = null;

    private JScrollPane jScrollPane = null;

    private JList jListLibraryClassDefinitions = null;

    private JButton jButtonAdd = null;

    private JButton jButtonRemove = null;

    private JButton jButtonClearAll = null;

    private JButton jButtonCancel = null;

    private JButton jButtonOk = null;

    private JLabel jLabel = null;

    private JTextField jTextField = null;

    private JButton jButton = null;

    /**
      This method initializes this
      
     **/
    private void initialize() {
        this.setTitle("MSA Files");
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);

    }

    /**
      This method initializes jScrollPane	
      	
      @return javax.swing.JScrollPane	
     **/
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(10, 85, 350, 210));
            jScrollPane.setViewportView(getJListLibraryClassDefinitions());
        }
        return jScrollPane;
    }

    /**
      This method initializes jListLibraryClassDefinitions	
      	
      @return javax.swing.JList	
     **/
    private JList getJListLibraryClassDefinitions() {
        if (jListLibraryClassDefinitions == null) {
            jListLibraryClassDefinitions = new JList(listItem);
        }
        return jListLibraryClassDefinitions;
    }

    /**
      This method initializes jButtonAdd	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonAdd() {
        if (jButtonAdd == null) {
            jButtonAdd = new JButton();
            jButtonAdd.setBounds(new java.awt.Rectangle(375, 132, 90, 20));
            jButtonAdd.setText("Add");
            jButtonAdd.addActionListener(this);
        }
        return jButtonAdd;
    }

    /**
      This method initializes jButtonRemove	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonRemove() {
        if (jButtonRemove == null) {
            jButtonRemove = new JButton();
            jButtonRemove.setBounds(new java.awt.Rectangle(375, 230, 90, 20));
            jButtonRemove.setText("Remove");
            jButtonRemove.addActionListener(this);
        }
        return jButtonRemove;
    }

    /**
      This method initializes jButtonRemoveAll	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButtonClearAll() {
        if (jButtonClearAll == null) {
            jButtonClearAll = new JButton();
            jButtonClearAll.setBounds(new java.awt.Rectangle(375, 260, 90, 20));
            jButtonClearAll.setText("Clear All");
            jButtonClearAll.addActionListener(this);
        }
        return jButtonClearAll;
    }

    /**
     This method initializes jButtonCancel	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonCancel() {
        if (jButtonCancel == null) {
            jButtonCancel = new JButton();
            jButtonCancel.setPreferredSize(new java.awt.Dimension(90, 20));
            jButtonCancel.setLocation(new java.awt.Point(390, 305));
            jButtonCancel.setText("Cancel");
            jButtonCancel.setSize(new java.awt.Dimension(90, 20));
            jButtonCancel.addActionListener(this);
        }
        return jButtonCancel;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButtonOk() {
        if (jButtonOk == null) {
            jButtonOk = new JButton();
            jButtonOk.setSize(new java.awt.Dimension(90, 20));
            jButtonOk.setText("OK");
            jButtonOk.setLocation(new java.awt.Point(290, 305));
            jButtonOk.addActionListener(this);
        }
        return jButtonOk;
    }

    /**
     This is the default constructor
     **/
    public PackageMsaFile(SpdFileContents sfc) {
        super();
        initialize();
        init();
        this.sfc = sfc;
    }

    /**
     Start the window at the center of screen
     
     */
    protected void centerWindow(int intWidth, int intHeight) {
        Dimension d = Toolkit.getDefaultToolkit().getScreenSize();
        this.setLocation((d.width - intWidth) / 2, (d.height - intHeight) / 2);
    }

    /**
     Start the window at the center of screen
     
     **/
    protected void centerWindow() {
        centerWindow(this.getSize().width, this.getSize().height);
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void init() {
        this.setContentPane(getJContentPane());
        this.setTitle("Library Class Declarations");
        this.setBounds(new java.awt.Rectangle(0, 0, 500, 370));
        this.centerWindow();
        initFrame();
    }

    /**
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jLabel = new JLabel();
            jLabel.setBounds(new java.awt.Rectangle(11,20,143,22));
            jLabel.setText("Msa File Path and Name");
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJButtonAdd(), null);
            jContentPane.add(getJButtonRemove(), null);
            jContentPane.add(getJButtonClearAll(), null);
            jContentPane.add(getJButtonCancel(), null);
            jContentPane.add(getJButtonOk(), null);
            jContentPane.add(jLabel, null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButton(), null);
        }
        return jContentPane;
    }

    private void initFrame() {

    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonOk) {
            this.dispose();
            this.save();
        }
        if (arg0.getSource() == jButtonCancel) {
            this.dispose();
        }

        if (arg0.getSource() == jButtonAdd) {

            listItem.addElement(jTextField.getText());
        }

        if (arg0.getSource() == jButtonRemove) {
            int intSelected[] = jListLibraryClassDefinitions.getSelectedIndices();
            if (intSelected.length > 0) {
                for (int index = intSelected.length - 1; index > -1; index--) {
                    listItem.removeElementAt(intSelected[index]);
                }
            }
            jListLibraryClassDefinitions.getSelectionModel().clearSelection();
        }

        if (arg0.getSource() == jButtonClearAll) {
            listItem.removeAllElements();
        }

    }

    protected void save() {
        try {
            int intLibraryCount = listItem.getSize();

            if (intLibraryCount > 0) {

                for (int index = 0; index < intLibraryCount; index++) {
                    String strAll = listItem.get(index).toString();
                    sfc.genSpdMsaFiles(strAll, null);
                }
            } else {

            }

        } catch (Exception e) {
            System.out.println(e.toString());
        }
    }

    /**
     This method initializes jTextField	
     	
     @return javax.swing.JTextField	
     **/
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(11,44,349,21));
        }
        return jTextField;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        final FileFilter filter = new PkgFileFilter("msa");
        
        if (jButton == null) {
            jButton = new JButton();
            jButton.setBounds(new java.awt.Rectangle(377,46,89,20));
            jButton.setText("Browse");
            jButton.setPreferredSize(new java.awt.Dimension(34,20));
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    JFileChooser chooser = new JFileChooser(System.getenv("WORKSPACE"));
                    File theFile = null;
                    String msaDest = null;
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    chooser.setFileFilter(filter);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        theFile = chooser.getSelectedFile();
                        String file = theFile.getPath();
                        if (!file.startsWith(System.getenv("WORKSPACE"))) {
                            JOptionPane.showMessageDialog(frame, "You can only select files in current workspace!");
                            return;
                        }
                        
                    }
                    else {
                        return;
                    }
                    
                    if (!theFile.getPath().startsWith(PackagingMain.dirForNewSpd)) {
                        //
                        //ToDo: copy elsewhere msa to new pkg dir, prompt user to chooser a location
                        //
                        JOptionPane.showMessageDialog(frame, "You must copy msa file into current package directory!");
                        return;
                    }
                    
                    msaDest = theFile.getPath();
                    int fileIndex = msaDest.indexOf(System.getProperty("file.separator"), PackagingMain.dirForNewSpd.length());
                    
                    jTextField.setText(msaDest.substring(fileIndex + 1).replace('\\', '/'));
                }
            });
        }
        return jButton;
    }

}
