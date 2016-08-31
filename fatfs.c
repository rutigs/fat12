#include <stdio.h>
#include "boot_sect.h"
#include <string.h> // not sure if needed yet
#include <stdlib.h>
#include <errno.h>

boot_sect_t bs;
int first_fat_sect, first_root_sect, first_data_sect;

int main(int argc, char **argv) 
{
	if(argc < 3) {
		printf("Usage: ./fatfs filename.ext filesystem\n");
		printf("Only fat12 or fat32 are valid filesystem inputs.\n");
		return 1;
	}

	char *filename = argv[1];
	char *filesys = argv[2];

	int filesystype = -1; // 0 = fat12, 1 = fat32

	printf("Using filesystem type: %s\n", argv[2]);

	if(!strcmp(argv[2], "fat12")) {
		filesystype = 0;
	}

	if(!strcmp(argv[2], "fat32"))
		filesystype = 1;

	if(filesystype == -1) {
		printf("Invalid filesystem type: only fat12 or fat32 are valid inputs\n");
		return 1;
	}

	set_and_print_info(filename, filesystype);

	if(!filesystype)
		print_fat12_root(filename, first_root_sect, two_bytes_to_int((char *) &bs.numroot), two_bytes_to_int((char *) &bs.ssize));
	else
		// print_fat32

	return 0;
}

/*
	PART 1: Print requested information
	- Boot Sector is 512 bytes but we don't care about most of it

	TODO add functionality for fat32
*/
void set_and_print_info(char *filename, int filesys) 
{
	printf("\nPrinting information for the file: %s\n\n", filename);
	
	int status;
	char buf[30];

	FILE *file = fopen(filename, "rb");
	// check the file was opened properly
	if(file == NULL) {
		perror("fopen");
		exit(1);
	}

	status = fread((void *) buf, 1, 30, file);
	// check the correct number of bytes are read
	if(status != 30) {
		perror("read");
		exit(1);
	}
	
	bs.ssize[0] = buf[11];
	bs.ssize[1] = buf[12];

	bs.csize = buf[13];

	bs.reserved[0] = buf[14];
	bs.reserved[1] = buf[15];

	bs.numfat = buf[16];

	bs.numroot[0] = buf[17];
	bs.numroot[1] = buf[18];

	bs.sectors16[0] = buf[19];
	bs.sectors16[1] = buf[20];

	bs.sectperfat16[0] = buf[22];
	bs.sectperfat16[1] = buf[23];

	bs.sectpertrack[0] = buf[24];
	bs.sectpertrack[1] = buf[25];

	bs.heads[0] = buf[26];
	bs.heads[1] = buf[27];

	bs.prevsect[0] = buf[28];
	bs.prevsect[1] = buf[29];

	printf("Sector size = %d\n", two_bytes_to_int((char *) &bs.ssize));
	printf("Cluster size in sectors = %d\n", bs.csize);
	printf("Number of entries in the root directory = %d\n", two_bytes_to_int((char *) &bs.numroot));
	printf("Number of sectors per FAT = %d\n", two_bytes_to_int((char *) &bs.sectperfat16));
	printf("Number of reserved sectors = %d\n", two_bytes_to_int((char *) &bs.reserved));	
	printf("Number of hidden sectors = %d\n", two_bytes_to_int((char *) &bs.prevsect));

	// The first FAT doesn't start until after the reserved sectors
	first_fat_sect = two_bytes_to_int((char *) &bs.reserved);
	printf("The sector number of the first FAT = %d\n", first_fat_sect);

	// The root directory isn't until after number of FAT's * sector's per FAT so we have to add the previous plus this
	first_root_sect = first_fat_sect + two_bytes_to_int((char *) &bs.sectperfat16) * bs.numfat;
	printf("The sector number of the first sector of the root directory = %d\n", 
		first_root_sect);

	// The root directory is of sector size (num root entries * 32) / (bytes per sector) + previous
	first_data_sect = first_root_sect + ( (two_bytes_to_int((char *) &bs.ssize) * 32) / two_bytes_to_int((char *) &bs.ssize) );
	printf("The sector number of the first sector of the first usable data cluster = %d\n\n", first_data_sect);

	// if(filesystype)
	//     do more shit

	fclose(file);
}

/*
	PART 2: Print information about each file in the root directory
*/
void print_fat12_root(char *filename, int start_sect, int num_entries, int sect_size)
{
	int status, i, j;

	// make the buffer the size of entries
	unsigned char buf[32];
	// for building the filename of entries
	char new_filename[1024];
	memset(new_filename, '\0', 1024);
		
	FILE *file = fopen(filename, "rb");
	// check if the file was opened properly
	if(file == NULL) {
		perror("fopen");
		exit(1);
	}

	int start_index = start_sect * two_bytes_to_int((char *) &bs.ssize);

	for(i = 0; i < num_entries; i++) {
		// i keeps track of which directory entry we are in of size 32 bytes each
		status = fseek(file, start_index + i*32, SEEK_SET);
		// check if the file position indicator  was set correctly
		if(status == -1){
			perror("fseek");
			exit(1);
		}

		status = fread((void *) buf, 1, 32, file);
		// check the correct number of bytes are read
		if(status != 32) {
			perror("fread");
			exit(1);
		}

		if(buf[0] == 0x00)
			return;
		if(buf[0] == 0xe5)
			continue;

		unsigned char name[9], ext[4];
		unsigned int attr, is_dir, is_volume_label;
		unsigned int start_cluster;
		unsigned int size;

		// get the name
		for(j = 0; j < 8; j++) {
			if(buf[j] == 0x20) {
				name[j] = '\0';			
				break;
			}

			if(j == 0 && buf[j] == 0x05)
				name[j] = 0xe5;
			else
				name[j] = buf[j]; // reverse for little endianness
		}
		name[8] = '\0';


		// get the extension
		for(j = 8; j < 11; j++) {
			if(buf[j] == ' ') {
				ext[j-8] = '\0';
				break;
			}
			ext[j-8] = buf[j]; // reverse for little endianness
		}
		ext[3] = '\0';

		// TODO: not sure what checking we might have to do to this
		attr = (unsigned int) buf[11];
		if (attr == 0 || attr >= 0x40)
			continue;	
	
		// check the 3rd bit using a mask to check for volume label
		is_volume_label = (( attr & 0x08 ) == 0x08);
		if(is_volume_label)
			continue;

		// check the 4th bit using a mask to check if dir
		is_dir = (( attr & 0x10 ) == 0x10);

		// get the starting cluster as an integer
		start_cluster = two_bytes_to_int((char *) &buf[26]);

		//  get the size of the cluster as an integer
		size = four_bytes_to_int((char *) &buf[28]);

		if(is_dir == 0 && size == 0)
			continue;

		strcat(new_filename, "\\");
		strcat(new_filename, name);
		printf("Filename: %s", new_filename);
		if(strcmp(ext, ""))
			printf(".%s", ext);
		printf("\n");
		
		if(is_dir)
			printf("This is a directory.\n");
		else
			printf("Size: %d\n", size);

		// print clusters
		printf("Cluster(s): ");
		print_clusters(start_cluster, filename, 0);	
		printf("\n\n");

		// recurse through the dir if necessary
		if(is_dir) {
			print_dir(start_cluster, filename, new_filename, file);
		}

		memset(new_filename, '\0', 1024);
	}

	fclose(file);
}

void print_clusters(unsigned int start_cluster, unsigned char *filename, int fat_32)
{
	int curr, prev;

	if(start_cluster == 0)
		printf("%d\n", start_cluster);
	else
		curr = start_cluster;
	prev = curr;

	char *fat;
	fat = read_sector(first_fat_sect, filename);

	// loops until we hit the last cluster
	while(1) {
		int temp;
		if(fat_32) {
			// TODO
		} else {
			if(is_last_cluster_fat12(curr, fat))
				break;
			temp = next_cluster_fat12(curr, fat);
		}
		if(temp - curr != 1) {
			if (prev != curr)
				printf("%d-%d", prev, curr);
			else
				printf("%d", curr);
			printf(",");
			prev = temp;
		}
		curr = temp;
	}

	if(prev != curr)
		printf("%d-%d", prev, curr);
	else
		printf("%d", curr);

	free(fat);
}

void print_dir(unsigned int first_cluster, unsigned char *filename, unsigned char *prefix, FILE *file)
{
	int status, i, j;

	// make the buffer the size of entries
	unsigned char buf[32];
	// for building the filename of entries
	unsigned char new_filename[1024];
	memset(new_filename, '\0', 1024);
	
	int size = two_bytes_to_int((char *) &bs.ssize);
	//int reserved = two_bytes_to_int((char *) &bs.reserved) * size;
	 
	int offset_index = ((first_cluster - 2) * size * bs.csize) + (41 * size);
	//int start_index = first_cluster * two_bytes_to_int((char *) &bs.ssize) * bs.csize;
	int num_entries = (two_bytes_to_int((char *) &bs.ssize) * bs.csize) / 32;
	//printf("Number of entries: %d\n", num_entries);
	//printf("The start index is: %d\n", start_index);
	for(i = 0; i < num_entries; i++) {
		// i keeps track of which directory entry we are in of size 32 bytes each
		status = fseek(file, offset_index + i*32, SEEK_SET);
		//status = fseek(file, start_index + i*32, SEEK_SET);
		// check if the file position indicator  was set correctly
		if(status == -1){
			perror("fseek");
			exit(1);
		}

		status = fread((void *) buf, 1, 32, file);
		// check the correct number of bytes are read
		if(status != 32) {
			perror("fread");
			exit(1);
		}

		if(buf[0] == 0x00)
			return;
		if(buf[0] == 0xe5)
			continue;

		unsigned char name[9], ext[4];
		unsigned int attr, is_dir, is_volume_label;
		unsigned int start_cluster;
		unsigned int size;

		// get the name
		for(j = 0; j < 8; j++) {
			if(buf[j] == 0x20) {
				name[j] = '\0';			
				break;
			}

			if(j == 0 && buf[j] == 0x05)
				name[j] = 0xe5;
			else
				name[j] = buf[j]; 
		}
		name[8] = '\0';
		//printf("Name retrieved = %s\n", name);
	
		//if(strcmp(name[0], ".") || strcmp(name[1], "."))
		//	continue;	
	
		// get the extension
		for(j = 8; j < 11; j++) {
			if(buf[j] == ' ') {
				ext[j-8] = '\0';
				break;
			}
			ext[j-8] = buf[j]; // reverse for little endianness
		}
		ext[3] = '\0';

		attr = buf[11];
		if (attr == 0 || attr >= 0x40)
			continue;
		
		// check the 3rd bit using a mask to check for volume label
		is_volume_label = (( attr & 0x08 ) == 0x08);
		if(is_volume_label)
			continue;

		// check the 4th bit using a mask to check if dir
		is_dir = (( attr & 0x10 ) == 0x10);

		// get the starting cluster as an integer
		start_cluster = two_bytes_to_int((unsigned char *) &buf[26]);

		//  get the size of the cluster as an integer
		size = four_bytes_to_int((unsigned char *) &buf[28]);

		if(is_dir == 0 && size == 0)
			continue;

		//printf("old filename: %s\n", new_filename);
		//printf("appending:    %s\n", name);
		strcat(new_filename, prefix);
		strcat(new_filename, "\\");
		strcat(new_filename, name);
		
		printf("Filename: %s", new_filename);
		if(strcmp(ext, ""))
			printf(".%s", ext);
		printf("\n");
		
		if(is_dir)
			printf("This is a directory.\n");
		else
			printf("Size: %d\n", size);

		// print clusters
		printf("Cluster(s): ");
		print_clusters(start_cluster, filename, 0);
		printf("\n\n");

		// recurse through the dir if necessary
		//if(is_dir && strcmp(new_filename, "..") && strcmp(new_filename, "."))
		if(is_dir && i >= 2)	
			print_dir(start_cluster, filename, new_filename, file);

		memset(new_filename, '\0', 1024);		
	}
}

// reads a sector of bs.ssize into a dynamic char array
char *read_sector(int sect_num, char *filename)
{
	int sect_size = two_bytes_to_int((void *) &bs.ssize);
	char *buf = malloc(sect_size);

	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
		perror("fopen");
		exit(1);
	}

	int status = fseek(file, sect_num * sect_size, SEEK_SET);
	if(status == -1) {
		perror("fseek");
		exit(1);
	}

	status = fread((void *) buf, 1, sect_size, file);
	if(status != sect_size) {
		perror("fread");
		exit(1);
	}

	fclose(file);
	return buf;
}

/*
	The following functions are for converting the hex to int while accounting for endianness

	The three_bytes_to_int_ functions require masking for comparing their results in other functions
*/

unsigned int two_bytes_to_int(unsigned char *buf) 
{
	return (unsigned int) buf[1]<<8 | buf[0];
}

unsigned int four_bytes_to_int(unsigned char *buf)
{
	return (unsigned int) (buf[3]<<24) | (buf[2]<<16) | (buf[1]<<8) | (buf[0]);
}

unsigned int three_bytes_to_int_low(unsigned char *buf)
{
	return (unsigned int) ((buf[2]<<16) | (buf[1]<<8) | (buf[0])) & 0xFFF;
}

unsigned int three_bytes_to_int_high(unsigned char *buf)
{
	return (unsigned int) ((buf[2]<<16) | (buf[1]<<8) | (buf[0])) >> 12 & 0xFFF ;
}



















