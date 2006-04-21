/** @file
  Java class GuiPkgInstall is GUI for package installation.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging;


import javax.swing.JPanel;
import javax.swing.JFrame;
import javax.swing.JButton;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JTextField;

import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.Toolkit;

import java.awt.FlowLayout;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;

import java.awt.ComponentOrientation;
import java.io.File;
import java.util.Hashtable;

import javax.swing.SwingConstants;
import javax.swing.JProgressBar;
import javax.swing.filechooser.FileFilter;

import org.apache.xmlbeans.XmlException;


/**
 GUI for package installation. 
  
 @since PackageEditor 1.0
**/
public class GuiPkgInstall extends JFrame implements MouseListener {

    final static long serialVersionUID = 0;

    static JFrame frame;

    ///
    /// backup of "this". As we cannot use "this" to refer outer class inside inner class
    ///
    private JFrame pThis = null;

    private JFileChooser chooser = null;

    private JPanel jPanel = null;

    private JPanel jPanel1 = null;

    private JTextField jTextField = null;

    private JButton jButton = null;

    private JPanel jPanel2 = null;

    private JLabel jLabel1 = null;

    private JPanel jPanel4 = null;

    private JTextField jTextField1 = null;

    private JButton jButton1 = null;

    private JPanel jPanel5 = null;

    private JPanel jPanel6 = null;

    private JPanel jPanel7 = null;

    private JLabel jLabel2 = null;

    private JTextField jTextField2 = null;

    private JButton jButton2 = null;

    private JButton jButton3 = null;

    private JPanel jPanel3 = null;

    private JLabel jLabel = null;

    private JProgressBar jProgressBar = null;

    private JButton jButton4 = null;


    public GuiPkgInstall() {
        super();
        initialize();

    }

    /**
     GUI initialization
    **/
    private void initialize() {
        this.setSize(new java.awt.Dimension(454, 313));
        this.setContentPane(getJPanel());
        this.setTitle("Package Installation");
        this.addWindowListener(new GuiPkgInstallAdapter(this));
        this.setDefaultCloseOperation(javax.swing.WindowConstants.DO_NOTHING_ON_CLOSE);
        this.centerWindow();
        pThis = this;
    }

    /**
     make window appear center of screen
     
     @param intWidth
     @param intHeight
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
     This method initializes jPanel
     @return javax.swing.JPanel
    **/
    private JPanel getJPanel() {
        if (jPanel == null) {
            GridLayout gridLayout = new GridLayout();
            gridLayout.setRows(7);
            gridLayout.setColumns(1);
            jPanel = new JPanel();
            jPanel.setLayout(gridLayout);
            jPanel.add(getJPanel3(), null);
            jPanel.add(getJPanel1(), null);
            jPanel.add(getJPanel2(), null);
            jPanel.add(getJPanel4(), null);
            jPanel.add(getJPanel5(), null);
            jPanel.add(getJPanel6(), null);
            jPanel.add(getJPanel7(), null);
        }
        return jPanel;
    }

    /**
      This method initializes jPanel1	
     	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel1 = new JPanel();
            jPanel1.setLayout(flowLayout);
            jPanel1.add(getJTextField(), null);
            jPanel1.add(getJButton(), null);
        }
        return jPanel1;
    }

    /**
      This method initializes jTextField	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setHorizontalAlignment(javax.swing.JTextField.LEFT);
            jTextField.setPreferredSize(new java.awt.Dimension(350, 20));
        }
        return jTextField;
    }

    /**
     This method initializes jButton	
     	
     @return javax.swing.JButton	
    **/
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setHorizontalAlignment(javax.swing.SwingConstants.LEFT);
            jButton.setText("Browse");
            jButton.setComponentOrientation(java.awt.ComponentOrientation.LEFT_TO_RIGHT);
            jButton.setHorizontalTextPosition(javax.swing.SwingConstants.CENTER);
            jButton.setToolTipText("Where is the package?");
            jButton.setFont(new java.awt.Font("Dialog", java.awt.Font.BOLD, 12));

            jButton.setPreferredSize(new java.awt.Dimension(80, 20));
            jButton.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (chooser == null) {
                        chooser = new JFileChooser();
                    }
                    //
                    // disable multi-selection, you can only select one item each time.
                    //
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
                    chooser.setFileFilter(new PkgFileFilter("fdp"));
                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        File theFile = chooser.getSelectedFile();
                        jTextField.setText(theFile.getPath());
                        //
                        // set a default directory for installation (WORKSPACE\PackageFileName)
                        //
                        if (jTextField1.getText().length() > 0) {
                            int indexbegin = jTextField.getText().lastIndexOf(System.getProperty("file.separator"));
                            int indexend = jTextField.getText().lastIndexOf('.');
                            if (indexbegin >= 0 && indexend >= 0) {
                                jTextField2.setText(jTextField1.getText()
                                                    + jTextField.getText().substring(indexbegin, indexend));
                            } else {
                                JOptionPane.showMessageDialog(frame, "Wrong Path:" + jTextField.getText());
                            }
                        }
                    }
                }
            });
        }
        return jButton;
    }

    /**
      This method initializes jPanel2	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout1.setVgap(20);
            jLabel1 = new JLabel();
            jLabel1.setText("Enter Workspace Location");
            jLabel1.setComponentOrientation(java.awt.ComponentOrientation.UNKNOWN);
            jLabel1.setHorizontalTextPosition(javax.swing.SwingConstants.TRAILING);
            jLabel1.setHorizontalAlignment(javax.swing.SwingConstants.TRAILING);
            jPanel2 = new JPanel();
            jPanel2.setLayout(flowLayout1);
            jPanel2.add(jLabel1, null);
        }
        return jPanel2;
    }

    /**
      This method initializes jPanel4	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            FlowLayout flowLayout2 = new FlowLayout();
            flowLayout2.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel4 = new JPanel();
            jPanel4.setLayout(flowLayout2);
            jPanel4.add(getJTextField1(), null);
            jPanel4.add(getJButton1(), null);
        }
        return jPanel4;
    }

    /**
      This method initializes jTextField1	
      	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextField1() {
        if (jTextField1 == null) {
            jTextField1 = new JTextField();
            jTextField1.setPreferredSize(new java.awt.Dimension(350, 20));

        }
        //
        // default value is WORKSPACE environmental variable value
        //
        jTextField1.setText(System.getenv("WORKSPACE"));
        return jTextField1;
    }

    /**
      This method initializes jButton1	
      	
      @return javax.swing.JButton	
    **/
    private JButton getJButton1() {
        if (jButton1 == null) {
            jButton1 = new JButton();
            jButton1.setComponentOrientation(java.awt.ComponentOrientation.LEFT_TO_RIGHT);
            
            jButton1.setHorizontalAlignment(javax.swing.SwingConstants.LEADING);
            jButton1.setHorizontalTextPosition(javax.swing.SwingConstants.TRAILING);
            jButton1.setText("Browse");
            jButton1.setPreferredSize(new java.awt.Dimension(80, 20));
            jButton1.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    if (chooser == null) {
                        chooser = new JFileChooser();
                    }
                    //
                    // only directories can be selected for workspace location.
                    //
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {

                        File theFile = chooser.getSelectedFile();
                        jTextField1.setText(theFile.getPath());
                        //
                        // set a default directory for installation (WORKSPACE\PackageFileName)
                        //
                        if (jTextField.getText().length() > 0) {
                            int indexbegin = jTextField.getText().lastIndexOf(System.getProperty("file.separator"));
                            int indexend = jTextField.getText().lastIndexOf('.');
                            if (indexbegin >= 0 && indexend >= 0) {
                                jTextField2.setText(jTextField1.getText()
                                                    + jTextField.getText().substring(indexbegin, indexend));
                            } else {
                                JOptionPane.showMessageDialog(frame, "Wrong Path:" + jTextField.getText());
                            }
                        }
                    }

                }
            });
        }
        return jButton1;
    }

    /**
      This method initializes jButton4 
       
      @return javax.swing.JButton  
     **/
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
            
            jButton4.setHorizontalAlignment(SwingConstants.LEADING);
            jButton4.setHorizontalTextPosition(SwingConstants.TRAILING);
            jButton4.setText("Browse");
            jButton4.setPreferredSize(new Dimension(80, 20));
            jButton4.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    JFileChooser chooser = new JFileChooser(jTextField1.getText());
                    
                    chooser.setMultiSelectionEnabled(false);
                    chooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);

                    int retval = chooser.showOpenDialog(frame);
                    if (retval == JFileChooser.APPROVE_OPTION) {
                        //
                        // specify installation directory from file chooser 
                        //
                        File theFile = chooser.getSelectedFile();
                        jTextField2.setText(theFile.getPath());
                        
                    }
                }
            });
            
        }
        return jButton4;
    }
    /**
      This method initializes jPanel5	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel5() {
        if (jPanel5 == null) {
            FlowLayout flowLayout3 = new FlowLayout();
            flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
            flowLayout3.setVgap(20);
            jLabel2 = new JLabel();
            jLabel2.setComponentOrientation(java.awt.ComponentOrientation.UNKNOWN);
            jLabel2.setHorizontalTextPosition(javax.swing.SwingConstants.TRAILING);
            jLabel2.setText("Enter Installation  Location Within Workspace");
            jLabel2.setHorizontalAlignment(javax.swing.SwingConstants.TRAILING);
            jPanel5 = new JPanel();
            jPanel5.setLayout(flowLayout3);
            jPanel5.add(jLabel2, null);
        }
        return jPanel5;
    }

    /**
      This method initializes jPanel6	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel6() {
        if (jPanel6 == null) {
            FlowLayout flowLayout4 = new FlowLayout();
            flowLayout4.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel6 = new JPanel();
            jPanel6.setLayout(flowLayout4);
            jPanel6.add(getJTextField2(), null);
            jPanel6.add(getJButton4(), null);
        }
        return jPanel6;
    }

    /**
      This method initializes jPanel7	
      	
      @return javax.swing.JPanel	
     **/
    private JPanel getJPanel7() {
        if (jPanel7 == null) {
            FlowLayout flowLayout5 = new FlowLayout();
            flowLayout5.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanel7 = new JPanel();
            jPanel7.setLayout(flowLayout5);
            jPanel7.add(getJProgressBar(), null);
            jPanel7.add(getJButton2(), null);
            jPanel7.add(getJButton3(), null);
        }
        return jPanel7;
    }

    /**
      This method initializes jTextField2	
     	
      @return javax.swing.JTextField	
     **/
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new java.awt.Dimension(350, 20));
        }
        return jTextField2;
    }

    /**
      This method initializes jButton2	
      	
      @return javax.swing.JButton	
     **/
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new java.awt.Dimension(80, 20));
            jButton2.setText("Ok");
            jButton2.addMouseListener(new java.awt.event.MouseAdapter() {
                public void mouseClicked(java.awt.event.MouseEvent e) {
                    try {
                        //
                        // The installation directory must under workspace directory
                        //
                        locationcheck(jTextField.getText(), jTextField1.getText(), jTextField2.getText());
                    } catch (Exception ee) {
                        JOptionPane.showMessageDialog(frame, "Package Location Error!");
                        return;
                    }
                    
                    try {
                        //
                        // create a new FrameworkPkg object with user-selected package, current workspace location.
                        // install the package to dest dir from jTextField2
                        //
                        int i = new FrameworkPkg(jTextField.getText(), jTextField1.getText())
                                                                                             .install(jTextField2
                                                                                                                 .getText());
                        //
                        // the package is installed smoothly
                        //
                        if (i == 0) {
                            JOptionPane.showMessageDialog(frame, "Package" + jTextField.getText()
                                                                 + " Installed Successfully!");
                        }
                    } catch (BasePkgNotInstalled bpni) {
                        //
                        // exception no base package installed
                        //
                        JOptionPane
                                   .showMessageDialog(frame,
                                                      "The Edk package needs to be installed before installing any other packages.");
                    } catch (VerNotEqual vne) {
                        //
                        // show modal GUI PkgInstallTypeChooser with user selected package name, 
                        // current workspace location and the list of package info with same base name
                        //
                        ModalFrameUtil.showAsModal(new PkgInstallTypeChooser(jTextField.getText(),
                                                                             jTextField1.getText(), vne.getVersion()),
                                                   pThis);

                    } catch (GuidNotEqual gne) {
                        //
                        // show modal GUI PkgInstallTypeChooser with user selected package name, 
                        // current workspace location and the list of package info with same base name and version
                        //
                        ModalFrameUtil.showAsModal(new PkgInstallTypeChooser(jTextField.getText(),
                                                                             jTextField1.getText(), gne.getGuid()),
                                                   pThis);

                    } catch (SameAll sa) {
                        //
                        // the package with same (base, version, guid) already exists. confirm user action.
                        // quit or replace the original info. (So only one package info entry in db file that may be triple same)
                        //
                        int retVal = JOptionPane
                                                .showConfirmDialog(
                                                                   frame,
                                                                   "Package already exists. Would you like to replace it?",
                                                                   "Package Installation", JOptionPane.YES_NO_OPTION);
                        if (retVal == JOptionPane.YES_OPTION) {
                            String installDir = sa.getVersion().listIterator().next().getPathArray(0).getStringValue();
                            try {
                                ForceInstallPkg f = new ForceInstallPkg(jTextField.getText(), jTextField1.getText());
                                //
                                // Get old packag info to meet the calling parameter layout of DbFileContents.updatePkgInfo
                                // ForceInstallPkg will call it after installation to update package info.
                                //
                                f.setOldVersion(sa.getVersion().listIterator().next().getVersionArray(0));
                                f.setOldGuid(sa.getVersion().listIterator().next().getGuidArray(0).getStringValue());
                                int i = f.install(jTextField1.getText() + System.getProperty("file.separator") + installDir);
                                if (i == 0) {
                                    JOptionPane.showMessageDialog(frame, "Package" + jTextField.getText()
                                                                         + " Installed Successfully!");
                                }
                            } catch (Exception sae) {
                                System.out.println(sae.toString());
                                JOptionPane.showMessageDialog(frame, "Extraction Error!");
                            }
                        }
                        return;
                    } catch (XmlException xmle) {
                        System.out.println(xmle.toString());
                        JOptionPane.showMessageDialog(frame, "Package Format Error!");
                    } catch (DirSame ds) {
                        //
                        // You cannot install different packages into the same directory.
                        //
                        System.out.println(ds.toString());
                        JOptionPane.showMessageDialog(frame,
                                                      "Another Package Exists There, Please Select Another Directory!");
                    } catch (Exception ext) {
                        System.out.println(ext.toString());
                        JOptionPane.showMessageDialog(frame, "Extraction Error!");
                    }
                }
            });
        }
        return jButton2;
    }

    /**
     * This method initializes jButton3	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton3() {
        if (jButton3 == null) {
            jButton3 = new JButton();
            jButton3.setPreferredSize(new java.awt.Dimension(80, 20));
            jButton3.setText("Cancel");
            jButton3.addMouseListener(this);
        }
        return jButton3;

    }

    /**
      This method initializes jPanel3	
      	
      @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            jLabel = new JLabel();
            jLabel.setComponentOrientation(ComponentOrientation.UNKNOWN);
            jLabel.setHorizontalTextPosition(SwingConstants.TRAILING);
            jLabel.setText("Enter Package Location");
            jLabel.setHorizontalAlignment(SwingConstants.TRAILING);
            FlowLayout flowLayout6 = new FlowLayout();
            flowLayout6.setVgap(20);
            flowLayout6.setAlignment(FlowLayout.LEFT);
            jPanel3 = new JPanel();
            jPanel3.setLayout(flowLayout6);
            jPanel3.add(jLabel, null);
        }
        return jPanel3;
    }

    /**
     check user input validity
    
     @param s package path
     @param s1 workspace path
     @param s2 installation path
     @throws Exception
    **/
    private void locationcheck(String s, String s1, String s2) throws Exception {
        if (new File(s).isFile() == false)
            throw new Exception();
        if (new File(s1).isDirectory() == false)
            throw new Exception();
        if (s2.startsWith(s1) == false)
            throw new Exception();
    }

 
    public void mouseClicked(MouseEvent arg0) {
        // TODO Auto-generated method stub
        int retVal = JOptionPane.showConfirmDialog(frame, "Are you sure to exit?", "Quit", JOptionPane.YES_NO_OPTION);
        if (retVal == JOptionPane.YES_OPTION) {
            this.dispose();
        }
        return;
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

    /**
      This method initializes jProgressBar	
      	
      @return javax.swing.JProgressBar	
     **/
    private JProgressBar getJProgressBar() {
        if (jProgressBar == null) {
            jProgressBar = new JProgressBar();
            jProgressBar.setComponentOrientation(java.awt.ComponentOrientation.LEFT_TO_RIGHT);
            jProgressBar.setVisible(false);
        }
        return jProgressBar;
    }

   

} //  @jve:decl-index=0:visual-constraint="24,82"

/**
Derived from WindowAdapter, Event adapter for windowClosing event
 
@since PackageEditor 1.0
**/
class GuiPkgInstallAdapter extends WindowAdapter {

   private JFrame frame = null;

   GuiPkgInstallAdapter(JFrame f) {
       super();
       frame = f;
   }

   /* (non-Javadoc)
    * @see java.awt.event.WindowAdapter#windowClosing(java.awt.event.WindowEvent)
    */
   @Override
   public void windowClosing(WindowEvent arg0) {
       // TODO Auto-generated method stub
       super.windowClosing(arg0);
       int retVal = JOptionPane.showConfirmDialog(frame, "Are you sure to exit?", "Quit",
                                                  JOptionPane.YES_NO_OPTION);
       if (retVal == JOptionPane.YES_OPTION) {
           frame.dispose();
       }

   }

}

/**
 Filter out some specific type of file
 
 @since PackageEditor 1.0
**/
class PkgFileFilter extends FileFilter {

    ///
    /// hash table used to store filter info.
    ///
    private Hashtable<String, String> filters = null;
    

    public PkgFileFilter() {
        this.filters = new Hashtable<String, String>();
    }
    
    /**
       Create filter and add extension to hash table
        
       @param extension file extension string (e.g. "exe")
    **/
    public PkgFileFilter(String extension) {
        this();
        if(extension!=null) {
            addExtension(extension);
        }
    
    }

    public PkgFileFilter(String[] fileFilters) {
        this();
        int i = 0; 
        while (i < fileFilters.length) {
            // add filters one by one
            addExtension(fileFilters[i]);
            i++;
        }
    
    }

 
    /* (non-Javadoc)
     * @see javax.swing.filechooser.FileFilter#accept(java.io.File)
     */
    public boolean accept(File f) {
        if (f != null) {
            if (f.isDirectory()) {
                return true;
            }
            
            if (getExtension(f) != null && filters.get(getExtension(f)) != null) {
                return true;
            }
        }
        return false;
    }

    
    /**
     Get the extension string of file
     
     @param f target file
     @return String
    **/
    public String getExtension(File f) {
        if (f != null) {
            int i = f.getName().lastIndexOf('.');
            if (i>0 && i<f.getName().length()-1) {
                return f.getName().substring(i+1).toLowerCase();
            }
        }
        return null;
    }

    
    /**
     Add extension info into hash table
     
     @param ext extension string for file name
    **/
    public void addExtension(String ext) {
        if (filters == null) {
            filters = new Hashtable<String, String>(5);
        }
        filters.put(ext.toLowerCase(), "ext");
   
    }
    
    /* (non-Javadoc)
     * @see javax.swing.filechooser.FileFilter#getDescription()
     */
    public String getDescription() {
        return null;
    }

}

