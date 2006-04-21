// FILE:        BufFileInput.cpp
// AUTHOR:      Alexey Demakov (AVD) demakov@kazbek.ispras.ru
// CREATION:    26-JAN-1998
// DESCRIPTION: File Input Stream with lookahead for Scanner.
//   See file BufFileInput.h for details

// Change History:
//
//   22-Jun-1998    assert.h -> PCCTS_ASSERT_H
//                  string.h -> PCCTS_STRING_H
//
//   28-May-1998    Add virtual destructor to release buffer.
//
//                  Add dummy definition for ANTLRTokenType
//                  to allow compilation without knowing
//                  token type codes.
//
//                  Manfred Kogler (km@cast.uni-linz.ac.at)
//                  (1.33MR14)
//
//   20-Jul-1998    MR14a - Reorder initialization list for ctor.
//

enum ANTLRTokenType {TER_HATES_CPP=0, SO_DO_OTHERS=9999 };

#include "pcctscfg.h"
#include "pccts_assert.h"
#include "pccts_string.h"

PCCTS_NAMESPACE_STD

#include "BufFileInput.h"

BufFileInput::BufFileInput( FILE *f, int buf_size )
: input( f ),
  buf( new int[buf_size] ),
  size( buf_size ),
  start( 0 ),
  len( 0 )
{
}

BufFileInput::~BufFileInput()
{
  delete [] buf;
}

int BufFileInput::nextChar( void )
{
    if( len > 0 )
    {
        // get char from buffer
        int c = buf[start];

        if( c != EOF )
        {
            start++; start %= size;
            len--;
        }
        return c;
    } else {
        // get char from file
        int c = getc( input );

        if( c == EOF )
        {
            // if EOF - put it in the buffer as indicator
            buf[start] = EOF;
            len++;
        }
        return c;
    }
}

int BufFileInput::lookahead( char* s )
{
    int l = strlen( s );

    assert( 0 < l && l <= size );

    while( len < l )
    {
        int c = getc( input );

        buf[ (start+len) % size ] = c;

        len++;

        if( c == EOF ) return 0;
    }

    for( int i = 0; i < l; i++ )
    {
        if( s[i] != buf[ (start+i) % size ] ) return 0;
    }
    return 1;
}

// End of file BufFileInput.cpp

