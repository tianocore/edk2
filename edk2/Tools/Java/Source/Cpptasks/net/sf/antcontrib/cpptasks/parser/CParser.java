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
import java.io.IOException;
import java.io.Reader;
import java.util.Vector;
/**
 * A parser that extracts #include statements from a Reader.
 * 
 * @author Adam Murdoch
 * @author Curt Arnold
 */
public final class CParser extends AbstractParser implements Parser {
    private final Vector includes = new Vector();
    private AbstractParserState newLineState;
    /**
     * 
     *  
     */
    public CParser() {
        AbstractParserState quote = new FilenameState(this, new char[]{'"'});
        AbstractParserState bracket = new FilenameState(this, new char[]{'>'});
        AbstractParserState postE = new PostE(this, bracket, quote);
        //
        //    nclude
        //
        AbstractParserState e = new LetterState(this, 'e', postE, null);
        AbstractParserState d = new LetterState(this, 'd', e, null);
        AbstractParserState u = new LetterState(this, 'u', d, null);
        AbstractParserState l = new LetterState(this, 'l', u, null);
        AbstractParserState c = new LetterState(this, 'c', l, null);
        AbstractParserState n = new LetterState(this, 'n', c, null);
        //
        //   mport is equivalent to nclude
        //
        AbstractParserState t = new LetterState(this, 't', postE, null);
        AbstractParserState r = new LetterState(this, 'r', t, null);
        AbstractParserState o = new LetterState(this, 'o', r, null);
        AbstractParserState p = new LetterState(this, 'p', o, null);
        AbstractParserState m = new LetterState(this, 'm', p, null);
        //
        //   switch between
        //
        AbstractParserState n_m = new BranchState(this, new char[]{'n', 'm'},
                new AbstractParserState[]{n, m}, null);
        AbstractParserState i = new WhitespaceOrLetterState(this, 'i', n_m);
        newLineState = new LetterState(this, '#', i, null);
    }
    public void addFilename(String include) {
        includes.addElement(include);
    }
    public String[] getIncludes() {
        String[] retval = new String[includes.size()];
        includes.copyInto(retval);
        return retval;
    }
    public AbstractParserState getNewLineState() {
        return newLineState;
    }
    public void parse(Reader reader) throws IOException {
        includes.setSize(0);
        super.parse(reader);
    }
}
