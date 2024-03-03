#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for int32_t

// https://www.pjrc.com/tech/8051/ide/fat32.html

unsigned short BPB_BytesPerSector = 0; //Bytes Per Sector, must be 512
unsigned char BPB_SecPerCluster = 0; //Sectors Per Cluster
unsigned short BPB_RsvdSecCnt = 0; //Number of Reserved Sectors, usually 0x20
unsigned char BPB_NumFATs = 0; //Number of FAT's, must be 2
int32_t BPB_FATSz32 = 0; //Sectors Per FAT
int32_t BPB_RootClus = 0; //Root Directory First Cluster

int init(unsigned char* disk) {
    unsigned char *disk_start = disk;

    BPB_BytesPerSector |= *(disk_start + 0x0B + 1);
    BPB_BytesPerSector <<= 8;
    BPB_BytesPerSector |= *(disk_start + 0x0B);
    printf("Bytes Per Sector: %d\n", BPB_BytesPerSector);
    if(BPB_BytesPerSector != 512) {
        printf("Bytes Per Sector NOT 512! Aborting.\n");
        return -1;
    }

    BPB_SecPerCluster |= *(disk_start + 0x0D);
    printf("Sectors Per Cluster: %d\n", BPB_SecPerCluster);

    BPB_RsvdSecCnt |= *(disk_start + 0x0E + 1);
    BPB_RsvdSecCnt <<= 8;
    BPB_RsvdSecCnt |= *(disk_start + 0x0E);
    printf("Number of Reserved Sectors: 0x%x\n", BPB_RsvdSecCnt);

    BPB_NumFATs |= *(disk_start + 0x10);
    printf("Number of FATs: %d\n", BPB_NumFATs);
    if(BPB_NumFATs != 2) {
        printf("Number of FATs NOT 2! Aborting.\n");
        return -2;
    }

    BPB_FATSz32 |= *(disk_start + 0x24 + 3);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk_start + 0x24 + 2);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk_start + 0x24 + 1);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk_start + 0x24);
    printf("Sectors Per FAT: 0x%x\n", BPB_FATSz32);

    BPB_RootClus |= *(disk_start + 0x2C + 3);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk_start + 0x2C + 2);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk_start + 0x2C + 1);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk_start + 0x2C);
    printf("Root Directory First Cluster: 0x%x\n", BPB_RootClus);

    unsigned short signature = 0;
    signature |= *(disk_start + 0x1FE + 1);
    signature <<= 8;
    signature |= *(disk_start + 0x1FE);
    printf("Signature: 0x%x\n", signature);
    if(signature != 0xAA55) {
        printf("Signature NOT 0xAA55! Aborting.\n");
        return -3;
    }

    printf("MBR check-up successfully completed!\n");
    return 0;
}

void format(unsigned char* disk) {

}

int main()
{
    FILE *image = fopen("/home/ob3r0n/fat32.disk", "r+b");
    if(!image) {
        perror("Error opening image!\n");
        //create then..
        return -1;
    }
    printf("Image opened successfully!\n");

    unsigned char *disk = malloc(20480);
    if(fread(disk, 1, 20480, image) != 20480) {
        perror("Error reading image!\n");
        return -1;
    }
    printf("Image readed successfully!\n");
    init(disk);

    free(disk);
    return 0;
}
