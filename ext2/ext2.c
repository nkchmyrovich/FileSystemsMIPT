#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "ext2_fs.h"

#define BLOCK_SIZE 1024
#define MAX_OFFSET 512
#define RESERVED_BLOCKS 5

void read_from_img(FILE* file_img, u_int32_t block, u_int16_t offset, u_int8_t* data, u_int16_t size) {
	if (offset >= MAX_OFFSET) {
		printf("ERROR: Offset is larger than block size.\n");
		return;
	}

	fseek(file_img, block * MAX_OFFSET + offset, SEEK_SET);
	fread(data, size, 1, file_img);
}

void list_dir(FILE* file_img, struct ext2_inode* ino) {
	u_int8_t block[BLOCK_SIZE];
	struct ext2_dir_entry* entry;
	printf("ino size: %d, blocks: %d\n", ino->i_size, ino->i_blocks);
	for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		read_from_img(file_img, 2 * ino->i_block[i], 0, block, BLOCK_SIZE);
		entry = (void*) block;
		while(entry->inode) {
			printf("%s\n", entry->name);
			entry = (void*) entry + entry->rec_len;
		}
	}

	if (ino->i_size < BLOCK_SIZE * EXT2_NDIR_BLOCKS) {
		printf("Finish\n");
		return;
	}
}

void get_inode(FILE* file_img, struct ext2_super_block* sb, struct ext2_inode* data, u_int32_t inode_num) {
	/* Find block group contains inode */
	u_int16_t block_group = (inode_num - 1) / sb->s_inodes_per_group;
	printf("block_group: %d\n", block_group);

	/* Find inode in group */
       	u_int16_t ino_ind = (inode_num - 1) % sb->s_inodes_per_group;
	printf("ino_ind: %d\n", ino_ind);	
	/* Block containing inode */
	u_int16_t containing_block = (ino_ind * sizeof(struct ext2_inode)) / BLOCK_SIZE;
	printf("containing block: %d\n", containing_block);
	u_int16_t offset = (ino_ind * sizeof(struct ext2_inode)) % MAX_OFFSET;
	u_int16_t block = 2 * RESERVED_BLOCKS + (2 * sb->s_blocks_per_group * block_group) + 2 * containing_block;
	printf("block: %d offset: %d\n", block, offset);
	read_from_img(file_img, block, offset, (u_int8_t*)data, sizeof(struct ext2_inode));
}	

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Invalid number of cmd args.\n");
		return -1;
	}

	char* path_to_img = argv[1];
	FILE* file_img = fopen(path_to_img, "r");

	if (!file_img) {
		printf("ERROR: Cannot open file %s\n", path_to_img);
		return -1;
	}
	
	/* Read superblock */
	static struct ext2_super_block sb;
	read_from_img(file_img, 2, 0, (u_int8_t*)&sb, sizeof(struct ext2_super_block));
	printf("%d\n", sb.s_log_block_size);

	/* Find root inode */
	struct ext2_inode root_ino;
	get_inode(file_img, &sb, &root_ino, EXT2_ROOT_INO);
	printf("mtime: %d\n", root_ino.i_mtime);
	list_dir(file_img, &root_ino);	
	
	
	return 0;
}
