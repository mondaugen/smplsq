#ifndef DEFS_H
#define DEFS_H 

/* Defines for convenience */
#define _M(t,s) (t*)malloc(sizeof(t)*s)
#define _C(t,s) (t*)calloc(s,sizeof(t)) 
#define _MZ(x,t,s) memset(x,0,sizeof(t)*s) 

#endif /* DEFS_H */
