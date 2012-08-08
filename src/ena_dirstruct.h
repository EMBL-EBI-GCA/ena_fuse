#ifndef ENA_DIRSTRUCT
#define ENA_DIRSTRUCT
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "kvec.h"

/*
 * struct representing a file or a directory.  
 * studies contains pointers to strings stored somewhere else in memory (i.e. don't need to free)
 * name has memory allocated (i.e. must free)
*/
typedef struct ena_object ena_object;
struct ena_object {
  char *name;
  bool is_dir;
  kvec_t(ena_object*) sub_objects;
  kvec_t(const char*) studies;
};

ena_object* init_ena_object(const char* name, bool is_dir);
void destroy_ena_object(ena_object* old_enao);
int add_file_by_path(ena_object* ena_dir, char* path, const char* study);
ena_object* find_object_by_path(ena_object* parent_object, const char* const path);
bool check_object_matches_study(ena_object* enao, const char* study);

#endif
