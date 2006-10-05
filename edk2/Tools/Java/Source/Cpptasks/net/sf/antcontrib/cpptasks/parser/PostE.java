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
public class PostE extends AbstractParserState {
    private AbstractParserState bracket;
    private AbstractParserState quote;
    public PostE(CParser parser, AbstractParserState bracket,
            AbstractParserState quote) {
        super(parser);
        this.bracket = bracket;
        this.quote = quote;
    }
    public AbstractParserState consume(char ch) {
        switch (ch) {
            case ' ' :
            case '\t' :
                return this;
            case '<' :
                return bracket;
            case '"' :
                return quote;
            case '\n' :
                return getParser().getNewLineState();
        }
        return null;
    }
}
