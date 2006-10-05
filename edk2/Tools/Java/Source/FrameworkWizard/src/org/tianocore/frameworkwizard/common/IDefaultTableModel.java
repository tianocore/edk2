/** @file
 
 The file is used to override DefaultTableModel to provides customized interfaces 
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common;

import javax.swing.table.DefaultTableModel;

public class IDefaultTableModel extends DefaultTableModel {

    ///
    ///
    ///
    private static final long serialVersionUID = -1782544418084080185L;

    public IDefaultTableModel() {
        super();
    }

    /* (non-Javadoc)
     * @see javax.swing.table.TableModel#isCellEditable(int, int)
     *
     */
    public boolean isCellEditable(int rowIndex, int columnIndex) {
        return false;
    }
}
