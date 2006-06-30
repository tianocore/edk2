/*++

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:
  ToolChainKey.java

Abstract:

--*/

package org.tianocore.build.toolchain;

import java.io.Serializable;
import java.util.AbstractMap;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;

import org.tianocore.build.exception.EdkException;

public class ToolChainKey implements java.io.Serializable, Comparable<ToolChainKey> {

    private String delimiter = "_";

    public final static int keyLength = 5;

    private String[] keySet = null;

    private String keyString = null;

    private int hashValue = 0;

    public ToolChainKey(String keyString, String delimiter) throws Exception {
        setKey(keyString, delimiter);
    }

    public ToolChainKey(String keyString) throws EdkException {
        setKey(keyString);
    }

    public ToolChainKey(String[] keySet) throws EdkException {
        setKey(keySet);
    }

    public int hashCode() {
        if (hashValue != 0) {
            return hashValue;
        }

        for (int i = 0; i < keySet.length; ++i) {
            char[] keyStringValue = new char[keySet[i].length()];
            this.keySet[i].getChars(0, keyStringValue.length, keyStringValue, 0);

            for (int j = 0; j < keyStringValue.length; ++j) {
                hashValue = keyStringValue[j] + hashValue * 31;
            }
        }

        return hashValue;
    }

    public int compareTo(ToolChainKey dstKey) {
        String[] dstKeySet = dstKey.getKeySet();
        int result = 0;
        for (int i = 0; i < this.keyLength; ++i) {
            result = this.keySet[i].compareToIgnoreCase(dstKeySet[i]);
            if (result != 0) {
                break;
            }
        }

        return result;
    }

    public boolean equals(Object o) {
        ToolChainKey dstKey = (ToolChainKey)o;
        String[] dstKeySet = dstKey.getKeySet();

        if (this == dstKey) {
            return true;
        }

        if (dstKeySet.length != this.keyLength) {
            return false;
        }

        for (int i = 0; i < this.keyLength; ++i) {
            if (!this.keySet[i].equalsIgnoreCase(dstKeySet[i])) {
                return false;
            }
        }

        return true;
    }

    public void setKey(String[] keySet) throws EdkException {
        if (keySet.length != this.keyLength) {
            throw new EdkException("Invalid ToolChain key");
        }

        this.keySet = new String[this.keyLength];
        System.arraycopy(keySet, 0, this.keySet, 0, this.keyLength);
        for (int i = 0; i < this.keyLength; ++i) {
            if (this.keySet[i] == null || this.keySet[i].length() == 0) {
                this.keySet[i] = "*";
            }
        }
        this.keyString = null;
        this.hashValue = 0;
    }

    public void setKey(String keySetString, int index) throws EdkException {
        if (index >= this.keyLength) {
            throw new EdkException("Invalid ToolChain key index");
        }

        if (keySetString == null || keySetString.length() == 0) {
            keySetString = "*";
        }
        this.keySet[index] = keySetString;
        this.keyString = null;
        this.hashValue = 0;
    }

    public void setKey(String keyString) throws EdkException {
        this.keySet = keyString.split(this.delimiter);

        if (this.keySet.length != this.keyLength) {
            throw new EdkException("Invalid ToolChain key");
        }

        this.keyString = keyString;
        this.hashValue = 0;
    }

    public void setKey(String keyString, String delimiter) throws Exception {
        this.keySet = keyString.split(delimiter);

        if (this.keySet.length != this.keyLength) {
            throw new Exception("Invalid ToolChain key");
        }

        this.keyString = keyString;
        this.delimiter = delimiter;
        this.hashValue = 0;
    }

    public String[] getKeySet() {
        return keySet;
    }

    public String toString() {
        if (this.keyString == null) {
            StringBuffer keyStringBuf = new StringBuffer(64);
            int i = 0;

            keyStringBuf.append(this.keySet[i++]);
            for (; i < this.keyLength; ++i) {
                keyStringBuf.append(this.delimiter);
                keyStringBuf.append(this.keySet[i]);
            }

            this.keyString = keyStringBuf.toString();
        }

        return this.keyString;
    }
}

