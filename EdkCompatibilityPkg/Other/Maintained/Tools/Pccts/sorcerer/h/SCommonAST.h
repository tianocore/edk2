#ifndef SCommonAST_h
#define SCommonAST_h

#include <stdio.h>
#include "PCCTSAST.h"
#include "SASTBase.h"

/* If you use SORCERER alone, you can subclass this to get a nice tree def */

class SORCommonAST : public SORASTBase {
protected:
  SORCommonAST *_right, *_down;
  int _type;

public:
  SORCommonAST() { _right = _down = NULL; }
  PCCTS_AST *right()  { return _right; }  // define the SORCERER interface
  PCCTS_AST *down()  { return _down; }
  int type()     { return _type; }
  void setRight(PCCTS_AST *t) { _right = (SORCommonAST *)t; }
  void setDown(PCCTS_AST *t)  { _down = (SORCommonAST *)t; }
  void setType(int t)     { _type = t; }
  virtual PCCTS_AST *shallowCopy() {return NULL;}
};

#endif
