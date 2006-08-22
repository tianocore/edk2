/** @file
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.migration;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;

public final class FirstPanel extends JPanel implements ActionListener, ItemListener, UI {
	/**
	 *  Define class Serial Version UID
	 */
	private static final long serialVersionUID = 207759413522910399L;
	
	private String modulepath;
	private ModuleInfo mi;
	
	private JButton moduleButton, goButton, msaEditorButton, criticButton;
	private JTextField moduletext;
	private JTextArea log;
	private JFileChooser fc;
	private JCheckBox filebox, screenbox, mibox, criticbox, defaultpathbox;
	
	private boolean tofile = true, toscreen = true;
	private PrintWriter logfile;

	FirstPanel() throws Exception {
        GridBagLayout gridbag = new GridBagLayout();
        setLayout(gridbag);
        
		GridBagConstraints cst = new GridBagConstraints();
		
		goButton = new JButton("Go");
		goButton.addActionListener(this);
		goButton.setActionCommand("go");
		
		moduleButton = new JButton("Choose ModulePath");
		moduleButton.addActionListener(this);

		msaEditorButton = new JButton("MsaEditor");
		msaEditorButton.addActionListener(this);
		
		criticButton = new JButton("Critic");
		criticButton.addActionListener(this);
		
		moduletext = new JTextField(30);
		
		filebox = new JCheckBox("Output to logfile", true);
		filebox.addItemListener(this);
		
		screenbox = new JCheckBox("Specify logfile", false);
		screenbox.addItemListener(this);
		
		mibox = new JCheckBox("Print ModuleInfo", false);
		mibox.addItemListener(this);
		MigrationTool.printModuleInfo = false;
		
		criticbox = new JCheckBox("Run Critic", true);
		criticbox.addItemListener(this);
		MigrationTool.doCritic = true;
		
		defaultpathbox = new JCheckBox("Use Default Output Path", true);
		defaultpathbox.addItemListener(this);
		
        JPanel modulePanel = new JPanel();
        modulePanel.add(moduleButton);
        modulePanel.add(moduletext);
        modulePanel.add(goButton);
        //modulePanel.add(msaEditorButton);
        cst.gridx = 0;
        cst.gridy = 0;
        //cst.gridwidth = GridBagConstraints.REMAINDER;
        gridbag.setConstraints(modulePanel, cst);
        add(modulePanel);

        cst.gridx = 1;
        cst.gridy = 0;
        gridbag.setConstraints(criticButton, cst);
        add(criticButton);
        
        JPanel checkboxPanel = new JPanel();
        checkboxPanel.setLayout(new BoxLayout(checkboxPanel, BoxLayout.Y_AXIS));
        checkboxPanel.add(filebox);
        checkboxPanel.add(screenbox);
        checkboxPanel.add(mibox);
        checkboxPanel.add(criticbox);
        checkboxPanel.add(defaultpathbox);
        cst.gridx = 1;
        cst.gridy = 1;
        //cst.gridheight = 2;
        gridbag.setConstraints(checkboxPanel, cst);
        add(checkboxPanel);
        
        log = new JTextArea(10,20);
        log.setMargin(new Insets(5,5,5,5));
        log.setEditable(false);
        JScrollPane logScrollPane = new JScrollPane(log);
        cst.gridx = 0;
        cst.gridy = 1;
        cst.fill = GridBagConstraints.BOTH;
        gridbag.setConstraints(logScrollPane, cst);
        add(logScrollPane);
        
		fc = new JFileChooser();
        fc.setFileSelectionMode(JFileChooser.FILES_AND_DIRECTORIES);
	}
	
	//---------------------------------------------------------------------------------------//
	
	public boolean yesOrNo(String question) {
		return JOptionPane.showConfirmDialog(this, question, "Yes or No", JOptionPane.YES_NO_OPTION) == JOptionPane.YES_OPTION;
	}
	
	public void print(String message) {
		if (toscreen == true) {
	        log.append(message);
	        System.out.print(message);
		}
		if (tofile == true) {
			logfile.append(message);
		}
	}
	
	public void println(String message) {
		print(message + "\n");
	}

	public void println(Set<String> hash) {
		if (toscreen == true) {
	        log.append(hash + "\n");
	        System.out.println(hash);
		}
		if (tofile == true) {
			logfile.append(hash + "\n");
		}
	}

	public String choose(String message, Object[] choicelist) {
		return (String)JOptionPane.showInputDialog(this, message,"Choose",JOptionPane.PLAIN_MESSAGE,null,choicelist,choicelist[0]);
	}
	
	public String getInput(String message) {
		return (String)JOptionPane.showInputDialog(message);
	}

	//---------------------------------------------------------------------------------------//

	public String getFilepath() {
		if (fc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
			log.append(fc.getSelectedFile().getAbsolutePath() + "\n");
			return fc.getSelectedFile().getAbsolutePath();
		}
		return null;
	}

	//---------------------------------------------------------------------------------------//

    public void actionPerformed(ActionEvent e) {
        if ( e.getSource() == moduleButton ) {
        	modulepath = getFilepath();
        }
        if ( e.getSource() == goButton ) {
        	try {
        		logfile = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "migration.log")));
        		ModuleInfo.triger(modulepath);
        		logfile.flush();
        	} catch (Exception en) {
        		println(en.getMessage());
        	}
        }
        if ( e.getSource() == msaEditorButton) {
        	try {
            	MsaTreeEditor.init(mi, this);
        	} catch (Exception en) {
        		println(en.getMessage());
        	}
        }
        if ( e.getSource() == criticButton) {
        	try {
        		Critic.fireAt(modulepath);
        	} catch (Exception en) {
        		println(en.getMessage());
        	}
        }
    }
    
    public void itemStateChanged(ItemEvent e) {
    	if (e.getSource() == filebox) {
        	if (e.getStateChange() == ItemEvent.DESELECTED) {
        		System.out.println("filebox DESELECTED");
        	} else if (e.getStateChange() == ItemEvent.SELECTED) {
        		System.out.println("filebox SELECTED");
        	}
    	} else if (e.getSource() == screenbox) {
        	if (e.getStateChange() == ItemEvent.DESELECTED) {
        		System.out.println("screenbox DESELECTED");
        	} else if (e.getStateChange() == ItemEvent.SELECTED) {
        		System.out.println("screenbox SELECTED");
        	}
    	} else if (e.getSource() == mibox) {
        	if (e.getStateChange() == ItemEvent.DESELECTED) {
        		MigrationTool.printModuleInfo = false;
        	} else if (e.getStateChange() == ItemEvent.SELECTED) {
        		MigrationTool.printModuleInfo = true;
        	}
    	} else if (e.getSource() == criticbox) {
        	if (e.getStateChange() == ItemEvent.DESELECTED) {
        		MigrationTool.doCritic = false;
        		System.out.println("criticbox DESELECTED");
        	} else if (e.getStateChange() == ItemEvent.SELECTED) {
        		MigrationTool.doCritic = true;
        		System.out.println("criticbox SELECTED");
        	}
    	} else if (e.getSource() == defaultpathbox) {
        	if (e.getStateChange() == ItemEvent.DESELECTED) {
        		System.out.println("defaultpathbox DESELECTED");
        	} else if (e.getStateChange() == ItemEvent.SELECTED) {
        		System.out.println("defaultpathbox SELECTED");
        	}
    	}
    }

    //---------------------------------------------------------------------------------------//
    
    public static FirstPanel init() throws Exception {
    	
    	//UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
    	UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    	//UIManager.setLookAndFeel("javax.swing.plaf.metal.MetalLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.windows.WindowsLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.gtk.GTKLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.motif.MotifLookAndFeel");
    	
		JFrame frame = new JFrame("MigrationTools");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        FirstPanel fp = new FirstPanel();
		//fp.setLayout(new GridBagLayout());
		//fp.setLayout(new BoxLayout(fp, BoxLayout.Y_AXIS));
		fp.setOpaque(true);
        frame.setContentPane(fp);

		frame.pack();
		frame.setVisible(true);
		
		return fp;
    }
}