
/************************************************************/
/*                              */
/*    Predefined char stream: Input from (c++) stream.  */
/*                              */
/* By Hubert Holin (Hubert.Holin@Bigfoot.com), 1998.    */
/*                              */
/* This is completely free stuff, do whatever you want with  */
/* it (but then, I will take no responsability for whatever  */
/* may happen if you do either... caveat emptor!).      */
/*                              */
/************************************************************/

#ifndef _DLG_STREAM_INPUT_H
#define _DLG_STREAM_INPUT_H

#include "pccts_istream.h"

PCCTS_NAMESPACE_STD

#ifndef DLGX_H
#include "DLexerBase.h"
#endif


// NOTES:  The semantics of the copy constructor
//      and the affectation operator may be unwaranted...
//      and the stream may not be reset.
//
//      It would have been so much nicer for nextChar()
//      to throw (of for the DLGInputStream to change status)
//      upon hiting EOF than to return an "int"...

template  <
        class E,
        class T = ::std::char_traits<E>
      >
class DLG_stream_input : public DLGInputStream
{
public:
  
            DLG_stream_input(::std::basic_istream<E,T> * p_input_stream)
  :  input(p_input_stream)
  {
    // nothing to do!
  };
  
            DLG_stream_input(const DLG_stream_input & a_recopier)
  :  input(a_recopier.input)
  {
    // nothing to do!
  };
  
  virtual        ~DLG_stream_input()
  {
    this->purge();  // bloody templarized lookup...
  };
  
  DLG_stream_input  operator = (const DLG_stream_input & a_affecter)
  {
    if (this != &a_affecter)
    {
      input = a_affecter.input;
    }

    return(*this);
  };
  
  virtual int      nextChar()
  {
    E  extracted_stuff;
    
    input->get(extracted_stuff);
    
    if  (*input)
    {
      return(int(extracted_stuff));
    }
    else
    {
      return(EOF);
    }
  };
  
protected:
  
  ::std::basic_istream<E,T> *  input;
  
private:
  
  void  purge()
  {
    // nothing to do!
  };
};

#endif /* _DLG_STREAM_INPUT_H */

