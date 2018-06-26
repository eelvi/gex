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
