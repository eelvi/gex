#ifndef GEX_H
#define GEX_H

#define TABLE_LIMIT 64

typedef struct rdata sdata;
typedef struct _request req;
typedef struct _mstate mstate;
typedef struct _rmatch rmatch;
typedef struct _table req_table;
typedef void (*state_fx)(sdata *);

int matchesn(const char *pat, const char *src, int end, rmatch *res);
int searchn(const char *pat, const char *src, int end, rmatch *res);
int search(const char *pat, const char *src, rmatch *res);
int matches(const char *pat, const char *src, rmatch *res);


enum STATES{
    S_ACCEPT,
    S_REJECT,
};

struct _request{
    int wants;
    int repeat;
    int m_beg;
    int m_len;
};

struct _table{
    int idx;
    int size;
    req dirs[TABLE_LIMIT];
};

struct _mstate{
    state_fx state;
    const char *pat;
    const char *str;
    int s_idx;
    int s_end;
    req r;
};

struct _rmatch{
    int matched;
    int end;
};

struct rdata{
    int idx;
    mstate cst;
    rmatch match;
    req_table table;
};

#endif
