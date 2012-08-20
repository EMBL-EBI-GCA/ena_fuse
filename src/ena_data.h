#ifndef ENA_DATA
#define ENA_DATA
#include "ena_dirstruct.h"
#include "ena_permission.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include "kvec.h"
#include <sys/stat.h>
#include <unistd.h>


/*
 * data to be stored as fuse private data
 * permissions is list of allowed users / studies / password hashes
 * studies points to strings with allocated memory (i.e. must free when destroying)
 * permissions have allocated memory (i.e. must be properly destroyed)
 * dirstruct has allocated memory (i.e. must be properly destroyed)
 * refresh_num keeps track of how many times data has been refreshed
 * has_data is true only if permissions, dirstruct and studies are properly set
*/
typedef struct {
    kvec_t(ena_permission*) permissions;
    ena_object* dirstruct;
    kvec_t(char*) studies;
    int refresh_num;
    bool has_data;
    const char* permissions_file;
    const char* paths_file;
    const char* rootdir;
} ena_data;

/*
 * represents the contents of a directory
 * contents points to strings with the directory contents
 * pointers are only valid if refresh_num matches ena_data.refresh_num
*/
typedef struct {
    kvec_t(const char*) contents;
    int refresh_num;
} ena_dir_list;

ena_data* init_ena_data(const char *rootdir, const char* permissions_file, const char* paths_file);
void destroy_ena_data(ena_data* enad);
ena_dir_list* init_ena_dir_list();
void destroy_ena_dir_list(ena_dir_list* enadl);

int add_permissions_from_file(ena_data* my_ena_data);
int add_dirstruct_from_file(ena_data* my_ena_data);
int refresh_ena_data(ena_data* my_ena_data);

int get_host_path(ena_data* data, const char* client_path, char* host_path);
int fill_dir_list(ena_data* data, const char* client_path, ena_dir_list *dir_list);

#endif
