/*
 * 
 * Copyright 2002-2004 The Ant-Contrib project
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package net.sf.antcontrib.cpptasks.types;
import java.lang.reflect.Method;

/**
 * Helper class which can be used for Ant task attribute setter methods to
 * allow the build file to specify a long in either decimal, octal, or
 * hexadecimal format.
 *  // FlexInteger author
 * @author Erik Hatcher
 * @see org.apache.tools.ant.types.FlexInteger
 */
public class FlexLong {
    private Long value;
    /**
     * Constructor used by Ant's introspection mechanism for attribute
     * population
     */
    public FlexLong(String value) {
        // Java 1.1 did not support Long.decode().. so we call it by
        // reflection.
        try {
            Method m = Long.class
                    .getMethod("decode", new Class[]{String.class});
            Object rc = m.invoke(null, new Object[]{value});
            this.value = (Long) rc;
        } catch (Exception e) {
            // Try it the old fashioned way, we must be on a 1.1 jre
            this.value = new Long(value);
        }
    }
    /**
     * Returns the decimal integer value
     */
    public long longValue() {
        return value.longValue();
    }
    /**
     * Overridden method to return the decimal value for display
     */
    public String toString() {
        return value.toString();
    }
}
