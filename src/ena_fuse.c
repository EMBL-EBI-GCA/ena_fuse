
#define FUSE_USE_VERSION 26

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#include "ena_data.h"

// default file permissions, read-only for everybody
#define FILE_MODE 0444
// default folder permissions, read-only access for everybody
#define DIR_MODE 0555

volatile sig_atomic_t refresh_data = 0;

static int ena_getattr(const char *client_path, struct stat *stbuf)
{
	int res = 0;
        char host_path[PATH_MAX];
        ena_data *pd = (ena_data*) (fuse_get_context()->private_data);
	memset(stbuf, 0, sizeof(struct stat));

        if (refresh_data) {
          refresh_data = 0;
          refresh_ena_data(pd);
        }
        if (! pd->has_data)
          return -EHOSTDOWN;

        res = get_host_path(pd, client_path, host_path);
        if (res != 0)
          return -ENOENT;

        res = lstat(host_path, stbuf);
        if (res != 0)
            return -errno;
        stbuf->st_mode = (stbuf->st_mode & S_IFDIR)
                        ? S_IFDIR | DIR_MODE
                        : S_IFREG | FILE_MODE;
        return 0;
}

static int ena_access(const char *client_path, int mask)
{
        int res = 0;
        char host_path[PATH_MAX];
        ena_data *pd = (ena_data*) (fuse_get_context()->private_data);

        if (refresh_data) {
          refresh_data = 0;
          refresh_ena_data(pd);
        }
        if (! pd->has_data)
          return -EHOSTDOWN;

        res = get_host_path(pd, client_path, host_path);
        if (res != 0)
          return -ENOENT;

        res = access(host_path, mask);
        if (res == -1)
            return -errno;
        return (mask & W_OK) ? -EACCES : 0;
}

static int ena_readlink(const char *client_path, char *buf, size_t size)
{
        int res = 0;
        char host_path[PATH_MAX];
        ena_data *pd = (ena_data*) (fuse_get_context()->private_data);
        if (refresh_data) {
          refresh_data = 0;
          refresh_ena_data(pd);
        }
        if (! pd->has_data)
          return -EHOSTDOWN;

        res = get_host_path(pd, client_path, host_path);
        if (res != 0)
          return -ENOENT;
        res = readlink(host_path, buf, size - 1);
        if (res == -1)
            return -errno;
        return 0;
}

static int ena_open(const char *client_path, struct fuse_file_info *fi)
{
        int fd;
        char host_path[PATH_MAX];
        ena_data *pd = (ena_data*) (fuse_get_context()->private_data);
        if (refresh_data) {
          refresh_data = 0;
          refresh_ena_data(pd);
        }
        if (! pd->has_data)
          return -EHOSTDOWN;

        if (get_host_path(pd, client_path, host_path) != 0)
          return -ENOENT;
        if ((fi->flags & 3) != O_RDONLY)
          return -EACCES;

        fd = open(host_path, fi->flags);
        if (fd < 0)
            return -errno;
        fi->fh = fd;
        return 0;
}

static int ena_read(const char *path, char *buf, size_t size, off_t offset,
		    struct fuse_file_info *fi)
{
        int ret = 0;
        if ((ret = pread(fi->fh, buf, size, offset)) < 0)
            return -errno;
        return ret;
}

static int ena_release(const char *path, struct fuse_file_info *fi)
{
    if (close(fi->fh) < 0)
        return -errno;
    return 0;
}

static int ena_opendir(const char *client_path, struct fuse_file_info *fi)
{
    ena_data *pd = (ena_data*) (fuse_get_context()->private_data);
    if (refresh_data) {
      refresh_data = 0;
      refresh_ena_data(pd);
    }
    if (! pd->has_data)
      return -EHOSTDOWN;

    ena_dir_list *dir_list = init_ena_dir_list(pd->refresh_num);
    switch (fill_dir_list(pd, client_path, dir_list)) {
        case -2:
          destroy_ena_dir_list(dir_list);
          return -ENOMEM;
        case -1:
          destroy_ena_dir_list(dir_list);
          return -ENOENT;
        default:
          break;
    }
    fi->fh = (intptr_t) dir_list;
    return 0;
}

static int ena_releasedir(const char *path, struct fuse_file_info *fi)
{
    ena_dir_list *dir_list = (ena_dir_list*) (fi->fh);
    if (dir_list != NULL)
      destroy_ena_dir_list(dir_list);
    return 0;
}



static int ena_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		       off_t offset, struct fuse_file_info *fi)
{
    int i;
    ena_dir_list *dir_list = (ena_dir_list*) (fi->fh);
    ena_data *pd = (ena_data*) (fuse_get_context()->private_data);
    if (dir_list == NULL)
      return -ENOENT;
    if (dir_list->refresh_num != pd->refresh_num)
      return  -ESTALE;
    if (filler(buf, ".", NULL, 0) != 0 || filler(buf, "..", NULL, 0) != 0)
      return -ENOMEM;
    for (i=0; i<kv_size(dir_list->contents); i++)
      if (filler(buf, kv_A(dir_list->contents, i), NULL, 0) != 0)
        return -ENOMEM;
    return 0;
}



static struct fuse_operations ena_oper = {
	.getattr	= ena_getattr,
	.access		= ena_access,
	.readlink	= ena_readlink,
	.open		= ena_open,
	.read		= ena_read,
	.release	= ena_release,
	.opendir	= ena_opendir,
	.releasedir	= ena_releasedir,
	.readdir	= ena_readdir,
};

void usage()
{
    fprintf(stderr, "usage:  ena_fuse mountPoint rootDir permissionsFile pathsFile\n");
    exit(1);
}

void catch_sigusr1(int sig) {
  refresh_data = 1;
  signal(sig, catch_sigusr1);
}

int main(int argc, char *argv[])
{
    int i;
    int fuse_stat;
    char *permissions_file, *paths_file, *root_dir;
    ena_data *my_ena_data;

    for (i = 1; (i < argc) && (argv[i][0] == '-'); i++)
	if (argv[i][1] == 'o') i++;
        
    if ((argc - i) != 4)
        usage();

    root_dir = realpath(argv[i+1], NULL);
    permissions_file = realpath(argv[i+2], NULL);
    paths_file = realpath(argv[i+3], NULL);
    argc -= 3;

    my_ena_data = init_ena_data(root_dir, permissions_file, paths_file);
    if (my_ena_data == NULL)
      return EXIT_FAILURE;
    if (add_permissions_from_file(my_ena_data) != 0)
      return EXIT_FAILURE;
    if (add_dirstruct_from_file(my_ena_data) != 0)
      return EXIT_FAILURE;
    my_ena_data->has_data = true;

    signal(SIGUSR1, catch_sigusr1);

    fuse_stat = fuse_main(argc, argv, &ena_oper, my_ena_data);
    
    destroy_ena_data(my_ena_data);

    free(root_dir);
    free(permissions_file);
    free(paths_file);
    return fuse_stat;
}

