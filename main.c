#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> //for intxx_t
#include <string.h> //memset
#include <time.h>

// https://www.pjrc.com/tech/8051/ide/fat32.html
// https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf
// http://elm-chan.org/docs/fat_e.html

uint16_t BPB_BytesPerSector = 0; //Bytes Per Sector, must be 512
uint8_t BPB_SecPerCluster = 0; //Sectors Per Cluster
uint16_t BPB_RsvdSecCnt = 0; //Number of Reserved Sectors, usually 0x20
uint8_t BPB_NumFATs = 0; //Number of FAT's, must be 2
uint8_t BPB_Media = 0; //0xF8 standart for non-removable media, 0xF0 for removable
uint32_t BPB_TotSec32 = 0; //count of sectors on the volume

uint32_t FS_FreeClusters = 0;

//FAT32-only fields
uint32_t BPB_FATSz32 = 0; //Sectors Per FAT
uint16_t BPB_ExtFlags = 0; //must be 0 if mirroring is disabled
uint16_t BPB_FSVer = 0; //must be 0?
uint32_t BPB_RootClus = 0; //Root Directory First Cluster
uint16_t BPB_FSInfo = 0; //usually 1
uint16_t BPB_BkBootSec = 0; //sector num of a copy of the Boot Record, usually 6
uint8_t BS_DrvNum = 0; // int 0x13 drive number (e.g, 0x80), offset 64 decimal for FAT32
uint8_t BS_BootSig = 0; // Extended boot signature, offset 66
uint32_t BS_VolID = 0; // Volume serial number, simply generated by combinint date and time into a 32-bit value
uint8_t BS_VolLab[11] = "NO NAME    "; // volume label, "NO NAME" where no volume label
uint8_t BS_FilSysType[8] = "FAT32   ";

//File attributes
enum {
    ATTR_READ_ONLY = 0x01,
    ATTR_HIDDEN = 0x02,
    ATTR_SYSTEM = 0x04,
    ATTR_VOLUME_ID = 0x08,
    ATTR_DIRECTORY = 0x10,
    ATTR_ARCHIVE = 0x20,
    ATTR_LONG_NAME = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID,
    ATTR_LONG_NAME_DIRECTORY = ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID | ATTR_DIRECTORY |  ATTR_ARCHIVE //why do i need it?
};

struct LDirectoryFAT32 {
    uint8_t LDIR_Ord; //Order of this entry
    uint8_t LDIR_Name1[10]; //Characters 1 to 5, portion of the long name
    uint8_t LDIR_Attr; //Attributes, must be ATTR_LONG_NAME
    uint8_t LDIR_Type; //must be 0
    uint8_t LDIR_Chksum; //checksum of name
    uint8_t LDIR_Name2[12]; //characters 6 to 11
    uint16_t LDIR_FstClusLO; //must be 0
    uint8_t LDIR_Name3[4]; //characters 12 and 13
};


struct DirectoryFAT32 {
    unsigned char DIR_NAME[11]; //Short Name
    uint8_t DIR_ATTR; //File attributes
    uint8_t DIR_NTRes; //Reserved for use by Windows NT, must be 0
    uint8_t DIR_CrtTimeTenth; //Millisecond stamp at file creation time, tenths of a second, 0-199.
    uint16_t DIR_CrtTime; //Time file was created;
    uint16_t DIR_CrtDate; //Date file was created;
    uint16_t DIR_LstAccDate; //Last access date, when write must be same as DIR_WrtTime
    uint16_t DIR_FstClusHI; //High word of this entry's first cluster number
    uint16_t DIR_WrtTime; //Time of last write
    uint16_t DIR_WrtDate; //Date of last write
    uint16_t DIR_FstClusLO; //Low word of this entry's first cluster number
    uint32_t DIR_FileSize; //32-bit DWORD holding file size in bytes

};

unsigned char ChkSum (unsigned char *pFcbName) //Microsoft's function for checksum generation, 7.2 of Microsoft FAT Specification
{
    short FcbNameLen;
    unsigned char Sum;
    Sum = 0;
    for (FcbNameLen=11; FcbNameLen!=0; FcbNameLen--) {
        // NOTE: The operation is an unsigned char rotate right
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
    }
    return (Sum);
}

unsigned char toupper(unsigned char input) { //converts char to uppercase; no need to include whole ctype.h for one simple func
    if(input >= 0x61 && input <= 0x7A) //if char is lowercase a-z
        return input - 0x20;
    else
        return input;
}

int init(uint8_t* disk) {

    //TODO: COUNT FS_FreeClusters = BPB_TotSec32 - (BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32)) - 3;

    BPB_BytesPerSector = 0;
    BPB_SecPerCluster = 0;
    BPB_RsvdSecCnt = 0;
    BPB_NumFATs = 0;
    BPB_Media = 0;
    BPB_TotSec32 = 0;
    BPB_FATSz32 = 0;
    BPB_ExtFlags = 0;
    BPB_FSVer = 0;
    BPB_RootClus = 0;
    BPB_FSInfo = 0;
    BPB_BkBootSec = 0;
    BS_DrvNum = 0;
    BS_BootSig = 0;

    //TODO: read and print all FAT32 information??

    BPB_BytesPerSector |= *(disk + 0x0B + 1);
    BPB_BytesPerSector <<= 8;
    BPB_BytesPerSector |= *(disk + 0x0B);
    printf("Bytes Per Sector: %d\n", BPB_BytesPerSector);
    if(BPB_BytesPerSector != 512) {
        printf("Bytes Per Sector NOT 512! Aborting.\n");
        return -1;
    }

    BPB_SecPerCluster |= *(disk + 0x0D);
    printf("Sectors Per Cluster: %d\n", BPB_SecPerCluster);

    BPB_RsvdSecCnt |= *(disk + 0x0E + 1);
    BPB_RsvdSecCnt <<= 8;
    BPB_RsvdSecCnt |= *(disk + 0x0E);
    printf("Number of Reserved Sectors: %d\n", BPB_RsvdSecCnt);

    BPB_NumFATs |= *(disk + 0x10);
    printf("Number of FATs: %d\n", BPB_NumFATs);
    if(BPB_NumFATs != 2) {
        printf("Number of FATs NOT 2! Aborting.\n");
        return -2;
    }


    BPB_FATSz32 |= *(disk + 0x24 + 3);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk + 0x24 + 2);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk + 0x24 + 1);
    BPB_FATSz32 <<= 8;
    BPB_FATSz32 |= *(disk + 0x24);
    printf("Sectors Per FAT: %d\n", BPB_FATSz32);

    BPB_RootClus |= *(disk + 0x2C + 3);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk + 0x2C + 2);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk + 0x2C + 1);
    BPB_RootClus <<= 8;
    BPB_RootClus |= *(disk + 0x2C);
    printf("Root Directory First Cluster: 0x%x\n", BPB_RootClus);

    unsigned short signature = 0;
    signature |= *(disk + 0x1FE);
    signature <<= 8;
    signature |= *(disk + 0x1FE + 1);
    printf("Signature: 0x%x\n", signature);
    if(signature != 0x55AA) {
        printf("Signature NOT 0x55AA! Aborting.\n");
        return -3;
    }

    printf("MBR check-up successfully completed!\n");
    return 0;
}

void writeMBR(uint8_t *disk) {
    //Dummy jump instruction, BS_jmpBoot
    // *disk = 0xEB;
    // *(disk+1) = 0x58;
    // *(disk+2) = 0x90;

    //OEM name, BS_OEMName
    char oem[] = {'f', 'a', 't', '-', 'e', 'm', 'u', 'l'};
    memcpy(disk+0x03, oem, 8);

    //BIOS Parameter Block (BPB)
    *((uint16_t*)(disk+0xB)) = BPB_BytesPerSector;
    *(disk+0xD) = BPB_SecPerCluster;
    *((uint16_t*)(disk+0xE)) = BPB_RsvdSecCnt;
    *(disk+0x10) = BPB_NumFATs;
    *(disk+21) = BPB_Media;
    *((uint32_t*)(disk+32)) = BPB_TotSec32;
    *((uint32_t*)(disk+0x24)) = BPB_FATSz32;
    *((uint16_t*)(disk+40)) = BPB_ExtFlags;
    *((uint16_t*)(disk+42)) = BPB_FSVer;
    *((uint32_t*)(disk+0x2C)) = BPB_RootClus;
    *((uint16_t*)(disk+48)) = BPB_FSInfo;
    *((uint16_t*)(disk+50)) = BPB_BkBootSec;

    //Boot Sector
    *((uint8_t*)(disk+64)) = BS_DrvNum;
    *((uint8_t*)(disk+66)) = BS_BootSig;
    *((uint32_t*)(disk+67)) = BS_VolID;
    memcpy(disk+71, BS_VolLab, 11);
    memcpy(disk+82, BS_FilSysType, 8);


    *((uint16_t*)(disk+0x1FE)) = 0xAA55;
}

void writeFSInfSector(uint8_t *disk) {
    uint8_t *FSInfo_start = disk+0x200;
    //RRaA sector signature, FSI_LeadSig
    *(FSInfo_start) = 0x52;
    *(FSInfo_start+1) = 0x52;
    *(FSInfo_start+2) = 0x61;
    *(FSInfo_start+3) = 0x41;

    //bytes 4 -> 484 (0x1E4) - FSI_Reserved1

    //rrAa sector signature, FSI_StrucSig
    *(FSInfo_start+0x1E4) = 0x72;
    *(FSInfo_start+0x1E4+1) = 0x72;
    *(FSInfo_start+0x1E4+2) = 0x41;
    *(FSInfo_start+0x1E4+3) = 0x61;

    //last known number of free data clusters on the volume, FSI_Free_Count
    //FS_FreeClusters = BPB_TotSec32 - (BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32)) - 3;
    memcpy(FSInfo_start+0x1E8, &FS_FreeClusters, 8);

    //number of most recently known to be allocated data cluster, FSI_Nxt_Free
    *(FSInfo_start+0x1EC) = 0x02;
    *(FSInfo_start+0x1EC+1) = 0x00;
    *(FSInfo_start+0x1EC+2) = 0x00;
    *(FSInfo_start+0x1EC+3) = 0x00;

    //bytes 496 - 508 (0x1FC) - FSI_Reserved2

    //FS information sector signature
    *(FSInfo_start+0x1FC) = 0x00;
    *(FSInfo_start+0x1FC+1) = 0x00;
    *(FSInfo_start+0x1FC+2) = 0x55;
    *(FSInfo_start+0x1FC+3) = 0xAA;
}

uint32_t getFreeSector(uint8_t* disk, uint32_t cluster) { //returns free sector offset in root;
    uint32_t firstData = (BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32)) * BPB_BytesPerSector;
    if(cluster != 0)
        firstData += (cluster - 2) * BPB_SecPerCluster * BPB_BytesPerSector;
    printf("Start search address for free sector: 0x%x\n", firstData);
    for(int sector = 0; sector < 16; sector++) { //16 for searching only in root dir for now
        printf("Checking 0x%x\n", firstData + sector*32);
        if(*(disk + firstData + sector*32) == 0 || *(disk + firstData + sector*32) == 0xE5) { //if sector free or contain removed dir
            printf("Found available sector at offset: 0x%x\n", firstData + sector*32);
            return firstData + sector*32;
        }
    }
    printf("Not found available sector?");
}

uint32_t getFreeCluster(uint8_t *disk) { //returns free cluster number
    uint32_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
    printf("Starting search of free cluster at 0x%x\n", FirstDataSector*BPB_BytesPerSector);
    for(int cluster = 1; cluster < (strlen(disk) / BPB_BytesPerSector) - FirstDataSector; cluster++) { //sectors 0 and 1 are reserved
        printf("Checking offset 0x%x\n", (FirstDataSector+cluster)*BPB_BytesPerSector);
        if(*(disk+(FirstDataSector+cluster)*BPB_BytesPerSector) == 0x0) { //if first cluster byte is zero it means it unused? TODO: add more checks like for 0xE5 and so on
            return cluster;
        }
    }
    return -1; //no free memory?
}

void format(uint8_t* disk, unsigned int size) {

    BPB_BytesPerSector = 512;
    BPB_SecPerCluster = 1;
    BPB_RsvdSecCnt = 32; //typical for FAT32
    BPB_NumFATs = 2;
    BPB_Media = 0xF8; //comes from MS-Dos ver.1 and not used anymore

    //FAT32-only fields
    BPB_TotSec32 = size / BPB_BytesPerSector;
    uint32_t n_clusters = (BPB_TotSec32 - BPB_RsvdSecCnt) / BPB_SecPerCluster;
    BPB_FATSz32 = ((((n_clusters + 2) * 4) + BPB_BytesPerSector - 1) / BPB_BytesPerSector);
    BPB_ExtFlags = 0x04; //without mirroring, fat0 is active
    BPB_FSVer = 0; //0:0 version
    BPB_RootClus = 2;
    BPB_FSInfo = 1;
    BPB_BkBootSec = 6;
    BS_DrvNum = 0x80;
    BS_BootSig = 0x29;
    BS_VolID = (uint32_t)time(NULL);
    memcpy(BS_VolLab, "NO NAME    ", sizeof(11));
    memcpy(BS_FilSysType, "FAT32   ", sizeof(8));

    FS_FreeClusters = BPB_TotSec32 - (BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32)) - 1;

    memset(disk, 0, size);

    writeMBR(disk); //Master Boot Record
    writeFSInfSector(disk); //File System Information Sector


    //Backup MBR at sector 6
    //Writing MBR, BPB_BkBootSec
    writeMBR(disk + BPB_BytesPerSector * BPB_BkBootSec);
    writeFSInfSector(disk + BPB_BytesPerSector * BPB_BkBootSec);  //A copy of the FSInfo sector is also there

    //sectors 0 and 1 are in use
    *(disk+0x4000) = 0xF8;
    *(disk+0x4000+1) = 0xFF;
    *(disk+0x4000+2) = 0xFF;
    *(disk+0x4000+3) = 0x0F;
    *(disk+0x4000+4) = 0xFF;
    *(disk+0x4000+5) = 0xFF;
    *(disk+0x4000+6) = 0xFF;
    *(disk+0x4000+7) = 0x0F;
    *(disk+0x4000+8) = 0xF8;
    *(disk+0x4000+9) = 0xFF;
    *(disk+0x4000+10) = 0xFF;
    *(disk+0x4000+11) = 0x0F;
}

void printBinaryWithZeros(uint32_t num) {
    int i;
    for (i = 31; i >= 0; --i) {
        putchar('0' + ((num >> i) & 1));
    }
    printf("\n");
}

uint8_t isSFN(char *name) { //is given filename is 8.3 filename
    int len = strlen(name);
    if(len > 11) return 0;
    int dots_count = 0; //8.3 specifies that filename MUST NOT contain more than one '.'
    int dot_index = -1;
    for(int i = 0; i < len; i++) {
        if(name[i] >= 97 && name[i] <= 122) //if name contain lowercase char
            return 0;

        if(name[i] == ' ') //if name contain space character
            return 0;

        if(name[i] == '.') {
            dots_count++;
            if(dots_count > 1)
                return 0;
            dot_index = 1;
        }


    }

    if(dots_count == 1) { //means that filename extension is present
        if((len - dot_index) < 1 || (len - dot_index) > 3) { //means filename extension is NOT 1-3 charachers in length
            return 0;
        }
    }

    return 1;
}

void mkdir(FILE *image, uint8_t* disk, char *name, uint32_t parent_cluster, uint8_t isFile) {
    int len = strlen(name);
    //creating shortname
    unsigned char shortname[11];
    //TODO: More 8.3 format transforms https://en.wikipedia.org/wiki/8.3_filename

    if(len > 11) { //then need to split by ~1 "the quick brown fox" -> "THEQUI~1FOX"
        for(int i = 0, sym = 0; i < 6; i++) {
            if(name[sym] == 0x20) {
                sym++;
                i--;
            }
            else
                shortname[i] = toupper(name[sym++]);
        }
        shortname[6] = 0x7E; //~ ,TODO: calculations for ~1, ~2 and so on
        shortname[7] = 0x31; //1
        for(int j = 8, i = len-3; j < 11; i++) {
            shortname[j++] = toupper(name[i]);
        }
    } else {
        for(int i = 0; i < 11; i++) {
            if(i < len) {
                shortname[i] = name[i];
            } else {
                shortname[i] = 0x20;
            }

        }
    }

    unsigned char checksum = ChkSum(shortname);

    //basic validations of name
    if(len > 255) {
        printf("Error! Name length can't be more 255. Aborting.\n");
        return;
    }

    //TODO: do checks for restricted symbols for shortname

    //Folder will consist from LDir entries + short dir entry if shortname is not 8.3 uppercase
    uint32_t free_sector = getFreeSector(disk, parent_cluster); //get free sector to where we can write new folder

    if(!isSFN(name)) {
        int num_entries = (len + 12) / 13; //alculate the number of entries needed
        int remaining_chars = len; //keep track of remaining characters to process
        for (int i = num_entries - 1; i >= 0; i--) {
            //converting name into LDirName, Section 7 of Microsoft FAT Specification
            char entry[13];

            strncpy(entry, name + (i * 13), 13);

            { //block for not storing memory for "writed" variable for long time
                short writed = 0; //write 0x00 0x00 at end of name
                for (int j = 0; j < 13; j++) {
                    if(entry[j] == 0) {
                        entry[j] = 0xFF;
                        if(writed++ < 2)
                            entry[j] = 0x00;
                    }

                }
            }

            printf("LDIR entry %d: %s\n", i + 1, entry);
            remaining_chars -= 13;

            //creating LDIR entry
            struct LDirectoryFAT32 ldir_entry;
            if(i+1 == num_entries) {
                ldir_entry.LDIR_Ord = num_entries | 0x40; //7.1 Spec, Nth member must contain a value N | LAST_LONG_ENTRY (0x40)
            } else {
                ldir_entry.LDIR_Ord = i+1;
            }

            for(int j = 0; j < 10; j+=2) { //adding symbols to LDIR_Name1
                ldir_entry.LDIR_Name1[j] = entry[j/2];
                if(ldir_entry.LDIR_Name1[j] != 0xFF)
                    ldir_entry.LDIR_Name1[j+1] = 0; //because LDir names in Unicode, so 2 bytes for char
                else ldir_entry.LDIR_Name1[j+1] = 0xFF;
            }

            ldir_entry.LDIR_Attr = ATTR_LONG_NAME;
            ldir_entry.LDIR_Type = 0;
            ldir_entry.LDIR_Chksum = checksum;

            for(int j = 0, symb = 5; j < 11; j+=2) { //adding symbols to LDIR_Name2
                ldir_entry.LDIR_Name2[j] = entry[symb++];
                if(ldir_entry.LDIR_Name2[j] != 0xFF)
                    ldir_entry.LDIR_Name2[j+1] = 0; //because LDir names in Unicode, so 2 bytes for char
                else ldir_entry.LDIR_Name2[j+1] = 0xFF;
            }

            ldir_entry.LDIR_FstClusLO = 0;

            for(int j = 0, symb = 11; j < 4; j+=2) { //adding symbols to LDIR_Name3
                ldir_entry.LDIR_Name3[j] = entry[symb++];
                if(ldir_entry.LDIR_Name3[j] != 0xFF)
                    ldir_entry.LDIR_Name3[j+1] = 0; //because LDir names in Unicode, so 2 bytes for char
                else ldir_entry.LDIR_Name3[j+1] = 0xFF;
            }

            //writing long dir entry into disk
            memcpy(disk+free_sector, &ldir_entry, 32);
            printf("Writed 32 bytes LDir to 0x%x\n", free_sector);
            free_sector+=32; //32 bytes writed, so go to the next sector
        }
    }

    //adding short directory entry
    struct DirectoryFAT32 dir_entry;
    memcpy(dir_entry.DIR_NAME, shortname, 11);
    if(isFile)
        dir_entry.DIR_ATTR = 0;
    else
        dir_entry.DIR_ATTR = ATTR_DIRECTORY;
    dir_entry.DIR_NTRes = 0;

    //getting current time
    struct timespec current_time_spec;
    clock_gettime(CLOCK_REALTIME, &current_time_spec);

    //calculate and write tenths
    dir_entry.DIR_CrtTimeTenth = current_time_spec.tv_nsec / 10000000;

    time_t current_time_t;
    struct tm *utc_time;

    time(&current_time_t);
    utc_time = gmtime(&current_time_t);

    uint8_t hours = utc_time->tm_hour;
    uint8_t minutes = utc_time->tm_min;
    uint8_t seconds = utc_time->tm_sec;

    uint8_t year = utc_time->tm_year - 2000; //year since 2000
    uint8_t month = utc_time->tm_mon + 1;    //month (0-11)
    uint8_t day = utc_time->tm_mday;         //day of the month

    printf("Date: %d-%d-%d %d:%d:%d\n", year, month, day, hours, minutes, seconds);

    dir_entry.DIR_CrtTime = (hours << 11) | minutes << 5 | (seconds / 2);
    dir_entry.DIR_CrtDate = (year << 9) | (month << 5) | day;


    dir_entry.DIR_LstAccDate = dir_entry.DIR_CrtDate;

    uint32_t freeCluster = getFreeCluster(disk) + 2;
    printf("Free cluster: %d\n", freeCluster - 2);

    dir_entry.DIR_FstClusHI = (freeCluster >> 16) & 0xFFFF;

    dir_entry.DIR_WrtTime = dir_entry.DIR_CrtTime;
    dir_entry.DIR_WrtDate = dir_entry.DIR_CrtDate;

    dir_entry.DIR_FstClusLO = freeCluster & 0xFFFF;
    dir_entry.DIR_FileSize = 0; //0 size for directory (?)

    //writing dir entry into disk
    memcpy(disk+free_sector, &dir_entry, 32);
    printf("Writed 32 bytes dir to 0x%x\n", free_sector);

    //creating . and .. folders
    uint16_t first_data_sector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
    uint16_t first_alloc_sector = (freeCluster-2 * BPB_SecPerCluster) + first_data_sector;
    uint32_t first_alloc_offset = first_alloc_sector * BPB_BytesPerSector;
    printf("Offset of %d sector: 0x%x\n", freeCluster-2, first_alloc_offset);
    dir_entry.DIR_NAME[0] = 0x2E; //'.'
    for(int i = 1; i < 11; i++) {
        dir_entry.DIR_NAME[i] = 0x20;
    }
    memcpy(disk+first_alloc_offset, &dir_entry, 32);
    printf("Writed 32 bytes dir allocation to 0x%x\n", first_alloc_offset);

    dir_entry.DIR_NAME[1] = 0x2E; //'..'
    dir_entry.DIR_FstClusHI = (parent_cluster >> 16) & 0xFFFF;
    dir_entry.DIR_FstClusLO = parent_cluster & 0xFFFF;

    memcpy(disk+first_alloc_offset+32, &dir_entry, 32);
    printf("Writed 32 bytes dir allocation to 0x%x\n", first_alloc_offset+32);

    //writing, that cluster freeCluster is in use
    //TODO: value is link to next F6-02?
    uint32_t inuse_endofchain_start = 0x0FFFFFF8;
    uint32_t inuse_endofchait_end = 0x0FFFFFFF;
    uint32_t inuse = 0xFFFFFFF0;
    memcpy(disk+0x4000+(freeCluster-2)*8, &inuse_endofchain_start, 4); //in use(end of chain)
    memcpy(disk+0x4000+(freeCluster-2)*8+4, &inuse_endofchait_end, 4);

    //updating free clusters info
    printf("Free clusters: %d\n", FS_FreeClusters);
    FS_FreeClusters--;
    writeFSInfSector(disk);

}

int main()
{
    FILE *image = fopen("/home/ob3r0n/fat32.disk", "wb");
    if(!image) {
        perror("Error opening image!\n");
        //create then..
        return -1;
    }
    printf("Image opened successfully!\n");

    uint8_t *disk = malloc(20971520);

    format(disk, 20971520);
    fwrite(disk, 1, 20971520, image);
    fclose(image);

    image = fopen("/home/ob3r0n/fat32.disk", "rb");
    if(!image) {
        perror("Error opening image:");
        //create then..
        free(disk);
        return -1;
    }

    if(fread(disk, 1, 20971520, image) != 20971520) {
        perror("Error reading image");
        return -1;
    }
    printf("Image readed successfully!\n");

    init(disk);

    uint16_t FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
    printf("First data sector: %d\n", FirstDataSector);
    printf("Address: 0x%x\n", FirstDataSector * BPB_BytesPerSector);

    fclose(image);

    image = fopen("/home/ob3r0n/fat32.disk", "wb");
    if(!image) {
        perror("Error opening image!\n");
        //create then..
        return -1;
    }

    mkdir(image, disk, "The quick brown.fox", 0, 0);
    mkdir(image, disk, "TEST", 3, 1);

    if(fwrite(disk, 1, 20971520, image) != 20971520) {
        perror("Error writing image");
    }

    fclose(image);
    free(disk);
    return 0;
}
