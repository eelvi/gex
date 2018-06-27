#ifndef GEX_H
#define GEX_H

#define TABLE_LIMIT 64
#define CHARCLASS_LIMIT 20
#define ASSOC_LIMIT 20


#define ASSOC_ESC 0

#define error__exit(code, ...)\
    do { fprintf(stderr, __VA_ARGS__); \
         fprintf(stderr, "code: %d\n", code); \
        exit(code); \
    } while (0)

typedef struct rdata sdata;
typedef struct _directive drcv;
typedef struct _mstate mstate;
typedef struct _rmatch rmatch;
typedef struct _table drcv_table;
typedef void (*state_fx)(sdata *);

typedef struct _charclass_element charclass_element;
typedef struct _charclass charclass;
typedef union _assoc_data assoc_data;

int matchesn(const char *pat, const char *src, int end, rmatch *res);
int searchn(const char *pat, const char *src, int end, rmatch *res);
int search(const char *pat, const char *src, rmatch *res);
int matches(const char *pat, const char *src, rmatch *res);


enum STATES{
    S_ACCEPT,
    S_REJECT,
};

struct _directive{
    //if wants is ASSOC_ESC then use assoc to find associated data
    int wants;
    int assoc;
    int repeat;
    int beg;
    int len;
};

enum CHARCLASS_SPECIAL_VAL{
    CHARCLASS_NORMAL,
    CHARCLASS_NOT,
};

enum ASSOC_TYPE{
    ASSOC_CHARCLASS = 1,
};

struct _charclass_element{
    short special;
    char low;
    char high;
};

//all _assoc structs must start with an integer reserved for identifying the type
struct _charclass{
    int assoc_type;
    int size;
    charclass_element cls[CHARCLASS_LIMIT];
};

//a type that will be used to store complicated directives
union _assoc_data{
    int atype;
    charclass chcls;
};

struct _table{
    int idx;
    int size;
    drcv dirs[TABLE_LIMIT];
    /* int ascs_idx; */
    int ascs_size;
    assoc_data ascs[ASSOC_LIMIT];
};

struct _mstate{
    state_fx state;
    const char *pat;
    const char *str;
    int s_idx;
    int s_end;
    drcv r;
};

struct _rmatch{
    int matched;
    int end;
};

struct rdata{
    int idx;
    mstate cst;
    rmatch match;
    drcv_table table;
};

#endif
