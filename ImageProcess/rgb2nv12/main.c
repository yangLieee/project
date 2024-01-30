#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int fin_val(unsigned char min,unsigned char max,unsigned char x)
{
    if(x>max)
        return max;
    else if(x<min)
        return min;
    else
        return x;
}

int rgb2nv12(unsigned char *rgb_buf,unsigned char *nv12_buf,int w,int h)
{
    unsigned char r,g,b,y,u,v;
    unsigned char *ptrY,*ptrU,*ptrV,*ptrRGB;
    int i,j;
    memset(nv12_buf,0,w*h*3/2);
    ptrY = nv12_buf;
    ptrU = nv12_buf + w*h;
    ptrV = nv12_buf + w*h +1;

    for(j=0;j<h;j++)
    {
        ptrRGB = rgb_buf + w*j*3;
        for(i=0;i<w;i++){
            r = *(ptrRGB++);
            g = *(ptrRGB++);
            b = *(ptrRGB++);
            y = (unsigned char)((66*r + 129*g + 25*b )>>8) + 16;
            u = (unsigned char)((-38*r - 74*g + 112*b)>>8) + 128;
            v = (unsigned char)((112*r - 94*g - 18*b )>>8) + 128;
            *(ptrY++) = fin_val(0,255,y);

            if ( j%2!=0 ){
                if( i%2==0 ){
                    *(ptrU++) =fin_val(0,255,u);
                    ptrU++;
                }
                else{
                    *(ptrV++) = fin_val(0,255,v);
                    ptrV++;
                }
            }
        }
    }
    return 0;
}


int main(int argc, char *argv[])
{
    if(argc < 4) {
        printf("Usage : ./a.out filepath width height");
        return -1;
    }
    int w = atoi(argv[2]);
    int h = atoi(argv[3]);
    FILE *fp = fopen(argv[1],"rb+");
    FILE *fp1 = fopen("nv12_pic","wb+");

    int size = w*h*3;
    int size1 = w*h*3/2;
    unsigned char *rgb_buffer = (unsigned char *)malloc(size);
    unsigned char *nv12_buffer = (unsigned char *)malloc(size1);

    fread(rgb_buffer,sizeof(char),size,fp);
    rgb2nv12(rgb_buffer,nv12_buffer,w,h);
    fwrite(nv12_buffer,sizeof(char),size1,fp1);

    free(rgb_buffer);
    free(nv12_buffer);
    fclose(fp);
    fclose(fp1);

    return 0;
}

