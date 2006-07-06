package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;

import javax.swing.ButtonGroup;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableModel;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import javax.swing.JCheckBox;
import java.awt.FlowLayout;
import javax.swing.JRadioButton;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JTextField;

public class FpdDynamicPcdBuildDefinitions extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private JPanel jContentPane = null;
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JPanel jPanel2 = null;
    private JScrollPane jScrollPane = null;
    private JTable jTable = null;
    private DynPcdTableModel model = null; 
    private DynPcdTableModel model1 = null;
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;
    private JPanel jPanel3 = null;
    private JCheckBox jCheckBox = null;
    private JPanel jPanel4 = null;
    private JRadioButton jRadioButton = null;
    private JRadioButton jRadioButton1 = null;
    private JScrollPane jScrollPane1 = null;
    private JTable jTable1 = null;
    private JButton jButton = null;
    private JLabel jLabel = null;
    private JTextField jTextField = null;
    private JLabel jLabel1 = null;
    private JTextField jTextField1 = null;
    private JLabel jLabel2 = null;
    private JLabel jLabel3 = null;
    private JTextField jTextField2 = null;
    private JLabel jLabel4 = null;
    private JTextField jTextField3 = null;
    private JTextField jTextField4 = null;
    private JLabel jLabel5 = null;
    private JTextField jTextField5 = null;
    private JRadioButton jRadioButton2 = null;
    private ButtonGroup bg = new ButtonGroup();
    private JLabel jLabel6 = null;
    /**
     * This is the default constructor
     */
    public FpdDynamicPcdBuildDefinitions() {
        super();
        initialize();
    }

    public FpdDynamicPcdBuildDefinitions(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd){
        this();
        init(fpd);
    }
    
    public FpdDynamicPcdBuildDefinitions(OpeningPlatformType opt) {
        this(opt.getXmlFpd());
        docConsole = opt;
    }
    
    public void init(PlatformSurfaceAreaDocument.PlatformSurfaceArea fpd) {
        if (ffc == null) {
            ffc = new FpdFileContents(fpd);
        }
        String[][] saa = new String[ffc.getDynamicPcdBuildDataCount()][5];
        ffc.getDynamicPcdBuildData(saa);
        for (int i = 0; i < saa.length; ++i) {
            model.addRow(saa[i]);
        }
        
        saa = new String[ffc.getPlatformDefsSkuInfoCount()][2];
        ffc.getPlatformDefsSkuInfos(saa);
        for (int i = 0; i < saa.length; ++i) {
            model1.addRow(saa[i]);
        }
        
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(661, 558);
        this.setTitle("Dynamic PCD Build Definitions");
        this.setContentPane(getJContentPane());
        this.setVisible(true);
    }

    /**
     * This method initializes jContentPane
     * 
     * @return javax.swing.JPanel
     */
    private JPanel getJContentPane() {
        if (jContentPane == null) {
            jContentPane = new JPanel();
            jContentPane.setLayout(new BorderLayout());
            jContentPane.add(getJPanel(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanel1(), java.awt.BorderLayout.CENTER);
            jContentPane.add(getJPanel2(), java.awt.BorderLayout.SOUTH);
        }
        return jContentPane;
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel() {
        if (jPanel == null) {
            jPanel = new JPanel();
        }
        return jPanel;
    }

    /**
     * This method initializes jPanel1	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel1() {
        if (jPanel1 == null) {
            jPanel1 = new JPanel();
            jPanel1.add(getJScrollPane(), null);
            jPanel1.add(getJPanel3(), null);
            jPanel1.add(getJPanel4(), null);
        }
        return jPanel1;
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            jPanel2 = new JPanel();
        }
        return jPanel2;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(600,200));
            jScrollPane.setViewportView(getJTable());
        }
        return jScrollPane;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable() {
        if (jTable == null) {
            model = new DynPcdTableModel();
            model.addColumn("CName");
            model.addColumn("Token");
            model.addColumn("TokenSpaceGuid");
            model.addColumn("MaxDatumSize");
            model.addColumn("DatumType");
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            jTable.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int selectedRow = lsm.getMinSelectionIndex();
                        
                        displayDetails(selectedRow);
                    }
                }
            });
        }
        return jTable;
    }
    //
    // should display default sku info here, as no selection event of table1 will be triggered when change selection of rows in table. 
    //
    private void displayDetails(int i) {
        jTable1.changeSelection(0, 1, false, false);
        int skuCount = ffc.getDynamicPcdSkuInfoCount(i);
        String defaultVal = ffc.getDynamicPcdBuildDataValue(i);
        if (defaultVal != null) {
            jRadioButton2.setSelected(true);
            jTextField5.setText(defaultVal);
            if ( skuCount == 1) {
                jCheckBox.setSelected(false);
            }
            else{
                jCheckBox.setSelected(true);
            }
        }
        
        else if (ffc.getDynamicPcdBuildDataVpdOffset(i) != null) {
            jRadioButton1.setSelected(true);
            jTextField4.setText(ffc.getDynamicPcdBuildDataVpdOffset(i));
            if (skuCount ==1) {
                jCheckBox.setSelected(false);
                
            }
            else{
                jCheckBox.setSelected(true);
            }
        }
        else {
            jRadioButton.setSelected(true);
            String[][] saa = new String[ffc.getDynamicPcdSkuInfoCount(i)][7];
            ffc.getDynamicPcdSkuInfos(i, saa);
            jTextField.setText(saa[0][1]);
            jTextField1.setText(saa[0][2]);
            jTextField2.setText(saa[0][3]);
            jTextField3.setText(saa[0][4]);
            if (skuCount ==1) {
                jCheckBox.setSelected(false);
            }
            else{
                jCheckBox.setSelected(true);
            }
        }
        
    }
    
    private void displaySkuInfoDetails(int i) {
        int pcdSelected = jTable.getSelectedRow();
        if (pcdSelected < 0) {
            return;
        }
        
        String[][] saa = new String[ffc.getDynamicPcdSkuInfoCount(pcdSelected)][7];
        ffc.getDynamicPcdSkuInfos(pcdSelected, saa);
        
        if (saa[i][5] != null){
            jRadioButton1.setSelected(true);
            jTextField4.setText(saa[i][5]);
        } 
        
        else if (saa[i][1] != null) {
            jRadioButton.setSelected(true);
            jTextField.setText(saa[i][1]);
            jTextField1.setText(saa[i][2]);
            jTextField2.setText(saa[i][3]);
            jTextField3.setText(saa[i][4]);
        }
        
        else{
            jRadioButton2.setSelected(true);
            jTextField5.setText(saa[i][6]);
        }
        
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel3 = new JPanel();
            jPanel3.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanel3.setLayout(flowLayout);
            jPanel3.setPreferredSize(new java.awt.Dimension(600,100));
            jPanel3.add(getJCheckBox(), null);
            jPanel3.add(getJScrollPane1(), null);
            jPanel3.add(getJButton(), null);
        }
        return jPanel3;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBox() {
        if (jCheckBox == null) {
            jCheckBox = new JCheckBox();
            jCheckBox.setText("SKU Enable");
            jCheckBox.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    jTable1.setEnabled(jCheckBox.isSelected());
                }
            });
        }
        return jCheckBox;
    }

  			
  /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            jLabel5 = new JLabel();
            jLabel5.setPreferredSize(new java.awt.Dimension(80,20));
            jLabel5.setText("VPD Offset");
            jLabel4 = new JLabel();
            jLabel4.setPreferredSize(new java.awt.Dimension(100,20));
            jLabel4.setText("HII Default Value");
            jLabel3 = new JLabel();
            jLabel3.setText("Variable Offset");
            jLabel3.setPreferredSize(new java.awt.Dimension(90,20));
            jLabel2 = new JLabel();
            jLabel2.setText("                           ");
            jLabel1 = new JLabel();
            jLabel1.setText("Variable GUID");
            jLabel1.setPreferredSize(new java.awt.Dimension(100,20));
            jLabel = new JLabel();
            jLabel.setText("Variable Name");
            jLabel.setToolTipText("");
            jLabel.setPreferredSize(new java.awt.Dimension(90,20));
            FlowLayout flowLayout1 = new FlowLayout();
            flowLayout1.setAlignment(java.awt.FlowLayout.LEFT);
            jPanel4 = new JPanel();
            jPanel4.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.LOWERED));
            jPanel4.setLayout(flowLayout1);
            jPanel4.setPreferredSize(new java.awt.Dimension(600,120));
            jPanel4.add(getJRadioButton(), null);
            jPanel4.add(jLabel, null);
            jPanel4.add(getJTextField(), null);
            jPanel4.add(jLabel1, null);
            jPanel4.add(getJTextField1(), null);
            jPanel4.add(jLabel2, null);
            jPanel4.add(jLabel3, null);
            jPanel4.add(getJTextField2(), null);
            jPanel4.add(jLabel4, null);
            jPanel4.add(getJTextField3(), null);
            jPanel4.add(getJRadioButton1(), null);
            jPanel4.add(jLabel5, null);
            jPanel4.add(getJTextField4(), null);
            jLabel6 = new JLabel();
            jLabel6.setText("                           ");
            jPanel4.add(jLabel6, null);
			jLabel.setEnabled(false);
			jLabel1.setEnabled(false);
			jLabel4.setEnabled(false);
			jLabel3.setEnabled(false);
			jLabel5.setEnabled(false);
			jPanel4.add(getJRadioButton2(), null);
			jPanel4.add(getJTextField5(), null);
            bg.add(jRadioButton);
            bg.add(jRadioButton1);
        }			

        return jPanel4;
    }

    /**
     * This method initializes jRadioButton	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton() {
        if (jRadioButton == null) {
            jRadioButton = new JRadioButton();
            jRadioButton.setText("HII Enable");
            jRadioButton.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    boolean selected = jRadioButton.isSelected();
                    jLabel.setEnabled(selected);
                    jLabel1.setEnabled(selected);
                    jLabel2.setEnabled(selected);
                    jLabel3.setEnabled(selected);
                    jLabel4.setEnabled(selected);
                    jTextField.setEnabled(selected);
                    jTextField1.setEnabled(selected);
                    jTextField2.setEnabled(selected);
                    jTextField3.setEnabled(selected);
                }
            });
        }
        return jRadioButton;
    }

    /**
     * This method initializes jRadioButton1	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton1() {
        if (jRadioButton1 == null) {
            jRadioButton1 = new JRadioButton();
            jRadioButton1.setText("VPD Enable");
            jRadioButton1.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    boolean selected = jRadioButton1.isSelected();
                    jTextField4.setEnabled(selected);
                    jLabel5.setEnabled(selected);
                }
            });
        }
        return jRadioButton1;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setPreferredSize(new java.awt.Dimension(300,80));
            jScrollPane1.setViewportView(getJTable1());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable1() {
        if (jTable1 == null) {
            model1 = new DynPcdTableModel();
            jTable1 = new JTable(model1);
            model1.addColumn("SKU ID");
            model1.addColumn("SKU Name");
            
            jTable1.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable1.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (jTable.getSelectedRow() < 0) {
                        return;
                    }
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int selected = lsm.getMinSelectionIndex();
                        displaySkuInfoDetails(selected);
                    }
                }
            });
        }
        return jTable1;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(180,20));
            jButton.setText(" Update SKU Information");
            jButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int pcdSelected = jTable.getSelectedRow();
                    if (pcdSelected < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    updateSkuInfo(pcdSelected);
                    
                }
            });
        }
        return jButton;
    }

    private void updateSkuInfo (int pcdSelected) {
        int skuCount = ffc.getDynamicPcdSkuInfoCount(pcdSelected);
        
        String varName = null;
        String varGuid = null;
        String varOffset = null;
        String hiiDefault = null;
        String value = null;
        String vpdOffset = null;
        if (jRadioButton.isSelected()) {
            varName = jTextField.getText();
            varGuid = jTextField1.getText();
            varOffset = jTextField2.getText();
            hiiDefault = jTextField3.getText();
        }
        if (jRadioButton1.isSelected()) {
            vpdOffset = jTextField4.getText();
        }
        if (jRadioButton2.isSelected()) {
            value = jTextField5.getText();
        }
        //
        // SKU disabled. only modify data for default SKU.
        //
        if (!jCheckBox.isSelected()) {
            if (true) {
                ffc.removeDynamicPcdBuildDataSkuInfo(pcdSelected);
                if (jRadioButton.isSelected()) {
                    ffc.genDynamicPcdBuildDataSkuInfo("0", varName, varGuid, varOffset, hiiDefault, null, null, pcdSelected);
                }
                else if (jRadioButton1.isSelected()){
                    ffc.genDynamicPcdBuildDataSkuInfo("0", null, null, null, null, vpdOffset, null, pcdSelected);
                }
                else{
                    ffc.genDynamicPcdBuildDataSkuInfo("0", null, null, null, null, null, value, pcdSelected);
                }
            }
        }
        //
        // SKU Enabled, need add data to all SKUs.
        //
        if (jCheckBox.isSelected()) {
            if (skuCount == 1) {
                
                for (int i = 1; i < jTable1.getRowCount(); ++i) {
                    ffc.genDynamicPcdBuildDataSkuInfo(jTable1.getValueAt(i, 0)+"", varName, varGuid, varOffset, hiiDefault, vpdOffset, value, pcdSelected);
                }
            }
            else {
                int row = jTable1.getSelectedRow();
                if (row < 0) {
                    return;
                }
                ffc.updateDynamicPcdBuildDataSkuInfo(jTable1.getValueAt(row, 0)+"", varName, varGuid, varOffset, hiiDefault, vpdOffset, value, pcdSelected);
            }
        }
    }
    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setPreferredSize(new java.awt.Dimension(150,20));
            jTextField.setEnabled(false);
        }
        return jTextField;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField1() {
        if (jTextField1 == null) {
            jTextField1 = new JTextField();
            jTextField1.setPreferredSize(new java.awt.Dimension(150,20));
            jTextField1.setEnabled(false);
        }
        return jTextField1;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField2() {
        if (jTextField2 == null) {
            jTextField2 = new JTextField();
            jTextField2.setPreferredSize(new java.awt.Dimension(150,20));
            jTextField2.setEnabled(false);
        }
        return jTextField2;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField3() {
        if (jTextField3 == null) {
            jTextField3 = new JTextField();
            jTextField3.setPreferredSize(new java.awt.Dimension(150,20));
            jTextField3.setEnabled(false);
        }
        return jTextField3;
    }

    /**
     * This method initializes jTextField4	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField4() {
        if (jTextField4 == null) {
            jTextField4 = new JTextField();
            jTextField4.setPreferredSize(new java.awt.Dimension(150,20));
            jTextField4.setEnabled(false);
        }
        return jTextField4;
    }

    /**
     * This method initializes jTextField5	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField5() {
        if (jTextField5 == null) {
            jTextField5 = new JTextField();
            jTextField5.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextField5;
    }

    /**
     * This method initializes jRadioButton2	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButton2() {
        if (jRadioButton2 == null) {
            jRadioButton2 = new JRadioButton();
            jRadioButton2.setText("Default PCD Value");
            jRadioButton2.setSelected(true);
            jRadioButton2.setPreferredSize(new java.awt.Dimension(175,20));
            jRadioButton2.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    jTextField5.setEnabled(jRadioButton2.isSelected());
                }
            });
            bg.add(jRadioButton2);
        }
        return jRadioButton2;
    }

}  //  @jve:decl-index=0:visual-constraint="10,10"

class DynPcdTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        
        return false;
    }
}
