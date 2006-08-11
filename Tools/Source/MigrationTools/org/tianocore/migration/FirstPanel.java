package org.tianocore.migration;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.util.*;
import javax.swing.*;

public class FirstPanel extends JPanel implements ActionListener, UI {
	/**
	 *  Define class Serial Version UID
	 */
	private static final long serialVersionUID = 207759413522910399L;
	
	private String modulepath;
	
	private JButton moduleButton , goButton;
	private JTextField moduletext;
	private JTextArea log;
	private JFileChooser fc;
	private JCheckBox filebox, screenbox;
	
	private boolean tofile = true, toscreen = true;
	private PrintWriter logfile;

	FirstPanel() throws Exception {
		goButton = new JButton("Go");
		goButton.addActionListener(this);
		goButton.setActionCommand("go");
		
		moduleButton = new JButton("Choose ModulePath");
		moduleButton.addActionListener(this);

		moduletext = new JTextField(30);
		
		filebox = new JCheckBox("Output to logfile", true);
		screenbox = new JCheckBox("Specify logfile", false);
		
        JPanel modulePanel = new JPanel();
        modulePanel.add(moduleButton);
        modulePanel.add(moduletext);
        modulePanel.add(filebox);
        modulePanel.add(screenbox);
        modulePanel.add(goButton);
        add(modulePanel);

        log = new JTextArea(50,25);
        log.setMargin(new Insets(5,5,5,5));
        log.setEditable(false);
        JScrollPane logScrollPane = new JScrollPane(log);
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

	//---------------------------------------------------------------------------------------//
	
	/*
	public boolean getOption(String item) {
		if (item.matches("file")) {
		}
	}
	*/
	
    public void actionPerformed(ActionEvent e) {
        if ( e.getSource() == moduleButton ) {
        	int ret = fc.showOpenDialog(this);
        	if (ret == JFileChooser.APPROVE_OPTION) {
        		modulepath = fc.getSelectedFile().getAbsolutePath();
        		moduletext.setText(modulepath);
                log.append("ModulePath: " + modulepath + "\n");
        	}
        }
        if ( e.getSource() == goButton ) {
        	try {
        		logfile = new PrintWriter(new BufferedWriter(new FileWriter(modulepath + File.separator + "migration.log")));
        		println("Project MsaGen");
        		println("Copyright (c) 2006, Intel Corporation");
        		new ModuleInfo(modulepath, this, new Database());
        		logfile.flush();
        	} catch (Exception en) {
        		println(en.getMessage());
        	}
        }
    }
    
    public void itemStateChanged(ItemEvent e) {
    	if (e.getStateChange() == ItemEvent.DESELECTED) {
    		System.out.println("changed");
    	}
    }

    //---------------------------------------------------------------------------------------//
    
    public static void init() throws Exception {
    	
    	//UIManager.setLookAndFeel(UIManager.getCrossPlatformLookAndFeelClassName());
    	UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
    	//UIManager.setLookAndFeel("javax.swing.plaf.metal.MetalLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.windows.WindowsLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.gtk.GTKLookAndFeel");
    	//UIManager.setLookAndFeel("com.sun.java.swing.plaf.motif.MotifLookAndFeel");
    	
		JFrame frame = new JFrame("MigrationTools");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        FirstPanel fp = new FirstPanel();
		fp.setLayout(new BoxLayout(fp, BoxLayout.Y_AXIS));
		fp.setOpaque(true);
        frame.setContentPane(fp);

		frame.setSize(800,600);
		frame.setVisible(true);
    }
}