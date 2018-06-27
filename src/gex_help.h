#ifndef GEX_HELPERS_H
#define GEX_HELPERS_H

const char *flatten(int nl){
    static char se[100];
    char *f;
    f = se;
        while (nl != 0){
            *(f++) = (nl & (0xFF));
            nl >>= 8;
        }
        *f = 0;
    return se;
}

const char *tr(int v){
    return ((v)?"true":"false");
}

#if 1 == 1
#include <fcntl.h>
#include <unistd.h>
 const char *dr_repr(sdata *d, int sde, const char *term){
    int fd = open(term, O_WRONLY);
    if (fd < 0){
        perror("open");
        fprintf(stderr, "dr_repr(): couldn't open terminal\n");
        exit(1);
        return 1;
    }
    dprintf(fd,  "\n\
    current directive: ");
    if (d->cst.r.wants != ASSOC_ESC){
        dprintf(fd,  "%s\n", flatten(d->cst.r.wants));
    }
    else if (d->table.ascs[d->cst.r.assoc].atype == ASSOC_CHARCLASS){
        dprintf(fd, "\t[ ");
        charclass *chcls = &d->table.ascs[d->cst.r.assoc].chcls;
        for (int i=0; i<chcls->size; i++){
            if ((i==0) && (chcls->cls[i].special == CHARCLASS_NOT)){
                dprintf(fd, "NOT: ");
            }
            else{
                dprintf(fd, "%c-%c,",chcls->cls[i].low, chcls->cls[i].high );
            }
        }
        dprintf(fd, "]\n");
    }



    /* static char buff[512]; */
    int mtch = check(d->cst.str + sde, &d->cst.r, &d->table);
    dprintf(fd,  "\
    current str: \"%s\"\n\
    d->cst.s_idx: %d\n\
    d->cst.sde: %d\n\
    current directive match: %s\n\
    current directive repeat: (%c)\n\n", d->cst.str + sde,  d->cst.s_idx, sde, tr(mtch), d->cst.r.repeat);
   
 }
#endif

int _isdigit(char d)
{
    return (d >= '0' && d <= '9');
}
int _isalpha(char d)
{
    return (d >= 'a' && d<='z');
}
int _strncmp(const char *a, const char *b, int n)
{
    while (*a && *b && a==b && (n > 0)){
        a++; 
        b++;
        n--;
    }
    return *a - *b;
}
int _strlen(const char *a)
{
    const char *s = a;
    while(*s)
        s++;
    return (int) (s - a);
}
int _memcpy(void *dest, void *src, int len)
{
    char *d = dest, *s = src, *end = s + len;
    while(s < end){
        *(d++) = *(s++);
    }
    return 0;
}
int _memset(void *dest, unsigned char byte, int len){
    char *d = dest;
    while(len--){
        *(d++) = byte;
    }
    return 0;
};

#endif
