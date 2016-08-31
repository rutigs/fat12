#include <stdint.h>

typedef struct
{
    uint8_t jump_code[3];   /* Ignore this */
    char oemname[8];        /* Might as well ignore this one too */
    uint8_t ssize[2];       /* Sector size in bytes */
    uint8_t csize;          /* Cluster size in sectors */
    uint8_t reserved[2];    /* Number of reserved sectors for boot sectors */
    uint8_t numfat;         /* Number of FATs */
    uint8_t numroot[2];     /* Number of Root directory entries */
    uint8_t sectors16[2];   /* number of sectors in the file system */
    uint8_t media[1];       /* Media descriptor type */
    uint8_t sectperfat16[2];/* Number of sectors per FAT */
    uint8_t sectpertrack[2];/* Number of sectors per track */
    uint8_t heads[2];       /* Number of heads */
    uint8_t prevsect[2];    /* Number of sectors before FS partition */
    uint8_t ignore[482];    /* Ignore these */
} boot_sect_t;

void set_and_print_info(char *filename, int filesys);
void print_fat12_root(char *filename, int start_sect, int num_entries, int sect_size);
unsigned int two_bytes_to_int(unsigned char *buf);
unsigned int four_bytes_to_int(unsigned char *buf);
unsigned int three_bytes_to_int_low(unsigned char *buf);
unsigned int three_bytes_to_int_high(unsigned char *buf);
void print_clusters(unsigned int first_cluster, unsigned char *filename, int fat_32);
void print_dir(unsigned int first_cluster, unsigned char *filename, unsigned char *prefix, FILE *file);
char *read_sector(int sect_num, char *filename);
