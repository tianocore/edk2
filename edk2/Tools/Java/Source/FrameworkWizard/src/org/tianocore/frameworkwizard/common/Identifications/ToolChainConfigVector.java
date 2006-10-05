/** @file
 
 The file is used to define Tool Chain Configuration Vector
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
 
 **/
package org.tianocore.frameworkwizard.common.Identifications;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.Vector;

public class ToolChainConfigVector {

    private Vector<ToolChainConfigId> vToolChainConfigs = new Vector<ToolChainConfigId>();

    public int findToolChainConfigs(ToolChainConfigId sfi) {
        for (int index = 0; index < vToolChainConfigs.size(); index++) {
            if (vToolChainConfigs.elementAt(index).equals(sfi)) {
                return index;
            }
        }
        return -1;
    }

    public int findToolChainConfigs(String name) {
        for (int index = 0; index < vToolChainConfigs.size(); index++) {
            if (vToolChainConfigs.elementAt(index).getName().equals(name)) {
                return index;
            }
        }
        return -1;
    }

    public ToolChainConfigId getToolChainConfigs(int index) {
        if (index > -1) {
            return vToolChainConfigs.elementAt(index);
        } else {
            return null;
        }
    }

    public Vector<String> toStringVector(int index) {
        Vector<String> v = new Vector<String>();
        v.addElement(getToolChainConfigs(index).getName());
        v.addElement(getToolChainConfigs(index).getValue());
        return v;
    }

    public void addToolChainConfigs(ToolChainConfigId arg0) {
        vToolChainConfigs.addElement(arg0);
    }

    public void updateToolChainConfigs(ToolChainConfigId arg0, int arg1) {
        vToolChainConfigs.setElementAt(arg0, arg1);
    }

    public void removeToolChainConfigs(ToolChainConfigId arg0) {
        int index = findToolChainConfigs(arg0);
        if (index > -1) {
            vToolChainConfigs.removeElementAt(index);
        }
    }

    public void removeToolChainConfigs(int index) {
        if (index > -1 && index < this.size()) {
            vToolChainConfigs.removeElementAt(index);
        }
    }

    public void removeAll() {
        vToolChainConfigs = new Vector<ToolChainConfigId>();
    }

    public Vector<String> getToolChainConfigsName() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vToolChainConfigs.size(); index++) {
            v.addElement(vToolChainConfigs.get(index).getName());
        }
        return v;
    }

    public Vector<String> getToolChainConfigsValue() {
        Vector<String> v = new Vector<String>();
        for (int index = 0; index < this.vToolChainConfigs.size(); index++) {
            v.addElement(vToolChainConfigs.get(index).getValue());
        }
        return v;
    }

    public int size() {
        return this.vToolChainConfigs.size();
    }

    public void saveFile(String file) throws IOException {
        if (size() > 0) {
            FileWriter fw = new FileWriter(file);
            BufferedWriter bw = new BufferedWriter(fw);
            for (int index = 0; index < size(); index++) {
                String line = this.getToolChainConfigs(index).getName() + " " + ToolChainConfigId.EQUALS + " "
                              + this.getToolChainConfigs(index).getValue();
                bw.write(line);
                bw.newLine();
            }
            bw.flush();
            bw.close();
            fw.close();
        }
    }

    /**
     
     @param file
     @throws IOException
     @throws FileNotFoundException
     
     **/
    public void parseFile(String file) throws IOException {
        FileReader fr = new FileReader(file);
        BufferedReader br = new BufferedReader(fr);
        String line = br.readLine();
        while (line != null) {
            parseLine(line);
            line = br.readLine();
        }
    }

    /**
     Parse the input string and add name, value to vector 
     
     @param line
     
     **/
    private void parseLine(String line) {
        String name = "";
        String value = "";
        if (line.indexOf(ToolChainConfigId.COMMENTS) != 0 && line.indexOf(ToolChainConfigId.EQUALS) > -1) {
            name = line.substring(0, line.indexOf(ToolChainConfigId.EQUALS)).trim();
            value = line.substring(line.indexOf(ToolChainConfigId.EQUALS) + 1).trim();
            this.addToolChainConfigs(new ToolChainConfigId(name, value));
        }
    }
}
