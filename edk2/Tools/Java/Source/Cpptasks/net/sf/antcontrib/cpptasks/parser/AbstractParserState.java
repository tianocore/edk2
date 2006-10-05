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
package net.sf.antcontrib.cpptasks.parser;
/**
 * An base class for objects that represent the state of an AbstractParser.
 * 
 * @author CurtArnold
 * @see AbstractParser
 */
public abstract class AbstractParserState {
    private AbstractParser parser;
    protected AbstractParserState(AbstractParser parser) {
        if (parser == null) {
            throw new NullPointerException("parser");
        }
        this.parser = parser;
    }
    /**
     * Consume a character
     * 
     * @return new state, may be null to ignore the rest of the line
     */
    public abstract AbstractParserState consume(char ch);
    protected AbstractParser getParser() {
        return parser;
    }
}
