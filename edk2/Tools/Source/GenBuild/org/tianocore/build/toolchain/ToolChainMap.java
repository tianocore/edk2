/** @file
ToolChainMap class

ToolChainMap class is used for storing tool chain configurations.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.build.toolchain;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;

/**
 ToolChainMap is a wrapper class for a generic Map class which uses ToolChainKey
 class as its key. It's used to store and retrieve tool chain configuration 
 information.
 **/
public class ToolChainMap {
    //
    // From which part of key can be used to match "*"
    // 
    private int matchLevel = ToolChainKey.keyLength - 2;

    //
    // A Map object in which tool chain configuration information will be stored
    // 
    private Map<ToolChainKey, String> map = null;

    /**
       Public constructor. It just initializes the private Map object.
     **/
    public ToolChainMap() {
        this.map = new HashMap<ToolChainKey, String>();
    }

    /**
       Wrapper function for Map.put(). It's used when default delimiter of
       ToolChainKey is not wanted and will be overrided by "delimiter" parameter.

       @param key           Key string which is concatenated with "delimiter"
       @param delimiter     The delimiter string in the key string
       @param value         Value string associated with the "key"
       
       @retval String       The "value" string if the "key" is valid.
       @retval null         if the "key" is invalid
     **/
    public String put(String key, String delimiter, String value) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key, delimiter);
        } catch (Exception e) {
            return null;
        }
        return (String)map.put(toolChainKey, value);
    }

    /**
       Wrapper function for Map.put().

       @param key           Key string which is concatenated with default "delimiter"
       @param value         Value string associated with the "key"
       
       @retval String       The "value" string if the "key" is valid.
       @retval null         if the "key" is invalid
     **/
    public String put(String key, String value) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            return null;
        }
        return (String)map.put(toolChainKey, value);
    }

    /**
       Wrapper function for Map.put(). The key is given in the form of string
       array.

       @param key           Key string array
       @param value         Value string associated with the "key"
       
       @retval String       The "value" string if the "key" is valid.
       @retval null         if the "key" is invalid
     **/
    public String put(String[] key, String value) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            return null;
        }
        return (String)map.put(toolChainKey, value);
    }

    /**
       Wrapper function for Map.put(). The key is given in ToolChainKey class.

       @param key           ToolChainKey class
       @param value         Value string associated with the "key"
       
       @retval String       The "value" string if the "key" is valid.
       @retval null         if the "key" is invalid
     **/
    public String put(ToolChainKey key, String value) {
        return (String)map.put(key, value);
    }

    /**
       Wrapper function for Map.get().

       @param key           Key string which is concatenated with default "delimiter"
       
       @return String
     **/
    public String get(String key) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            return null;
        }
        return get(toolChainKey);
    }

    /**
       Wrapper function for Map.get(). It's used when default delimiter of
       ToolChainKey is not wanted and will be overrided by "delimiter" parameter.

       @param key           Key string which is concatenated with "delimiter"
       @param delimiter     The delimiter string in the key string
       
       @return String
     **/
    public String get(String key, String delimiter) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key, delimiter);
        } catch (Exception e) {
            return null;
        }
        return get(toolChainKey);
    }

    /**
       Wrapper function for Map.get(). The key is given in the form of string
       array.

       @param key           Key string array
       
       @return String
     **/
    public String get(String[] key) {
        ToolChainKey toolChainKey;

        try {
            toolChainKey = new ToolChainKey(key);
        } catch (Exception e) {
            return null;
        }
        return get(toolChainKey);
    }

    /**
       Wrapper function for Map.get(). The key is given in ToolChainKey class.
       All other form of get() method will eventually call this form of get. It
       will do real job of finding the value associated with the given key. Most
       of the job is to try to match the key with "wildcard".

       @param key           ToolChainKey class
       
       @return String       The value associated with the key
     **/
    public String get(ToolChainKey key) {
        ///
        /// First, we'll try to get the value through the exact given key
        /// 
        String result = map.get(key);
        if (result != null || map.containsKey(key)) {
            return result;
        }

        ///
        /// If nothing is found, then, we'll try all possible keys combined with
        /// wildcard "*". In order not to change the original key value, we have
        /// to clone one for later use. 
        /// 
        String[] keySet = key.getKeySet();
        ToolChainKey tmpKey;
        try {
            tmpKey = new ToolChainKey(keySet);
        } catch (Exception e) {
            return null;
        }

        ///
        /// In the current tool chain definition format (in name/value pair), 
        /// there're five parts in the "name". The last part of the "name" must
        /// not be "wildcard". So we should start combining "*" from the fourth part.
        /// We'll try all the possible combinations until the value can be fetched.
        ///  
        /// The following code implements the logic which will try to use, for example,
        /// following key parts combinations sequentially to get the value.
        /// 
        /// TARGET_TOOLCHAIN_ARCH_TOOLCODE_ATTRIBUTE
        /// TARGET_TOOLCHAIN_ARCH_*_ATTRIBUTE
        /// TARGET_TOOLCHAIN_*_TOOLCODE_ATTRIBUTE
        /// TARGET_TOOLCHAIN_*_*_ATTRIBUTE
        /// TARGET_*_ARCH_TOOLCODE_ATTRIBUTE
        /// TARGET_*_ARCH_*_ATTRIBUTE
        /// TARGET_*_*_TOOLCODE_ATTRIBUTE
        /// TARGET_*_*_*_ATTRIBUTE
        /// *_TOOLCHAIN_ARCH_TOOLCODE_ATTRIBUTE
        /// *_TOOLCHAIN_ARCH_*_ATTRIBUTE
        /// *_TOOLCHAIN_*_TOOLCODE_ATTRIBUTE
        /// *_TOOLCHAIN_*_*_ATTRIBUTE
        /// *_*_ARCH_TOOLCODE_ATTRIBUTE
        /// *_*_ARCH_*_ATTRIBUTE
        /// *_*_*_TOOLCODE_ATTRIBUTE
        /// *_*_*_*_ATTRIBUTE
        /// 

        //
        // level is used to control if all parts of "name" have been "wildcarded"
        // 
        int level = matchLevel;
        while (level >= 0) {
            //
            // tmplevel is used to control if all parts of "name" between first
            // "*" and fourth name part have been "wildcarded".
            // 
            int tmpLevel = level;
            while (tmpLevel >= level) {
                String[] tmpKeySet = tmpKey.getKeySet();
                try {
                    if (!tmpKeySet[tmpLevel].equals("*")) {
                        //
                        // If "tmplevel" part is not "*", set it to "*".
                        // For example, at first loop, the key will become
                        // TARGET_TOOLCHAIN_ARCH_*_ATTRIBUTE, and at next loop,
                        // become TARGET_TOOLCHAIN_*_ARCH_ATTRIBUTE
                        // 
                        tmpKey.setKey("*", tmpLevel);
                        //
                        // We'll try all possible combinations between current
                        // part and the fourth part.
                        // 
                        tmpLevel = matchLevel;
                    } else {
                        //
                        // Restore original value of key if "*" at "tmplevel"
                        // part of "name" has been checked
                        // 
                        tmpKey.setKey(keySet[tmpLevel], tmpLevel);
                        //
                        // Try "*" at part left to "tmplevel" part of "name"
                        // 
                        --tmpLevel;
                        continue;
                    }
                } catch (Exception e) {
                    return null;
                }

                //
                // Try get the value from the map
                // 
                result = map.get(tmpKey);
                if (result != null) {
                    //
                    // The map actually has no exact key as the given "key", 
                    // putting it back into map can speed up the get() next time
                    // 
                    map.put(key, result);
                    return result;
                }
            }
            ///
            /// If all possible combinations of "wildcard" between "level" and 
            /// the fourth part of "name" have been tried, try the left part
            /// 
            --level;
        }

        //
        // The map actually has no exact key as the given "key", putting it back
        // into map can speed up the get() next time even we got nothing.
        // 
        map.put(key, result);
        return result;
    }

    /**
       Wrapper function for Map.size().

       @return int  The size of map
     **/
    public int size() {
        return map.size();
    }

    /**
       Wrapper function for Map.keySet().

       @return Set<ToolChainKey>    A set of ToolChainKey objects
     */
    public Set<ToolChainKey> keySet() {
        return (Set<ToolChainKey>)map.keySet();
    }
}

