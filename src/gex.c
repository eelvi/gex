/*
    license: GPL
    author: /eelvi

supported: 
    \d, \w, \W, text, +, *, .
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

#define SETPAT_SUCCESS 0
#define SETPAT_FAIL 1
#define SETPAT_END 2

//the function definition of each of the states
#define STATE( STATE_NAME )\
    void S_ ## STATE_NAME (sdata *d)

// sets the function pointer the the struct pointed to by STK then return from the current function
// to be used in a loop executing until S_ACCEPT or S_REJECT is set
#define transition( STK )\
    do{\
        d->cst.state = (state_fx) S_ ## STK ;\
        return;\
    } while (0)

#define is_optional( REP ) (((REP) == '?') || ((REP) == '*'))

#define is_repeating( REP ) (((REP) == '*') || ((REP) == '+'))

// returns whether a character follows what was before it, ex. the '+' in \w+
#define follows(CH) ( ((CH) == '+') || ((CH) == '*') || ((CH) == '?') )


// check whether current source character matches the character class
int check_charclass(const char *src_str, charclass *chcls){
    int i = 0;
    int inverse = 0;
    if (chcls->size <= 0){
        error__exit(2, "check_charclass(): invalid chrclass\n");
    }
    else if (chcls->cls[0].special == CHARCLASS_NOT){
        i++;
        inverse = 1;
    }

/////////////////////////////////////
#define loop(IFMTCH, ELSE )\
for ( i; i < chcls->size; i++){\
    if ( (*src_str >= chcls->cls[i].low) && (*src_str <= chcls->cls[i].high) ){\
        return IFMTCH;\
    }\
}\
return ELSE;
/////////////////////////////////////

    if (inverse){
        loop(0, 1);
    }
    else {
        loop(1, 0);
    }

#undef loop
}

// serves as a polymorhpic checker for directives that contain a drcv.wants==ASSOC_ESC value
int check_assoc(const char *src_str, drcv *directive, drcv_table *tbl){

    if (!src_str || !directive || !tbl){
        error__exit(1, "null passed to check_assoc()");
    }
    if ( tbl->ascs[directive->assoc].atype == ASSOC_CHARCLASS){
        return check_charclass(src_str, &tbl->ascs[directive->assoc].chcls);
    }
    else
        error__exit(1, "check_assoc(): unknown assoc type.");
}

int check(const char *src_str, drcv *directive, drcv_table *tbl){
    int src_ch = *src_str;
    int pat_ch = directive->wants & 0xFF;
    int pat_peek = (directive->wants & 0xFF00) >> 8;

    if (pat_ch == '\\')
        switch(pat_peek){
            case 'w':
                return _isalpha(src_ch);
            case 'd':
                return _isdigit(src_ch);
            case 's':
                return _isspace(src_ch);
            case 'W':
                return _isalpha(src_ch) == 0;
            case 'D':
                return _isdigit(src_ch) == 0;
            case 'S':
                return _isspace(src_ch) == 0;
            default:
                return pat_peek == src_ch;
        }
    else if (pat_ch == '.'){
        return 1;
    }
    else if (pat_ch == ASSOC_ESC){
        return check_assoc(src_str, directive, tbl);
    }
    else
        return src_ch == pat_ch;
}

// goal: fill a directive with necessary info indicating that the directive is using
// associated data (stored in d->table) and register an entry in the associated data table which contains
// the character class as (min,max) ascii integers
//*starts with the character '['
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

// goal: parse a single directive from the pattern, to be called repetedly until
// it returns SETPAT_END
int set_match(sdata *d, drcv *r){
    int ch =  *(d->cst.pat);
    int rs = 0;
    int ext_handeled = 0;
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
        ext_handeled = 1;
    }
    else{
        (d->cst.pat)++;
    }
    r->wants = (ext_handeled)? ASSOC_ESC : ch;
    //(d->cst.pat) is at next unseen char

    ch = *(d->cst.pat);
    if ( follows(ch) ){
        r->repeat = ch;
        d->cst.pat++;
    }
    return SETPAT_SUCCESS;
}

//forward declarations
STATE(execute);
STATE(get_next);

STATE(pat_fail){
    error__exit(2,  "pattern failed: %s\n", d->cst.pat);
}

//goal: this is arrived to whenever a directive fails to match
//so what we do in the current implementation is look at previous directives and attempt to reduce their yeild
// if that is possible, otherwise if we reach the first directive (meaning all directives after it can't be reduced any further)
// then we reject the whole input
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

//this is called to finalize our match, it should probably be refactored
STATE(local_accept){
    d->match.matched = 1;
    d->match.end = d->cst.s_idx;
    transition(ACCEPT);
}

//this fetches the next directive and executes it, it serves a central role in the flow of execution
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


// this does the task of:
// 1- checking if the current source character matches the current directive
// 2- if so and our directive is a one that repeats then do so until we can no longer match
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

// this prepares the match by repeatedly inserting directives into our directive table then starts the thing by 
// transitioning into prepare_pat()
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

