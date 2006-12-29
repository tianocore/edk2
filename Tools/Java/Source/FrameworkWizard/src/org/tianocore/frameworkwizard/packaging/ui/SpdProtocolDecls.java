/** @file
  Java class SpdProtocolDecls is GUI for create library definition elements of spd file.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.frameworkwizard.packaging.ui;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableModel;

import org.tianocore.PackageSurfaceAreaDocument;
import org.tianocore.frameworkwizard.common.Identifications.OpeningPackageType;
import org.tianocore.frameworkwizard.platform.ui.ListEditor;

/**
GUI for create library definition elements of spd file.
 
@since PackageEditor 1.0
**/
public class SpdProtocolDecls extends SpdGuidDecls {

    /**
     * 
     */
    private static final long serialVersionUID = 1L;
    private SpdFileContents sfc = null;
    private OpeningPackageType docConsole = null;
    
    public SpdProtocolDecls(JFrame frame) {
        super(frame);
        // TODO Auto-generated constructor stub
    }

    public SpdProtocolDecls(PackageSurfaceAreaDocument.PackageSurfaceArea inPsa, JFrame frame) {
        this(frame);
        sfc = new SpdFileContents(inPsa);
        init(sfc);
    }
    
    public SpdProtocolDecls(OpeningPackageType opt, JFrame frame) {
        this(opt.getXmlSpd(), frame);
        docConsole = opt;
        if (sfc.getSpdPkgDefsRdOnly().equals("true")) {
            JOptionPane.showMessageDialog(this, "This is a read-only package. You will not be able to edit contents in table.");
        }
        initFrame();
    }
    
    protected void initFrame() {
        super.initFrame();
        this.setTitle("Protocol Declarations");
        starLabel.setVisible(false);
        ((ListEditor)getJTable().getColumnModel().getColumn(6).getCellEditor()).setCanNotBeEmpty(false);
    }
    
    protected void init(SpdFileContents sfc){
        //
        // initialize table using SpdFileContents object
        //
        DefaultTableModel model = getModel();
        if (sfc.getSpdProtocolDeclarationCount() == 0) {
            return ;
        }
        saa = new String[sfc.getSpdProtocolDeclarationCount()][7];
        sfc.getSpdProtocolDeclarations(saa);
        int i = 0;
        while (i < saa.length) {
            model.addRow(saa[i]);
            i++;
        }
        
    }
    
    protected void updateRow(int row, int column, TableModel m){
        String[] sa = new String[7];
        sfc.getSpdProtocolDeclaration(sa, row);
        Object cellData = m.getValueAt(row, column);
        if (cellData == null) {
            cellData = "";
        }
        if (cellData.equals(sa[column])) {
            return;
        }
        if (cellData.toString().length() == 0 && sa[column] == null) {
            return;
        }
        
        String name = m.getValueAt(row, 0) + "";
        String cName = m.getValueAt(row, 1) + "";
        String guid = m.getValueAt(row, 2) + "";
        String help = m.getValueAt(row, 3) + "";
        String archList = null;
        if (m.getValueAt(row, 4) != null) {
            archList = m.getValueAt(row, 4).toString();
        }
        String modTypeList = null;
        if (m.getValueAt(row, 5) != null) {
            modTypeList = m.getValueAt(row, 5).toString();
        }
        String guidTypeList = null;
        if (m.getValueAt(row, 6) != null) {
            guidTypeList = m.getValueAt(row, 6).toString();
        }
        String[] rowData = {name, cName, guid, help};
        if (!dataValidation(rowData)){
            return;
        }
        docConsole.setSaved(false);
        sfc.updateSpdProtocolDecl(row, name, cName, guid, help, archList, modTypeList, guidTypeList);
    }
    
    protected int addRow(String[] row) {
        if (!dataValidation(row)){
            return -1;
        }
        docConsole.setSaved(false);
        sfc.genSpdProtocolDeclarations(row[0], row[1], row[2], row[3], stringToVector(row[4]), stringToVector(row[5]), stringToVector(row[6]));
        return 0;
    }
    
    protected void removeRow(int i){
        sfc.removeSpdProtocolDeclaration(i);
        docConsole.setSaved(false);
    }
    
    protected void clearAllRow(){
        sfc.removeSpdProtocolDeclaration();
        docConsole.setSaved(false);
    }
    
    /**
     * @return Returns the sfc.
     */
    protected SpdFileContents getSfc() {
        return sfc;
    }
}
