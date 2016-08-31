#include <stdio.h>
#include "fat12.h"

int is_last_cluster_fat12(int cluster, void *fat)
{
	int temp = cluster / 2;
	if(cluster % 2)
		return three_bytes_to_int_high(fat + temp*3) >= 0xFEF;
	return three_bytes_to_int_low(fat + temp*3) >= 0xFEF;
}

int next_cluster_fat12(int cluster, void *fat)
{
	int temp = cluster / 2;
	if(cluster % 2)
		return three_bytes_to_int_high(fat + temp*3);
	return three_bytes_to_int_low(fat + temp*3);
}