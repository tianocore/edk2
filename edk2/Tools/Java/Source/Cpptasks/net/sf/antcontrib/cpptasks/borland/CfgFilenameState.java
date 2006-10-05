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
package net.sf.antcontrib.cpptasks.borland;
import net.sf.antcontrib.cpptasks.parser.AbstractParser;
import net.sf.antcontrib.cpptasks.parser.AbstractParserState;
import net.sf.antcontrib.cpptasks.parser.FilenameState;
public class CfgFilenameState extends FilenameState {
    private char terminator;
    public CfgFilenameState(AbstractParser parser, char[] terminators) {
        super(parser, terminators);
        terminator = terminators[0];
    }
    public AbstractParserState consume(char ch) {
        //
        //   if a ';' is encountered then
        //      close the previous filename by sending a
        //         recognized terminator to our super class
        //      and stay in this state for more filenamese
        if (ch == ';') {
            super.consume(terminator);
            return this;
        }
        AbstractParserState newState = super.consume(ch);
        //
        //   change null (consume to end of line)
        //      to look for next switch character
        if (newState == null) {
            newState = getParser().getNewLineState();
        }
        return newState;
    }
}
