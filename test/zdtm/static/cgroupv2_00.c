#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "zdtmtst.h"

const char *test_doc = "Check that some cgroup-v2 properties in kernel controllers are preserved";
const char *test_author = "Bui Quang Minh <minhquangbui99@gmail.com>";

char *dirname;
TEST_OPTION(dirname, string, "cgroup-v2 directory name", 1);
const char *cgname = "subcg";

int write_value(const char *path, const char *value)
{
	int fd, l;

	fd = open(path, O_WRONLY);
	if (fd < 0) {
		pr_perror("open %s", path);
		return -1;
	}

	l = write(fd, value, strlen(value));
	if (l < 0) {
		pr_perror("failed to write %s to %s", value, path);
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int read_value(const char *path, char *value, int size)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		pr_perror("open %s", path);
		return -1;
	}

	ret = read(fd, (void *)value, size);
	if (ret < 0) {
		pr_perror("read %s", path);
		close(fd);
		return -1;
	}

	value[ret] = '\0';
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	char path[1024], aux[1024];
	int ret = -1;

	test_init(argc, argv);

	if (mkdir(dirname, 0700) < 0 && errno != EEXIST) {
		pr_perror("Can't make dir");
		return -1;
	}

	if (mount("cgroup2", dirname, "cgroup2", 0, NULL)) {
		pr_perror("Can't mount cgroup-v2");
		return -1;
	}

	sprintf(path, "%s/%s", dirname, cgname);
	if (mkdir(path, 0700) < 0 && errno != EEXIST) {
		pr_perror("Can't make dir");
		goto out;
	}

	/* Make cpuset controllers available in children directory */
	sprintf(path, "%s/%s", dirname, "cgroup.subtree_control");
	sprintf(aux, "%s", "+cpuset");
	if (write_value(path, aux))
		goto out;

	sprintf(path, "%s/%s/%s", dirname, cgname, "cgroup.subtree_control");
	sprintf(aux, "%s", "+cpuset");
	if (write_value(path, aux))
		goto out;

	sprintf(path, "%s/%s/%s", dirname, cgname, "cgroup.type");
	sprintf(aux, "%s", "threaded");
	if (write_value(path, aux))
		goto out;

	sprintf(path, "%s/%s/%s", dirname, cgname, "cgroup.procs");
	sprintf(aux, "%d", getpid());
	if (write_value(path, aux))
		goto out;

	test_daemon();
	test_waitsig();

	sprintf(path, "%s/%s/%s", dirname, cgname, "cgroup.subtree_control");
	if (read_value(path, aux, sizeof(aux)))
		goto out;

	if (strcmp(aux, "cpuset\n")) {
		fail("cgroup.subtree_control mismatches");
		goto out;
	}

	sprintf(path, "%s/%s/%s", dirname, cgname, "cgroup.type");
	if (read_value(path, aux, sizeof(aux)))
		goto out;

	if (strcmp(aux, "threaded\n")) {
		fail("cgroup.type mismatches");
		goto out;
	}

	pass();

	ret = 0;

out:
	sprintf(path, "%s", dirname);
	umount(path);
	return ret;
}
