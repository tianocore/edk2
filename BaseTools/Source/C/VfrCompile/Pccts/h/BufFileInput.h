// FILE:        BufFileInput.h
// AUTHOR:      Alexey Demakov (AVD) demakov@kazbek.ispras.ru
// CREATION:    26-JAN-1998
// DESCRIPTION: File Input Stream with lookahead for Scanner
// Tested under Win32 with ANTLR 1.33 MR10 and MSVC 5.0

// Change History:
//
//   28-May-1998    Add virtual destructor to release buffer
//                  Manfred Kogler (km@cast.uni-linz.ac.at)
//                  (1.33MR14)

#ifndef BufFileInput_h
#define BufFileInput_h

#include "pcctscfg.h"

#include "pccts_stdio.h"

PCCTS_NAMESPACE_STD

#include "DLexerBase.h"

class DllExportPCCTS BufFileInput : public DLGInputStream
{
public:
    // constructor
    // f - input stream
    // buf_size - size of buffer (maximal length for string in is_in)

    BufFileInput(FILE *f, int buf_size = 8 );

    virtual ~BufFileInput();

    // gets next char from stream

    virtual int nextChar( void );

    // looks in stream and compares next l characters with s
    // returns the result of comparision

    int lookahead( char* s );

private:
    FILE *input; // input stream;
    int* buf;    // buffer
    int  size;   // size of buffer
    int  start;  // position of the first symbol in buffer
    int  len;    // count of characters in buffers
};

#endif
// end of file BufFileInput.h
