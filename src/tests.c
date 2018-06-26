
#include <stdio.h>
#include <string.h>
#include "gex.h"

#define test 1

#define does( pat, str, expected)\
    t = matches(pat,str, &rs);\
    printf("%s == %s : %s (%s)\n", str, pat, ((t)?"match":"no match"), ((t == expected )?"success":"fail"));


void print_match(const char *pat, const char *src);

int main(int argc, const char **argv)
{
    rmatch rs;
    int t;
    does("\\w+", "hello", 1);
    does("w23", "w23", 1);
    does("w23", "w21", 0);
    does("x+s","xxxxxs", 1);
    does("x2?s","xs", 1);
    does("x2?s","x2s", 1);
    does("h.+lo","hello", 1);
    does("h.+lo","hello12345", 1);
    does("h.+lo","helso", 0);
    does("ha.+lo\\d+","haelslo231", 1);
    does("ha.+lod+","haelslodddd", 1);
    does("hello.+","hello world", 1);
    does("f\\w*\\d+foo", "foo23foo", 1);
    does("f\\w*\\d+\\w+\\d+foo", "f2oo23foo", 1);
    does("f\\w*\\d+foo", "f2oso23foo", 0);
    search("f\\w*\\d+foo", "foo23foo", &rs);

    print_match("h.+lo" ,"hello12345");
    print_match("zip: *\\d+", " bla bla bla zip: 23314 bla bla bla");
    print_match("zip: *\\d+", " bla  bla bla blabla bla zip:93354 bla bla bla");
    print_match("zip: *\\d+", " bla bla blazip:   13314 bla bla bla bla bla bla");
    print_match("hello12345" ,"h.+lo");
    print_match("foo23foo", "f\\w*\\d+foo");
   


    //TODO: future me, figure out why
    //BUGS:
    // FIXED 95% OF THEM


#if test > 2

    int x = 0;
    int m ,s;
    char a[50], b[50];
    while ( 1 ){
        printf("enter  pat: ");
        scanf("%40s", a);
        printf("\"%s\"\n",a);
        printf("enter text: ");
        scanf("%40s", b);
        printf("\"%s\"\n",b);
        if ( (*a =='x') || (*b=='x') )
            break;
        m = matches(a,b);
        s = search(a,b);
        printf(" match: %d\n"
               "search: %d\n", m,s);
    }
#endif

}

void print_match(const char *pat, const char *src)
{
    rmatch rs;
    char buf[40];
    int at = search(pat, src, &rs);
    if (at != -1){
        memcpy(buf, src+at, rs.end);
        buf[rs.end]=0;
        printf("match: \"%s\"\n", buf);
    }
    else 
        printf("no match\n");
}
