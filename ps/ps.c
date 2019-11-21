#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/stat.h>

#define STAT_COUNT 4

int print_from_stat_file (FILE* file) {
	char curr;
	
	for (int i = 0; i < STAT_COUNT; i++) {

		while ((curr = fgetc(file)) != ' ') {
			if (curr == EOF) {
				printf("ERROR: Unexpected EOF in stat file.\n");
				return -1;
			}

			printf("%c", curr);
		}
	
		printf("\t");
	}

	printf("\n");
	return 0;
}

int main() {
	DIR* dir;
	struct dirent* dir_entr;
	char proc_path [] = "/proc/";
	size_t proc_path_len = strlen(proc_path);

	if ((dir = opendir(proc_path)) == NULL) {
		printf("ERROR: Cannot open dir %s\n", proc_path);
		return -1;
	}

	while ((dir_entr = readdir(dir)) != NULL) {
		struct stat status;
		lstat(dir_entr->d_name, &status);

		if (!S_ISDIR(status.st_mode)) {
			continue;
		}

		char* d_name = dir_entr->d_name;

		if ((!strcmp(d_name, ".")) || (!strcmp(d_name, ".."))) {
			continue;
		}

		bool is_pid = true;
		for (int i = 0; i < strlen(d_name); i++) {
			if (!isdigit(d_name[i])) {
				is_pid = false;
			}
		}
		
		if (is_pid == false) continue;

		FILE* stat_file;
		strcat(d_name, "/stat");
		strcat(proc_path, d_name);
		//printf("%s\n", d_name);
		if ((stat_file = fopen(proc_path, "r")) == NULL) {
			printf("ERROR: Cannot open %s.\n", proc_path);
			return -1;
		}

		if (print_from_stat_file(stat_file) == -1) {
			printf("ERROR: in printing %s file.\n", proc_path);
			return -1;
		}

		proc_path[proc_path_len] = '\0';
	}

	return 0;
}
