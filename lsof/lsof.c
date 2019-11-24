#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <ctype.h>

int lsof(char* pid) {
	printf("PID: %s\n", pid);
	static char proc_path[6];
	sprintf(proc_path, "%s", "/proc/");
	static char* fd_path = "/fd/";
	char* tmp_path = strcat(proc_path, pid);
	char* path = strcat(tmp_path, fd_path);
	size_t path_len = strlen(path);
	DIR* dir = opendir(path);

	if (dir == NULL) {
		printf("ERROR: Cannot open %s\n", path);
		return -1;
	}

	struct dirent* dir_entr;
	printf("Opened files: ");
	while ((dir_entr = readdir(dir))) {
		char* d_name = dir_entr->d_name;
		
		if ((!strcmp(d_name, ".")) || (!strcmp(d_name, ".."))) {
			continue;
		}
		
		strcat(path, d_name);	
		char buf [1024];
		ssize_t len;
		
		if ((len = readlink(path, buf, sizeof(buf)) - 1) != -1) {
			buf[len] = '\0';
		} else {
			printf("ERROR: Cannot readlink() fd.\n");
			return -1;
		}

		printf("%s ", buf);
		path[path_len] = '\0';
	}

	closedir(dir);
	return 0;
}


int main(int argc, char** argv) {
	long int pid = 0;
	char* endptr;

	if (argc == 2) {
		pid = strtol(argv[1], &endptr, 10);
	}

	if (pid) {
		char pid_str [8];
		sprintf(pid_str, "%d", pid);

		if (lsof(pid_str) == -1) {
			printf("ERROR: Cannot complete lsof.\n");
			return -1;
		}

	} else {
		DIR* dir = opendir("/proc");
		
		if (dir == NULL) {
			printf("Cannot open /proc.\n");
			return -1;
		}

		struct dirent* dir_entr;
		while((dir_entr = readdir(dir))) {
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
					break;
				}
			}
			
			if (!is_pid) continue;

			printf("name: %s\n.", dir_entr->d_name);

			if (lsof(d_name) == -1) {
				printf("ERROR: Cannot complete lsof.\n");
				return -1;
			}
		}

		closedir(dir);
	}

	return 0;
}

