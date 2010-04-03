/* T e m p l a t e  F o r  S y m b o l  T a b l e  M a n a g e r */

/* define some hash function */
#ifndef HASH
#define HASH(p, h) while ( *p != '\0' ) h = (h<<1) + *p++;
#endif

/* minimum symbol table record */
typedef struct _sym {
			char *symbol;
			struct _sym *next, *prev, **head, *scope;
			unsigned int hash;
		} Sym, *SymPtr;

#ifdef __USE_PROTOS
void zzs_init(int, int);
void zzs_done(void);
void zzs_add(char *, Sym *);
Sym *zzs_get(char *);
void zzs_del(Sym *);
void zzs_keydel(char *);
Sym **zzs_scope(Sym **);
Sym *zzs_rmscope(Sym **);
void zzs_stat(void);
Sym *zzs_new(char *);
Sym *zzs_newadd(char *);
char *zzs_strdup(char *);
#else
void zzs_init();
void zzs_done();
void zzs_add();
Sym *zzs_get();
void zzs_del();
void zzs_keydel();
Sym **zzs_scope();
Sym *zzs_rmscope();
void zzs_stat();
Sym *zzs_new();
Sym *zzs_newadd();
char *zzs_strdup();
#endif
