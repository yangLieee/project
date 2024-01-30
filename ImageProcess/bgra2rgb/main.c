#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc , char *argv[])
{
    if(argc < 4) {
        printf("Usage : ./a.out filepath width height");
        return -1;
    }
    int w = atoi(argv[2]);
    int h = atoi(argv[3]);
    int size = w * h * 4;
    int size1 = size / 4 * 3;
    unsigned char *buffer = (unsigned char * )malloc(size);

    FILE *fp = fopen(argv[1],"rb");
    if(fp==NULL){
        printf("Open Error\n");
        return -1;
    }
    fread(buffer,sizeof(char),size,fp);
    fclose(fp);

    fp = fopen("rgb_file", "wb");
    for(int i=0,j=0;j<size;i+=3,j+=4)
    {
        buffer[i+0] = buffer[j+2];
        buffer[i+1] = buffer[j+1];
        buffer[i+2] = buffer[j+0];
   }
    fwrite(buffer,1,size1,fp);
    fclose(fp);
    free(buffer);
    return 0;
}

