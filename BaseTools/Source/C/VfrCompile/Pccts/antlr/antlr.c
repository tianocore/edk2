/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   ..\bin\antlr -gh antlr.g
 *
 */

#define ANTLR_VERSION	13333
#include "pcctscfg.h"
#include "pccts_stdio.h"

#include "pcctscfg.h"
#include "set.h"
#include <ctype.h>
#include "syn.h"
#include "hash.h"
#include "generic.h"
#define zzcr_attr(attr,tok,t)
#define zzSET_SIZE 20
#include "antlr.h"
#include "tokens.h"
#include "dlgdef.h"
#include "mode.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif

ANTLR_INFO


/* MR20 G. Hobbelt For Borland C++ 4.x & 5.x compiling with ALL warnings enabled */
#if defined(__TURBOC__)
#pragma warn -aus  /* unused assignment of 'xxx' */
#endif


#ifdef __USE_PROTOS
static void chkToken(char *, char *, char *, int);
#else
static void chkToken();
#endif

#ifdef __USE_PROTOS
static int isDLGmaxToken(char *Token);				     /* MR3 */
#else
static int isDLGmaxToken();				                             /* MR3 */
#endif

static int class_nest_level = 0;

/* MR20 G. Hobbelt extern definitions moved to antlr.h */

  

void
#ifdef __USE_PROTOS
grammar(void)
#else
grammar()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  Graph g;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x1))) break;
      if ( (LA(1)==94) ) {
        zzmatch(94); zzCONSUME;
        zzmatch(Action);
        
        if ( HdrAction==NULL ) {
          HdrAction = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
          require(HdrAction!=NULL, "rule grammar: cannot allocate header action");
          strcpy(HdrAction, LATEXT(1));
        }
        else warn("additional #header statement ignored");
 zzCONSUME;

      }
      else {
        if ( (LA(1)==95) ) {
          zzmatch(95); zzCONSUME;
          zzmatch(Action);
          
          if ( FirstAction==NULL ) {
            FirstAction = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
            require(FirstAction!=NULL, "rule grammar: cannot allocate #first action");
            strcpy(FirstAction, LATEXT(1));
          } else {
            warn("additional #first statement ignored");
          };
 zzCONSUME;

        }
        else {
          if ( (LA(1)==96) ) {
            zzmatch(96); zzCONSUME;
            zzmatch(QuotedTerm);
            
            if ( GenCC ) {
              warn("#parser meta-op incompatible with -CC; ignored");
            }
            else {
              if ( strcmp(ParserName,"zzparser")==0 ) {
                ParserName=StripQuotes(mystrdup(LATEXT(1)));
                if ( RulePrefix[0]!='\0' )
                {
                  warn("#parser meta-op incompatible with '-gp prefix'; '-gp' ignored");
                  RulePrefix[0]='\0';
                }
              }
              else warn("additional #parser statement ignored");
            }
 zzCONSUME;

          }
          else {
            if ( (LA(1)==97) ) {
              zzmatch(97); zzCONSUME;
              zzmatch(QuotedTerm);
              {
                char *fname;
                zzantlr_state st; FILE *f; struct zzdlg_state dst;
                UserTokenDefsFile = mystrdup(LATEXT(1));
                zzsave_antlr_state(&st);
                zzsave_dlg_state(&dst);
                fname = mystrdup(LATEXT(1));
                f = fopen(StripQuotes(fname), "r");
                if ( f==NULL ) {warn(eMsg1("cannot open token defs file '%s'", fname+1));}
                else {
                  ANTLRm(enum_file(fname+1), f, PARSE_ENUM_FILE);
                  UserDefdTokens = 1;
                }
                zzrestore_antlr_state(&st);
                zzrestore_dlg_state(&dst);
              }
 zzCONSUME;

            }
            else break; /* MR6 code for exiting loop "for sure" */
          }
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x2))) break;
      if ( (LA(1)==Action) ) {
        zzmatch(Action);
        {
          UserAction *ua = newUserAction(LATEXT(1));
          ua->file = action_file; ua->line = action_line;
          if ( class_nest_level>0 ) list_add(&class_before_actions, ua);
          else list_add(&BeforeActions, ua);
        }
 zzCONSUME;

      }
      else {
        if ( (LA(1)==108) ) {
          laction();
        }
        else {
          if ( (LA(1)==109) ) {
            lmember();
          }
          else {
            if ( (LA(1)==110) ) {
              lprefix();
            }
            else {
              if ( (LA(1)==116) ) {
                aLexclass();
              }
              else {
                if ( (LA(1)==120) ) {
                  token();
                }
                else {
                  if ( (LA(1)==117) ) {
                    error();
                  }
                  else {
                    if ( (LA(1)==118) ) {
                      tclass();
                    }
                    else {
                      if ( (LA(1)==111) ) {
                        aPred();
                      }
                      else {
                        if ( (LA(1)==133) ) {
                          default_exception_handler();
                        }
                        else {
                          if ( (LA(1)==99) ) {
                            class_def();
                          }
                          else {
                            if ( (LA(1)==98) ) {
                              zzmatch(98);
                              
                              if ( class_nest_level==0 )
                              warn("missing class definition for trailing '}'");
                              class_nest_level--;
 zzCONSUME;

                            }
                            else break; /* MR6 code for exiting loop "for sure" */
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  rule();
  g=zzaArg(zztasp1,3); SynDiag = (Junction *) zzaArg(zztasp1,3 ).left;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x4))) break;
      if ( (LA(1)==NonTerminal) ) {
        rule();
        if ( zzaArg(zztasp2,1 ).left!=NULL ) {
          g.right = NULL;
          
/* MR21a */             /*  Avoid use of a malformed graph when CannotContinue */
          /* MR21a */             /*  is already set                                     */
          /* MR21a */
          /* MR21a */             if (! (CannotContinue && g.left == NULL)) {
            /* MR21a */               g = Or(g, zzaArg(zztasp2,1));
            /* MR21a */             }
          /* MR21a */		      }
      }
      else {
        if ( (LA(1)==116) ) {
          aLexclass();
        }
        else {
          if ( (LA(1)==120) ) {
            token();
          }
          else {
            if ( (LA(1)==117) ) {
              error();
            }
            else {
              if ( (LA(1)==118) ) {
                tclass();
              }
              else {
                if ( (LA(1)==111) ) {
                  aPred();
                }
                else {
                  if ( (LA(1)==99) ) {
                    class_def();
                  }
                  else {
                    if ( (LA(1)==98) ) {
                      zzmatch(98);
                      
                      if ( class_nest_level==0 )
                      warn("missing class definition for trailing '}'");
                      class_nest_level--;
 zzCONSUME;

                    }
                    else break; /* MR6 code for exiting loop "for sure" */
                  }
                }
              }
            }
          }
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    for (;;) {
      if ( !((setwd1[LA(1)]&0x8))) break;
      if ( (LA(1)==Action) ) {
        zzmatch(Action);
        {
          UserAction *ua = newUserAction(LATEXT(1));
          ua->file = action_file; ua->line = action_line;
          if ( class_nest_level>0 ) list_add(&class_after_actions, ua);
          else list_add(&AfterActions, ua);
        }
 zzCONSUME;

      }
      else {
        if ( (LA(1)==108) ) {
          laction();
        }
        else {
          if ( (LA(1)==109) ) {
            lmember();
          }
          else {
            if ( (LA(1)==110) ) {
              lprefix();
            }
            else {
              if ( (LA(1)==117) ) {
                error();
              }
              else {
                if ( (LA(1)==118) ) {
                  tclass();
                }
                else {
                  if ( (LA(1)==99) ) {
                    class_def();
                  }
                  else {
                    if ( (LA(1)==111) ) {
                      aPred();
                    }
                    else {
                      if ( (LA(1)==98) ) {
                        zzmatch(98);
                        
                        if ( class_nest_level==0 )
                        warn("missing class definition for trailing '}'");
                        class_nest_level--;
 zzCONSUME;

                      }
                      else break; /* MR6 code for exiting loop "for sure" */
                    }
                  }
                }
              }
            }
          }
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(Eof); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x10);
  }
}

void
#ifdef __USE_PROTOS
class_def(void)
#else
class_def()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int go=1; char name[MaxRuleName+1];
  zzmatch(99); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==NonTerminal) ) {
      zzmatch(NonTerminal);
      if(go) strncpy(name,LATEXT(1),MaxRuleName);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==TokenTerm) ) {
        zzmatch(TokenTerm);
        if(go) strncpy(name,LATEXT(1),MaxRuleName);
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  if ( CurrentClassName[0]!='\0' && strcmp(CurrentClassName,name)!=0
  && GenCC ) {
    err("only one grammar class allowed in this release");
    go = 0;
  }
  else strcpy(CurrentClassName, name);
  if ( !GenCC ) { err("class meta-op used without C++ option"); }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (setwd1[LA(1)]&0x20) ) {
      zzsetmatch(zzerr2, zzerr3);
      if (ClassDeclStuff == NULL) {
        /* MR10 */                   ClassDeclStuff=(char *)calloc(MaxClassDeclStuff+1,sizeof(char));
        /* MR10 */              };
      /* MR10 */              strncat(ClassDeclStuff," ",MaxClassDeclStuff);
      /* MR10 */              strncat(ClassDeclStuff,LATEXT(1),MaxClassDeclStuff);
      /* MR22 */              do {
        /* MR22 */                if (0 == strcmp(LATEXT(1),"public")) break;
        /* MR22 */                if (0 == strcmp(LATEXT(1),"private")) break;
        /* MR22 */                if (0 == strcmp(LATEXT(1),"protected")) break;
        /* MR22 */                if (0 == strcmp(LATEXT(1),"virtual")) break;
        /* MR22 */                if (0 == strcmp(LATEXT(1),",")) break;
        /* MR22 */                if (0 == strcmp(LATEXT(1),":")) break;
        /* MR22 */                if (BaseClassName != NULL) break;
        /* MR22 */                BaseClassName=(char *)calloc(strlen(LATEXT(1))+1,sizeof(char));
        /* MR22 */                require(BaseClassName!=NULL, "rule grammar: cannot allocate base class name");
        /* MR22 */				  strcpy(BaseClassName,LATEXT(1));
        /* MR22 */              } while (0);
      /* MR10 */
 zzCONSUME;

      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(102);
  
  no_classes_found = 0;
  if ( class_nest_level>=1 ) {warn("cannot have nested classes");}
  else class_nest_level++;
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x40);
  }
}

void
#ifdef __USE_PROTOS
rule(void)
#else
rule()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  
			ExceptionGroup *eg;
  RuleEntry *q; Junction *p; Graph r; int f, l; ECnode *e;
  set toksrefd, rulesrefd;
  char *pdecl=NULL, *ret=NULL, *a; CurRetDef = CurParmDef = NULL;
  CurExGroups = NULL;
  CurElementLabels = NULL;
  CurAstLabelsInActions = NULL; /* MR27 */
  /* We want a new element label hash table for each rule */
  if ( Elabel!=NULL ) killHashTable(Elabel);
  Elabel = newHashTable();
  attribsRefdFromAction = empty;
  zzmatch(NonTerminal);
  q=NULL;
  if ( hash_get(Rname, LATEXT(1))!=NULL ) {
    err(eMsg1("duplicate rule definition: '%s'",LATEXT(1)));
    CannotContinue=TRUE;
  }
  else
  {
    q = (RuleEntry *)hash_add(Rname,
    LATEXT(1),
    (Entry *)newRuleEntry(LATEXT(1)));
    CurRule = q->str;
  }
  CurRuleNode = q;
  f = CurFile; l = zzline;
  NumRules++;
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==103) ) {
      zzmatch(103);
      if ( q!=NULL ) q->noAST = TRUE;
 zzCONSUME;

    }
    else {
      if ( (setwd1[LA(1)]&0x80) ) {
      }
      else {zzFAIL(1,zzerr4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    ;
    if ( (setwd2[LA(1)]&0x1) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==104) ) {
          zzmatch(104); zzCONSUME;
        }
        else {
          if ( (LA(1)==PassAction) ) {
          }
          else {zzFAIL(1,zzerr5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(PassAction);
      pdecl = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(pdecl!=NULL, "rule rule: cannot allocate param decl");
      strcpy(pdecl, LATEXT(1));
      CurParmDef = pdecl;
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x2) ) {
      }
      else {zzFAIL(1,zzerr6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==105) ) {
      zzmatch(105); zzCONSUME;
      zzmatch(PassAction);
      ret = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(ret!=NULL, "rule rule: cannot allocate ret type");
      strcpy(ret, LATEXT(1));
      CurRetDef = ret;
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x4) ) {
      }
      else {zzFAIL(1,zzerr7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==QuotedTerm) ) {
      zzmatch(QuotedTerm);
      if ( q!=NULL ) q->egroup=mystrdup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (LA(1)==106) ) {
      }
      else {zzFAIL(1,zzerr8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  if ( GenEClasseForRules && q!=NULL ) {
    e = newECnode;
    require(e!=NULL, "cannot allocate error class node");
    if ( q->egroup == NULL ) {a = q->str; a[0] = (char)toupper(a[0]);}
    else a = q->egroup;
    if ( Tnum( a ) == 0 )
    {
      e->tok = addTname( a );
      list_add(&eclasses, (char *)e);
      if ( q->egroup == NULL ) a[0] = (char)tolower(a[0]);
      /* refers to itself */
      list_add(&(e->elist), mystrdup(q->str));
    }
    else {
      warn(eMsg1("default errclass for '%s' would conflict with token/errclass/tokclass",a));
      if ( q->egroup == NULL ) a[0] = (char)tolower(a[0]);
      free((char *)e);
    }
  }
  BlkLevel++;
  if (BlkLevel >= MAX_BLK_LEVEL) fatal("Blocks nested too deeply");
  /* MR23 */    CurBlockID_array[BlkLevel] = CurBlockID;
  /* MR23 */    CurAltNum_array[BlkLevel] = CurAltNum;
  zzmatch(106);
  inAlt=1;
 zzCONSUME;

  block( &toksrefd, &rulesrefd );
  r = makeBlk(zzaArg(zztasp1,7),0, NULL /* pFirstSetSymbol */ );
  CurRuleBlk = (Junction *)r.left;
  CurRuleBlk->blockid = CurBlockID;
  CurRuleBlk->jtype = RuleBlk;
  if ( q!=NULL ) CurRuleBlk->rname = q->str;
  CurRuleBlk->file = f;
  CurRuleBlk->line = l;
  CurRuleBlk->pdecl = pdecl;
  CurRuleBlk->ret = ret;
  CurRuleBlk->lock = makelocks();
  CurRuleBlk->pred_lock = makelocks();
  CurRuleBlk->tokrefs = toksrefd;
  CurRuleBlk->rulerefs = rulesrefd;
  p = newJunction();	/* add EndRule Node */
  ((Junction *)r.right)->p1 = (Node *)p;
  r.right = (Node *) p;
  p->jtype = EndRule;
  p->lock = makelocks();
  p->pred_lock = makelocks();
  CurRuleBlk->end = p;
  if ( q!=NULL ) q->rulenum = NumRules;
  zzaArg(zztasp1,7) = r;
  
  /* MR23 */      CurBlockID_array[BlkLevel] = (-1);
  /* MR23 */      CurAltNum_array[BlkLevel] = (-1);                
  --BlkLevel;
  altFixup();leFixup();egFixup();
  zzmatch(107);
  inAlt=0;
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==Action) ) {
      zzmatch(Action);
      a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(a!=NULL, "rule rule: cannot allocate error action");
      strcpy(a, LATEXT(1));
      CurRuleBlk->erraction = a;
 zzCONSUME;

    }
    else {
      if ( (setwd2[LA(1)]&0x8) ) {
      }
      else {zzFAIL(1,zzerr9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==133) ) {
       eg  = exception_group();

      if ( eg!=NULL ) {
        list_add(&CurExGroups, (void *)eg);
        if (eg->label == NULL || *eg->label=='\0' ) q->has_rule_exception = 1;
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  if ( q==NULL ) zzaArg(zztasp1,0 ).left = NULL; else zzaArg(zztasp1,0) = zzaArg(zztasp1,7);
  CurRuleBlk->exceptions = CurExGroups;
  CurRuleBlk->el_labels = CurElementLabels;
  CurRuleNode->ast_labels_in_actions = CurAstLabelsInActions;
  CurRuleNode = NULL;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x10);
  }
}

void
#ifdef __USE_PROTOS
laction(void)
#else
laction()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *a;
  zzmatch(108); zzCONSUME;
  zzmatch(Action);
  
  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
  require(a!=NULL, "rule laction: cannot allocate action");
  strcpy(a, LATEXT(1));
  list_add(&LexActions, a);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x20);
  }
}

void
#ifdef __USE_PROTOS
lmember(void)
#else
lmember()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *a;
  zzmatch(109); zzCONSUME;
  zzmatch(Action);
  
  /* MR1 */		if (! GenCC) {
    /* MR1 */		  err("Use #lexmember only in C++ mode (to insert code in DLG class header");
    /* MR1 */	        } else {
    /* MR1 */		  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
    /* MR1 */		  require(a!=NULL, "rule lmember: cannot allocate action");
    /* MR1 */		  strcpy(a, LATEXT(1));
    /* MR1 */		  list_add(&LexMemberActions, a);
    /* MR1 */		};
  /* MR1 */
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x40);
  }
}

void
#ifdef __USE_PROTOS
lprefix(void)
#else
lprefix()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *a;
  zzmatch(110); zzCONSUME;
  zzmatch(Action);
  
  /* MR1 */		if (! GenCC) {
    /* MR1 */		  err("Use #lexprefix only in C++ mode (to insert code in DLG class header");
    /* MR1 */	        } else {
    /* MR1 */		  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
    /* MR1 */		  require(a!=NULL, "rule lprefix: cannot allocate action");
    /* MR1 */		  strcpy(a, LATEXT(1));
    /* MR1 */		  list_add(&LexPrefixActions, a);
    /* MR1 */		};
  /* MR1 */
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x80);
  }
}

void
#ifdef __USE_PROTOS
aPred(void)
#else
aPred()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  PredEntry     *predEntry=NULL;
  char          *name=NULL;
  Predicate     *predExpr=NULL;
  char          *predLiteral=NULL;
  int           save_file;
  int           save_line;
  int           predExprPresent=0;
  zzmatch(111);
  
  MR_usingPredNames=1;      /* will need to use -mrhoist version of genPredTree */
 zzCONSUME;

  zzmatch(TokenTerm);
  name=mystrdup(LATEXT(1));
 zzCONSUME;

  
  /* don't free - referenced in predicates */
  
            CurPredName=(char *)calloc(1,strlen(name) + 10);
  strcat(CurPredName,"#pred ");
  strcat(CurPredName,name);
  
            predEntry=(PredEntry *) hash_get(Pname,name);
  if (predEntry != NULL) {
  warnFL(eMsg1("#pred %s previously defined - ignored",name),
  FileStr[action_file],action_line);
  name=NULL;
};
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==Pred) ) {
      zzmatch(Pred);
      predLiteral=mystrdup(LATEXT(1));
      save_line=action_line;
      save_file=action_file;
 zzCONSUME;

      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (setwd3[LA(1)]&0x1) ) {
           predExpr  = predOrExpr();

          predExprPresent=1;
        }
        else {
          if ( (setwd3[LA(1)]&0x2) ) {
          }
          else {zzFAIL(1,zzerr10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      if (predLiteral != NULL && name != NULL) {
        
                      /*
        *  predExpr may be NULL due to syntax errors
        *    or simply omitted by the user
        */
        
                      predEntry=newPredEntry(name);
        predEntry->file=save_file;
        predEntry->line=save_line;
        predExpr=MR_predFlatten(predExpr);
        predEntry->predLiteral=predLiteral;
        if (! predExprPresent || predExpr == NULL) {
          predExpr=new_pred();
          predExpr->expr=predLiteral;
          predExpr->source=newActionNode();
          predExpr->source->action=predExpr->expr;
          predExpr->source->rname=CurPredName;
          predExpr->source->line=action_line;
          predExpr->source->file=action_file;
          predExpr->source->is_predicate=1;
          predExpr->k=predicateLookaheadDepth(predExpr->source);
        };
        predEntry->pred=predExpr;
        hash_add(Pname,name,(Entry *)predEntry);
        predExpr=NULL;
      };
      predicate_free(predExpr);
    }
    else {
      if ( (setwd3[LA(1)]&0x4) ) {
        save_line=zzline; save_file=CurFile;
         predExpr  = predOrExpr();

        if (predExpr != NULL && name != NULL) {
          predEntry=newPredEntry(name);
          predEntry->file=CurFile;
          predEntry->line=zzline;
          predExpr=MR_predFlatten(predExpr);
          predEntry->pred=predExpr;
          hash_add(Pname,name,(Entry *)predEntry);
          predExpr=NULL;
        };
        predicate_free(predExpr);
      }
      else {zzFAIL(1,zzerr11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==107) ) {
      zzmatch(107); zzCONSUME;
    }
    else {
      if ( (setwd3[LA(1)]&0x8) ) {
      }
      else {zzFAIL(1,zzerr12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  predicate_free(predExpr);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x10);
  }
}

Predicate *
#ifdef __USE_PROTOS
predOrExpr(void)
#else
predOrExpr()
#endif
{
  Predicate *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(Predicate *  ))
  zzMake0;
  {
  Predicate     *ORnode;
  Predicate     *predExpr;
  Predicate     **tail=NULL;
   predExpr  = predAndExpr();

  
  ORnode=new_pred();
  ORnode->expr=PRED_OR_LIST;
  if (predExpr != NULL) {
    ORnode->down=predExpr;
    tail=&predExpr->right;
  };
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==112) ) {
      zzmatch(112); zzCONSUME;
       predExpr  = predAndExpr();

      
      if (predExpr != NULL) {
        *tail=predExpr;
        tail=&predExpr->right;
      };
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  
  _retv=ORnode;
  ORnode=NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  predicate_free(ORnode);  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x20);
  return _retv;
  }
}

Predicate *
#ifdef __USE_PROTOS
predAndExpr(void)
#else
predAndExpr()
#endif
{
  Predicate *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(Predicate *  ))
  zzMake0;
  {
  Predicate     *ANDnode;
  Predicate     *predExpr;
  Predicate     **tail=NULL;
   predExpr  = predPrimary();

  
  ANDnode=new_pred();
  ANDnode->expr=PRED_AND_LIST;
  if (predExpr != NULL) {
    ANDnode->down=predExpr;
    tail=&predExpr->right;
  };
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==113) ) {
      zzmatch(113); zzCONSUME;
       predExpr  = predPrimary();

      
      if (predExpr != NULL) {
        *tail=predExpr;
        tail=&predExpr->right;
      };
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  
  _retv=ANDnode;
  ANDnode=NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  predicate_free(ANDnode);  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x40);
  return _retv;
  }
}

Predicate *
#ifdef __USE_PROTOS
predPrimary(void)
#else
predPrimary()
#endif
{
  Predicate *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(Predicate *  ))
  zzMake0;
  {
  
  char          *name=NULL;
  PredEntry     *predEntry=NULL;
  Predicate     *predExpr=NULL;
  if ( (LA(1)==TokenTerm) ) {
    zzmatch(TokenTerm);
    name=mystrdup(LATEXT(1));
 zzCONSUME;

    
    predEntry=(PredEntry *) hash_get(Pname,name);
    if (predEntry == NULL) {
      warnFL(eMsg1("no previously defined #pred with name \"%s\"",name),
      FileStr[CurFile],zzline);
      name=NULL;
      _retv=NULL;
    } else {
      predExpr=predicate_dup(predEntry->pred);
      predExpr->predEntry=predEntry;
      _retv=predExpr;
    };
  }
  else {
    if ( (LA(1)==114) ) {
      zzmatch(114); zzCONSUME;
       predExpr  = predOrExpr();

      zzmatch(115);
      
      _retv=predExpr;
 zzCONSUME;

    }
    else {
      if ( (LA(1)==103) ) {
        zzmatch(103); zzCONSUME;
         predExpr  = predPrimary();

        
        predExpr->inverted=!predExpr->inverted;
        _retv=predExpr;
      }
      else {zzFAIL(1,zzerr13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  
  predicate_free(predExpr);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x80);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
aLexclass(void)
#else
aLexclass()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(116); zzCONSUME;
  zzmatch(TokenTerm);
  lexclass(mystrdup(LATEXT(1)));
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x1);
  }
}

void
#ifdef __USE_PROTOS
error(void)
#else
error()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *t=NULL; ECnode *e; int go=1; TermEntry *p;
  zzmatch(117); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    ;
    if ( (LA(1)==TokenTerm) ) {
      zzmatch(TokenTerm);
      t=mystrdup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (LA(1)==QuotedTerm) ) {
        zzmatch(QuotedTerm);
        t=mystrdup(LATEXT(1));
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  e = newECnode;
  require(e!=NULL, "cannot allocate error class node");
  e->lexclass = CurrentLexClass;
  if ( Tnum( (t=StripQuotes(t)) ) == 0 )
  {
    if ( hash_get(Texpr, t) != NULL )
    warn(eMsg1("errclass name conflicts with regular expression  '%s'",t));
    e->tok = addTname( t );
    set_orel(e->tok, &imag_tokens);
    require((p=(TermEntry *)hash_get(Tname, t)) != NULL,
    "hash table mechanism is broken");
    p->classname = 1;	/* entry is errclass name, not token */
    list_add(&eclasses, (char *)e);
  }
  else
  {
  warn(eMsg1("redefinition of errclass or conflict w/token or tokclass '%s'; ignored",t));
  free( (char *)e );
  go=0;
}
  zzmatch(102); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==NonTerminal) ) {
      zzmatch(NonTerminal);
      if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (LA(1)==TokenTerm) ) {
        zzmatch(TokenTerm);
        if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

      }
      else {
        if ( (LA(1)==QuotedTerm) ) {
          zzmatch(QuotedTerm);
          if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

        }
        else {zzFAIL(1,zzerr15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzEXIT(zztasp2);
    }
  }
  if ( go ) list_add(&(e->elist), t);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (setwd4[LA(1)]&0x2) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==NonTerminal) ) {
          zzmatch(NonTerminal);
          if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

        }
        else {
          if ( (LA(1)==TokenTerm) ) {
            zzmatch(TokenTerm);
            if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

          }
          else {
            if ( (LA(1)==QuotedTerm) ) {
              zzmatch(QuotedTerm);
              if ( go ) t=mystrdup(LATEXT(1));
 zzCONSUME;

            }
            else {zzFAIL(1,zzerr16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzEXIT(zztasp3);
        }
      }
      if ( go ) list_add(&(e->elist), t);
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(98); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x4);
  }
}

void
#ifdef __USE_PROTOS
tclass(void)
#else
tclass()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *t=NULL; TCnode *e; int go=1,tok,totok; TermEntry *p, *term, *toterm;
  char *akaString=NULL; int save_file; int save_line;
  char *totext=NULL;
  zzmatch(118); zzCONSUME;
  zzmatch(TokenTerm);
  t=mystrdup(LATEXT(1));
 zzCONSUME;

  e = newTCnode;
  require(e!=NULL, "cannot allocate token class node");
  e->lexclass = CurrentLexClass;
  if ( Tnum( t ) == 0 )
  {
    e->tok = addTname( t );
    set_orel(e->tok, &imag_tokens);
    set_orel(e->tok, &tokclasses);
    require((p=(TermEntry *)hash_get(Tname, t)) != NULL,
    "hash table mechanism is broken");
    p->classname = 1;	/* entry is class name, not token */
    p->tclass = e;		/* save ptr to this tclass def */
    list_add(&tclasses, (char *)e);
  }
  else
  {
  warn(eMsg1("redefinition of tokclass or conflict w/token '%s'; ignored",t));
  free( (char *)e );
  go=0;
}
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==114) ) {
      zzmatch(114); zzCONSUME;
      zzmatch(QuotedTerm);
      akaString=mystrdup(StripQuotes(LATEXT(1)));
      /* MR11 */                   save_file=CurFile;save_line=zzline;
      /* MR23 */
 zzCONSUME;

      zzmatch(115); zzCONSUME;
    }
    else {
      if ( (LA(1)==102) ) {
      }
      else {zzFAIL(1,zzerr17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  /* MR23 */         if (p!= NULL && akaString != NULL) {
    /* MR23 */           if (p->akaString != NULL) {
      /* MR23 */             if (strcmp(p->akaString,akaString) != 0) {
        /* MR23 */                warnFL(eMsg2("this #tokclass statment conflicts with a previous #tokclass %s(\"%s\") statement",
        /* MR23 */                              t,p->akaString),
        /* MR23 */			                    FileStr[save_file],save_line);
        /* MR23 */             };
      /* MR23 */            } else {
      /* MR23 */              p->akaString=akaString;
      /* MR23 */            };
    /* MR23 */          };
  /* MR23 */
  zzmatch(102); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==TokenTerm) ) {
          zzmatch(TokenTerm);
          if ( go ) {
            term = (TermEntry *) hash_get(Tname, LATEXT(1));
            if ( term==NULL && UserDefdTokens ) {
              err("implicit token definition not allowed with #tokdefs");
              go = 0;
            }
            else {t=mystrdup(LATEXT(1)); tok=addTname(LATEXT(1));}
          }
 zzCONSUME;

          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==119) ) {
              zzmatch(119); zzCONSUME;
              zzmatch(TokenTerm);
              if ( go ) {
                toterm = (TermEntry *) hash_get(Tname, LATEXT(1));
                if ( toterm==NULL && UserDefdTokens ) {
                  err("implicit token definition not allowed with #tokdefs");
                  go = 0;
                } else {
                  totext=mystrdup(LATEXT(1)); totok=addTname(LATEXT(1));
                }
              }
 zzCONSUME;

            }
            else {
              if ( (setwd4[LA(1)]&0x8) ) {
              }
              else {zzFAIL(1,zzerr18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
        }
        else {
          if ( (LA(1)==QuotedTerm) ) {
            zzmatch(QuotedTerm);
            if ( go ) {
              term = (TermEntry *) hash_get(Texpr, LATEXT(1));
              if ( term==NULL && UserDefdTokens ) {
                err("implicit token definition not allowed with #tokdefs");
                go = 0;
              }
              else {t=mystrdup(LATEXT(1)); tok=addTexpr(LATEXT(1));}
            }
 zzCONSUME;

          }
          else {zzFAIL(1,zzerr19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      if ( go ) {
        if (totext == NULL) {
          list_add(&(e->tlist), t);
        } else {
          list_add(&(e->tlist),"..");
          list_add(&(e->tlist),t);
          list_add(&(e->tlist),totext);
        }
        totext=NULL;
      }
      zzLOOP(zztasp2);
    } while ( (setwd4[LA(1)]&0x10) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(98); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x20);
  }
}

void
#ifdef __USE_PROTOS
token(void)
#else
token()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  char *t=NULL, *e=NULL, *a=NULL; int tnum=0;
  char *akaString=NULL; TermEntry *te;int save_file=0,save_line=0;
  zzmatch(120);
  tokenActionActive=1;
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==TokenTerm) ) {
      zzmatch(TokenTerm);
      t=mystrdup(LATEXT(1));
 zzCONSUME;

      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==114) ) {
          zzmatch(114); zzCONSUME;
          zzmatch(QuotedTerm);
          akaString=mystrdup(StripQuotes(LATEXT(1)));
          /* MR11 */                   save_file=CurFile;save_line=zzline;
          /* MR11 */
 zzCONSUME;

          zzmatch(115); zzCONSUME;
        }
        else {
          if ( (setwd4[LA(1)]&0x40) ) {
          }
          else {zzFAIL(1,zzerr20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==121) ) {
          zzmatch(121); zzCONSUME;
          zzmatch(122);
          tnum = atoi(LATEXT(1));
 zzCONSUME;

        }
        else {
          if ( (setwd4[LA(1)]&0x80) ) {
          }
          else {zzFAIL(1,zzerr21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (setwd5[LA(1)]&0x1) ) {
      }
      else {zzFAIL(1,zzerr22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==QuotedTerm) ) {
      zzmatch(QuotedTerm);
      e=mystrdup(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (setwd5[LA(1)]&0x2) ) {
      }
      else {zzFAIL(1,zzerr23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==Action) ) {
      zzmatch(Action);
      
      a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(a!=NULL, "rule token: cannot allocate action");
      strcpy(a, LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (setwd5[LA(1)]&0x4) ) {
      }
      else {zzFAIL(1,zzerr24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==107) ) {
      zzmatch(107); zzCONSUME;
    }
    else {
      if ( (setwd5[LA(1)]&0x8) ) {
      }
      else {zzFAIL(1,zzerr25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  chkToken(t, e, a, tnum);
  if (t != NULL) {
    te=(TermEntry *)hash_get(Tname,t);
    if (te != NULL && akaString != NULL) {
      if (te->akaString != NULL) {
        if (strcmp(te->akaString,akaString) != 0) {
          warnFL(eMsg2("this #token statment conflicts with a previous #token %s(\"%s\") statement",
          t,te->akaString),
          FileStr[save_file],save_line);
        };
      } else {
        te->akaString=akaString;
      };
    };
  };
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x10);
  }
}

void
#ifdef __USE_PROTOS
block(set * toksrefd,set * rulesrefd)
#else
block(toksrefd,rulesrefd)
 set *toksrefd;
set *rulesrefd ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  Graph g, b;
  set saveblah;
  int saveinalt = inAlt;
  ExceptionGroup *eg;
  * toksrefd = empty;
  * rulesrefd = empty;
  set_clr(AST_nodes_refd_in_actions);
  CurBlockID++;
  /* MR23 */      CurBlockID_array[BlkLevel] = CurBlockID;
  CurAltNum = 1;
  /* MR23 */      CurAltNum_array[BlkLevel] = CurAltNum;                
  saveblah = attribsRefdFromAction;
  attribsRefdFromAction = empty;
  alt( toksrefd,rulesrefd );
  b = g = zzaArg(zztasp1,1);
  
  if ( ((Junction *)g.left)->p1->ntype == nAction )
  {
    ActionNode *actionNode=(ActionNode *)
    ( ( (Junction *)g.left) ->p1);
    if (!actionNode->is_predicate )
    {
      actionNode->init_action = TRUE;
      /* MR12c */  		if (actionNode->noHoist) {
        /* MR12c */           errFL("<<nohoist>> appears as init-action - use <<>> <<nohoist>>",
        /* MR12c */                       FileStr[actionNode->file],actionNode->line);
        /* MR12c */         };
    }
  }
  ((Junction *)g.left)->blockid = CurBlockID;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==133) ) {
       eg  = exception_group();

      
      if ( eg!=NULL ) {
        /* MR7 *****       	eg->altID = makeAltID(CurBlockID,CurAltNum);        *****/
        /* MR7 *****		CurAltStart->exception_label = eg->altID;           *****/
        list_add(&CurExGroups, (void *)eg);
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  CurAltNum++;
  /* MR23 */    CurAltNum_array[BlkLevel] = CurAltNum;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==123) ) {
      zzmatch(123);
      inAlt=1;
 zzCONSUME;

      alt( toksrefd,rulesrefd );
      g = Or(g, zzaArg(zztasp2,2));
      
      ((Junction *)g.left)->blockid = CurBlockID;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        while ( (LA(1)==133) ) {
           eg  = exception_group();

          
          if ( eg!=NULL ) {
            /* MR7 *****       	eg->altID = makeAltID(CurBlockID,CurAltNum);        *****/
            /* MR7 *****		CurAltStart->exception_label = eg->altID;           *****/
            list_add(&CurExGroups, (void *)eg);
          }
          zzLOOP(zztasp3);
        }
        zzEXIT(zztasp3);
        }
      }
      CurAltNum++;
      /* MR23 */        CurAltNum_array[BlkLevel] = CurAltNum;
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzaArg(zztasp1,0) = b;
  attribsRefdFromAction = saveblah; inAlt = saveinalt;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x20);
  }
}

void
#ifdef __USE_PROTOS
alt(set * toksrefd,set * rulesrefd)
#else
alt(toksrefd,rulesrefd)
 set *toksrefd;
set *rulesrefd ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int n=0; Graph g; int e_num=0, old_not=0; Node *node; set elems, dif;
  int first_on_line = 1, use_def_MT_handler = 0;
  g.left=NULL; g.right=NULL;
  
			CurAltStart = NULL;
  elems = empty;
  inAlt = 1;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==88) ) {
      zzmatch(88);
      use_def_MT_handler = 1;
 zzCONSUME;

    }
    else {
      if ( (setwd5[LA(1)]&0x40) ) {
      }
      else {zzFAIL(1,zzerr26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    ;
    while ( (setwd5[LA(1)]&0x80) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        old_not=0;
        if ( (LA(1)==124) ) {
          zzmatch(124);
          old_not=1;
 zzCONSUME;

        }
        else {
          if ( (setwd6[LA(1)]&0x1) ) {
          }
          else {zzFAIL(1,zzerr27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
       node  = element( old_not, first_on_line, use_def_MT_handler );

      if ( node!=NULL && node->ntype!=nAction ) first_on_line = 0;
      
      if ( zzaArg(zztasp2,2 ).left!=NULL ) {
        g = Cat(g, zzaArg(zztasp2,2));
        n++;
        if ( node!=NULL ) {
          if ( node->ntype!=nAction ) e_num++;
          /* record record number of all rule and token refs */
          if ( node->ntype==nToken ) {
            TokNode *tk = (TokNode *)((Junction *)zzaArg(zztasp2,2 ).left)->p1;
            tk->elnum = e_num;
            set_orel(e_num, &elems);
          }
          else if ( node->ntype==nRuleRef ) {
            RuleRefNode *rn = (RuleRefNode *)((Junction *)zzaArg(zztasp2,2 ).left)->p1;
            rn->elnum = e_num;
            set_orel(e_num,  rulesrefd);
          }
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  if ( n == 0 ) g = emptyAlt();
  zzaArg(zztasp1,0) = g;
  /* We want to reduce number of LT(i) calls and the number of
  * local attribute variables in C++ mode (for moment, later we'll
  * do for C also).  However, if trees are being built, they
  * require most of the attrib variables to create the tree nodes
  * with; therefore, we gen a token ptr for each token ref in C++
  */
  if ( GenCC && !GenAST )
  {
  /* This now free's the temp set -ATG 5/6/95 */
  set temp;
  temp = set_and(elems, attribsRefdFromAction);
  set_orin( toksrefd, temp);
  set_free(temp);
}
else set_orin( toksrefd, elems);
if ( GenCC ) {
  dif = set_dif(attribsRefdFromAction, elems);
  if ( set_deg(dif)>0 )
  err("one or more $i in action(s) refer to non-token elements");
  set_free(dif);
}
set_free(elems);
set_free(attribsRefdFromAction);
inAlt = 0;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd6, 0x2);
  }
}

LabelEntry *
#ifdef __USE_PROTOS
element_label(void)
#else
element_label()
#endif
{
  LabelEntry *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(LabelEntry *  ))
  zzMake0;
  {
  TermEntry *t=NULL; LabelEntry *l=NULL; RuleEntry *r=NULL; char *lab;
  zzmatch(LABEL);
  lab = mystrdup(LATEXT(1));
 zzCONSUME;

  
  UsedNewStyleLabel = 1;
  if ( UsedOldStyleAttrib ) err("cannot mix with new-style labels with old-style $i");
  t = (TermEntry *) hash_get(Tname, lab);
  if ( t==NULL ) t = (TermEntry *) hash_get(Texpr, lab);
  if ( t==NULL ) r = (RuleEntry *) hash_get(Rname, lab);
  if ( t!=NULL ) {
    err(eMsg1("label definition clashes with token/tokclass definition: '%s'", lab));
    _retv = NULL;
  }
  else if ( r!=NULL ) {
    err(eMsg1("label definition clashes with rule definition: '%s'", lab));
    _retv = NULL;
  }
  else {
    /* we don't clash with anybody else */
    l = (LabelEntry *) hash_get(Elabel, lab);
    if ( l==NULL ) {	/* ok to add new element label */
    l = (LabelEntry *)hash_add(Elabel,
    lab,
    (Entry *)newLabelEntry(lab));
    /* add to list of element labels for this rule */
    list_add(&CurElementLabels, (void *)lab);
    /* MR7 */       leAdd(l);  /* list of labels waiting for exception group definitions */
    _retv = l;
  }
  else {
  err(eMsg1("label definitions must be unique per rule: '%s'", lab));
  _retv = NULL;
}
}
  zzmatch(106); zzCONSUME;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd6, 0x4);
  return _retv;
  }
}

Node *
#ifdef __USE_PROTOS
element(int old_not,int first_on_line,int use_def_MT_handler)
#else
element(old_not,first_on_line,use_def_MT_handler)
 int old_not;
int first_on_line;
int use_def_MT_handler ;
#endif
{
  Node *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(Node *  ))
  zzMake0;
  {
  
  Attrib blk;
  Predicate *pred = NULL;
  int local_use_def_MT_handler=0;
  ActionNode *act;
  RuleRefNode *rr;
  set toksrefd, rulesrefd;
  TermEntry *term;
  TokNode *p=NULL; RuleRefNode *q; int approx=0;
  LabelEntry *label=NULL;
  int predMsgDone=0;
  int semDepth=0;
  int   ampersandStyle;
  int   height;         /* MR11 */
  int   equal_height;   /* MR11 */
  
          char* pFirstSetSymbol = NULL; /* MR21 */
  
		  _retv = NULL;
  if ( (setwd6[LA(1)]&0x8) ) {
    {
      zzBLOCK(zztasp2);
      zzMake0;
      {
      if ( (LA(1)==LABEL) ) {
         label  = element_label();

      }
      else {
        if ( (setwd6[LA(1)]&0x10) ) {
        }
        else {zzFAIL(1,zzerr28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp2);
      }
    }
    {
      zzBLOCK(zztasp2);
      zzMake0;
      {
      if ( (LA(1)==TokenTerm) ) {
        zzmatch(TokenTerm);
        
        term = (TermEntry *) hash_get(Tname, LATEXT(1));
        if ( term==NULL && UserDefdTokens ) {
          err("implicit token definition not allowed with #tokdefs");
          zzaRet.left = zzaRet.right = NULL;
        }
        else {
          zzaRet = buildToken(LATEXT(1));
          p=((TokNode *)((Junction *)zzaRet.left)->p1);
          term = (TermEntry *) hash_get(Tname, LATEXT(1));
          require( term!= NULL, "hash table mechanism is broken");
          p->tclass = term->tclass;
          p->complement =  old_not;
          if ( label!=NULL ) {
            p->el_label = label->str;
            label->elem = (Node *)p;
          }
        }
 zzCONSUME;

        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==119) ) {
            zzmatch(119); zzCONSUME;
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              if ( (LA(1)==QuotedTerm) ) {
                zzmatch(QuotedTerm);
                if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

              }
              else {
                if ( (LA(1)==TokenTerm) ) {
                  zzmatch(TokenTerm);
                  if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

                }
                else {zzFAIL(1,zzerr29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp4);
              }
            }
          }
          else {
            if ( (setwd6[LA(1)]&0x20) ) {
            }
            else {zzFAIL(1,zzerr30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        
        if ( p!=NULL && (p->upper_range!=0 || p->tclass ||  old_not) )
        list_add(&MetaTokenNodes, (void *)p);
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==125) ) {
            zzmatch(125);
            if ( p!=NULL ) p->astnode=ASTroot;
 zzCONSUME;

          }
          else {
            if ( (setwd6[LA(1)]&0x40) ) {
              if ( p!=NULL ) p->astnode=ASTchild;
            }
            else {
              if ( (LA(1)==103) ) {
                zzmatch(103);
                if ( p!=NULL ) p->astnode=ASTexclude;
 zzCONSUME;

              }
              else {zzFAIL(1,zzerr31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
          zzEXIT(zztasp3);
          }
        }
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==88) ) {
            zzmatch(88);
            local_use_def_MT_handler = 1;
 zzCONSUME;

          }
          else {
            if ( (setwd6[LA(1)]&0x80) ) {
            }
            else {zzFAIL(1,zzerr32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
        
        if ( p!=NULL &&  first_on_line ) {
          CurAltStart = (Junction *)zzaRet.left;
          altAdd(CurAltStart);                                 /* MR7 */
          p->altstart = CurAltStart;
        }
        if ( p!=NULL )
        p->use_def_MT_handler =  use_def_MT_handler || local_use_def_MT_handler;
        _retv = (Node *)p;
      }
      else {
        if ( (LA(1)==QuotedTerm) ) {
          zzmatch(QuotedTerm);
          
          term = (TermEntry *) hash_get(Texpr, LATEXT(1));
          if ( term==NULL && UserDefdTokens ) {
            err("implicit token definition not allowed with #tokdefs");
            zzaRet.left = zzaRet.right = NULL;
          }
          else {
            zzaRet = buildToken(LATEXT(1)); p=((TokNode *)((Junction *)zzaRet.left)->p1);
            p->complement =  old_not;
            if ( label!=NULL ) {
              p->el_label = label->str;
              label->elem = (Node *)p;
            }
          }
 zzCONSUME;

          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==119) ) {
              zzmatch(119); zzCONSUME;
              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                if ( (LA(1)==QuotedTerm) ) {
                  zzmatch(QuotedTerm);
                  if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

                }
                else {
                  if ( (LA(1)==TokenTerm) ) {
                    zzmatch(TokenTerm);
                    if ( p!=NULL ) setUpperRange(p, LATEXT(1));
 zzCONSUME;

                  }
                  else {zzFAIL(1,zzerr33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp4);
                }
              }
            }
            else {
              if ( (setwd7[LA(1)]&0x1) ) {
              }
              else {zzFAIL(1,zzerr34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==125) ) {
              zzmatch(125);
              if ( p!=NULL ) p->astnode=ASTroot;
 zzCONSUME;

            }
            else {
              if ( (setwd7[LA(1)]&0x2) ) {
                if ( p!=NULL ) p->astnode=ASTchild;
              }
              else {
                if ( (LA(1)==103) ) {
                  zzmatch(103);
                  if ( p!=NULL ) p->astnode=ASTexclude;
 zzCONSUME;

                }
                else {zzFAIL(1,zzerr35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            zzEXIT(zztasp3);
            }
          }
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            if ( (LA(1)==88) ) {
              zzmatch(88);
              local_use_def_MT_handler = 1;
 zzCONSUME;

            }
            else {
              if ( (setwd7[LA(1)]&0x4) ) {
              }
              else {zzFAIL(1,zzerr36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp3);
            }
          }
          
          if ( p!=NULL && (p->upper_range!=0 || p->tclass ||  old_not) )
          list_add(&MetaTokenNodes, (void *)p);
          
          if (  first_on_line ) {
            CurAltStart = (Junction *)zzaRet.left;
            altAdd(CurAltStart);                                 /* MR7 */
            p->altstart = CurAltStart;
          }
          if ( p!=NULL )
          p->use_def_MT_handler =  use_def_MT_handler || local_use_def_MT_handler;
          _retv = (Node *)p;
        }
        else {
          if ( (LA(1)==WildCard) ) {
            if (  old_not ) warn("~ WILDCARD is an undefined operation (implies 'nothing')");
            zzmatch(WildCard);
            zzaRet = buildWildCard(LATEXT(1)); p=((TokNode *)((Junction *)zzaRet.left)->p1);
 zzCONSUME;

            {
              zzBLOCK(zztasp3);
              zzMake0;
              {
              if ( (LA(1)==125) ) {
                zzmatch(125);
                p->astnode=ASTroot;
 zzCONSUME;

              }
              else {
                if ( (setwd7[LA(1)]&0x8) ) {
                  p->astnode=ASTchild;
                }
                else {
                  if ( (LA(1)==103) ) {
                    zzmatch(103);
                    p->astnode=ASTexclude;
 zzCONSUME;

                  }
                  else {zzFAIL(1,zzerr37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              zzEXIT(zztasp3);
              }
            }
            list_add(&MetaTokenNodes, (void *)p);
            
            if (  first_on_line ) {
              CurAltStart = (Junction *)zzaRet.left;
              altAdd(CurAltStart);                                 /* MR7 */
              p->altstart = CurAltStart;
              if ( label!=NULL ) {
                p->el_label = label->str;
                label->elem = (Node *)p;
              }
            }
            _retv = (Node *)p;
          }
          else {
            if ( (LA(1)==NonTerminal) ) {
              if (  old_not ) warn("~ NONTERMINAL is an undefined operation");
              zzmatch(NonTerminal);
              zzaRet = buildRuleRef(LATEXT(1));
 zzCONSUME;

              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                if ( (LA(1)==103) ) {
                  zzmatch(103);
                  q = (RuleRefNode *) ((Junction *)zzaRet.left)->p1;
                  q->astnode=ASTexclude;
 zzCONSUME;

                }
                else {
                  if ( (setwd7[LA(1)]&0x10) ) {
                  }
                  else {zzFAIL(1,zzerr38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp3);
                }
              }
              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                if ( (setwd7[LA(1)]&0x20) ) {
                  {
                    zzBLOCK(zztasp4);
                    zzMake0;
                    {
                    if ( (LA(1)==104) ) {
                      zzmatch(104); zzCONSUME;
                    }
                    else {
                      if ( (LA(1)==PassAction) ) {
                      }
                      else {zzFAIL(1,zzerr39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                    zzEXIT(zztasp4);
                    }
                  }
                  zzmatch(PassAction);
                  addParm(((Junction *)zzaRet.left)->p1, LATEXT(1));
 zzCONSUME;

                }
                else {
                  if ( (setwd7[LA(1)]&0x40) ) {
                  }
                  else {zzFAIL(1,zzerr40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp3);
                }
              }
              rr=(RuleRefNode *) ((Junction *)zzaRet.left)->p1;
              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                char *a;
                if ( (LA(1)==105) ) {
                  zzmatch(105); zzCONSUME;
                  zzmatch(PassAction);
                  
                  a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
                  require(a!=NULL, "rule element: cannot allocate assignment");
                  strcpy(a, LATEXT(1));
                  rr->assign = a;
 zzCONSUME;

                }
                else {
                  if ( (setwd7[LA(1)]&0x80) ) {
                  }
                  else {zzFAIL(1,zzerr41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp3);
                }
              }
              
              if ( label!=NULL ) {
                rr->el_label = label->str;
                label->elem = (Node *)rr;
              }
              if (  first_on_line ) {
                CurAltStart = (Junction *)zzaRet.left;
                altAdd(CurAltStart);                                 /* MR7 */
                ((RuleRefNode *)((Junction *)zzaRet.left)->p1)->altstart = CurAltStart;
              }
              _retv = (Node *)rr;
            }
            else {zzFAIL(1,zzerr42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      zzEXIT(zztasp2);
      }
    }
  }
  else {
    if ( (LA(1)==Action) ) {
      if (  old_not )	warn("~ ACTION is an undefined operation");
      zzmatch(Action);
      zzaArg(zztasp1,0) = buildAction(LATEXT(1),action_file,action_line, 0);
 zzCONSUME;

      if (  first_on_line ) {                                /* MR7 */
        CurAltStart = (Junction *)zzaArg(zztasp1,0 ).left;                   /* MR7 */
        altAdd(CurAltStart);                                 /* MR7 */
      };
      _retv = (Node *) ((Junction *)zzaArg(zztasp1,0 ).left)->p1;
    }
    else {
      if ( (LA(1)==Pred) ) {
        if (  old_not )	warn("~ SEMANTIC-PREDICATE is an undefined operation");
        zzmatch(Pred);
        zzaArg(zztasp1,0) = buildAction(LATEXT(1),action_file,action_line, 1);
 zzCONSUME;

        act = (ActionNode *) ((Junction *)zzaArg(zztasp1,0 ).left)->p1;
        if (numericActionLabel) {             /* MR10 */
          list_add(&NumericPredLabels,act);   /* MR10 */
          numericActionLabel=0;               /* MR10 */
        };                                    /* MR10 */
        {
          zzBLOCK(zztasp2);
          zzMake0;
          {
          char *a;
          if ( (LA(1)==PassAction) ) {
            zzmatch(PassAction);
            
            a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
            require(a!=NULL, "rule element: cannot allocate predicate fail action");
            strcpy(a, LATEXT(1));
            act->pred_fail = a;
 zzCONSUME;

          }
          else {
            if ( (setwd8[LA(1)]&0x1) ) {
            }
            else {zzFAIL(1,zzerr43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp2);
          }
        }
        if (  first_on_line ) {                                /* MR7 */
          CurAltStart = (Junction *)zzaArg(zztasp1,0 ).left;                   /* MR7 */
          altAdd(CurAltStart);                                 /* MR7 */
        };
        _retv = (Node *)act;
      }
      else {
        if ( (setwd8[LA(1)]&0x2) ) {
          if (  old_not )	warn("~ BLOCK is an undefined operation");
          BlkLevel++;
          if (BlkLevel >= MAX_BLK_LEVEL) fatal("Blocks nested too deeply");
          /* MR23 */    CurBlockID_array[BlkLevel] = CurBlockID;
          /* MR23 */    CurAltNum_array[BlkLevel] = CurAltNum;
          {
            zzBLOCK(zztasp2);
            zzMake0;
            {
            if ( (LA(1)==Pragma) ) {
              zzmatch(Pragma); zzCONSUME;
              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                if ( (LA(1)==126) ) {
                  zzmatch(126);
                  approx=LL_k;
 zzCONSUME;

                }
                else {
                  if ( (LA(1)==127) ) {
                    zzmatch(127);
                    approx = 1;
 zzCONSUME;

                  }
                  else {
                    if ( (LA(1)==128) ) {
                      zzmatch(128);
                      approx = 2;
 zzCONSUME;

                    }
                    else {zzFAIL(1,zzerr44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                }
                zzEXIT(zztasp3);
                }
              }
            }
            else {
              if ( (setwd8[LA(1)]&0x4) ) {
              }
              else {zzFAIL(1,zzerr45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp2);
            }
          }
          {
            zzBLOCK(zztasp2);
            zzMake0;
            {
            if ( (LA(1)==FirstSetSymbol) ) {
              zzmatch(FirstSetSymbol); zzCONSUME;
              zzmatch(114); zzCONSUME;
              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                if ( (LA(1)==NonTerminal) ) {
                  zzmatch(NonTerminal);
                  
                  /* MR21 */                     pFirstSetSymbol = (char *) calloc(strlen(LATEXT(1))+1,
                  /* MR21 */                                                    sizeof(char));
                  /* MR21 */                          require(pFirstSetSymbol!=NULL,
                  /* MR21 */                                  "cannot allocate first set name");
                  /* MR21 */                          strcpy(pFirstSetSymbol, LATEXT(1));
                  /* MR21 */
 zzCONSUME;

                }
                else {
                  if ( (LA(1)==TokenTerm) ) {
                    zzmatch(TokenTerm);
                    
                    /* MR21 */                      pFirstSetSymbol = (char *) calloc(strlen(LATEXT(1))+1,
                    /* MR21 */                                                        sizeof(char));
                    /* MR21 */                      require(pFirstSetSymbol!=NULL,
                    /* MR21 */                              "cannot allocate first set name");
                    /* MR21 */                      strcpy(pFirstSetSymbol, LATEXT(1));
                    /* MR21 */
 zzCONSUME;

                  }
                  else {zzFAIL(1,zzerr46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp3);
                }
              }
              zzmatch(115); zzCONSUME;
            }
            else {
              if ( (setwd8[LA(1)]&0x8) ) {
              }
              else {zzFAIL(1,zzerr47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp2);
            }
          }
          {
            zzBLOCK(zztasp2);
            zzMake0;
            {
            if ( (LA(1)==114) ) {
              zzmatch(114); zzCONSUME;
              block( &toksrefd,&rulesrefd );
              zzmatch(115);
              blk = zzaRet = zzaArg(zztasp2,2);
              /* MR23 */      CurBlockID_array[BlkLevel] = (-1);
              /* MR23 */      CurAltNum_array[BlkLevel] = (-1);                
              --BlkLevel;
 zzCONSUME;

              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                if ( (LA(1)==129) ) {
                  zzmatch(129);
                  zzaRet = makeLoop(zzaRet,approx,pFirstSetSymbol);
 zzCONSUME;

                }
                else {
                  if ( (LA(1)==130) ) {
                    zzmatch(130);
                    zzaRet = makePlus(zzaRet,approx,pFirstSetSymbol);
 zzCONSUME;

                  }
                  else {
                    if ( (LA(1)==131) ) {
                      zzmatch(131); zzCONSUME;
                      {
                        zzBLOCK(zztasp4);
                        zzMake0;
                        {
                        if ( (setwd8[LA(1)]&0x10) ) {
                          {
                            zzBLOCK(zztasp5);
                            zzMake0;
                            {
                            if ( (LA(1)==132) ) {
                              zzmatch(132);
                              ampersandStyle=0;
 zzCONSUME;

                            }
                            else {
                              if ( (LA(1)==113) ) {
                                zzmatch(113);
                                ampersandStyle=1;
 zzCONSUME;

                              }
                              else {zzFAIL(1,zzerr48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                            zzEXIT(zztasp5);
                            }
                          }
                          zzmatch(Pred);
                          zzaRet = buildAction(LATEXT(1),action_file,action_line,1);
 zzCONSUME;

                          act = (ActionNode *) ((Junction *)zzaRet.left)->p1;
                          semDepth=predicateLookaheadDepth(act);
                          if (numericActionLabel) {             /* MR10 */
                            list_add(&NumericPredLabels,act);   /* MR10 */
                            numericActionLabel=0;               /* MR10 */
                          };                                    /* MR10 */
                          {
                            zzBLOCK(zztasp5);
                            zzMake0;
                            {
                            char *a;
                            if ( (LA(1)==PassAction) ) {
                              zzmatch(PassAction);
                              
                              a = (char *)calloc(strlen(LATEXT(1))+1, sizeof(char));
                              require(a!=NULL, "rule element: cannot allocate predicate fail action");
                              strcpy(a, LATEXT(1));
                              act->pred_fail = a;
 zzCONSUME;

                            }
                            else {
                              if ( (setwd8[LA(1)]&0x20) ) {
                              }
                              else {zzFAIL(1,zzerr49,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                            zzEXIT(zztasp5);
                            }
                          }
                          if ( first_on_line) {                      /* MR7 */
                            CurAltStart=(Junction *)zzaRet.left;         /* MR7 */
                            altAdd(CurAltStart);                     /* MR7 */
                          };
                          _retv = (Node *)act;
                          
                          pred = computePredFromContextGuard(blk,&predMsgDone);           /* MR10 */
                          if ( pred==NULL) {                                              /* MR10 */
                            if ( !predMsgDone) err("invalid or missing context guard");   /* MR10 */
                            predMsgDone=1;                                                /* MR10 */
                          } else {                                                        /* MR10 */
                            act->guardNodes=(Junction *)blk.left;                       /* MR11 */
                            pred->expr = act->action;
                            pred->source = act;
                            /* MR10 */                  pred->ampersandStyle = ampersandStyle;  /* 0 means (g)? => ... 1 means (g)? && ... */
                            /* MR13 */                  if (pred->tcontext != NULL) {
                              /* MR13 */                    height=MR_max_height_of_tree(pred->tcontext);
                              /* MR13 */                    equal_height=MR_all_leaves_same_height(pred->tcontext,height);
                              /* MR13 */                    if (! equal_height) {
                                /* MR13 */                       errFL("in guarded predicates all tokens in the guard must be at the same height",
                                /* MR13 */                              FileStr[act->file],act->line);
                                /* MR13 */                    };
                              /* MR13 */                  }
                            /* MR10 */                  if (ampersandStyle) {
                              /* MR10 */			  		  act->ampersandPred = pred;
                              /* MR11 */                    if (! HoistPredicateContext) {
                                /* MR11 */                      errFL("without \"-prc on\" (guard)? && <<pred>>? ... doesn't make sense",
                                /* MR11 */                              FileStr[act->file],act->line);
                                /* MR11 */                    };
                              /* MR10 */                  } else {
                              /* MR10 */			  		  act->guardpred = pred;
                              /* MR10 */                  };
                            /* MR10 */                  if (pred->k != semDepth) {
                              /* MR10 */                     warn(eMsgd2("length of guard (%d) does not match the length of semantic predicate (%d)",
                              /* MR10 */                                  pred->k,semDepth));
                              /* MR10 */                  };
                          }
                        }
                        else {
                          if ( (setwd8[LA(1)]&0x40) ) {
                            zzaRet = makeBlk(zzaRet,approx,pFirstSetSymbol);
                            FoundGuessBlk = 1;
                            ((Junction *) ((Junction *)zzaRet.left)->p1)->guess=1;
                            if ( ! first_on_line ) {
                              err("(...)? predicate must be first element of production");
                            }
                          }
                          else {zzFAIL(1,zzerr50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                        }
                        zzEXIT(zztasp4);
                        }
                      }
                    }
                    else {
                      if ( (setwd8[LA(1)]&0x80) ) {
                        zzaRet = makeBlk(zzaRet,approx,pFirstSetSymbol);
                      }
                      else {zzFAIL(1,zzerr51,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                  }
                }
                zzEXIT(zztasp3);
                }
              }
              
              if ( pred==NULL && !predMsgDone) {                                      /* MR10 */
                ((Junction *)((Junction *)zzaRet.left)->p1)->blockid = CurBlockID;
                ((Junction *)((Junction *)zzaRet.left)->p1)->tokrefs = toksrefd;
                ((Junction *)((Junction *)zzaRet.left)->p1)->rulerefs = rulesrefd;
                if (  first_on_line ) {                         /* MR7 */
                  CurAltStart = (Junction *)((Junction *)((Junction *)zzaRet.left)->p1);  /* MR7 */
                  altAdd(CurAltStart);                         /* MR7 */
                };                                              /* MR7 */
                _retv = (Node *) ((Junction *)zzaRet.left)->p1;
              }
            }
            else {
              if ( (LA(1)==102) ) {
                zzmatch(102); zzCONSUME;
                block( &toksrefd,&rulesrefd );
                zzaRet = makeOpt(zzaArg(zztasp2,2),approx,pFirstSetSymbol);
                /* MR23 */      CurBlockID_array[BlkLevel] = (-1);
                /* MR23 */      CurAltNum_array[BlkLevel] = (-1);                
                --BlkLevel;
                zzmatch(98);
                
                ((Junction *)((Junction *)zzaRet.left)->p1)->blockid = CurBlockID;
                ((Junction *)((Junction *)zzaRet.left)->p1)->tokrefs = toksrefd;
                ((Junction *)((Junction *)zzaRet.left)->p1)->rulerefs = rulesrefd;
 zzCONSUME;

                if (  first_on_line ) {                            /* MR7 */
                  CurAltStart = (Junction *) ((Junction *)((Junction *)zzaRet.left)->p1);  /* MR7 */
                  altAdd(CurAltStart);                             /* MR7 */
                };
                _retv = (Node *) ((Junction *)zzaRet.left)->p1;
              }
              else {zzFAIL(1,zzerr52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp2);
            }
          }
        }
        else {
          if ( (LA(1)==129) ) {
            zzmatch(129);
            warn("don't you want a ')' with that '*'?"); CannotContinue=TRUE;
 zzCONSUME;

          }
          else {
            if ( (LA(1)==130) ) {
              zzmatch(130);
              warn("don't you want a ')' with that '+'?"); CannotContinue=TRUE;
 zzCONSUME;

            }
            else {
              if ( (LA(1)==105) ) {
                zzmatch(105);
                warn("'>' can only appear after a nonterminal"); CannotContinue=TRUE;
 zzCONSUME;

              }
              else {
                if ( (LA(1)==PassAction) ) {
                  zzmatch(PassAction);
                  warn("[...] out of context 'rule > [...]'");
                  CannotContinue=TRUE;
 zzCONSUME;

                }
                else {zzFAIL(1,zzerr53,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x1);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
default_exception_handler(void)
#else
default_exception_handler()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
   DefaultExGroup  = exception_group();

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x2);
  }
}

ExceptionGroup *
#ifdef __USE_PROTOS
exception_group(void)
#else
exception_group()
#endif
{
  ExceptionGroup *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(ExceptionGroup *  ))
  zzMake0;
  {
  ExceptionHandler *h; LabelEntry *label=NULL;	  /* MR6 */
  FoundException = 1; FoundExceptionGroup = 1;
  zzmatch(133);
  _retv = (ExceptionGroup *)calloc(1, sizeof(ExceptionGroup));
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    char *p;
    if ( (LA(1)==PassAction) ) {
      zzmatch(PassAction);
      
      p = LATEXT(1)+1;
      p[strlen(p)-1] = '\0';		/* kill trailing space */
      label = (LabelEntry *) hash_get(Elabel, LATEXT(1)+1);
      if ( label==NULL )
      {
        err(eMsg1("unknown label in exception handler: '%s'", LATEXT(1)+1));
      }
 zzCONSUME;

    }
    else {
      if ( (setwd9[LA(1)]&0x4) ) {
      }
      else {zzFAIL(1,zzerr54,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==135) ) {
       h  = exception_handler();

      list_add(&(_retv->handlers), (void *)h);
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==134) ) {
      zzmatch(134); zzCONSUME;
      zzmatch(106); zzCONSUME;
      zzmatch(Action);
      {
        ExceptionHandler *eh = (ExceptionHandler *)
        calloc(1, sizeof(ExceptionHandler));
        char *a = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
        require(eh!=NULL, "exception: cannot allocate handler");
        require(a!=NULL, "exception: cannot allocate action");
        strcpy(a, LATEXT(1));
        eh->action = a;
        eh->signalname = (char *) calloc(strlen("default")+1, sizeof(char));
        require(eh->signalname!=NULL, "exception: cannot allocate sig name");
        strcpy(eh->signalname, "default");
        list_add(&(_retv->handlers), (void *)eh);
      }
 zzCONSUME;

    }
    else {
      if ( (setwd9[LA(1)]&0x8) ) {
      }
      else {zzFAIL(1,zzerr55,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  if ( label!=NULL ) {
    /* Record ex group in sym tab for this label */
    if ( label->ex_group!=NULL ) {
      err(eMsg1("duplicate exception handler for label '%s'",label->str));
    } else {
      label->ex_group = _retv;
      /* Label the exception group itself */
      _retv->label = label->str;
      /* Make the labelled element pt to the exception also */
      /* MR6 */	  if (label->elem == NULL) {
        /* MR6 */	     err(eMsg1("reference in exception handler to undefined label '%s'",label->str));
        /* MR6 */	  } else {
        switch ( label->elem->ntype ) {
          case nRuleRef :
          {
            RuleRefNode *r = (RuleRefNode *)label->elem;
            r->ex_group = _retv;
            break;
          }
          case nToken :
          {
            TokNode *t = (TokNode *)label->elem;
            t->ex_group = _retv;
            break;
          }
        } /* end switch */
        /* MR6 */	  }; /* end test on label->elem */
    } /* end test on label->ex_group */
    
		} /* end test on exception label */
  
/* MR7 */
  /* MR7 */   if (BlkLevel == 1 && label == NULL) {
    /* MR7 */     _retv->forRule=1;
    /* MR7 */   } else if (label == NULL) {
    /* MR7 */     _retv->altID = makeAltID(CurBlockID_array[BlkLevel], CurAltNum_array[BlkLevel]);
    /* MR7 */     egAdd(_retv);
    /* MR7 */   } else {
    /* MR7 */     _retv->labelEntry=label;
    /* MR7 */   };
  /* MR7 */
  /* MR7 */	    /* You may want to remove this exc from the rule list  */
  /* MR7 */		/* and handle at the labeled element site.             */
  /* MR7 */
  /* MR7 */   if (label != NULL) {
    /* MR7 */     _retv = NULL;
    /* MR7 */   };
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x10);
  return _retv;
  }
}

ExceptionHandler *
#ifdef __USE_PROTOS
exception_handler(void)
#else
exception_handler()
#endif
{
  ExceptionHandler *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(ExceptionHandler *  ))
  zzMake0;
  {
  ;
  zzmatch(135);
  
  _retv = (ExceptionHandler *)calloc(1, sizeof(ExceptionHandler));
  require(_retv!=NULL, "exception: cannot allocate handler");
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==NonTerminal) ) {
      zzmatch(NonTerminal);
      
      _retv->signalname = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(_retv->signalname!=NULL, "exception: cannot allocate sig name");
      strcpy(_retv->signalname, LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (LA(1)==TokenTerm) ) {
        zzmatch(TokenTerm);
        
        _retv->signalname = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
        require(_retv->signalname!=NULL, "exception: cannot allocate sig name");
        strcpy(_retv->signalname, LATEXT(1));
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr56,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(106); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    _retv->action = NULL;
    if ( (LA(1)==Action) ) {
      zzmatch(Action);
      
      _retv->action = (char *) calloc(strlen(LATEXT(1))+1, sizeof(char));
      require(_retv->action!=NULL, "exception: cannot allocate action");
      strcpy(_retv->action, LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (setwd9[LA(1)]&0x20) ) {
      }
      else {zzFAIL(1,zzerr57,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  CannotContinue=TRUE;  
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x40);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
enum_file(char * fname)
#else
enum_file(fname)
 char *fname ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  if ( (setwd9[LA(1)]&0x80) ) {
    {
      zzBLOCK(zztasp2);
      zzMake0;
      {
      if ( (LA(1)==143) ) {
        zzmatch(143); zzCONSUME;
        zzmatch(ID); zzCONSUME;
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          if ( (LA(1)==149) ) {
            zzmatch(149); zzCONSUME;
            zzmatch(ID); zzCONSUME;
          }
          else {
            if ( (setwd10[LA(1)]&0x1) ) {
            }
            else {zzFAIL(1,zzerr58,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp3);
          }
        }
      }
      else {
        if ( (setwd10[LA(1)]&0x2) ) {
        }
        else {zzFAIL(1,zzerr59,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp2);
      }
    }
    {
      zzBLOCK(zztasp2);
      zzMake0;
      {
      if ( (LA(1)==151) ) {
        {
          zzBLOCK(zztasp3);
          int zzcnt=1;
          zzMake0;
          {
          do {
            enum_def(  fname );
            zzLOOP(zztasp3);
          } while ( (LA(1)==151) );
          zzEXIT(zztasp3);
          }
        }
      }
      else {
        if ( (LA(1)==149) ) {
          defines(  fname );
        }
        else {zzFAIL(1,zzerr60,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp2);
      }
    }
  }
  else {
    if ( (LA(1)==Eof) ) {
    }
    else {zzFAIL(1,zzerr61,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd10, 0x4);
  }
}

void
#ifdef __USE_PROTOS
defines(char * fname)
#else
defines(fname)
 char *fname ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int v; int maxt=(-1); char *t;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      zzmatch(149); zzCONSUME;
      zzmatch(ID);
      t = mystrdup(LATEXT(1));
 zzCONSUME;

      zzmatch(INT);
      
      v = atoi(LATEXT(1));
      /*			fprintf(stderr, "#token %s=%d\n", t, v);*/
      
	/* MR2 Andreas Magnusson (Andreas.Magnusson@mailbox.swipnet.se) */
      /* MR2 Fix to bug introduced by 1.33MR1 for #tokdefs            */
      /* MR2 Don't let #tokdefs be confused by 			*/
      /* MR2   DLGminToken and DLGmaxToken     			*/
      
			if ( ! isDLGmaxToken(t)) {		/* MR2 */
      TokenNum = v;
      if ( v>maxt ) maxt=v;
      if ( Tnum( t ) == 0 ) {
      addForcedTname( t, v );
    } else {
    warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
  };
};
 zzCONSUME;

      zzLOOP(zztasp2);
    } while ( (LA(1)==149) );
    zzEXIT(zztasp2);
    }
  }
  TokenNum = maxt + 1;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd10, 0x8);
  }
}

void
#ifdef __USE_PROTOS
enum_def(char * fname)
#else
enum_def(fname)
 char *fname ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int v= 0; int maxt=(-1); char *t;
  zzmatch(151); zzCONSUME;
  zzmatch(ID); zzCONSUME;
  zzmatch(152); zzCONSUME;
  zzmatch(ID);
  t = mystrdup(LATEXT(1));
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==153) ) {
      zzmatch(153); zzCONSUME;
      zzmatch(INT);
      v=atoi(LATEXT(1));
 zzCONSUME;

    }
    else {
      if ( (setwd10[LA(1)]&0x10) ) {
        v++;
      }
      else {zzFAIL(1,zzerr62,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  
  /*			fprintf(stderr, "#token %s=%d\n", t, v);*/
  TokenNum = v;
  if ( v>maxt ) maxt=v;				/* MR3 */
  if ( Tnum( t ) == 0 ) addForcedTname( t, v );
  else {
    warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==154) ) {
      zzmatch(154); zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==ID)&&(isDLGmaxToken(LATEXT(1))) ) {
          if (!(isDLGmaxToken(LATEXT(1)))            ) {zzfailed_pred("  isDLGmaxToken(LATEXT(1))",0 /* report */, { 0; /* no user action */ } );}
          zzmatch(ID); zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==153) ) {
              zzmatch(153); zzCONSUME;
              zzmatch(INT); zzCONSUME;
            }
            else {
              if ( (setwd10[LA(1)]&0x20) ) {
              }
              else {zzFAIL(1,zzerr63,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
        }
        else {
          if ( (LA(1)==ID) ) {
            zzmatch(ID);
            t = mystrdup(LATEXT(1));
 zzCONSUME;

            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              if ( (LA(1)==153) ) {
                zzmatch(153); zzCONSUME;
                zzmatch(INT);
                v=atoi(LATEXT(1));
 zzCONSUME;

              }
              else {
                if ( (setwd10[LA(1)]&0x40) ) {
                  v++;
                }
                else {zzFAIL(1,zzerr64,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp4);
              }
            }
            
            /*					fprintf(stderr, "#token %s=%d\n", t, v);*/
            TokenNum = v;
            if ( v>maxt ) maxt=v;				/* MR3 */
            if ( Tnum( t ) == 0 ) addForcedTname( t, v );
            else {
              warnFL(eMsg1("redefinition of token %s; ignored",t), fname,zzline);
            }
          }
          else {
            if ( (setwd10[LA(1)]&0x80) ) {
            }
            else {zzFAIL(1,zzerr65,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(155); zzCONSUME;
  zzmatch(156);
  TokenNum = maxt + 1;
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd11, 0x1);
  }
}


/* MR2 Andreas Magnusson (Andreas.Magnusson@mailbox.swipnet.se) */
/* MR2 Fix to bug introduced by 1.33MR1 for #tokdefs            */
/* MR2 Don't let #tokdefs be confused by 			*/
/* MR2   DLGminToken and DLGmaxToken     			*/

/* semantic check on DLGminToken and DLGmaxmaxToken in #tokdefs */

#ifdef __USE_PROTOS
static int isDLGmaxToken(char *Token)
#else
static int isDLGmaxToken(Token)
char *	Token;
#endif
{
static char checkStr1[] = "DLGmaxToken";
static char checkStr2[] = "DLGminToken";

   if (strcmp(Token, checkStr1) == 0)
return 1;
else if (strcmp(Token, checkStr2) == 0)
return 1;
else
return 0;
}

/* semantics of #token */
static void
#ifdef __USE_PROTOS
chkToken(char *t, char *e, char *a, int tnum)
#else
chkToken(t,e,a,tnum)
char *t, *e, *a;
int tnum;
#endif
{
TermEntry *p;

	/* check to see that they don't try to redefine a token as a token class */
if ( t!=NULL ) {
p = (TermEntry *) hash_get(Tname, t);
if ( p!=NULL && p->classname ) {
  err(eMsg1("redefinition of #tokclass '%s' to #token not allowed; ignored",t));
  if ( a!=NULL ) free((char *)a);
  return;
}
}

	if ( t==NULL && e==NULL ) {			/* none found */
err("#token requires at least token name or rexpr");
}
else if ( t!=NULL && e!=NULL ) {	/* both found */
if ( UserDefdTokens ) {			/* if #tokdefs, must not define new */
  p = (TermEntry *) hash_get(Tname, t);
  if ( p == NULL) {
    err(eMsg1("new token definition '%s' not allowed - only #token with name already defined by #tokdefs file allowed",t));
    return;
  };
}
Tklink(t, e);
if ( a!=NULL ) {
  if ( hasAction(e) ) {
    err(eMsg1("redefinition of action for %s; ignored",e));
  }
  else setHasAction(e, a);
}
}
else if ( t!=NULL ) {				/* only one found */
if ( UserDefdTokens ) {
  p = (TermEntry *) hash_get(Tname, t);
  if (p == NULL) {
    err(eMsg1("new token definition '%s' not allowed - only #token with name already defined by #tokdefs file allowed",t));
  };
  return;
}
if ( Tnum( t ) == 0 ) addTname( t );
else {
  err(eMsg1("redefinition of token %s; ignored",t));
}
if ( a!=NULL ) {
  err(eMsg1("action cannot be attached to a token name (%s); ignored",t));
  free((char *)a);
}
}
else if ( e!=NULL ) {
if ( Tnum( e ) == 0 ) addTexpr( e );
else {
  if ( hasAction(e) ) {
    err(eMsg1("redefinition of action for expr %s; ignored",e));
  }
  else if ( a==NULL ) {
    err(eMsg1("redefinition of expr %s; ignored",e));
  }
}
if ( a!=NULL ) setHasAction(e, a);
}

	/* if a token type number was specified, then add the token ID and 'tnum'
* pair to the ForcedTokens list.  (only applies if an id was given)
*/
if ( t!=NULL && tnum>0 )
{
if ( set_el(tnum, reserved_positions) )
{
  err(eMsgd("a token has already been forced to token number %d; ignored", tnum));
}
else
{
  list_add(&ForcedTokens, newForcedToken(t,tnum));
  set_orel(tnum, &reserved_positions);
}
}
}

static int
#ifdef __USE_PROTOS
match_token(char *s, char **nxt)
#else
match_token(s,nxt)
char *s;
char **nxt;
#endif
{
  if ( !(*s>='A' && *s<='Z') ) return 0;
  s++;
  while ( (*s>='a' && *s<='z') ||
  (*s>='A' && *s<='Z') ||
  (*s>='0' && *s<='9') ||
  *s=='_' )
  {
    s++;
  }
  if ( *s!=' ' && *s!='}' ) return 0;
  *nxt = s;
  return 1;
}

static int
#ifdef __USE_PROTOS
match_rexpr(char *s, char **nxt)
#else
match_rexpr(s,nxt)
char *s;
char **nxt;
#endif
{
  if ( *s!='"' ) return 0;
  s++;
  while ( *s!='"' )
  {
    if ( *s=='\n' || *s=='\r' )                   /* MR13 */
    warn("eoln found in regular expression");
    if ( *s=='\\' ) s++;
    s++;
  }
  *nxt = s+1;
  return 1;
}

/*
* Walk a string "{ A .. Z }" where A..Z is a space separated list
* of token references (either labels or reg exprs).  Return a
* string "inlineX_set" for some unique integer X.  Basically,
* we pretend as if we had seen "#tokclass inlineX { A .. Z }"
* on the input stream outside of an action.
*/
char *
#ifdef __USE_PROTOS
inline_set(char *s)
#else
inline_set(s)
char *s;
#endif
{
  char *nxt;
  fprintf(stderr, "found consumeUntil( {...} )\n");
  while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
  if ( *s!='{' )
  {
    err("malformed consumeUntil( {...} ); missing '{'");
    return "bad_set";
  }
  s++;
  while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
  while ( *s!='}' )
  {
    if ( match_token(s,&nxt) ) fprintf(stderr, "found token %s\n", s);
    else if ( match_rexpr(s,&nxt) ) fprintf(stderr, "found rexpr %s\n", s);
    else {
      err("invalid element in consumeUntil( {...} )");
      return "bad_set";
    }
    s = nxt;
    while ( *s==' ' || *s=='\t' || *s=='\n' || *s=='\r' ) {s++;}
  }
  return "inlineX_set";
}

/* ANTLR-specific syntax error message generator
* (define USER_ZZSYN when compiling so don't get 2 definitions)
*/
void
#ifdef __USE_PROTOS
zzsyn(char *text, int tok, char *egroup, SetWordType *eset, int etok,
int k, char *bad_text)
#else
zzsyn(text, tok, egroup, eset, etok, k, bad_text)
char *text, *egroup, *bad_text;
int tok;
int etok;
int k;
SetWordType *eset;
#endif
{
fprintf(stderr, ErrHdr, FileStr[CurFile]!=NULL?FileStr[CurFile]:"stdin", zzline);
fprintf(stderr, " syntax error at \"%s\"", (tok==zzEOF_TOKEN)?"EOF":text);
if ( !etok && !eset ) {fprintf(stderr, "\n"); return;}
if ( k==1 ) fprintf(stderr, " missing");
else
{
fprintf(stderr, "; \"%s\" not", bad_text);
if ( zzset_deg(eset)>1 ) fprintf(stderr, " in");
}
if ( zzset_deg(eset)>0 ) zzedecode(eset);
else fprintf(stderr, " %s", zztokens[etok]);
if ( strlen(egroup) > (size_t)0 ) fprintf(stderr, " in %s", egroup);
fprintf(stderr, "\n");
}
