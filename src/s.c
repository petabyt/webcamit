// Scan for v4l2 ports
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
	const char *dev_file = "/dev/video6";
	int proc_count = 0;
	DIR *proc_dir = opendir("/proc");
	if (!proc_dir)
		return 1;

	struct dirent *entry;
	while ((entry = readdir(proc_dir))) {
		if (!isdigit(entry->d_name[0]))
			continue;

		char fd_dir[256];
		snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", entry->d_name);

		DIR *fd_dir_ptr = opendir(fd_dir);
		if (!fd_dir_ptr)
			continue;

		struct dirent *fd_entry;
		while ((fd_entry = readdir(fd_dir_ptr))) {
			char fd_path[256], link_path[256];
			ssize_t link_len;

			snprintf(fd_path, sizeof(fd_path), "%s/%s", fd_dir, fd_entry->d_name);
			link_len = readlink(fd_path, link_path, sizeof(link_path) - 1);

			if (link_len > 0) {
				link_path[link_len] = '\0';
				if (strcmp(link_path, dev_file) == 0) {
					proc_count++;
					break;
				}
			}
		}
		closedir(fd_dir_ptr);
	}

	closedir(proc_dir);
	printf("%d\n", proc_count);
	return 0;
}
