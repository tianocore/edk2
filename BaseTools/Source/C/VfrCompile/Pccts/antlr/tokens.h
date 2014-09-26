#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: antlr.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33MR33
 */
#define zzEOF_TOKEN 1
#define Eof 1
#define QuotedTerm 2
#define Action 34
#define Pred 35
#define PassAction 36
#define WildCard 87
#define LABEL 89
#define Pragma 92
#define FirstSetSymbol 93
#define NonTerminal 100
#define TokenTerm 101
#define ID 148
#define INT 150

#ifdef __USE_PROTOS
void grammar(void);
#else
extern void grammar();
#endif

#ifdef __USE_PROTOS
void class_def(void);
#else
extern void class_def();
#endif

#ifdef __USE_PROTOS
void rule(void);
#else
extern void rule();
#endif

#ifdef __USE_PROTOS
void laction(void);
#else
extern void laction();
#endif

#ifdef __USE_PROTOS
void lmember(void);
#else
extern void lmember();
#endif

#ifdef __USE_PROTOS
void lprefix(void);
#else
extern void lprefix();
#endif

#ifdef __USE_PROTOS
void aPred(void);
#else
extern void aPred();
#endif

#ifdef __USE_PROTOS
extern Predicate * predOrExpr(void);
#else
extern Predicate * predOrExpr();
#endif

#ifdef __USE_PROTOS
extern Predicate * predAndExpr(void);
#else
extern Predicate * predAndExpr();
#endif

#ifdef __USE_PROTOS
extern Predicate * predPrimary(void);
#else
extern Predicate * predPrimary();
#endif

#ifdef __USE_PROTOS
void aLexclass(void);
#else
extern void aLexclass();
#endif

#ifdef __USE_PROTOS
void error(void);
#else
extern void error();
#endif

#ifdef __USE_PROTOS
void tclass(void);
#else
extern void tclass();
#endif

#ifdef __USE_PROTOS
void token(void);
#else
extern void token();
#endif

#ifdef __USE_PROTOS
void block(set * toksrefd,set * rulesrefd);
#else
extern void block();
#endif

#ifdef __USE_PROTOS
void alt(set * toksrefd,set * rulesrefd);
#else
extern void alt();
#endif

#ifdef __USE_PROTOS
extern LabelEntry * element_label(void);
#else
extern LabelEntry * element_label();
#endif

#ifdef __USE_PROTOS
extern Node * element(int old_not,int first_on_line,int use_def_MT_handler);
#else
extern Node * element();
#endif

#ifdef __USE_PROTOS
void default_exception_handler(void);
#else
extern void default_exception_handler();
#endif

#ifdef __USE_PROTOS
extern ExceptionGroup * exception_group(void);
#else
extern ExceptionGroup * exception_group();
#endif

#ifdef __USE_PROTOS
extern ExceptionHandler * exception_handler(void);
#else
extern ExceptionHandler * exception_handler();
#endif

#ifdef __USE_PROTOS
void enum_file(char * fname);
#else
extern void enum_file();
#endif

#ifdef __USE_PROTOS
void defines(char * fname);
#else
extern void defines();
#endif

#ifdef __USE_PROTOS
void enum_def(char * fname);
#else
extern void enum_def();
#endif

#endif
extern SetWordType zzerr1[];
extern SetWordType zzerr2[];
extern SetWordType zzerr3[];
extern SetWordType zzerr4[];
extern SetWordType setwd1[];
extern SetWordType zzerr5[];
extern SetWordType zzerr6[];
extern SetWordType zzerr7[];
extern SetWordType zzerr8[];
extern SetWordType zzerr9[];
extern SetWordType setwd2[];
extern SetWordType zzerr10[];
extern SetWordType zzerr11[];
extern SetWordType zzerr12[];
extern SetWordType zzerr13[];
extern SetWordType setwd3[];
extern SetWordType zzerr14[];
extern SetWordType zzerr15[];
extern SetWordType zzerr16[];
extern SetWordType zzerr17[];
extern SetWordType zzerr18[];
extern SetWordType zzerr19[];
extern SetWordType zzerr20[];
extern SetWordType zzerr21[];
extern SetWordType setwd4[];
extern SetWordType zzerr22[];
extern SetWordType zzerr23[];
extern SetWordType zzerr24[];
extern SetWordType zzerr25[];
extern SetWordType zzerr26[];
extern SetWordType setwd5[];
extern SetWordType zzerr27[];
extern SetWordType zzerr28[];
extern SetWordType zzerr29[];
extern SetWordType zzerr30[];
extern SetWordType zzerr31[];
extern SetWordType zzerr32[];
extern SetWordType zzerr33[];
extern SetWordType setwd6[];
extern SetWordType zzerr34[];
extern SetWordType zzerr35[];
extern SetWordType zzerr36[];
extern SetWordType zzerr37[];
extern SetWordType zzerr38[];
extern SetWordType zzerr39[];
extern SetWordType zzerr40[];
extern SetWordType zzerr41[];
extern SetWordType zzerr42[];
extern SetWordType setwd7[];
extern SetWordType zzerr43[];
extern SetWordType zzerr44[];
extern SetWordType zzerr45[];
extern SetWordType zzerr46[];
extern SetWordType zzerr47[];
extern SetWordType zzerr48[];
extern SetWordType zzerr49[];
extern SetWordType zzerr50[];
extern SetWordType zzerr51[];
extern SetWordType zzerr52[];
extern SetWordType zzerr53[];
extern SetWordType setwd8[];
extern SetWordType zzerr54[];
extern SetWordType zzerr55[];
extern SetWordType zzerr56[];
extern SetWordType zzerr57[];
extern SetWordType setwd9[];
extern SetWordType zzerr58[];
extern SetWordType zzerr59[];
extern SetWordType zzerr60[];
extern SetWordType zzerr61[];
extern SetWordType zzerr62[];
extern SetWordType zzerr63[];
extern SetWordType zzerr64[];
extern SetWordType zzerr65[];
extern SetWordType setwd10[];
extern SetWordType setwd11[];
