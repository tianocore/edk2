#define Atom	256		/* token Atom (an impossible char value) */
#define Epsilon	257		/* epsilon arc (an impossible char value) */

/* track field must be same for all node types */
typedef struct _a {
					struct _a *track;	/* track mem allocation */
					int label;
					struct _a *next;
					struct _n *target;
				} Arc, *ArcPtr;

typedef struct _n {
					struct _n *track;
					ArcPtr arcs, arctail;
				} Node, *NodePtr;

typedef struct	{
					NodePtr left,
						 	right;
				} Graph, *GraphPtr;

#ifdef __USE_PROTOS
int rexpr( char *expr, char *s );
int match( NodePtr automaton, char *s );
#else
int rexpr();
int match();
#endif


