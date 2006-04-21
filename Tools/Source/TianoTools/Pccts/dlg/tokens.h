#ifndef tokens_h
#define tokens_h
/* tokens.h -- List of labelled tokens and stuff
 *
 * Generated from: dlg_p.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33MR33
 */
#define zzEOF_TOKEN 1
#define L_EOF 4
#define PER_PER 5
#define NAME_PER_PER 6
#define LEXMEMBER 7
#define LEXACTION 8
#define PARSERCLASS 9
#define LEXPREFIX 10
#define ACTION 11
#define GREAT_GREAT 12
#define L_BRACE 13
#define R_BRACE 14
#define L_PAR 15
#define R_PAR 16
#define L_BRACK 17
#define R_BRACK 18
#define ZERO_MORE 19
#define ONE_MORE 20
#define OR 21
#define RANGE 22
#define NOT 23
#define OCTAL_VALUE 24
#define HEX_VALUE 25
#define DEC_VALUE 26
#define TAB 27
#define NL 28
#define CR 29
#define BS 30
#define CONTINUATION 31
#define LIT 32
#define REGCHAR 33

#ifdef __USE_PROTOS
void grammar(void);
#else
extern void grammar();
#endif

#ifdef __USE_PROTOS
void start_states(void);
#else
extern void start_states();
#endif

#ifdef __USE_PROTOS
void do_conversion(void);
#else
extern void do_conversion();
#endif

#ifdef __USE_PROTOS
void rule_list(void);
#else
extern void rule_list();
#endif

#ifdef __USE_PROTOS
void rule(void);
#else
extern void rule();
#endif

#ifdef __USE_PROTOS
void reg_expr(void);
#else
extern void reg_expr();
#endif

#ifdef __USE_PROTOS
void and_expr(void);
#else
extern void and_expr();
#endif

#ifdef __USE_PROTOS
void repeat_expr(void);
#else
extern void repeat_expr();
#endif

#ifdef __USE_PROTOS
void expr(void);
#else
extern void expr();
#endif

#ifdef __USE_PROTOS
void atom_list(void);
#else
extern void atom_list();
#endif

#ifdef __USE_PROTOS
void near_atom(void);
#else
extern void near_atom();
#endif

#ifdef __USE_PROTOS
void atom(void);
#else
extern void atom();
#endif

#ifdef __USE_PROTOS
void anychar(void);
#else
extern void anychar();
#endif

#endif
extern SetWordType zzerr1[];
extern SetWordType zzerr2[];
extern SetWordType zzerr3[];
extern SetWordType setwd1[];
extern SetWordType zzerr4[];
extern SetWordType zzerr5[];
extern SetWordType zzerr6[];
extern SetWordType setwd2[];
extern SetWordType zzerr7[];
extern SetWordType zzerr8[];
extern SetWordType zzerr9[];
extern SetWordType setwd3[];
