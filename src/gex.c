/*
    license: GPL
    author: /eelvi

supported: 
    \d, \w, text, +, *, .

missing features:
    everything
work in progress planned over the next decade or so:
    ^ and $
    [^ charecter classes]
    { n, m} repeat
    (groups)
    non-greedy repeat

*/

#include <stdio.h>
#include <stdlib.h>
#include "gex_help.h"
#include "gex.h"

#define test 1

#if test >= 1
    #define test_func main
#endif



#define SETPAT_SUCCESS 0
#define SETPAT_FAIL 1
#define SETPAT_END 2

#define STATE( STATE_NAME )\
    void S_ ## STATE_NAME (sdata *d)

#define transition( STK )\
    do{\
        d->cst.state = (state_fx) S_ ## STK ;\
        return;\
    } while (0)

#define is_optional( REP ) (((REP) == '?') || ((REP) == '*'))

#define is_repeating( REP ) (((REP) == '*') || ((REP) == '+'))

#define follows(CH) ( ((CH) == '+') || ((CH) == '*') || ((CH) == '?') )

int check(const char *srcs, req *r){
    int src = *srcs;
    int pat_ch = r->wants & 0xFF;
    int pat_peek = (r->wants & 0xFF00) >> 8;

    if (pat_ch == '\\')
        switch(pat_peek){
            case 'w':
                return _isalpha(src);
            case 'd':
                return _isdigit(src);
            default:
                return pat_peek == src;
        }
    else if (pat_ch == '.')
        return src != '\0';
    else
        return src == pat_ch;
}

int set_match(sdata *d, req *r){
    int ch =  *(d->cst.pat++);
    int peek = *d->cst.pat;

    if (!ch)
        return SETPAT_END;
    
    if (ch == '\\'){
        ch |= (peek << 8);
        peek = *(++(d->cst.pat));
    }
    else if (ch == '[')
        return SETPAT_FAIL;

    _memset(r, 0, sizeof(req));
    r->wants = ch;
    if (follows(peek)){
        r->repeat = peek;
        d->cst.pat++;
    }
    return SETPAT_SUCCESS;
}

STATE(execute);
STATE(get_next);

STATE(pat_fail){
    fprintf(stderr, "pattern failed: %s", d->cst.pat);
    exit(1);
}

STATE(local_reject){
    int fix = 0;
#define tabel (d->table)
#define current_dir (d->table.dirs[i])
    for (int i = tabel.idx - 1; i >= 0; i--){
        if (is_repeating( current_dir.repeat )){
            if  ((current_dir.m_len > 1) ||
                ((current_dir.m_len == 1) && is_optional(current_dir.repeat)))
            {
                fix = 1;
                d->cst.s_idx = (current_dir.m_beg) + ( --current_dir.m_len );
                tabel.idx = i;
                _memcpy( &d->cst.r, &(current_dir), sizeof(req) );
                break;
            }
        }
    }
    if (fix)
        transition(get_next);
    else
        transition(REJECT);
#undef tabel
#undef current_dir
}

STATE(local_accept){
    d->match.matched = 1;
    d->match.end = d->cst.s_idx;
    transition(ACCEPT);
}

//optimize str claculation
STATE(get_next){
    int dir_idx = (d->table.idx)++;
    if (d->table.idx < (d->table.size)){
        d->cst.s_idx = 0; 
        if(dir_idx >= 0){
            _memcpy( &(d->table.dirs[dir_idx]), &d->cst.r, sizeof(req) );
            d->cst.s_idx = d->table.dirs[dir_idx].m_beg + d->table.dirs[dir_idx].m_len;
        }
        _memcpy(&d->cst.r,  &(d->table.dirs[dir_idx + 1]), sizeof(req));
        d->cst.r.m_beg = d->cst.s_idx;
        d->cst.r.m_len = 0;

        transition(execute);
    }
    else
        transition(local_accept);
}

//TODO: optimize
STATE(execute){

    int sdx = d->cst.s_idx;
    int sde = sdx;
    sde += check(d->cst.str + sde, &d->cst.r);
    if ( !(sde > sdx) ){
        if (is_optional(d->cst.r.repeat)){
            transition(get_next);
        }
        transition(local_reject);
    }

    if (is_repeating(d->cst.r.repeat))
        while( (d->cst.s_idx < d->cst.s_end) && check(d->cst.str + sde, &d->cst.r)){
            sde++;
        }

    d->cst.s_idx = sde;
    d->cst.r.m_len += sde-sdx;
    transition(get_next);
}

STATE(prepare_pat){
    int s;
    d->table.size = 0;
    s = set_match(d, &(d->table.dirs[0]));

    while (s == SETPAT_SUCCESS){
        d->table.size++;
        s = set_match(d, &( d->table.dirs[d->table.size] ));
    }
    if (s == SETPAT_FAIL)
        transition(pat_fail);

    d->table.idx = -1;
    transition(get_next);
}

STATE(START){
    transition(prepare_pat);
}
 
int matchesn(const char *pat, const char *src, int end, rmatch *res)
{
    sdata d;
    _memset(&d, 0, sizeof(sdata));

    d.cst.state = S_START;
    d.cst.pat = pat;
    d.cst.str = src;
    d.cst.s_end = end;

    while ( (d.cst.state != (state_fx)S_REJECT) && (d.cst.state != (state_fx)S_ACCEPT) ){
        d.cst.state(&d);
    }

    if (res)
        _memcpy(res, &d.match, sizeof(rmatch));

    return (d.cst.state == S_ACCEPT);
}


int searchn(const char *pat, const char *src, int end, rmatch *res)
{
    int s, at = 0;
    s = matchesn(pat, src + at, end, res);

    while(!s && *(src+(++at))){
        s = matchesn(pat, src + at, end-at, res);
    }
    return (s)? at : -1;
}

int matches(const char *pat, const char *src, rmatch *res)
{
    return matchesn(pat,src,_strlen(src), res);
}
int search(const char *pat, const char *src, rmatch *res)
{
    return searchn(pat, src, _strlen(src), res);
}


