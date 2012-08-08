#ifndef ENA_PERMISSION
#define ENA_PERMISSION
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "kvec.h"

/*
 * struct representing valid entry for the first three 'directories' in a path
 * name, study and hash have memory allocated (i.e. must free after use)
*/
typedef struct {
    char *user;
    char *study;
    char *hash;
} ena_permission;

ena_permission* init_ena_permission(const char* user, const char* study, const char* hash);
void destroy_ena_permission(ena_permission* perm);

#endif
