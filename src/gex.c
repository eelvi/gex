/*
    license: GPL
    author: /eelvi

supported: 
    \d, \w, text, +, *, .
    [^a-z0-9] 

missing features:
    everything
work in progress planned over the next decade or so:
    ^ and $
    { n, m} repeat
    (groups)
    non-greedy repeat

*/

#include <stdio.h>
#include <stdlib.h>
#include "gex.h"
#include "gex_help.h"

#define test 1

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


int check_charclass(const char *srcs, charclass *chcls){
    int i = 0;
    int inverse = 0;
    if (chcls->size <= 0){
        error__exit(2, "check_charclass(): invalid chrclass\n");
    }
    else if (chcls->cls[0].special == CHARCLASS_NOT){
        i++;
        inverse = 1;
    }
#define loop(IFMTCH, ELSE )\
for ( i; i < chcls->size; i++){\
    if ( (*srcs >= chcls->cls[i].low) && (*srcs <= chcls->cls[i].high) ){\
        return IFMTCH;\
    }\
}\
return ELSE;
    if (inverse){
        loop(0, 1);
    }
    else {
        loop(1, 0);
    }
#undef loop
}

int check_assoc(const char *srcs, drcv *r, drcv_table *tbl){
    if (!srcs || !r || !tbl){
        error__exit(1, "null passed to check_assoc()");
    }
    if ( tbl->ascs[r->assoc].atype == ASSOC_CHARCLASS){
        return check_charclass(srcs, &tbl->ascs[r->assoc].chcls);
    }
    else
        error__exit(1, "check_assoc(): unknown assoc type.");

}
int check(const char *srcs, drcv *r, drcv_table *tbl){
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
    else if (pat_ch == '.'){
        return src != '\0';
    }
    else if (pat_ch == ASSOC_ESC){
        return check_assoc(srcs, r, tbl);
    }
    else
        return src == pat_ch;
}

//will start with the character '['
int parse_charclass(drcv *target, sdata *d){
#define curr() ((charclass *)(& (d->table.ascs[ascs_idx])))
#define do_getch() (*(++(d->cst.pat)))
    int ascs_idx = (d->table.ascs_size)++;
    int ch = *(d->cst.pat);;

    target->assoc = ascs_idx;
    target->wants = ASSOC_ESC;
    curr()->assoc_type = ASSOC_CHARCLASS;
    if ((ascs_idx >= ASSOC_LIMIT) || (ch != '[')){
        return SETPAT_FAIL;
    }
    //skip the '['
    ch = do_getch();
    for (int i=0; i<CHARCLASS_LIMIT; i++){
        if (!ch){
            return SETPAT_FAIL;
        }
        else if (ch == ']'){
            ch = do_getch();
            return SETPAT_SUCCESS;
        }
        curr()->size++;
        if ((i == 0) && (ch == '^')){
            curr()->cls[i].special = CHARCLASS_NOT;
            ch = do_getch();
        }
        else{
            //regular char
            curr()->cls[i].special = 0;
            curr()->cls[i].low = curr()->cls[i].high = ch;
            ch = do_getch();
            if (ch == '-'){
                ch = do_getch();
                if ((ch == ']') || (!ch)){
                    return SETPAT_FAIL;
                }
                curr()->cls[i].high = ch;
            }
        }
        //ends with ch == next unseen 
#undef curr
#undef do_getch
    }
    return SETPAT_SUCCESS;
}
int set_match(sdata *d, drcv *r){
    int ch =  *(d->cst.pat);
    int rs = 0;
    int handeled = 0;
    _memset(r, 0, sizeof(drcv));

    if (!ch)
        return SETPAT_END;
    
    if (ch == '\\'){
        ch |= (*(++(d->cst.pat)) << 8);
        (d->cst.pat)++;
    }
    else if (ch == '['){
        if( (rs = parse_charclass(r, d)) != SETPAT_SUCCESS ){
            return rs;
        }
        handeled = 1;
    }
    else{
        (d->cst.pat)++;
    }
    r->wants = (handeled)? ASSOC_ESC : ch;
    //(d->cst.pat) is at next unseen char

    ch = *(d->cst.pat);
    if ( follows(ch) ){
        r->repeat = ch;
        d->cst.pat++;
    }
    return SETPAT_SUCCESS;
}

STATE(execute);
STATE(get_next);

STATE(pat_fail){
    error__exit(2,  "pattern failed: %s\n", d->cst.pat);
}

STATE(local_reject){
    int fix = 0;
#define tabel (d->table)
#define current_dir (d->table.dirs[i])
    for (int i = tabel.idx - 1; i >= 0; i--){
        if (is_repeating( current_dir.repeat )){
            if  ((current_dir.len > 1) ||
                ((current_dir.len == 1) && is_optional(current_dir.repeat)))
            {
                fix = 1;
                d->cst.s_idx = (current_dir.beg) + ( --current_dir.len );
                tabel.idx = i;
                _memcpy( &d->cst.r, &(current_dir), sizeof(drcv) );
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

//optimize str calculation
STATE(get_next){
    int dir_idx = (d->table.idx)++;
    if (d->table.idx < (d->table.size)){
        d->cst.s_idx = 0; 
        if(dir_idx >= 0){
            _memcpy( &(d->table.dirs[dir_idx]), &d->cst.r, sizeof(drcv) );
            d->cst.s_idx = d->table.dirs[dir_idx].beg + d->table.dirs[dir_idx].len;
        }
        _memcpy(&d->cst.r,  &(d->table.dirs[dir_idx + 1]), sizeof(drcv));
        d->cst.r.beg = d->cst.s_idx;
        d->cst.r.len = 0;

        transition(execute);
    }
    else
        transition(local_accept);
}

//TODO: optimize
STATE(execute){

    int sdx = d->cst.s_idx;
#define current_pos (d->cst.s_idx)
    /* dr_repr(d, current_pos, "/dev/pts/8"); */
    current_pos += check(d->cst.str + current_pos, &d->cst.r, &d->table);
    if ( !(current_pos > sdx) ){
        if (is_optional(d->cst.r.repeat)){
            transition(get_next);
        }
        transition(local_reject);
    }

    if (is_repeating(d->cst.r.repeat))
        while( (d->cst.s_idx < d->cst.s_end) && check(d->cst.str + current_pos, &d->cst.r, &d->table)){
            current_pos++;
        }

    d->cst.s_idx = current_pos;
    d->cst.r.len += current_pos-sdx;
    transition(get_next);
#undef current_pos
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

