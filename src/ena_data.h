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


/*
 * data to be stored as fuse private data
 * permissions is list of allowed users / studies / password hashes
 * studies points to strings with allocated memory (i.e. must free when destroying)
 * permissions have allocated memory (i.e. must be properly destroyed)
 * dirstruct has allocated memory (i.e. must be properly destroyed)
*/
typedef struct {
    kvec_t(ena_permission*) permissions;
    ena_object* dirstruct;
    kvec_t(char*) studies;
} ena_data;

/*
 * represents the contents of a directory
*/
typedef kvec_t(const char*) ena_dir_list;

ena_data* init_ena_data(char *rootdir);
void destroy_ena_data(ena_data* enad);
ena_dir_list* init_ena_dir_list();
void destroy_ena_dir_list(ena_dir_list* enadl);

int add_permissions_from_file(ena_data* my_ena_data, char* filename);
int add_dirstruct_from_file(ena_data* my_ena_data, char* filename);

int get_host_path(ena_data* data, const char* client_path, char* host_path);
int fill_dir_list(ena_data* data, const char* client_path, ena_dir_list *dir_list);

#endif
