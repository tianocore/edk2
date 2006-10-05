package org.tianocore.frameworkwizard;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JTextPane;

import org.tianocore.frameworkwizard.common.DataType;
import javax.swing.JButton;


public class ToolChainConfigHelp extends JFrame implements ActionListener {

    ///
    ///
    ///
    private static final long serialVersionUID = -6315081029366587222L;

    private JPanel jContentPane = null;

    private String helpContent = "";

    private JTextPane jTextPane = null;

    private JButton jButtonClose = null;
    
    private static ToolChainConfigHelp tcch = null;

    /**
     * This method initializes jTextPane	
     * 	
     * @return javax.swing.JTextPane	
     */
    private JTextPane getJTextPane() {
        if (jTextPane == null) {
            jTextPane = new JTextPane();
            jTextPane.setBounds(new java.awt.Rectangle(10,10,600,420));
            jTextPane.setBackground(new java.awt.Color(238,238,238));
            jTextPane.setEditable(false);
            helpContent = helpContent
            + "The template for the Property is: TARGET_TAGNAME_ARCH_COMMAND_ATTR" + DataType.UNIX_LINE_SEPARATOR
            + "The Value, is either a full path, full path and filename or a reserved word." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "TARGET  - DEBUG and RELEASE are predefined, however the user may define one or more of their own TARGET types in this file." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "TAGNAME - HOST, MSFT, GCC, INTC are predefined, however the user may define one or more of their own TAGNAME keywords in this file." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "ARCH    - EDK II supports IA32, X64, IPF and EBC at this time." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "COMMAND - Predefined command codes are listed in the tools_def.txt file, however the user can specify additional command codes for their one, non-standard tools." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "ATTR    - Predefined Attributes are listed in the tools_def.txt file." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "NOTE: The TAGNAME: HOST is reserved and MUST be defined in order to build the included Tiano tools from their C source files.  These tools have been built and tested using both Microsoft and GCC tool chains." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR
            + "NOTE: The \"*\" symbol may be used as a wildcard character in most of these fields, refer to the tools_def.txt and the \"EDK II Build and Packaging Architecture Specification\" for more details." + DataType.UNIX_LINE_SEPARATOR
            + DataType.UNIX_LINE_SEPARATOR;
            
            jTextPane.setText(helpContent);
        }
        return jTextPane;
    }

    /**
     * This method initializes jButtonClose	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonClose() {
        if (jButtonClose == null) {
            jButtonClose = new JButton();
            jButtonClose.setBounds(new java.awt.Rectangle(480,450,80,20));
            jButtonClose.setText("Close");
            jButtonClose.addActionListener(this);
        }
        return jButtonClose;
    }

    public static ToolChainConfigHelp getInstance() {
        if (tcch == null) {
            tcch = new ToolChainConfigHelp();
        }
        return tcch;
    }
    
    /**
     
     @param args
     
     **/
    public static void main(String[] args) {
        ToolChainConfigHelp tcch = new ToolChainConfigHelp();
        tcch.setVisible(true);
    }

    /**
     * This is the default constructor
     */
    public ToolChainConfigHelp() {
        super();
        initialize();
    }

    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(625, 520);
        this.setResizable(false);
        this.setTitle("How to Modify a Tool Chain Configuration");
        this.setContentPane(getJContentPane());
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(null);
            jContentPane.add(getJTextPane(), null);
            jContentPane.add(getJButtonClose(), null);
        }
        return jContentPane;
    }

    public void actionPerformed(ActionEvent arg0) {
        if (arg0.getSource() == jButtonClose) {
            this.dispose();
        }
    }
}
