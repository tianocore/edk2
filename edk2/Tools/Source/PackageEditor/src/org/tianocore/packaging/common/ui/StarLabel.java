/** @file
  Java class StarLabel is used to create star label.
 
Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
package org.tianocore.packaging.common.ui;

import javax.swing.JLabel;

/**
 Derived from JLabel class to have a red star on it.
  
 @since PackageEditor 1.0
**/
public class StarLabel extends JLabel{
	/**
	 * This is the default constructor
	 */
	public StarLabel() {
		super();
		init();
	}
	
	/**
	 Create a label with red star * appear on it
	**/
	private void init() {
		this.setText("*");
		this.setSize(new java.awt.Dimension(10,20));
		this.setForeground(java.awt.Color.red);
		this.setFont(new java.awt.Font("DialogInput", java.awt.Font.BOLD, 14));
		this.setHorizontalAlignment(javax.swing.SwingConstants.CENTER);
	}
}
