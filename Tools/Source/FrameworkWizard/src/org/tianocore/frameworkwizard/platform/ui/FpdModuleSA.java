package org.tianocore.frameworkwizard.platform.ui;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JDialog;
import javax.swing.JTabbedPane;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.JTextArea;
import javax.swing.JSplitPane;
import javax.swing.JButton;
import javax.swing.ListSelectionModel;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TableModelEvent;
import javax.swing.event.TableModelListener;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.apache.xmlbeans.XmlObject;
import org.tianocore.frameworkwizard.platform.ui.global.GlobalData;
import org.tianocore.frameworkwizard.platform.ui.global.SurfaceAreaQuery;
import org.tianocore.frameworkwizard.platform.ui.id.ModuleIdentification;
import org.tianocore.frameworkwizard.platform.ui.id.PackageIdentification;

import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import javax.swing.JTextField;
import java.awt.GridLayout;
import javax.swing.JComboBox;

public class FpdModuleSA extends JDialog implements ActionListener {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    static JFrame frame;
    private JPanel jContentPane = null;
    private JTabbedPane jTabbedPane = null;
    private JPanel jPanel = null;
    private JPanel jPanel1 = null;
    private JLabel jLabel = null;
    private JScrollPane jScrollPane = null;
    private JTable jTable = null;
    private JPanel jPanel2 = null;
    private JScrollPane jScrollPane1 = null;
    private JTextArea jTextArea = null;
    private JPanel jPanel3 = null;
    private JSplitPane jSplitPane = null;
    private JPanel jPanel4 = null;
    private JPanel jPanel5 = null;
    private JLabel jLabel1 = null;
    private JLabel jLabel2 = null;
    private JScrollPane jScrollPane2 = null;
    private JTable jTable1 = null;
    private JScrollPane jScrollPane3 = null;
    private JTable jTable2 = null;
    private JScrollPane jScrollPane4 = null;
    private JTable jTable3 = null;
    private JPanel jPanel6 = null;
    private JPanel jPanel7 = null;
    private JScrollPane jScrollPane5 = null;
    private JTextArea jTextArea1 = null;
    private JLabel jLabel3 = null;
    private JLabel jLabel4 = null;
    private JButton jButton = null;
    private JButton jButton1 = null;
    private JLabel jLabel5 = null;
    private JButton jButton2 = null;
    private JButton jButton3 = null;
    private PartialEditableTableModel model = null;
    private LibraryTableModel model1 = null;
    private LibraryTableModel model2 = null;
    private LibraryTableModel model3 = null;
    private DefaultTableModel optionsTableModel = null;
    private FpdFileContents ffc = null;
    private String moduleKey = null;
    private HashMap<String, ArrayList<String>> classInstanceMap = null;
    private ArrayList<String> classProduced = null;
    private HashMap<String, ArrayList<String>> classConsumed = null;
    private JPanel jPanel8 = null;
    private JLabel jLabel6 = null;
    private JTextField jTextField = null;
    private JLabel jLabel7 = null;
    private JTextField jTextField1 = null;
    private JLabel jLabel8 = null;
    private JTextField jTextField2 = null;
    private JScrollPane jScrollPane6 = null;
    private JTable jTable4 = null;
    private JButton jButton4 = null;
    private JButton jButton5 = null;
    private JPanel jPanel9 = null;
    private JPanel jPanel10 = null;
    private JPanel jPanel11 = null;
    private JPanel jPanel12 = null;
    private JLabel jLabel9 = null;
    private JComboBox jComboBox = null;
    private JLabel jLabel10 = null;
    private JTextField jTextField3 = null;
    private JLabel jLabel11 = null;
    private JTextField jTextField4 = null;
    private JButton jButton6 = null;
    private JComboBox jComboBox1 = null;
    /**
     * This is the default constructor
     */
    public FpdModuleSA() {
        super();
        initialize();
    }
    public FpdModuleSA(FpdFileContents ffc) {
        this();
        this.ffc = ffc;
    }
    
    public void setKey(String k){
        this.moduleKey = k;
        jTabbedPane.setSelectedIndex(0);
    }

    /**
      init will be called each time FpdModuleSA object is to be shown.
      @param key Module information.
     **/
    public void initPcdBuildDefinition(String key) {
        //
        // display pcd for key.
        //
        model.setRowCount(0);
        int pcdCount = ffc.getPcdDataCount(key);
        if (pcdCount != 0) {
            String[][] saa = new String[pcdCount][7];
            ffc.getPcdData(key, saa);
            for (int i = 0; i < saa.length; ++i) {
                model.addRow(saa[i]);
            }
        }
    }
    
    public void initLibraries(String key) {
        //
        // display library classes that need to be resolved. also potential instances for them.
        //
        resolveLibraryInstances(key);
        //
        // display lib instances already selected for key
        //
        model1.setRowCount(0);
        int instanceCount = ffc.getLibraryInstancesCount(key);
        if (instanceCount != 0) {
            String[][] saa = new String[instanceCount][5];
            ffc.getLibraryInstances(key, saa);
            for (int i = 0; i < saa.length; ++i) {
                ModuleIdentification mi = getModuleId(saa[i][1] + " " + saa[i][2] + " " + saa[i][3] + " " + saa[i][4]);
                if (mi != null) {
                    saa[i][0] = mi.getName();
                    saa[i][2] = mi.getVersion();
                    saa[i][4] = mi.getPackage().getVersion();
                    //
                    // re-evaluate lib instance usage when adding a already-selected lib instance.
                    //
                    resolveLibraryInstances(saa[i][1] + " " + saa[i][2] + " " + saa[i][3] + " " + saa[i][4]);
                    model1.addRow(saa[i]);
                }
                
                
            }
        }
    }
    
    public void initModuleSAOptions(String key) {
        //
        // display module SA options
        //
        String fvBinding = ffc.getFvBinding(key);
        if (fvBinding != null) {
            jTextField.setText(fvBinding);
        }
        String fileGuid = ffc.getFfsFileNameGuid(key);
        if (fileGuid != null) {
            jTextField1.setText(fileGuid);
        }
        String ffsKey = ffc.getFfsFormatKey(key);
        if (ffsKey != null) {
            jTextField2.setText(ffsKey);
        }
    }
    
    private void resolveLibraryInstances(String key) {
        ModuleIdentification mi = getModuleId(key);
        PackageIdentification[] depPkgList = null;
        try{
            Map<String, XmlObject> m = GlobalData.getNativeMsa(mi);
            SurfaceAreaQuery.setDoc(m);
            //
            // Get dependency pkg list into which we will search lib instances.
            //
            depPkgList = SurfaceAreaQuery.getDependencePkg(null);
            //
            // Get the lib class consumed, produced by this module itself.
            //
            String[] classConsumed = SurfaceAreaQuery.getLibraryClasses("ALWAYS_CONSUMED");
            
            if (this.classConsumed == null) {
                this.classConsumed = new HashMap<String, ArrayList<String>>();
            }
            
            for(int i = 0; i < classConsumed.length; ++i){
                ArrayList<String> consumedBy = this.classConsumed.get(classConsumed[i]);
                if (consumedBy == null) {
                    consumedBy = new ArrayList<String>();
                }
                consumedBy.add(key);
                this.classConsumed.put(classConsumed[i], consumedBy);
            }
            
            String[] classProduced = SurfaceAreaQuery.getLibraryClasses("ALWAYS_PRODUCED");
            if (this.classProduced == null) {
                this.classProduced = new ArrayList<String>();
            }
            for(int i = 0; i < classProduced.length; ++i){
                if (!this.classProduced.contains(classProduced[i])){
                    this.classProduced.add(classProduced[i]);
                }
            }
            //
            // Get classes unresolved
            //
//            Iterator<String> lip = this.classProduced.listIterator();
//            while(lip.hasNext()){
//                String clsProduced = lip.next();
//                this.classConsumed.remove(clsProduced);
//
//            }
            //
            // find potential instances in all dependency pkgs for classes still in classConsumed.
            //
            if (classInstanceMap == null){
                classInstanceMap = new HashMap<String, ArrayList<String>>();
            }
            Iterator<String> lic = this.classConsumed.keySet().iterator();
            while(lic.hasNext()){
                String cls = lic.next();
                if (this.classProduced.contains(cls) || classInstanceMap.containsKey(cls)) {
                    continue;
                }
                ArrayList<String> instances = getInstancesForClass(cls, depPkgList);
                if (instances.size() == 0){
                    JOptionPane.showMessageDialog(frame, "No Applicable Instance for Library Class " + 
                                                  cls + ", Platform Build will Fail.");
                }
                classInstanceMap.put(cls, instances);
                
            }
            
            showClassToResolved();
        }
        catch(Exception e) {
            e.printStackTrace();
        }
    }
    
    private ArrayList<String> getInstancesForClass(String cls, PackageIdentification[] depPkgList) throws Exception{
        ArrayList<String> al = new ArrayList<String>();
        
        for (int i = 0; i < depPkgList.length; ++i) {
            Set<ModuleIdentification> smi = GlobalData.getModules(depPkgList[i]);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                
                String[] clsProduced = getClassProduced(mi);
                
                boolean isPotential = false;
                for (int j = 0; j < clsProduced.length; ++j) {
                    if (clsProduced[j] == null) {
                        continue;
                    }
                    if (clsProduced[j].equals(cls)){
                        isPotential = true;
                    }
                    if (classProduced.contains(clsProduced[j])) {
                        isPotential = false;
                        break;
                    }
                }
                if (isPotential) {
                    al.add(mi.getGuid() + " " + mi.getVersion() + " " + 
                           depPkgList[i].getGuid() + " " + depPkgList[i].getVersion());
                }
            }
        }
        
        return al;
    }
    
    private void removeInstance(String key) {
        ModuleIdentification mi = getModuleId(key); 
        //
        // remove pcd information of instance from current ModuleSA
        //
        ffc.removePcdData(moduleKey, mi);
        //
        // remove class produced by this instance and add back these produced class to be bound.
        //
        String[] clsProduced = getClassProduced(mi);
        for (int i = 0; i < clsProduced.length; ++i) {
            
            classProduced.remove(clsProduced[i]);
        }
        //
        // remove class consumed by this instance. we do not need to bound it now.
        //
        String[] clsConsumed = getClassConsumed(mi);
        for (int i = 0; i < clsConsumed.length; ++i) {
            ArrayList<String> al = classConsumed.get(clsConsumed[i]);
            
            if (al == null ) {
                classConsumed.remove(clsConsumed[i]);
                continue;
            }
            al.remove(key);
            if (al.size() == 0) {
                classConsumed.remove(clsConsumed[i]);
            }
           
        }

        showClassToResolved();
        
    }
    
    private ModuleIdentification getModuleId(String key){
        //
        // Get ModuleGuid, ModuleVersion, PackageGuid, PackageVersion into string array.
        //
        String[] keyPart = key.split(" ");
        Set<PackageIdentification> spi = GlobalData.getPackageList();
        Iterator ispi = spi.iterator();
        
        while(ispi.hasNext()) {
            PackageIdentification pi = (PackageIdentification)ispi.next();
            if ( !pi.getGuid().equals(keyPart[2])){
//                            || !pi.getVersion().equals(keyPart[3])){
                continue;
            }
            Set<ModuleIdentification> smi = GlobalData.getModules(pi);
            Iterator ismi = smi.iterator();
            while(ismi.hasNext()) {
                ModuleIdentification mi = (ModuleIdentification)ismi.next();
                if (mi.getGuid().equals(keyPart[0])){
//                                && mi.getVersion().equals(keyPart[1])){
                    return mi;
                }
            }
        }
        return null;
    }
    
    private String[] getClassProduced(ModuleIdentification mi){
        
        try{
            Map<String, XmlObject> m = GlobalData.getNativeMsa(mi);
            SurfaceAreaQuery.setDoc(m);
            String[] clsProduced = SurfaceAreaQuery.getLibraryClasses("ALWAYS_PRODUCED");
            return clsProduced;
            
        }catch (Exception e) {
            e.printStackTrace();
        }
        return new String[0];
        
    }
    
    private String[] getClassConsumed(ModuleIdentification mi){
        
        String[] clsConsumed = null;
        try{
            Map<String, XmlObject> m = GlobalData.getNativeMsa(mi);
            SurfaceAreaQuery.setDoc(m);
            clsConsumed = SurfaceAreaQuery.getLibraryClasses("ALWAYS_CONSUMED");
            
        }catch (Exception e) {
            e.printStackTrace();
        }
        return clsConsumed;
    }
    
    private void showClassToResolved(){
        model2.setRowCount(0);
        if (classConsumed.size() == 0) {
            return;
        }
        Iterator<String> li = classConsumed.keySet().iterator();
        while(li.hasNext()){
            
            String[] s = {li.next()};
            if (classConsumed.get(s[0]) == null) {
                continue;
            }
            if (classConsumed.get(s[0]).size() == 0) {
                continue;
            }
            if (!classProduced.contains(s[0])){
                model2.addRow(s);
            }
        }
        model3.setRowCount(0);
    }
    /**
     * This method initializes this
     * 
     * @return void
     */
    private void initialize() {
        this.setSize(664, 515);
        this.centerWindow();
        this.setModal(true);
        this.setTitle("Module Settings");
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
            jContentPane.setLayout(new BorderLayout());
            jContentPane.add(getJTabbedPane(), java.awt.BorderLayout.CENTER);
            jContentPane.add(getJPanel3(), java.awt.BorderLayout.SOUTH);
        }
        return jContentPane;
    }

    /**
     * This method initializes jTabbedPane	
     * 	
     * @return javax.swing.JTabbedPane	
     */
    private JTabbedPane getJTabbedPane() {
        if (jTabbedPane == null) {
            jTabbedPane = new JTabbedPane();
            jTabbedPane.addTab("PCD Build Definition", null, getJPanel(), null);
            jTabbedPane.addTab("Module SA Options", null, getJPanel8(), null);
            jTabbedPane.addTab("Libraries", null, getJPanel1(), null);
        }
        return jTabbedPane;
    }

    /**
     * This method initializes jPanel	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel() {
        if (jPanel == null) {
            jLabel = new JLabel();
            jLabel.setText("PcdData");
            jPanel = new JPanel();
            jPanel.setLayout(new BorderLayout());
            jPanel.add(jLabel, java.awt.BorderLayout.NORTH);
            jPanel.add(getJScrollPane(), java.awt.BorderLayout.CENTER);
            jPanel.add(getJPanel2(), java.awt.BorderLayout.SOUTH);
            jPanel.addComponentListener(new java.awt.event.ComponentAdapter() {
                public void componentShown(java.awt.event.ComponentEvent e) {
                    initPcdBuildDefinition(moduleKey);
                }
            });
            
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
            jPanel1.setLayout(new BorderLayout());
            jPanel1.add(getJSplitPane(), java.awt.BorderLayout.NORTH);
            jPanel1.add(getJPanel6(), java.awt.BorderLayout.SOUTH);
            jPanel1.add(getJPanel7(), java.awt.BorderLayout.CENTER);
            jPanel1.addComponentListener(new java.awt.event.ComponentAdapter() {
                public void componentShown(java.awt.event.ComponentEvent e) {
                    initLibraries(moduleKey);
                }
            });
        }
        return jPanel1;
    }

    /**
     * This method initializes jScrollPane	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane() {
        if (jScrollPane == null) {
            jScrollPane = new JScrollPane();
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
            model = new PartialEditableTableModel();
            jTable = new JTable(model);
            jTable.setRowHeight(20);
            jTable.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            model.addColumn("CName");
            model.addColumn("TokenSpaceGUID");
            model.addColumn("ItemType");
            model.addColumn("Token");
            model.addColumn("MaxDatumSize");
            model.addColumn("DataType");
            model.addColumn("DefaultValue");
                        
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
                        String cName = jTable.getValueAt(selectedRow, 0)+"";
                        String[] pcdInfo = {"", ""};
                        getPcdInfo(cName, pcdInfo);
                        jTextArea.setText(pcdInfo[0]);
                        initComboBox(pcdInfo[1]);
                        jComboBox.setSelectedItem(pcdInfo[1]);
                        jTextField3.setEnabled(true);
                        jTextField3.setVisible(true);
                        jTextField3.setText(jTable.getValueAt(selectedRow, 4)+"");
                        jTextField4.setEnabled(true);
                        jTextField4.setText(jTable.getValueAt(selectedRow, 6)+"");
                        if (jTable.getValueAt(selectedRow, 5).equals("VOID*")) {
                            if (pcdInfo[1].equals("FEATURE_FLAG")) {
                                jTextField3.setVisible(false);
                            }
                            else if (pcdInfo[1].equals("FIXED_AT_BUILD")) {
                                try{
                                    jTextField3.setEnabled(false);
                                    jTextField3.setText(ffc.setMaxSizeForPointer(jTable.getValueAt(selectedRow, 6)+"")+"");
                                }
                                catch(Exception except){
                                    JOptionPane.showMessageDialog(frame, "Unacceptable PCD Value: " + except.getMessage());
                                }
                            }
                            else{
                                jTextField3.setText(jTable.getValueAt(selectedRow, 4)+"");
                            }
                        }
                        else {
                            jTextField3.setEnabled(false);
                        }
                        
                        if (!jTable.getValueAt(selectedRow, 2).equals("DYNAMIC") && !jTable.getValueAt(selectedRow, 2).equals("DYNAMIC_EX")) {
                            jTextField4.setText(jTable.getValueAt(selectedRow, 6)+"");
                            if (jTable.getValueAt(selectedRow, 2).equals("FEATURE_FLAG")){
                                jTextField4.setVisible(false);
                                jComboBox1.setVisible(true);
                                jComboBox1.setSelectedItem(jTable.getValueAt(selectedRow, 6)+"");
                            }
                            else{
                                jTextField4.setVisible(true);
                                jTextField4.setEnabled(true);
                                jComboBox1.setVisible(false);
                            }
                        }
                        else{
                            jTextField4.setEnabled(false);
                        }
                    }
                    
                    
                }
            });
            
            jTable.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
//                    int row = arg0.getFirstRow();
//                    TableModel m = (TableModel)arg0.getSource();
                    
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //update xml doc here.
                        
                    }
                }
            });
        }
        return jTable;
    }
    
    private void initComboBox(String originalType) {
        jComboBox.removeAllItems();
        jComboBox.addItem(originalType);
        if (originalType.equals("PATCHABLE_IN_MODULE")) {
            jComboBox.addItem("FIXED_AT_BUILD");
        }
        if (originalType.equals("DYNAMIC")) {
            jComboBox.addItem("FIXED_AT_BUILD");
            jComboBox.addItem("PATCHABLE_IN_MODULE");
        }
    }
    
    private void getPcdInfo(String cName, String[] sa) {
        String[][] saa = new String[ffc.getLibraryInstancesCount(moduleKey)][5];
        ffc.getLibraryInstances(moduleKey, saa);
        
        try{
            if (ffc.getPcdBuildDataInfo(getModuleId(moduleKey), cName, sa)) {
                return;
            }
            for (int j = 0; j < saa.length; ++j) {
                if (ffc.getPcdBuildDataInfo(getModuleId(saa[j][1] + " " + saa[j][2] + " " + saa[j][3] + " " + saa[j][4]),
                                            cName, sa)) {
                    return;
                }
            }
        }
        catch(Exception e) {
            JOptionPane.showMessageDialog(this, "Get PCD details fail: " + e.getMessage());
        }
    }

    /**
     * This method initializes jPanel2	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel2() {
        if (jPanel2 == null) {
            jLabel5 = new JLabel();
            jLabel5.setText("PCD Description");
            jPanel2 = new JPanel();
            jPanel2.setPreferredSize(new java.awt.Dimension(607,200));
            jPanel2.add(jLabel5, null);
            jPanel2.add(getJScrollPane1(), null);
            jPanel2.add(getJPanel9(), null);
        }
        return jPanel2;
    }

    /**
     * This method initializes jScrollPane1	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane1() {
        if (jScrollPane1 == null) {
            jScrollPane1 = new JScrollPane();
            jScrollPane1.setPreferredSize(new java.awt.Dimension(500,100));
            jScrollPane1.setViewportView(getJTextArea());
        }
        return jScrollPane1;
    }

    /**
     * This method initializes jTextArea	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea() {
        if (jTextArea == null) {
            jTextArea = new JTextArea();
            jTextArea.setEditable(false);
        }
        return jTextArea;
    }

    /**
     * This method initializes jPanel3	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel3() {
        if (jPanel3 == null) {
            FlowLayout flowLayout = new FlowLayout();
            flowLayout.setAlignment(java.awt.FlowLayout.RIGHT);
            jPanel3 = new JPanel();
            jPanel3.setLayout(flowLayout);
            jPanel3.add(getJButton2(), null);
            jPanel3.add(getJButton3(), null);
        }
        return jPanel3;
    }

    /**
     * This method initializes jSplitPane	
     * 	
     * @return javax.swing.JSplitPane	
     */
    private JSplitPane getJSplitPane() {
        if (jSplitPane == null) {
            jSplitPane = new JSplitPane();
            jSplitPane.setDividerLocation(200);
            jSplitPane.setLeftComponent(getJPanel4());
            jSplitPane.setRightComponent(getJPanel5());
            jSplitPane.setPreferredSize(new java.awt.Dimension(202,200));
        }
        return jSplitPane;
    }

    /**
     * This method initializes jPanel4	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel4() {
        if (jPanel4 == null) {
            jLabel1 = new JLabel();
            jLabel1.setText("Library Classes Consumed");
            jPanel4 = new JPanel();
            jPanel4.add(jLabel1, null);
            jPanel4.add(getJScrollPane3(), null);
        }
        return jPanel4;
    }

    /**
     * This method initializes jPanel5	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel5() {
        if (jPanel5 == null) {
            jLabel2 = new JLabel();
            jLabel2.setText("Instances Available");
            jPanel5 = new JPanel();
            jPanel5.add(jLabel2, null);
            jPanel5.add(getJScrollPane4(), null);
        }
        return jPanel5;
    }

    /**
     * This method initializes jScrollPane2	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane2() {
        if (jScrollPane2 == null) {
            jScrollPane2 = new JScrollPane();
            jScrollPane2.setPreferredSize(new java.awt.Dimension(453,150));
            jScrollPane2.setViewportView(getJTable1());
        }
        return jScrollPane2;
    }

    /**
     * This method initializes jTable1	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable1() {
        if (jTable1 == null) {
            model1 = new LibraryTableModel();
            model1.addColumn("Name");
            model1.addColumn("ModuleGUID");
            model1.addColumn("ModuleVersion");
            model1.addColumn("PackageGUID");
            model1.addColumn("PackageVersion");
            jTable1 = new JTable(model1);
            jTable1.setRowHeight(20);
            jTable1.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            jTable1.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
        }
        return jTable1;
    }

    /**
     * This method initializes jScrollPane3	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane3() {
        if (jScrollPane3 == null) {
            jScrollPane3 = new JScrollPane();
            jScrollPane3.setPreferredSize(new java.awt.Dimension(200,170));
            jScrollPane3.setViewportView(getJTable2());
        }
        return jScrollPane3;
    }

    /**
     * This method initializes jTable2	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable2() {
        if (jTable2 == null) {
            model2 = new LibraryTableModel();
            model2.addColumn("LibraryClass");
            jTable2 = new JTable(model2);
            jTable2.setRowHeight(20);
            jTable2.setShowGrid(false);
            jTable2.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            jTable2.getSelectionModel().addListSelectionListener(new ListSelectionListener(){
                public void valueChanged(ListSelectionEvent e) {
                    if (e.getValueIsAdjusting()){
                        return;
                    }
                    ListSelectionModel lsm = (ListSelectionModel)e.getSource();
                    if (lsm.isSelectionEmpty()) {
                        return;
                    }
                    else{
                        int selectedRow2 = lsm.getMinSelectionIndex();
                        if (selectedRow2 < 0) {
                            return;
                        }
                        //
                        // display potential lib instances according to class selection
                        //
                        model3.setRowCount(0);
                        String cls = model2.getValueAt(selectedRow2, 0).toString();
                        ArrayList<String> al = classInstanceMap.get(cls);
                        ListIterator<String> li = al.listIterator();
                        while(li.hasNext()) {
                            String instance = li.next();
                            String[] s = {"", "", "", "", ""};
                            if (getModuleId(instance) != null) {
                                s[0] = getModuleId(instance).getName();
                            }
                            
                            String[] instancePart = instance.split(" ");
                            for (int i = 0; i < instancePart.length; ++i){
                                s[i+1] = instancePart[i];
                            }
                            model3.addRow(s);
                        }
                        
                    }
                }
            });
        }
        return jTable2;
    }

    /**
     * This method initializes jScrollPane4	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane4() {
        if (jScrollPane4 == null) {
            jScrollPane4 = new JScrollPane();
            jScrollPane4.setPreferredSize(new java.awt.Dimension(450,170));
            jScrollPane4.setViewportView(getJTable3());
        }
        return jScrollPane4;
    }

    /**
     * This method initializes jTable3	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable3() {
        if (jTable3 == null) {
            model3 = new LibraryTableModel();
            model3.addColumn("Name");
            model3.addColumn("ModuleGUID");
            model3.addColumn("ModuleVersion");
            model3.addColumn("PackageGUID");
            model3.addColumn("PackageVersion");
            jTable3 = new JTable(model3);
            jTable3.setRowHeight(20);
            jTable3.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            jTable3.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
            
        }
        return jTable3;
    }

    /**
     * This method initializes jPanel6	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel6() {
        if (jPanel6 == null) {
            jPanel6 = new JPanel();
        }
        return jPanel6;
    }

    /**
     * This method initializes jPanel7	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel7() {
        if (jPanel7 == null) {
            jLabel4 = new JLabel();
            jLabel4.setText("Instance Description");
            jLabel3 = new JLabel();
            jLabel3.setText("Selected Instances");
            jPanel7 = new JPanel();
            jPanel7.add(jLabel4, null);
            jPanel7.add(getJScrollPane5(), null);
            jPanel7.add(getJButton(), null);
            jPanel7.add(getJButton1(), null);
            jPanel7.add(jLabel3, null);
            jPanel7.add(getJScrollPane2(), null);
        }
        return jPanel7;
    }

    /**
     * This method initializes jScrollPane5	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane5() {
        if (jScrollPane5 == null) {
            jScrollPane5 = new JScrollPane();
            jScrollPane5.setPreferredSize(new java.awt.Dimension(300,50));
            jScrollPane5.setViewportView(getJTextArea1());
        }
        return jScrollPane5;
    }

    /**
     * This method initializes jTextArea1	
     * 	
     * @return javax.swing.JTextArea	
     */
    private JTextArea getJTextArea1() {
        if (jTextArea1 == null) {
            jTextArea1 = new JTextArea();
            jTextArea1.setEditable(false);
        }
        return jTextArea1;
    }

    /**
     * This method initializes jButton	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton() {
        if (jButton == null) {
            jButton = new JButton();
            jButton.setPreferredSize(new java.awt.Dimension(80,20));
            jButton.setText("Add");
            jButton.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int row = jTable3.getSelectedRow();
                    if (row < 0) {
                        return;
                    }
                    Object[] s = {model3.getValueAt(row, 0), model3.getValueAt(row, 1),
                                  model3.getValueAt(row, 2), model3.getValueAt(row, 3),
                                  model3.getValueAt(row, 4)};
                    model1.addRow(s);
                    String instanceValue = model3.getValueAt(row, 1) + " " + 
                    model3.getValueAt(row, 2) + " " +
                    model3.getValueAt(row, 3) + " " +
                    model3.getValueAt(row, 4);
                    ffc.genLibraryInstance(model3.getValueAt(row, 1)+"", model3.getValueAt(row, 2)+"", model3.getValueAt(row, 3)+"", model3.getValueAt(row, 4)+"", moduleKey);
                    //
                    // Add pcd information of selected instance to current moduleSA
                    //
                    try{
                        ffc.addFrameworkModulesPcdBuildDefs(getModuleId(instanceValue), ffc.getModuleSA(moduleKey));
                    }
                    catch (Exception exception) {
                        JOptionPane.showMessageDialog(frame, "PCD Insertion Fail. " + exception.getMessage());
                    }
                    resolveLibraryInstances(instanceValue);
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
            jButton1.setPreferredSize(new java.awt.Dimension(80,20));
            jButton1.setText("Delete");
            jButton1.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    int row = jTable1.getSelectedRow();
                    if (row < 0) {
                        return;
                    }
                    removeInstance(model1.getValueAt(row, 1) + " " + 
                                   model1.getValueAt(row, 2) + " " +
                                   model1.getValueAt(row, 3) + " " +
                                   model1.getValueAt(row, 4));
                    ffc.removeLibraryInstance(moduleKey, row);
                    model1.removeRow(row);
                    
                }
            });
        }
        return jButton1;
    }

    /**
     * This method initializes jButton2	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton2() {
        if (jButton2 == null) {
            jButton2 = new JButton();
            jButton2.setPreferredSize(new java.awt.Dimension(80,20));
            jButton2.setText("Ok");
            jButton2.addActionListener(this);
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
            jButton3.setPreferredSize(new java.awt.Dimension(80,20));
            jButton3.setText("Cancel");
            jButton3.setVisible(false);
        }
        return jButton3;
    }
    public void actionPerformed(ActionEvent arg0) {

        if (arg0.getSource() == jButton2) {
//            ffc.removeLibraryInstances(moduleKey);
//            for (int i = 0; i < model1.getRowCount(); ++i) {
//                String mg = model1.getValueAt(i, 1)+"";
//                String mv = model1.getValueAt(i, 2)+"";
//                String pg = model1.getValueAt(i, 3)+"";
//                String pv = model1.getValueAt(i, 4)+"";
//                ffc.genLibraryInstance(mg, mv, pg, pv, moduleKey);
//            }
            this.setVisible(false);
        }
    }
    /**
     * This method initializes jPanel8	
     * 	
     * @return javax.swing.JPanel	
     */
    private JPanel getJPanel8() {
        if (jPanel8 == null) {
            jLabel8 = new JLabel();
            jLabel8.setText("FFS Format Key");
            jLabel7 = new JLabel();
            jLabel7.setText("FFS File GUID");
            jLabel6 = new JLabel();
            jLabel6.setText("FV Binding");
            jPanel8 = new JPanel();
            jPanel8.add(jLabel6, null);
            jPanel8.add(getJTextField(), null);
            jPanel8.add(jLabel7, null);
            jPanel8.add(getJTextField1(), null);
            jPanel8.add(jLabel8, null);
            jPanel8.add(getJTextField2(), null);
            jPanel8.add(getJScrollPane6(), null);
            jPanel8.add(getJButton4(), null);
            jPanel8.add(getJButton5(), null);
            jPanel8.addComponentListener(new java.awt.event.ComponentAdapter() {
                public void componentShown(java.awt.event.ComponentEvent e) {
                    initModuleSAOptions(moduleKey);
                }
            });
        }
        return jPanel8;
    }
    /**
     * This method initializes jTextField	
     * 	
     * @return javax.swing.JTextField	
     */
    private JTextField getJTextField() {
        if (jTextField == null) {
            jTextField = new JTextField();
            jTextField.setPreferredSize(new java.awt.Dimension(100,20));
            jTextField.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    ffc.setFvBinding(moduleKey, jTextField.getText());
                }
            });
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
            jTextField1.setPreferredSize(new java.awt.Dimension(100,20));
            jTextField1.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    ffc.setFfsFileNameGuid(moduleKey, jTextField1.getText());
                }
            });
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
            jTextField2.setPreferredSize(new java.awt.Dimension(100,20));
            jTextField2.addFocusListener(new java.awt.event.FocusAdapter() {
                public void focusLost(java.awt.event.FocusEvent e) {
                    ffc.setFfsFormatKey(moduleKey, jTextField2.getText());
                }
            });
        }
        return jTextField2;
    }
    /**
     * This method initializes jScrollPane6	
     * 	
     * @return javax.swing.JScrollPane	
     */
    private JScrollPane getJScrollPane6() {
        if (jScrollPane6 == null) {
            jScrollPane6 = new JScrollPane();
            jScrollPane6.setPreferredSize(new java.awt.Dimension(600,200));
            jScrollPane6.setViewportView(getJTable4());
        }
        return jScrollPane6;
    }
    /**
     * This method initializes jTable4	
     * 	
     * @return javax.swing.JTable	
     */
    private JTable getJTable4() {
        if (jTable4 == null) {
            optionsTableModel = new DefaultTableModel();
            optionsTableModel.addColumn("BuildTargets");
            optionsTableModel.addColumn("ToolChainFamily");
            optionsTableModel.addColumn("TagName");
            optionsTableModel.addColumn("ToolCode");
            optionsTableModel.addColumn("SupportedArchs");
            optionsTableModel.addColumn("Contents");
            jTable4 = new JTable(optionsTableModel);
            jTable4.setRowHeight(20);
            Vector<String> vArch = new Vector<String>();
            vArch.add("IA32");
            vArch.add("X64");
            vArch.add("IPF");
            vArch.add("EBC");
            vArch.add("ARM");
            vArch.add("PPC");
            jTable4.getColumnModel().getColumn(4).setCellEditor(new ListEditor(vArch));
            jTable4.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
			jTable4.setAutoResizeMode(javax.swing.JTable.AUTO_RESIZE_OFF);
            jTable4.getModel().addTableModelListener(new TableModelListener() {
                public void tableChanged(TableModelEvent arg0) {
                    // TODO Auto-generated method stub
                    int row = arg0.getFirstRow();
                    TableModel m = (TableModel)arg0.getSource();
                    
                    if (arg0.getType() == TableModelEvent.UPDATE){
                        //ToDo Data Validition check.
                        String targets = m.getValueAt(row, 0) + "";
                        Vector<Object> targetName = null;
                        if (targets.length() > 0) {
                            targetName = new Vector<Object>();
                            String[] sArray = targets.split(" ");
                            for (int i = 0; i < sArray.length; ++i) {
                                targetName.add(sArray[i]);
                            }
                        }
                        
                        String toolChain = m.getValueAt(row, 1) + "";
                        String tagName = m.getValueAt(row, 2) + "";
                        String toolCode = m.getValueAt(row, 3) + "";
                        String archs = m.getValueAt(row, 4) + "";
                        Vector<Object> supArch = null;
                        if (archs.length() > 0) {
                            supArch = new Vector<Object>();
                            String[] sArray1 = archs.split(" ");
                            for (int i = 0; i < sArray1.length; ++i) {
                                supArch.add(sArray1[i]);
                            }
                        }
                        
                        String contents = m.getValueAt(row, 5) + "";
                        
                        ffc.updateModuleSAOptionsOpt(moduleKey, row, targetName, toolChain, tagName, toolCode, supArch, contents);
                    }
                }
            });
        }
        return jTable4;
    }
    /**
     * This method initializes jButton4	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton4() {
        if (jButton4 == null) {
            jButton4 = new JButton();
            jButton4.setPreferredSize(new java.awt.Dimension(80,20));
            jButton4.setText("New");
            jButton4.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    String[] row = {"", "", "", "", "IA32", ""};
                    optionsTableModel.addRow(row);
                    Vector<Object> v = new Vector<Object>();
                    Vector<Object> v1 = new Vector<Object>();
                    v1.add("IA32");
                    ffc.genModuleSAOptionsOpt(moduleKey, v, "", "", "", v1, "");
                }
            });
        }
        return jButton4;
    }
    /**
     * This method initializes jButton5	
     * 	
     * @return javax.swing.JButton	
     */
    private JButton getJButton5() {
        if (jButton5 == null) {
            jButton5 = new JButton();
            jButton5.setPreferredSize(new java.awt.Dimension(80,20));
            jButton5.setText("Delete");
            jButton5.addActionListener(new java.awt.event.ActionListener() {
                public void actionPerformed(java.awt.event.ActionEvent e) {
                    if (jTable4.getSelectedRow() < 0) {
                        return;
                    }
                    
                    ffc.removeModuleSAOptionsOpt(moduleKey, jTable4.getSelectedRow());
                    optionsTableModel.removeRow(jTable4.getSelectedRow());
                }
            });
        }
        return jButton5;
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
 * This method initializes jPanel9	
 * 	
 * @return javax.swing.JPanel	
 */
private JPanel getJPanel9() {
    if (jPanel9 == null) {
        GridLayout gridLayout = new GridLayout();
        gridLayout.setRows(3);
        gridLayout.setColumns(2);
        jPanel9 = new JPanel();
        jPanel9.setLayout(gridLayout);
        jPanel9.setPreferredSize(new java.awt.Dimension(600,90));
        jPanel9.add(getJPanel12(), null);
        jPanel9.add(getJPanel10(), null);
        jPanel9.add(getJPanel11(), null);
    }
    return jPanel9;
}
/**
 * This method initializes jPanel10	
 * 	
 * @return javax.swing.JPanel	
 */
private JPanel getJPanel10() {
    if (jPanel10 == null) {
        FlowLayout flowLayout2 = new FlowLayout();
        flowLayout2.setAlignment(java.awt.FlowLayout.LEFT);
        jLabel10 = new JLabel();
        jLabel10.setText("Max Datum Size");
        jPanel10 = new JPanel();
        jPanel10.setLayout(flowLayout2);
        jPanel10.add(jLabel10, null);
        jPanel10.add(getJTextField3(), null);
    }
    return jPanel10;
}
/**
 * This method initializes jPanel11	
 * 	
 * @return javax.swing.JPanel	
 */
private JPanel getJPanel11() {
    if (jPanel11 == null) {
        FlowLayout flowLayout3 = new FlowLayout();
        flowLayout3.setAlignment(java.awt.FlowLayout.LEFT);
        jLabel11 = new JLabel();
        jLabel11.setText("Default Value");
        jLabel11.setPreferredSize(new java.awt.Dimension(91,16));
        jPanel11 = new JPanel();
        jPanel11.setLayout(flowLayout3);
        jPanel11.add(jLabel11, null);
        jPanel11.add(getJTextField4(), null);
        jPanel11.add(getJComboBox1(), null);
    }
    return jPanel11;
}
/**
 * This method initializes jPanel12	
 * 	
 * @return javax.swing.JPanel	
 */
private JPanel getJPanel12() {
    if (jPanel12 == null) {
        FlowLayout flowLayout1 = new FlowLayout();
        flowLayout1.setAlignment(java.awt.FlowLayout.LEFT);
        jLabel9 = new JLabel();
        jLabel9.setText("Item Type");
        jLabel9.setPreferredSize(new java.awt.Dimension(91,16));
        jPanel12 = new JPanel();
        jPanel12.setLayout(flowLayout1);
        jPanel12.add(jLabel9, null);
        jPanel12.add(getJComboBox(), null);
        jPanel12.add(getJButton6(), null);
    }
    return jPanel12;
}
/**
 * This method initializes jComboBox	
 * 	
 * @return javax.swing.JComboBox	
 */
private JComboBox getJComboBox() {
    if (jComboBox == null) {
        jComboBox = new JComboBox();
        jComboBox.setPreferredSize(new java.awt.Dimension(200,20));
        jComboBox.addItemListener(new java.awt.event.ItemListener() {
            public void itemStateChanged(java.awt.event.ItemEvent e) {
                int row = jTable.getSelectedRow();
                if (row < 0 || jTable.getValueAt(row, 2).equals(jComboBox.getSelectedItem())) {
                    return;
                }
                if (jComboBox.getItemCount() == 3) {
                    if (!jComboBox.getSelectedItem().equals("DYNAMIC")) {
                        pcdDynamicToNonDynamic(jTable.getValueAt(row, 0)+"", jTable.getValueAt(row, 1)+"");
                    }
                    else{
                        pcdNonDynamicToDynamic(jTable.getValueAt(row, 0)+"", jTable.getValueAt(row, 1)+"");
                    }
                }
            }
        });
    }
    return jComboBox;
}

private void pcdDynamicToNonDynamic(String cName, String tsGuid) {
    String[][] saa = new String[ffc.getDynamicPcdBuildDataCount()][5];
    ffc.getDynamicPcdBuildData(saa);
    String maxSize = "";
    String value = "";
    for (int i = 0; i < saa.length; ++i) {
        if (saa[i][0].equals(cName) && saa[i][2].equals(tsGuid)) {
            maxSize = saa[i][3];
            value = ffc.getDynamicPcdBuildDataValue(i);
            break;
        }
    }
    
    ArrayList<String> al = ffc.getDynPcdMapValue(cName + " " + tsGuid);
    for (int i = 0; i < al.size(); ++i) {
        String[] s = al.get(i).split(" ");
        String mKey = s[0] + s[1] + s[2] + s[3];
        ffc.updatePcdData(mKey, cName, tsGuid, jComboBox.getSelectedItem()+"", maxSize, value);
        s[4] = jComboBox.getSelectedItem()+"";
        al.set(i, s[0]+" "+s[1]+" "+s[2]+" "+s[3]+" "+s[4]);
    }
    
    ffc.removeDynamicPcdBuildData(cName, tsGuid);
}

private void pcdNonDynamicToDynamic(String cName, String tsGuid) {
    ArrayList<String> al = ffc.getDynPcdMapValue(cName + " " + tsGuid);
    for (int i = 0; i < al.size(); ++i) {
        String[] s = al.get(i).split(" ");
        String mKey = s[0] + s[1] + s[2] + s[3];
        ffc.updatePcdData(mKey, cName, tsGuid, jComboBox.getSelectedItem()+"", jTextField3.getText(), jTextField4.isVisible() ? jTextField4.getText() : jComboBox1.getSelectedItem()+"");
        s[4] = jComboBox.getSelectedItem()+"";
        al.set(i, s[0]+" "+s[1]+" "+s[2]+" "+s[3]+" "+s[4]);
    }
    try{
        ffc.addDynamicPcdBuildData(cName, jTable.getValueAt(jTable.getSelectedRow(), 3), tsGuid, "DYNAMIC", jTable.getValueAt(jTable.getSelectedRow(), 5)+"", jTextField4.isVisible() ? jTextField4.getText() : jComboBox1.getSelectedItem()+"");
    }
    catch(Exception e){
        JOptionPane.showMessageDialog(frame, "PCD value format: " + e.getMessage());
    }
}
/**
 * This method initializes jTextField3	
 * 	
 * @return javax.swing.JTextField	
 */
private JTextField getJTextField3() {
    if (jTextField3 == null) {
        jTextField3 = new JTextField();
        jTextField3.setPreferredSize(new java.awt.Dimension(200,20));
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
        jTextField4.setPreferredSize(new java.awt.Dimension(200,20));
    }
    return jTextField4;
}
/**
 * This method initializes jButton6	
 * 	
 * @return javax.swing.JButton	
 */
private JButton getJButton6() {
    if (jButton6 == null) {
        jButton6 = new JButton();
        jButton6.setPreferredSize(new java.awt.Dimension(150,20));
        jButton6.setText("Update PCD Data");
        jButton6.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent e) {
                int row = jTable.getSelectedRow();
                if (row < 0) {
                    return;
                }
                model.setValueAt(jComboBox.getSelectedItem(), row, 2);
                model.setValueAt(jTextField3.getText(), row, 4);
                model.setValueAt(jTextField4.isVisible()? jTextField4.getText():jComboBox1.getSelectedItem(), row, 6);
                ffc.updatePcdData(moduleKey, model.getValueAt(row, 0)+"", model.getValueAt(row, 1)+"", model.getValueAt(row, 2)+"", model.getValueAt(row, 4)+"", model.getValueAt(row, 6)+"");
            }
        });
    }
    return jButton6;
}
/**
 * This method initializes jComboBox1	
 * 	
 * @return javax.swing.JComboBox	
 */
private JComboBox getJComboBox1() {
    if (jComboBox1 == null) {
        jComboBox1 = new JComboBox();
        jComboBox1.setPreferredSize(new java.awt.Dimension(100,20));
        jComboBox1.setVisible(false);
        jComboBox1.addItem("true");
        jComboBox1.addItem("false");
    }
    return jComboBox1;
}


}  //  @jve:decl-index=0:visual-constraint="10,10"

class PartialEditableTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        switch (col){
        case 2:
            return false;
        default:
            return false; 
        }
           
    }
}

class LibraryTableModel extends DefaultTableModel {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    public boolean isCellEditable(int row, int col) {
        return false;
    }
}
