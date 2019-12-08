#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include "ext2_fs.h"

#define BLOCK_SIZE 1024
#define MAX_OFFSET 512
#define RESERVED_BLOCKS 5
#define MODE_MASK 0xF000
#define EXT2_ADDR_PER_BLOCK_ 255
#define EXT2_S_IFDIR 0x4000
#define EXT2_S_IFREG 0x8000

void read_from_img(FILE* file_img, u_int32_t block, u_int16_t offset, u_int8_t* data, u_int16_t size) {
	if (offset >= MAX_OFFSET) {
		printf("ERROR: Offset is larger than block size.\n");
		return;
	}

	fseek(file_img, block * MAX_OFFSET + offset, SEEK_SET);
	fread(data, size, 1, file_img);
}

void print_inode(char* name, u_int16_t name_len) {
	u_int8_t spaces = 30;
	u_int8_t i;

	for (i = 0; i < name_len; i++, spaces--) {
		printf("%c", name[i]);
	}

	while(spaces--) {
		printf(" ");
	}
}

void list_dir(FILE* file_img, struct ext2_inode* ino) {
	u_int8_t block[BLOCK_SIZE];
	struct ext2_dir_entry* entry;
	struct ext2_inode entry_ino;
	printf("ino size: %d, blocks: %d\n", ino->i_size, ino->i_blocks);
	for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		read_from_img(file_img, 2 * ino->i_block[i], 0, block, BLOCK_SIZE);
		entry = (void*) block;
		while(entry->inode) {
			int j = 0;
			print_inode(entry->name, entry->name_len);
			printf("\n");
			entry = (void*) entry + entry->rec_len;
		}	
	}

	if (ino->i_size < BLOCK_SIZE * EXT2_NDIR_BLOCKS) {
		printf("Finish\n");
		return;
	}
}

void cat_file(FILE* file, struct ext2_inode* ino) {
	u_int8_t block[BLOCK_SIZE];
       	int curr_block = 0;
	
	/* Check direct blocks first */
	for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
		read_from_img(file, 2 * ino->i_block[i], 0, block, BLOCK_SIZE);
		//printf("%s", (char*)block);
	}

	if (ino->i_size < BLOCK_SIZE * EXT2_NDIR_BLOCKS) {
		return;
	}

	/* Check singly indirect blocks */
	u_int32_t* indir_blocks;
	read_from_img(file, 2 * ino->i_block[EXT2_IND_BLOCK], 0, (u_int8_t*)indir_blocks, sizeof(u_int8_t));
	
	for (int i = 0; i < EXT2_ADDR_PER_BLOCK_; i++) {
		read_from_img(file, 2 * indir_blocks[i], 0, block, BLOCK_SIZE);
		printf("%s", (char*)block);
	}

	if (ino->i_size < BLOCK_SIZE * EXT2_NDIR_BLOCKS + EXT2_ADDR_PER_BLOCK_ * BLOCK_SIZE) {
                return;
        }

	/* Check doubly indirect blocks */
	u_int32_t* d_indir_blocks;
	
        read_from_img(file, 2 * ino->i_block[EXT2_DIND_BLOCK], 0, (u_int8_t*)d_indir_blocks, sizeof(u_int8_t));
	
	for (int i = 0; i < EXT2_ADDR_PER_BLOCK_; i++) {
                read_from_img(file, 2 * d_indir_blocks[i], 0, (u_int8_t*)indir_blocks, BLOCK_SIZE);
                for (int j = 0; j < EXT2_ADDR_PER_BLOCK_; j++) {
			read_from_img(file, 2 * indir_blocks[j], 0, block, BLOCK_SIZE);
                	printf("%s", (char*)block);
		}
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

void get_ino_by_path(FILE* file_img, char* path, struct ext2_inode* root_ino, struct ext2_super_block* sb, struct ext2_inode* ino) {
	u_int8_t block[BLOCK_SIZE];
        struct ext2_dir_entry* entry;
	char* delimiter = "/";
	char* token = strtok(path, delimiter);
	struct ext2_inode curr_ino = *root_ino;
	struct ext2_inode found_ino;
	bool found = false;
	u_int32_t curr_ino_num = 0;

	if (!strcmp(path, "/")) {
		get_inode(file_img, sb, ino, 2);
		return;
	}

	while (token != NULL) {
		for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
			read_from_img(file_img, 2 * curr_ino.i_block[i], 0, block, BLOCK_SIZE);
                	entry = (void*) block;
                	while (entry->inode) {
				printf("ent_name: %s, tok: %s\n", entry->name, token);
                        	if (!strcmp(token, entry->name)) {
					get_inode(file_img, sb, &found_ino, entry->inode);
					found = true;
					curr_ino_num = entry->inode;
					break;
				}
                        	entry = (void*)entry + entry->rec_len;
                	}

			if (found)
				break;
        	}

		if (!found) {
			printf("Path %s not found.\n", path);
			return;
		}

		curr_ino = found_ino;
		token = strtok(NULL, path);
	}

	get_inode(file_img, sb, ino, curr_ino_num);
}	




int main(int argc, char** argv) {
	if (argc < 2) {
		printf("Invalid number of cmd args.\n");
		return -1;
	}

	char* path_to_img = argv[1];
	char* cmd = argv[2];
	char* path_to_obj = argv[3];
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
	list_dir(file_img, &root_ino);	
	if (!strcmp(cmd, "ls")) {
		struct ext2_inode ino;
		get_ino_by_path(file_img, path_to_obj, &root_ino, &sb, &ino);
		
		/* if ((ino.i_mode & 0xF000) != EXT2_S_IFDIR) {
                        printf("The given path does not belong to a directory.\n");
                        return -1;
                } */

		list_dir(file_img, &ino);

	}
	
	if (!strcmp(cmd, "cat")) {
		struct ext2_inode ino;
                get_ino_by_path(file_img, path_to_obj, &root_ino, &sb, &ino);
		
		/* if ((ino.i_mode & 0xF000)  != EXT2_S_IFREG) {
			printf("The given path does not belong to a regular file.\n");
			return -1;
		} */
		
		cat_file(file_img, &ino);
	}

	fclose(file_img);
	return 0;
}
