/** @file
ToolChainKey class

ToolChainKey class is representing the "name" part of tool chain definition.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.build.toolchain;

import org.tianocore.build.exception.GenBuildException;

/**
   ToolChainKey class is the java class form of the "name" of tool chain definition.
   It's primarily for the key of a Map data structure.
 **/
public class ToolChainKey implements java.io.Serializable, Comparable<ToolChainKey> {
    static final long serialVersionUID = -8034897190740066933L;

    ///
    /// The part number of key. Currently we only support fixed five parts.
    /// 
    public final static int keyLength = 5;

    //
    // Default delimiter which is used for concatenating the parts of key
    // 
    private String delimiter = "_";

    //
    // Key value in string array form
    // 
    private String[] keySet = null;

    //
    // Key value in one string form
    // 
    private String keyString = null;

    //
    // Key hash value used for hash table 
    // 
    private int hashValue = 0;

    /**
       Public constructor which can override default delimiter.

       @param keyString     The key string value
       @param delimiter     Delimiter charater concatenating the key parts
     **/
    public ToolChainKey(String keyString, String delimiter) throws GenBuildException {
        setKey(keyString, delimiter);
    }

    /**
       Public constructor which uses default delimiter.

       @param keyString     The key string value
     **/
    public ToolChainKey(String keyString) throws GenBuildException {
        setKey(keyString);
    }

    /**
       Public constructor which doesn't use any delimiter.

       @param keySet
     **/
    public ToolChainKey(String[] keySet) throws GenBuildException {
        setKey(keySet);
    }

    /**
       Calculate hash value of the key string (without the delimiter). It's used
       for Hash Table kind of Map.

       @return int      The hash value
     **/
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

    /**
       Compare the string value of two keys . It's used for Tree kind of Map.
       
       @param dstKey    Another key to compare to.
       
       @retval  0       Two keys are equal
       @retval  >0      This key is after the given key
       @retval  <0      This key is before the given key
     **/
    public int compareTo(ToolChainKey dstKey) {
        String[] dstKeySet = dstKey.getKeySet();
        int result = 0;
        for (int i = 0; i < ToolChainKey.keyLength; ++i) {
            result = this.keySet[i].compareToIgnoreCase(dstKeySet[i]);
            if (result != 0) {
                break;
            }
        }

        return result;
    }

    /**
       Check if this key is the same as the given key.

       @param o     Another key to compare to
       
       @return boolean
     **/
    public boolean equals(Object o) {
        ToolChainKey dstKey = (ToolChainKey)o;
        String[] dstKeySet = dstKey.getKeySet();

        if (this == dstKey) {
            return true;
        }

        if (dstKeySet.length != ToolChainKey.keyLength) {
            return false;
        }

        for (int i = 0; i < ToolChainKey.keyLength; ++i) {
            if (!this.keySet[i].equalsIgnoreCase(dstKeySet[i])) {
                return false;
            }
        }

        return true;
    }

    /**
       Set the key value in form of string array.

       @param keySet    The string array of key value
     **/
    public void setKey(String[] keySet) throws GenBuildException {
        if (keySet.length != ToolChainKey.keyLength) {
            throw new GenBuildException("Invalid ToolChain key");
        }

        //
        // Clone the string array because we don't want to change original one
        // 
        this.keySet = new String[ToolChainKey.keyLength];
        System.arraycopy(keySet, 0, this.keySet, 0, ToolChainKey.keyLength);
        for (int i = 0; i < ToolChainKey.keyLength; ++i) {
            if (this.keySet[i] == null || this.keySet[i].length() == 0) {
                this.keySet[i] = "*";
            }
        }

        //
        // We need to re-generate the single key string and hash value.
        // 
        this.keyString = null;
        this.hashValue = 0;
    }

    /**
       Set key value at the specified key part .
       
       @param keySetString      The new value of "index" part of key
       @param index             The key part index
     **/
    public void setKey(String keySetString, int index) throws GenBuildException {
        if (index >= ToolChainKey.keyLength) {
            throw new GenBuildException("Invalid ToolChain key index");
        }

        //
        // Allow wildcard in key string
        // 
        if (keySetString == null || keySetString.length() == 0) {
            keySetString = "*";
        }
        this.keySet[index] = keySetString;

        //
        // We need to re-generate the single key string and hash value.
        // 
        this.keyString = null;
        this.hashValue = 0;
    }

    /**
       Set key value in the form of single string.
       
       @param keyString     The key value string
     **/
    public void setKey(String keyString) throws GenBuildException {
        this.keySet = keyString.split(this.delimiter);

        if (this.keySet.length != ToolChainKey.keyLength) {
            throw new GenBuildException("Invalid ToolChain key");
        }

        this.keyString = keyString;
        //
        // We need to re-generate hash value.
        // 
        this.hashValue = 0;
    }

    /**
       Set key value in the form of single string with specified delimiter.
       
       @param keyString     The key value string
       @param delimiter     The delimiter concatenating the key string
     **/
    public void setKey(String keyString, String delimiter) throws GenBuildException {
        this.keySet = keyString.split(delimiter);

        if (this.keySet.length != ToolChainKey.keyLength) {
            throw new GenBuildException("Invalid ToolChain key");
        }

        this.keyString = keyString;
        this.delimiter = delimiter;
        //
        // We need to re-generate hash value.
        // 
        this.hashValue = 0;
    }

    /**
       Return the string array form of key

       @return String[]
     **/
    public String[] getKeySet() {
        return keySet;
    }

    /**
       Return the single string form of key.

       @return String
     **/
    public String toString() {
        if (this.keyString == null) {
            StringBuffer keyStringBuf = new StringBuffer(64);

            keyStringBuf.append(this.keySet[0]);
            for (int i = 1; i < ToolChainKey.keyLength; ++i) {
                keyStringBuf.append(this.delimiter);
                keyStringBuf.append(this.keySet[i]);
            }

            this.keyString = keyStringBuf.toString();
        }

        return this.keyString;
    }
}

