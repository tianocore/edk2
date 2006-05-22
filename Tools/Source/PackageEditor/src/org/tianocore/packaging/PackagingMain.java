/** @file
  Java class PackagingMain is top level GUI for PackageEditor.
 
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

import javax.swing.JFileChooser;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JFrame;
import java.awt.FlowLayout;
import javax.swing.JButton;
import java.awt.GridLayout;
import java.io.File;
import java.io.FileOutputStream;
import java.util.jar.JarOutputStream;

/**
 GUI for show various GUI wizards for create, update spd file; install, remove package;
 create distributable package file.
  
 @since PackageEditor 1.0
**/
public class PackagingMain extends JFrame {

    static JFrame frame;

    static String dirForNewSpd = null;

    private JPanel jContentPane = null;

    private JButton jButton = null;

    private JButton jButton1 = null;

    private JButton jButton2 = null;

    private JButton jButton3 = null;

    private JButton jButton4 = null;

    private JButton jButton5 = null;

    private JFrame pThis = null;

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setEnabled(true);
            jButton.setText("Exit");
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    pThis.dispose();
                }
            });
        }
        return jButton;
    }

    /**
     This method initializes jButton1	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setText("Create an Installable Package");
            jButton1.setEnabled(true);
            jButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    File theFile = null;
                    JFileChooser chooser = new JFileChooser();
                    //
                    // select the directory that contains files to be distribute
                    //
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        try {
                            theFile = chooser.getSelectedFile();
                            //
                            // find the FDPManifest.xml file that should exist 
                            // in the root directory of package 
                            //
                            String[] list = theFile.list();
                            boolean manifestExists = false;
                            for (int i = 0; i < list.length; i++) {
                                if (list[i].equals("FDPManifest.xml")) {
                                    manifestExists = true;
                                    break;
                                }
                            }
                            if (!manifestExists) {
                                JOptionPane.showMessageDialog(frame,
                                                              "Please Put the FDPManifest.xml File under the Directory You Selected!");
                                return;
                            }
                            //
                            // create the distribute package .fdp file in the same directory with 
                            // the package root directory selected above.
                            //
                            JarOutputStream jos = new JarOutputStream(new FileOutputStream(theFile.getPath() + ".fdp"));
                            CreateFdp.create(theFile, jos, theFile.getPath());
                            jos.close();
                            JOptionPane.showMessageDialog(frame,
                                                          "FDP File Created Successfully!");
          
                            
                        } catch (Exception ee) {
                            System.out.println(ee.toString());
                        } 
                    } else {
                        return;
                    }
                }
            });
        }
        return jButton1;
    }

    /**
     This method initializes jButton2	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setText("Remove Package");
            jButton2.setEnabled(true);
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new GuiPkgUninstall(), pThis);
                }
            });
        }
        return jButton2;
    }

    /**
     This method initializes jButton3	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setText("Install Package");
            jButton3.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    ModalFrameUtil.showAsModal(new GuiPkgInstall(), pThis);
                }
            });
        }
        return jButton3;
    }

    /**
     This method initializes jButton4	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setText("Update Package Description File");
            jButton4.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    File theFile = null;
                    JFileChooser chooser = new JFileChooser();
                    //
                    // select the spd file to be updated first
                    //
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileFilter(new PkgFileFilter("spd"));
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        try {
                            theFile = chooser.getSelectedFile();
                            if (!theFile.isFile()) {
                                JOptionPane.showMessageDialog(frame, "Please Select one Spd File!");
                                return;
                            }

                        } catch (Exception ee) {
                            System.out.println(ee.toString());
                        }
                    } else {
                        return;
                    }
                    //
                    // create a SpdFileContents for this file and pass it to GUI
                    //
                    SpdFileContents sfc = new SpdFileContents(theFile);
                    ModalFrameUtil.showAsModal(new UpdateAction(sfc), pThis);
                }
            });
        }
        return jButton4;
    }

    /**
     This method initializes jButton5	
     	
     @return javax.swing.JButton	
     **/
    private JButton getJButton5() {
        if (jButton5 == null) {
            jButton5 = new JButton();
            jButton5.setText("Create Package Description File");
            jButton5.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    JFileChooser chooser = new JFileChooser(System.getenv("WORKSPACE"));
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setDialogTitle("Please specify where to save the new spd file");

                    int retval = chooser.showSaveDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        try {
                            File theFile = chooser.getSelectedFile();
                            PackagingMain.dirForNewSpd = theFile.getPath();

                        } catch (Exception ee) {
                            System.out.println(ee.toString());
                        }
//                        pThis.dispose();
                    }
                    else {
                        return;
                    }
                    SpdFileContents sfc = new SpdFileContents();
                    ModalFrameUtil.showAsModal(new PackageAction(sfc), pThis);
                }
            });
        }
        return jButton5;
    }

    /**
     Main for all package editor
     
     @param args
     **/
    public static void main(String[] args) {
        // TODO Auto-generated method stub
        new PackagingMain().setVisible(true);
    }

    /**
     This is the default constructor
     **/
    public PackagingMain() {
        super();
        initialize();
        pThis = this;
    }

    /**
     This method initializes this
     
     @return void
     **/
    private void initialize() {
        this.setSize(300, 357);
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
        this.setTitle("Packaging");
        this.setContentPane(getJContentPane());
        this.centerWindow();
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
     This method initializes jContentPane
     
     @return javax.swing.JPanel
     **/
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(6);
            gridLayout.setColumns(1);
            jContentPane = new JPanel();
            jContentPane.setLayout(gridLayout);
            jContentPane.add(getJButton5(), null);
            jContentPane.add(getJButton4(), null);
            jContentPane.add(getJButton3(), null);
            jContentPane.add(getJButton2(), null);
            jContentPane.add(getJButton1(), null);
            jContentPane.add(getJButton(), null);
        }
        return jContentPane;
    }

} //  @jve:decl-index=0:visual-constraint="125,31"
