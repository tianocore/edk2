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
 * A parser that extracts INCLUDE statements from a Reader.
 *
 * @author Curt Arnold
 */
public final class FortranParser
    extends AbstractParser
    implements Parser {
  /**
   * List of included filenames.
   */
  private final Vector includes = new Vector();

  /**
   * State that starts consuming content at the beginning of a line.
   */
  private final AbstractParserState newLineState;

  /**
   * Default constructor.
   *
   */
  public FortranParser() {
    AbstractParserState filename = new FilenameState(this, new char[] {'\'',
        '/'});
    AbstractParserState apos = new WhitespaceOrLetterState(this, '\'',
        filename);
    AbstractParserState blank = new LetterState(this, ' ', apos, null);
    AbstractParserState e = new CaseInsensitiveLetterState(this, 'E',
        blank, null);
    AbstractParserState d = new CaseInsensitiveLetterState(this, 'D', e,
        null);
    AbstractParserState u = new CaseInsensitiveLetterState(this, 'U', d,
        null);
    AbstractParserState l = new CaseInsensitiveLetterState(this, 'L', u,
        null);
    AbstractParserState c = new CaseInsensitiveLetterState(this, 'C', l,
        null);
    AbstractParserState n = new CaseInsensitiveLetterState(this, 'N', c,
        null);
    newLineState = new WhitespaceOrCaseInsensitiveLetterState(this, 'I', n);
  }

  /**
   * Called by FilenameState at completion of file name production.
   *
   * @param include
   *            include file name
   */
  public void addFilename(final String include) {
    includes.addElement(include);
  }

  /**
   * Gets collection of include file names encountered in parse.
   * @return include file names
   */
  public String[] getIncludes() {
    String[] retval = new String[includes.size()];
    includes.copyInto(retval);
    return retval;
  }

  /**
   * Get the state for the beginning of a new line.
   * @return start of line state
   */
  public AbstractParserState getNewLineState() {
    return newLineState;
  }

  /**
   * Collects all included files from the content of the reader.
   *
   * @param reader
   *            character reader containing a FORTRAN source module
   * @throws IOException
   *             throw if I/O error during parse
   */
  public void parse(final Reader reader) throws IOException {
    includes.setSize(0);
    super.parse(reader);
  }
}
