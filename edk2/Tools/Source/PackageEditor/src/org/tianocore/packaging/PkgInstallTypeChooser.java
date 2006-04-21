/** @file
  Java class PkgInstallTypeChooser is GUI for upgrade package installation.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;

import java.awt.Dimension;
import java.awt.Toolkit;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.io.File;
import java.util.List;
import java.util.ListIterator;
import java.util.Vector;

import javax.swing.JFileChooser;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JDialog;
import javax.swing.JRadioButton;
import javax.swing.JButton;

import org.tianocore.PackageListDocument;
import javax.swing.JList;
import javax.swing.JTextField;
import javax.swing.JScrollPane;

/**
 GUI for speicial circumstances of package installation.
  
 @since PackageEditor 1.0
**/
public class PkgInstallTypeChooser extends JFrame implements MouseListener {

    final static long serialVersionUID = 0;

    static JFrame frame;

    private JPanel jContentPane = null;

    private JRadioButton jRadioButton = null;

    private JRadioButton jRadioButton1 = null;

    private JButton jButton = null;

    private JButton jButton1 = null;

    private String pn = null;

    ///
    /// list of package info from db file
    ///
    private List<PackageListDocument.PackageList.Package> dd = null;

    private String wk = null;

    private JList jList = null;
    
    private JScrollPane jScrollPane = null;

    private JTextField jTextField = null;

    private JButton jButton2 = null;

    private JFileChooser chooser = null;

    /**
      This is the default constructor
     **/
    public PkgInstallTypeChooser(String pkgName, String wkSpace, List<PackageListDocument.PackageList.Package> destDir) {
        super();
        pn = pkgName;
        dd = destDir;
        wk = wkSpace;
        initialize();
    }

    /**
      This method initializes this
      
      @return void
     **/
    private void initialize() {
        this.setSize(359, 328);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("Chooser Installation Type");
        this.setContentPane(getJContentPane());
        this.centerWindow();
        this.insertList();
    }

    /**
      Start the window at the center of screen
     
     **/
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
     initialize jList with package info. from db file
    **/
    private void insertList() {

        Vector<String> v = new Vector<String>();

        ListIterator lpi = dd.listIterator();
        while (lpi.hasNext()) {
            PackageListDocument.PackageList.Package p = (PackageListDocument.PackageList.Package) lpi.next();
            v.addElement(p.getPackageNameArray(0).getStringValue() + " " + p.getVersionArray(0) + " "
                         + p.getGuidArray(0).getStringValue());
        }
        jList.setListData(v);
    }

    /**
      This method initializes jContentPane
      
      @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJRadioButton(), null);
            jContentPane.add(getJRadioButton1(), null);
            jContentPane.add(getJButton(), null);
            jContentPane.add(getJButton1(), null);
            jContentPane.add(getJScrollPane(), null);
            jContentPane.add(getJTextField(), null);
            jContentPane.add(getJButton2(), null);
        }
        return jContentPane;
    }

 
    private JRadioButton getJRadioButton() {
        if (jRadioButton == null) {
            jRadioButton = new JRadioButton();
            jRadioButton.setBounds(new java.awt.Rectangle(17, 39, 186, 21));
            jRadioButton.setSelected(true);
            jRadioButton.setText("Reinstall Existing Package");
            jRadioButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (jRadioButton.isSelected()) {
                        jRadioButton1.setSelected(false);
                        jButton2.setEnabled(false);
                        jTextField.setEnabled(false);
                        jList.setEnabled(true);
                        return;
                    }
                    if (jRadioButton1.isSelected()) {
                        jRadioButton.setSelected(true);
                        jRadioButton1.setSelected(false);
                        jList.setEnabled(true);
                        return;
                    }

                }
            });
        }
        return jRadioButton;
    }

    private JRadioButton getJRadioButton1() {
        if (jRadioButton1 == null) {
            jRadioButton1 = new JRadioButton();
            jRadioButton1.setBounds(new java.awt.Rectangle(17, 155, 176, 21));
            jRadioButton1.setText("Install to Directory");
            jRadioButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (jRadioButton1.isSelected()) {
                        jRadioButton.setSelected(false);
                        jList.setEnabled(false);
                        jButton2.setEnabled(true);
                        jTextField.setEnabled(true);
                        return;
                    }
                    if (jRadioButton.isSelected()) {
                        jRadioButton1.setSelected(true);
                        jRadioButton.setSelected(false);
                        jButton2.setEnabled(true);
                        jTextField.setEnabled(true);
                        return;
                    }
                }
            });
        }
        return jRadioButton1;
    }

    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(34, 20));
            jButton.setSize(new java.awt.Dimension(76, 20));
            jButton.setText("Ok");
            jButton.setLocation(new java.awt.Point(141, 241));
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    try {
                        int i = -1;
                        //
                        // user selects replace existing package
                        //
                        if (jRadioButton.isSelected()) {
                            int j = jList.getSelectedIndex();
                            if (j == -1) {
                                JOptionPane.showMessageDialog(JOptionPane.getRootFrame(),
                                                              "Please Select One Package to Replace!");
                                return;
                            }
                            //
                            // the sequence of jList is the same with List
                            //
                            String destDir = dd.get(j).getPathArray(0).getStringValue();
                            ForceInstallPkg f = new ForceInstallPkg(pn, wk);
                            //
                            // record the package info. to be replaced
                            //
                            f.setOldVersion(dd.get(j).getVersionArray(0));
                            f.setOldGuid(dd.get(j).getGuidArray(0).getStringValue());
                            i = f.install(wk + System.getProperty("file.separator") + destDir);
                        } else {
                            //
                            // user selects install to another directory
                            //
                            File f = new File(wk + System.getProperty("file.separator") + FrameworkPkg.dbConfigFile);
                            if (new DbFileContents(f).checkDir(jTextField.getText().substring(wk.length() + 1)) != 0) {
                                throw new DirSame();
                            }
                            i = new ForceInstallPkg(pn, wk).install(jTextField.getText());
                        }
                        if (i == 0) {
                            JOptionPane.showMessageDialog(JOptionPane.getRootFrame(), "Package " + pn
                                                                                      + " Installed Successfully!");
                        }
                    } catch (DirSame ds) {
                        System.out.println(ds.toString());
                        JOptionPane.showMessageDialog(frame,
                                                      "Another Package Exists There, Please Select Another Directory!");
                    } catch (Exception ee) {
                        System.out.println(ee.toString());
                    }
                }
            });
        }
        return jButton;
    }

    /**
     * This method initializes jButton1	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setBounds(new java.awt.Rectangle(238, 241, 78, 20));
            jButton1.setText("Cancel");
            jButton1.setPreferredSize(new java.awt.Dimension(34, 20));
            jButton1.addMouseListener(this);
        }
        return jButton1;
    }

    public void mouseClicked(MouseEvent arg0) {
        // TODO Auto-generated method stub
        this.dispose();
    }

    public void mouseEntered(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mouseExited(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mousePressed(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    public void mouseReleased(MouseEvent arg0) {
        // TODO Auto-generated method stub

    }

    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setBounds(new java.awt.Rectangle(22, 68, 318, 58));
            jScrollPane.setHorizontalScrollBarPolicy(javax.swing.JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
            jScrollPane.setViewportView(getJList());
        }
        return jScrollPane;
    }
   
    private JList getJList() {
        if (jList == null) {
            jList = new JList();
            
          jList.setBounds(new java.awt.Rectangle(22, 68, 318, 58));

        }
        return jList;
    }

    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setBounds(new java.awt.Rectangle(22, 184, 224, 20));
            jTextField.setEnabled(false);
            jTextField.setText(wk);
        }
        return jTextField;
    }

    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setLocation(new java.awt.Point(259, 183));
            jButton2.setText("Browse");
            jButton2.setEnabled(false);
            jButton2.setSize(new java.awt.Dimension(81, 20));
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (chooser == null) {
                        chooser = new JFileChooser(wk);
                    }
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        jTextField.setText(chooser.getSelectedFile().getPath());

                    }

                }
            });
        }
        return jButton2;
    }

} //  @jve:decl-index=0:visual-constraint="134,45"
