#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

char *cat(char *path, const char *name)
{
	int len = strlen(path) + strlen(name) + 1;
	char *buf = calloc(len + 1, 1);

	assert(sprintf(buf, "%s/%s", path, name) == len);
	return buf;
}

void go_go(char *root_path, char *path, int fd)
{
	DIR *dir;

	dir = opendir(path);
	const struct dirent *curr;

	while (curr = readdir(dir)) {
		if (curr->d_type != 4) {
			//printf("\n%s\n", curr->d_name);
			char *path_new = cat(path, curr->d_name);

			int len = strlen(path_new) - strlen(root_path);

			assert(write(fd, &len, sizeof(int)) != -1);
			assert(write(fd, path_new + strlen(root_path),
				len) != -1);
			int in_fd = open(path_new, O_RDONLY);
			int size_f = lseek(in_fd, 0, SEEK_END);

			assert(write(fd, &size_f, sizeof(int)) != -1);
			lseek(in_fd, 0, SEEK_SET);
			void *buf = malloc(size_f);

			read(in_fd, buf, size_f);
			assert(write(fd, buf, size_f) != -1);
			free(buf);
			free(path_new);
			close(in_fd);
		} else {
			if (strcmp(curr->d_name, ".") == 0 ||
				strcmp(curr->d_name, "..") == 0)
				continue;
			char *path_new = cat(path, curr->d_name);

			go_go(root_path, path_new, fd);
			free(path_new);
		}
	}
	closedir(dir);
}

void make_dirs(char *path, mode_t mode)
{
	size_t len = strlen(path);
	char *path_temp = strdup(path);

	// path not may be less than 1 char
	size_t offset = 1;

	// check such paths as ./a/b/c/d
	if (strncmp(path_temp, "./", 2) == 0)
		offset = 2;

	for (size_t i = offset; i < len; i++) {
		if (path_temp[i] == '/') {
			path_temp[i] = 0;
			mkdir(path_temp, mode);
			path_temp[i] = '/';
		}
	}
	free(path_temp);
}

int create_path(char *path, mode_t mode)
{
	make_dirs(path, mode);
	return creat(path, mode);
}

void look_around(char *path, int fd)
{
	int len;

	while (read(fd, &len, sizeof(int)) == sizeof(int)) {
		char *path_curr = calloc(len + 1, 1);

		read(fd, path_curr, len);
		int len_data;

		read(fd, &len_data, sizeof(int));
		void *data = malloc(len_data);

		read(fd, data, len_data);
		char *path_full = cat(path, path_curr);

		free(path_curr);
		int fd_out = create_path(path_full, 0700);

		write(fd_out, data, len_data);
		close(fd_out);
		free(path_full);
		free(data);
	}
}

int main(int argc, char *argv[])
{
	//dir pack output
	//input unpack dir
	//puts(argv[1]);
	if (argc != 4) {
		puts("I need to know the path before start!");
		return 0;
	}
	if (strcmp(argv[2], "pack") == 0) {
		int fd = creat(argv[3], 0700);

		go_go(argv[1], argv[1], fd);
		close(fd);
	} else if (strcmp(argv[2], "unpack") == 0) {
		char *path_bin = argv[1];
		int fd = open(path_bin, O_RDONLY);

		look_around(argv[3], fd);
		close(fd);
	}
	return 0;
}
