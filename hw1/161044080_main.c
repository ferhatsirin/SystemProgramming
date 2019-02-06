#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <math.h>
#include <unistd.h>

struct IFDInfo{

    int height;
    int width;
    int bitsPerPixel;
    int photoInter;  // Photometric Interpretation 0 means white is zero 1 for black
    int stripOffset; //image data offset
    int stripByteCount;
    int type; // 1 big endian 0 little endian 
    int IFDsize;
    int IFDstart; 
};

int hexToInt(char *str,int length,int type);

void readIFD(char *file,struct IFDInfo* imageInfo); // reads file descriptor save values into imageInfo

void printImage(char *file,struct IFDInfo imageInfo);

void hexToBinary(char c, int bitsPerPixels, char* result);

int binaryToDec(char *bin, int bits);

void printPixels(char *arr,int length,char comp);

int main(int argc, char** argv) {
          
    if(argc != 2 ){
    
        fprintf(stderr,"Wrong line argument enter only the file name");
        exit(EXIT_FAILURE);
    }
    int fd =open(argv[1],O_RDONLY);
    
    if(fd == -1){
    
        fprintf(stderr,"File does not exist or can not be read ");
        exit (EXIT_FAILURE);
    }
    // resources 
    char *file;
    struct stat fileInfo;
    struct IFDInfo imageInfo;
    
    if(stat(argv[1],&fileInfo) < 0){
    
        fprintf(stderr,"Program can't read file information");
        exit (EXIT_FAILURE);
    }

    file =(char*)malloc(fileInfo.st_size);
    
    int fileSize =read(fd,file,fileInfo.st_size);
    if(fileSize != fileInfo.st_size){
    
        fprintf(stderr,"The program could not read the whole file");
        exit(EXIT_FAILURE);
    }

    if((int)file[1] == 73){
    
        imageInfo.type =0; //little endian
    }
    else{
    
        imageInfo.type =1;  // big endian
    }  

    imageInfo.IFDstart = hexToInt(&file[4],4,imageInfo.type);
    imageInfo.IFDsize =hexToInt(&file[imageInfo.IFDstart],2,imageInfo.type);  
  
    readIFD(file,&imageInfo);

    printf("Width : %d pixels \n",imageInfo.width);    
    printf("Height : %d pixels \n",imageInfo.height);
    printf("Byte order : %s \n", (imageInfo.type)==1 ? "Motorola" : "Intel" );

    printImage(file,imageInfo);
    
    free(file);
    
    if(close(fd)== -1){
    
        fprintf(stderr,"File close error ");
        exit(EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}

int hexToInt(char *str,int length,int type){ // return int number corresponding to hex number 

    int i;
    int result =0;
       
    for(i=0; i<length; ++i){    
        result +=((unsigned char)str[i])*pow(16,fabs(2*(type*(length-1)-i))); 
    }
    
    return result;
}

void readIFD(char *file,struct IFDInfo* imageInfo){ // reads file descriptor save values into imageInfo
    int i, found;
    int tag, type, count, valueSize;
    found =0;
    for(i =imageInfo->IFDstart+2; found <6 && i < imageInfo->IFDstart+2+imageInfo->IFDsize*12; i +=12){
        tag =hexToInt(&file[i],2,imageInfo->type);
        type =hexToInt(&file[i+2],2,imageInfo->type);
        count =hexToInt(&file[i+4],4,imageInfo->type);
        switch(type){
            case 1:
            case 2: //byte
                type =1;
                break;
            case 3: //short
                type =2;
                break;
            case 4: //long
                type =4;
                break;
            case 5 : //rational
                type =8;
                break;
        }
        valueSize =type*count;
        
        switch(tag){
            case 256: //image width
                imageInfo->width =hexToInt(&file[i+8],valueSize,imageInfo->type);
                ++found;
                break;
            case 257: // image height
                imageInfo->height =hexToInt(&file[i+8],valueSize,imageInfo->type);
                ++found;
                break;
            case 258: // bitsPerPixel
                imageInfo->bitsPerPixel =hexToInt(&file[i+8],valueSize,imageInfo->type);
                ++found;
                break;   
            case 262: // Photometric Interpretation 
                imageInfo->photoInter =hexToInt(&file[i+8],valueSize,imageInfo->type);
                ++found;
                break;
            case 273: // stripOffset
                if(valueSize <= 4){
                    imageInfo->stripOffset =hexToInt(&file[i+8],valueSize,imageInfo->type);
                }
                else{
                    imageInfo->stripOffset =hexToInt(&file[i+8],4,imageInfo->type);
                    imageInfo->stripOffset =hexToInt(&file[imageInfo->stripOffset],type,imageInfo->type);
                }
                ++found;
                break;
            case 279: // stripByteCount
                if(valueSize <= 4){
                   imageInfo->stripByteCount =hexToInt(&file[i+8],valueSize,imageInfo->type);       
                }
                else{
                    int offset =hexToInt(&file[i+8],4,imageInfo->type);
                    imageInfo->stripByteCount =0;
                    for(int k =offset;k < offset+valueSize;k +=4)
                      imageInfo->stripByteCount +=hexToInt(&file[k],type,imageInfo->type);
                }
                ++found;
                break;
        }
    }
}

int binaryToDec(char *bin, int bits){

    int result =0;
    for(int i=0;i<bits;++i){
        result +=(bin[i]-'0')*pow(2,bits-1-i);  
    }
    
    return result;
}

void hexToBinary(char c, int bitsPerPixels, char* result){
   
    int num =hexToInt(&c,1,1);
    int i =7;
   
    while(num != 0) {
        result[i] = (num%2) + '0';
        num = num/2;
        --i;
    }
    while(0 <= i){
        result[i] =0+'0';
        --i;
    }
    
    if(1 < bitsPerPixels){
        for(int i=0,j =0;i<8/bitsPerPixels;++i, j +=bitsPerPixels){
            result[i] =binaryToDec(&result[j],bitsPerPixels) +'0';
        }
    }
}

void printPixels(char *arr,int length,char comp){

    for(int i=0;i<length;++i){
        if(arr[i] == comp)
            printf("1");
        else
            printf("0");
    }
}

void printImage(char *file,struct IFDInfo imageInfo){

    int i,j;
    
    if(8 <= imageInfo.bitsPerPixel ){
        int comp;
        comp =imageInfo.photoInter *(pow(2,imageInfo.bitsPerPixel)-1);
        for(i= imageInfo.stripOffset; i<imageInfo.stripOffset+imageInfo.stripByteCount;i +=imageInfo.width*(imageInfo.bitsPerPixel/8)){
            for(j =0; j <imageInfo.width*(imageInfo.bitsPerPixel/8); j +=imageInfo.bitsPerPixel/8){
                if(hexToInt(&file[i+j],imageInfo.bitsPerPixel/8,1) == comp){   
                    printf("1");   
                }
                else{   
                    printf("0");       
                }      
            }
            printf("\n");
        }
    }
    else{ 
        char pixels[8]; int remainder;
        char comp;
        comp = (imageInfo.photoInter *(pow(2,imageInfo.bitsPerPixel)-1))+'0';
        for(i= imageInfo.stripOffset; i<imageInfo.stripOffset+imageInfo.stripByteCount;i +=imageInfo.width/(8/imageInfo.bitsPerPixel)+remainder){
            remainder =0;
            for(j =0; j <imageInfo.width /(8/imageInfo.bitsPerPixel); ++j){
                
                hexToBinary(file[i+j],imageInfo.bitsPerPixel,pixels);
                printPixels(pixels,8/imageInfo.bitsPerPixel,comp);
            }
            remainder =imageInfo.width %(8/imageInfo.bitsPerPixel);
            if(0 <remainder){
                hexToBinary(file[i+j],imageInfo.bitsPerPixel,pixels);
                printPixels(pixels,remainder,comp);
                remainder =1;
            }      
            printf("\n");
        }
    }
}

