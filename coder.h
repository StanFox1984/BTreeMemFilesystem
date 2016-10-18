#ifndef CODER_H
#define CODER_H

#include <string.h>
#include <unistd.h>
#include <stdlib.h>

class Coder
{
public:
    bool transparent;
    Coder(char *pass, bool _transparent = true)
    {
        strncpy(password, pass, 255);
        password[255] = '\0';
        transparent = _transparent;
    }
    virtual int encode(char *src, char *dst, int size)
    {
        if(transparent) return size;
        int len = strlen(password);
        int num = size / len;
        int remain = size % len;
        char *tmp = new char[len];
        for(int i=0;i<num;i++)
        {
            memcpy(tmp, &src[i*len], len);
            for(int j=0;j<len;j++)
            {
                tmp[j]^=password[j];
            }
            memcpy(&dst[i*len], tmp, len);
        }
        memcpy(tmp, &src[num*len],remain);
        for(int j=0;j<remain;j++)
        {
            tmp[j]^=password[j];
        }
        memcpy(&dst[num*len], tmp, remain);
        delete tmp;
        return size;
    }
    virtual int decode(char *src, char *dst, int size)
    {
        if(transparent) return size;
        return encode(src, dst, size);;
    }
protected:
    char password[256];
};


































#endif