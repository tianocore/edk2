package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FontMetrics;

import javax.swing.ButtonGroup;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumn;

import org.tianocore.PlatformSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.DataValidation;
import org.tianocore.frameworkwizard.common.Tools;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPlatformType;
import org.tianocore.frameworkwizard.common.ui.IInternalFrame;
import javax.swing.JCheckBox;
import javax.swing.JRadioButton;
import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JTextField;
import java.awt.GridLayout;
import java.util.ArrayList;

public class FpdDynamicPcdBuildDefinitions extends IInternalFrame {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private JPanel jContentPane = null;
    private JScrollPane jScrollPaneDynPcd = null;
    private JTable jTableDynPcd = null;
    private DynPcdTableModel modelPcd = null; 
    private DynPcdTableModel modelSku = null;
    private FpdFileContents ffc = null;
    private OpeningPlatformType docConsole = null;
    private JPanel jPanelSkuInfo = null;
    private JCheckBox jCheckBoxSkuEnable = null;
    private JPanel jPanelDynPcdValue = null;
    private JRadioButton jRadioButtonHii = null;
    private JRadioButton jRadioButtonVpd = null;
    private JScrollPane jScrollPaneSkuInfo = null;
    private JTable jTableSkuInfo = null;
    private JButton jButtonSkuInfoUpdate = null;
    private JLabel jLabelVarName = null;
    private JTextField jTextFieldVarName = null;
    private JLabel jLabelVarGuid = null;
    private JTextField jTextFieldVarGuid = null;
    private JLabel jLabelPad = null;
    private JLabel jLabelVarOffset = null;
    private JTextField jTextFieldVarOffset = null;
    private JLabel jLabelHiiDefaultValue = null;
    private JTextField jTextFieldHiiDefaultValue = null;
    private JTextField jTextFieldVpdOffset = null;
    private JLabel jLabelVpdOffset = null;
    private JTextField jTextFieldDefaultValue = null;
    private JRadioButton jRadioButtonDefaultValue = null;
    private ButtonGroup bg = new ButtonGroup();
    private JLabel jLabelPadd = null;
    private JLabel jLabelPad1 = null;
    private JScrollPane jScrollPane = null;
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
            ffc.initDynPcdMap();
        }
        String[][] saa = new String[ffc.getDynamicPcdBuildDataCount()][5];
        ffc.getDynamicPcdBuildData(saa);
        for (int i = 0; i < saa.length; ++i) {
            modelPcd.addRow(saa[i]);
        }
        
        saa = new String[ffc.getPlatformDefsSkuInfoCount()][2];
        ffc.getPlatformDefsSkuInfos(saa);
        for (int i = 0; i < saa.length; ++i) {
            modelSku.addRow(saa[i]);
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
            jContentPane.add(getJScrollPaneDynPcd(), java.awt.BorderLayout.NORTH);
            jContentPane.add(getJPanelSkuInfo(), java.awt.BorderLayout.CENTER);
        }
        return jContentPane;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneDynPcd() {
        if (jScrollPaneDynPcd == null) {
            jScrollPaneDynPcd = new JScrollPane();
            jScrollPaneDynPcd.setPreferredSize(new java.awt.Dimension(100,250));
            jScrollPaneDynPcd.setViewportView(getJTableDynPcd());
        }
        return jScrollPaneDynPcd;
    }

    /**
     * This method initializes jTable	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableDynPcd() {
        if (jTableDynPcd == null) {
            modelPcd = new DynPcdTableModel();
            modelPcd.addColumn("CName");
            modelPcd.addColumn("Token");
            modelPcd.addColumn("TokenSpaceGuid");
            modelPcd.addColumn("MaxDatumSize");
            modelPcd.addColumn("DatumType");
            jTableDynPcd = new JTable(modelPcd);
            jTableDynPcd.setRowHeight(20);
            TableColumn tokenColumn = jTableDynPcd.getColumnModel().getColumn(1);
            jTableDynPcd.removeColumn(tokenColumn);
            jTableDynPcd.getColumnModel().getColumn(0).setMinWidth(250);
            
            jTableDynPcd.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableDynPcd.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
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
                        jTextFieldDefaultValue.setText("");
                        jTextFieldVpdOffset.setText("");
                        jTextFieldVarName.setText("");
                        jTextFieldVarGuid.setText("");
                        jTextFieldVarOffset.setText("");
                        jTextFieldHiiDefaultValue.setText("");
                        displayDetails(selectedRow);
                    }
                }
            });
        }
        return jTableDynPcd;
    }
    //
    // should display default sku info here, as no selection event of table1 will be triggered when change selection of rows in table. 
    //
    private void displayDetails(int i) {
    	int defaultSkuRow = getDefaultSkuInfoRow();
		jTableSkuInfo.changeSelection(defaultSkuRow, 0, false, false);
		
        int skuCount = ffc.getDynamicPcdSkuInfoCount(i);
        String defaultVal = ffc.getDynamicPcdBuildDataValue(i);
        if (defaultVal != null) {
            jRadioButtonDefaultValue.setSelected(true);
            jTextFieldDefaultValue.setText(defaultVal);
            if ( skuCount == 1) {
                jCheckBoxSkuEnable.setSelected(false);
            }
            else{
                jCheckBoxSkuEnable.setSelected(true);
            }
        }
        
        else if (ffc.getDynamicPcdBuildDataVpdOffset(i) != null) {
            jRadioButtonVpd.setSelected(true);
            jTextFieldVpdOffset.setText(ffc.getDynamicPcdBuildDataVpdOffset(i));
            if (skuCount ==1) {
                jCheckBoxSkuEnable.setSelected(false);
                
            }
            else{
                jCheckBoxSkuEnable.setSelected(true);
            }
        }
        else {
            jRadioButtonHii.setSelected(true);
            String[][] saa = new String[ffc.getDynamicPcdSkuInfoCount(i)][7];
            ffc.getDynamicPcdSkuInfos(i, saa);
            
            String varDisplayName = Tools.convertUnicodeHexStringToString(saa[0][1]);
            jTextFieldVarName.setText(varDisplayName);
            
            jTextFieldVarGuid.setText(saa[0][2]);
            jTextFieldVarOffset.setText(saa[0][3]);
            jTextFieldHiiDefaultValue.setText(saa[0][4]);
            if (skuCount ==1) {
                jCheckBoxSkuEnable.setSelected(false);
            }
            else{
                jCheckBoxSkuEnable.setSelected(true);
            }
        }
        
    }
    
    private void displaySkuInfoDetails(String id) {
        int pcdSelected = jTableDynPcd.getSelectedRow();
        if (pcdSelected < 0) {
            return;
        }
        
        int skuInfoCount = ffc.getDynamicPcdSkuInfoCount(pcdSelected);
        String[][] saa = new String[skuInfoCount][7];
        ffc.getDynamicPcdSkuInfos(pcdSelected, saa);
        int i = 0;
        while (i < skuInfoCount) {
        	if (id.equals(saa[i][0])) {
        		if (saa[i][5] != null){
                    jRadioButtonVpd.setSelected(true);
                    jTextFieldVpdOffset.setText(saa[i][5]);
                } 
                
                else if (saa[i][1] != null) {
                    jRadioButtonHii.setSelected(true);
                    String varDisplayName = Tools.convertUnicodeHexStringToString(saa[i][1]);
                    jTextFieldVarName.setText(varDisplayName);
                    jTextFieldVarGuid.setText(saa[i][2]);
                    jTextFieldVarOffset.setText(saa[i][3]);
                    jTextFieldHiiDefaultValue.setText(saa[i][4]);
                }
                
                else{
                    jRadioButtonDefaultValue.setSelected(true);
                    jTextFieldDefaultValue.setText(saa[i][6]);
                }
        		return;
        	}
        	++i;
        }
    }
    
    private int getDefaultSkuInfoRow () {
    	for (int i = 0; i < modelSku.getRowCount(); ++i) {
    		if (modelSku.getValueAt(i, 0).equals("0")) {
    			return i;
    		}
    	}
    	return 0;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelSkuInfo() {
        if (jPanelSkuInfo == null) {
            jPanelSkuInfo = new JPanel();
            jPanelSkuInfo.setLayout(new BorderLayout());
            jPanelSkuInfo.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.RAISED));
            jPanelSkuInfo.setPreferredSize(new java.awt.Dimension(600,120));

//            jPanelSkuInfo.add(getJPanelSkuInfoN(), java.awt.BorderLayout.NORTH);
            jPanelSkuInfo.add(getJScrollPane(), java.awt.BorderLayout.NORTH);
            jPanelSkuInfo.add(getJScrollPaneSkuInfo(), java.awt.BorderLayout.CENTER);
            
            
//            jPanelSkuInfo.add(getJPanelDynPcdValue(), java.awt.BorderLayout.SOUTH);
        }
        return jPanelSkuInfo;
    }

    /**
     * This method initializes jCheckBox	
     * 	
     * @return javax.swing.JCheckBox	
     */
    private JCheckBox getJCheckBoxSkuEnable() {
        if (jCheckBoxSkuEnable == null) {
            jCheckBoxSkuEnable = new JCheckBox();
            jCheckBoxSkuEnable.setText("SKU Enable");
            jCheckBoxSkuEnable.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                	if (!jCheckBoxSkuEnable.isSelected()) {
                		int defaultSkuRow = getDefaultSkuInfoRow();
                		jTableSkuInfo.changeSelection(defaultSkuRow, 0, false, false);
                	}
                    jTableSkuInfo.setEnabled(jCheckBoxSkuEnable.isSelected());
                }
            });
        }
        return jCheckBoxSkuEnable;
    }

  			
  /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanelDynPcdValue() {
        if (jPanelDynPcdValue == null) {
            jLabelPad1 = new JLabel();
            jLabelPad1.setText(" ");
            GridLayout gridLayout = new GridLayout();
            gridLayout.setColumns(5);
            gridLayout.setRows(4);
            gridLayout.setHgap(20);
            gridLayout.setVgap(20);
            
            jLabelVpdOffset = new JLabel();
            jLabelVpdOffset.setPreferredSize(new java.awt.Dimension(80,20));
            jLabelVpdOffset.setText("VPD Offset");
            jLabelHiiDefaultValue = new JLabel();
            jLabelHiiDefaultValue.setPreferredSize(new java.awt.Dimension(100,20));
            jLabelHiiDefaultValue.setText("HII Default Value");
            jLabelVarOffset = new JLabel();
            jLabelVarOffset.setText("Variable Offset");
            jLabelVarOffset.setPreferredSize(new java.awt.Dimension(90,20));
            jLabelPad = new JLabel();
            jLabelPad.setText("                           ");
            jLabelVarGuid = new JLabel();
            jLabelVarGuid.setText("Variable GUID");
            jLabelVarGuid.setPreferredSize(new java.awt.Dimension(100,20));
            jLabelVarName = new JLabel();
            jLabelVarName.setText("Variable Name");
            jLabelVarName.setToolTipText("");
            jLabelVarName.setPreferredSize(new java.awt.Dimension(90,20));
            jPanelDynPcdValue = new JPanel();
            jPanelDynPcdValue.setBorder(javax.swing.BorderFactory.createEtchedBorder(javax.swing.border.EtchedBorder.LOWERED));
            jPanelDynPcdValue.setPreferredSize(new java.awt.Dimension(1000,150));
            jPanelDynPcdValue.setLayout(gridLayout);
            jPanelDynPcdValue.add(getJRadioButtonHii(), null);
            jPanelDynPcdValue.add(jLabelVarName, null);
            jPanelDynPcdValue.add(getJTextFieldVarName(), null);
            jPanelDynPcdValue.add(jLabelVarGuid, null);
            jPanelDynPcdValue.add(getJTextFieldVarGuid(), null);
            jPanelDynPcdValue.add(jLabelPad, null);
            jPanelDynPcdValue.add(jLabelVarOffset, null);
            jPanelDynPcdValue.add(getJTextFieldVarOffset(), null);
            jPanelDynPcdValue.add(jLabelHiiDefaultValue, null);
            jPanelDynPcdValue.add(getJTextFieldHiiDefaultValue(), null);
            jPanelDynPcdValue.add(getJRadioButtonVpd(), null);
            jPanelDynPcdValue.add(jLabelVpdOffset, null);
            jLabelPadd = new JLabel();
            jLabelPadd.setText("                           ");
            jPanelDynPcdValue.add(getJTextFieldVpdOffset(), null);
			jLabelVarName.setEnabled(false);
			jLabelVarGuid.setEnabled(false);
			jLabelHiiDefaultValue.setEnabled(false);
			jLabelVarOffset.setEnabled(false);
			jLabelVpdOffset.setEnabled(false);
			jPanelDynPcdValue.add(jLabelPadd, null);
			jPanelDynPcdValue.add(jLabelPad1, null);
			jPanelDynPcdValue.add(getJRadioButtonDefaultValue(), null);
			jPanelDynPcdValue.add(getJTextFieldDefaultValue(), null);
            jPanelDynPcdValue.add(getJCheckBoxSkuEnable(), null);
            jPanelDynPcdValue.add(getJButtonSkuInfoUpdate(), null);
            bg.add(jRadioButtonHii);
            bg.add(jRadioButtonVpd);
        }			

        return jPanelDynPcdValue;
    }

    /**
     * This method initializes jRadioButton	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonHii() {
        if (jRadioButtonHii == null) {
            jRadioButtonHii = new JRadioButton();
            jRadioButtonHii.setText("HII Enable");
            jRadioButtonHii.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    boolean selected = jRadioButtonHii.isSelected();
                    jLabelVarName.setEnabled(selected);
                    jLabelVarGuid.setEnabled(selected);
                    jLabelPad.setEnabled(selected);
                    jLabelVarOffset.setEnabled(selected);
                    jLabelHiiDefaultValue.setEnabled(selected);
                    jTextFieldVarName.setEnabled(selected);
                    jTextFieldVarGuid.setEnabled(selected);
                    jTextFieldVarOffset.setEnabled(selected);
                    jTextFieldHiiDefaultValue.setEnabled(selected);
                }
            });
        }
        return jRadioButtonHii;
    }

    /**
     * This method initializes jRadioButton1	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonVpd() {
        if (jRadioButtonVpd == null) {
            jRadioButtonVpd = new JRadioButton();
            jRadioButtonVpd.setText("VPD Enable");
            jRadioButtonVpd.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    boolean selected = jRadioButtonVpd.isSelected();
                    jTextFieldVpdOffset.setEnabled(selected);
                    jLabelVpdOffset.setEnabled(selected);
                }
            });
        }
        return jRadioButtonVpd;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPaneSkuInfo() {
        if (jScrollPaneSkuInfo == null) {
            jScrollPaneSkuInfo = new JScrollPane();
            jScrollPaneSkuInfo.setPreferredSize(new java.awt.Dimension(300,50));
            jScrollPaneSkuInfo.setViewportView(getJTableSkuInfo());
        }
        return jScrollPaneSkuInfo;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTableSkuInfo() {
        if (jTableSkuInfo == null) {
            modelSku = new DynPcdTableModel();
            jTableSkuInfo = new JTable(modelSku);
            modelSku.addColumn("SKU ID");
            modelSku.addColumn("SKU Name");
            jTableSkuInfo.setEnabled(false);
            jTableSkuInfo.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTableSkuInfo.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (jTableDynPcd.getSelectedRow() < 0) {
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
                        jTextFieldDefaultValue.setText("");
                        jTextFieldVpdOffset.setText("");
                        jTextFieldVarName.setText("");
                        jTextFieldVarGuid.setText("");
                        jTextFieldVarOffset.setText("");
                        jTextFieldHiiDefaultValue.setText("");
                        int selected = lsm.getMinSelectionIndex();
                        String skuId = modelSku.getValueAt(selected, 0)+"";
                        displaySkuInfoDetails(skuId);
                    }
                }
            });
        }
        return jTableSkuInfo;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButtonSkuInfoUpdate() {
        if (jButtonSkuInfoUpdate == null) {
            jButtonSkuInfoUpdate = new JButton();
            jButtonSkuInfoUpdate.setPreferredSize(new java.awt.Dimension(180,20));
            jButtonSkuInfoUpdate.setText("Update SKU Value");
            FontMetrics fm = jButtonSkuInfoUpdate.getFontMetrics(jButtonSkuInfoUpdate.getFont());
            jButtonSkuInfoUpdate.setPreferredSize(new Dimension (fm.stringWidth(jButtonSkuInfoUpdate.getText()) + 40, 20));
            jButtonSkuInfoUpdate.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int pcdSelected = jTableDynPcd.getSelectedRow();
                    if (pcdSelected < 0) {
                        return;
                    }
                    docConsole.setSaved(false);
                    updateSkuInfo(pcdSelected);
                    
                }
            });
        }
        return jButtonSkuInfoUpdate;
    }
    
    private void updateSkuInfo (int pcdSelected) {
        int skuCount = ffc.getDynamicPcdSkuInfoCount(pcdSelected);
        String cName = modelPcd.getValueAt(pcdSelected, 0)+"";
        String tsGuid = modelPcd.getValueAt(pcdSelected, 2)+"";
        
        String varName = null;
        String varGuid = null;
        String varOffset = null;
        String hiiDefault = null;
        String value = null;
        String vpdOffset = null;
        if (jRadioButtonHii.isSelected()) {
            varName = Tools.convertStringToUnicodeHexString(jTextFieldVarName.getText());
            if (varName.length() == 0) {
                JOptionPane.showMessageDialog(this, "Variable Name is Empty.");
                return;
            }
            varGuid = jTextFieldVarGuid.getText();
            if (varGuid.length() == 0) {
                JOptionPane.showMessageDialog(this, "Variable GUID is Empty.");
                return;
            }
            varOffset = jTextFieldVarOffset.getText();
            if (!DataValidation.isHex64BitDataType(varOffset)) {
                JOptionPane.showMessageDialog(this, "Variable Offset is NOT Hex64BitDataType.");
                return;
            }
            hiiDefault = jTextFieldHiiDefaultValue.getText();
            if (!DataValidation.isDefaultValueType(hiiDefault)) {
                JOptionPane.showMessageDialog(this, "Default Value is NOT DefaultValueType.");
                return;
            }
        }
        if (jRadioButtonVpd.isSelected()) {
            vpdOffset = jTextFieldVpdOffset.getText();
            if (!DataValidation.isHex64BitDataType(vpdOffset)) {
                JOptionPane.showMessageDialog(this, "VPD Offset is NOT Hex64BitDataType.");
                return;
            }
        }
        if (jRadioButtonDefaultValue.isSelected()) {
            value = jTextFieldDefaultValue.getText();
            if (!DataValidation.isDefaultValueType(value)) {
                JOptionPane.showMessageDialog(this, "Value is NOT DefaultValueType.");
                return;
            }
        }
        //
        // SKU disabled. only modify data for default SKU.
        //
        if (!jCheckBoxSkuEnable.isSelected()) {
            if (true) {
                ffc.removeDynamicPcdBuildDataSkuInfo(pcdSelected);
                if (jRadioButtonHii.isSelected()) {
                    ffc.genDynamicPcdBuildDataSkuInfo("0", varName, varGuid, varOffset, hiiDefault, null, null, pcdSelected);
                    ArrayList<String> al = ffc.getDynPcdMapValue(cName + " " + tsGuid);
                    if (al == null) {
                        return;
                    }
                    for (int i = 0; i < al.size(); ++i) {
                        String mKey = moduleInfo (al.get(i));
                        ffc.updatePcdData(mKey, cName, tsGuid, null, null, hiiDefault);
                    }
                }
                else if (jRadioButtonVpd.isSelected()){
                    ffc.genDynamicPcdBuildDataSkuInfo("0", null, null, null, null, vpdOffset, null, pcdSelected);
                    ArrayList<String> al = ffc.getDynPcdMapValue(cName + " " + tsGuid);
                    if (al == null) {
                        return;
                    }
                    for (int i = 0; i < al.size(); ++i) {
                        String mKey = moduleInfo (al.get(i));
                        ffc.updatePcdData(mKey, cName, tsGuid, null, null, vpdOffset);
                    }
                }
                else{
                    ffc.genDynamicPcdBuildDataSkuInfo("0", null, null, null, null, null, value, pcdSelected);
                    ArrayList<String> al = ffc.getDynPcdMapValue(cName + " " + tsGuid);
                    if (al == null) {
                        return;
                    }
                    for (int i = 0; i < al.size(); ++i) {
                        String mKey = moduleInfo (al.get(i));
                        ffc.updatePcdData(mKey, cName, tsGuid, null, null, value);
                    }
                }
            }
        }
        //
        // SKU Enabled, need add data to all SKUs.
        //
        if (jCheckBoxSkuEnable.isSelected()) {
            if (skuCount == 1) {
                
                for (int i = 1; i < jTableSkuInfo.getRowCount(); ++i) {
                    ffc.genDynamicPcdBuildDataSkuInfo(modelSku.getValueAt(i, 0)+"", varName, varGuid, varOffset, hiiDefault, vpdOffset, value, pcdSelected);
                }
            }
            else {
                int row = jTableSkuInfo.getSelectedRow();
                if (row < 0) {
                    return;
                }
                ffc.updateDynamicPcdBuildDataSkuInfo(modelSku.getValueAt(row, 0)+"", varName, varGuid, varOffset, hiiDefault, vpdOffset, value, pcdSelected);
            }
        }
    }
    
    private String moduleInfo (String pcdInfo) {
        
        return pcdInfo.substring(0, pcdInfo.lastIndexOf(" "));
    }
    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVarName() {
        if (jTextFieldVarName == null) {
            jTextFieldVarName = new JTextField();
            jTextFieldVarName.setPreferredSize(new java.awt.Dimension(150,20));
            jTextFieldVarName.setEnabled(false);
        }
        return jTextFieldVarName;
    }

    /**
     * This method initializes jTextField1	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVarGuid() {
        if (jTextFieldVarGuid == null) {
            jTextFieldVarGuid = new JTextField();
            jTextFieldVarGuid.setPreferredSize(new java.awt.Dimension(150,20));
            jTextFieldVarGuid.setEnabled(false);
        }
        return jTextFieldVarGuid;
    }

    /**
     * This method initializes jTextField2	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVarOffset() {
        if (jTextFieldVarOffset == null) {
            jTextFieldVarOffset = new JTextField();
            jTextFieldVarOffset.setPreferredSize(new java.awt.Dimension(150,20));
            jTextFieldVarOffset.setEnabled(false);
        }
        return jTextFieldVarOffset;
    }

    /**
     * This method initializes jTextField3	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldHiiDefaultValue() {
        if (jTextFieldHiiDefaultValue == null) {
            jTextFieldHiiDefaultValue = new JTextField();
            jTextFieldHiiDefaultValue.setPreferredSize(new java.awt.Dimension(150,20));
            jTextFieldHiiDefaultValue.setEnabled(false);
        }
        return jTextFieldHiiDefaultValue;
    }

    /**
     * This method initializes jTextField4	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldVpdOffset() {
        if (jTextFieldVpdOffset == null) {
            jTextFieldVpdOffset = new JTextField();
            jTextFieldVpdOffset.setPreferredSize(new java.awt.Dimension(150,20));
            jTextFieldVpdOffset.setEnabled(false);
        }
        return jTextFieldVpdOffset;
    }

    /**
     * This method initializes jTextField5	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextFieldDefaultValue() {
        if (jTextFieldDefaultValue == null) {
            jTextFieldDefaultValue = new JTextField();
            jTextFieldDefaultValue.setPreferredSize(new java.awt.Dimension(150,20));
        }
        return jTextFieldDefaultValue;
    }

    /**
     * This method initializes jRadioButton2	
     * 	
     * @return javax.swing.JRadioButton	
     */
    private JRadioButton getJRadioButtonDefaultValue() {
        if (jRadioButtonDefaultValue == null) {
            jRadioButtonDefaultValue = new JRadioButton();
            jRadioButtonDefaultValue.setText("Default PCD Value");
            jRadioButtonDefaultValue.setSelected(true);
            jRadioButtonDefaultValue.setPreferredSize(new java.awt.Dimension(175,20));
            jRadioButtonDefaultValue.addItemListener(new java.awt.event.ItemListener() {
                public void itemStateChanged(java.awt.event.ItemEvent e) {
                    jTextFieldDefaultValue.setEnabled(jRadioButtonDefaultValue.isSelected());
                }
            });
            bg.add(jRadioButtonDefaultValue);
        }
        return jRadioButtonDefaultValue;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
            jScrollPane.setPreferredSize(new java.awt.Dimension(1003,180));
            jScrollPane.setViewportView(getJPanelDynPcdValue());
        }
        return jScrollPane;
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
