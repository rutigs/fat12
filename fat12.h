#ifndef FAT12_H
#define FAT12_H

int is_last_cluster_fat12(int cluster, void *fat);
int next_cluster_fat12(int cluster, void *fat);

#endif
