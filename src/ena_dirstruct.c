#include "ena_dirstruct.h"

/*
 * allocates memory for a new ena_object
 * ena_object must be destroyed when no longer needed
 * Returns pointer to the object, or NULL if there is an error
*/
ena_object* init_ena_object(const char* name, bool is_dir) {
  ena_object* new_enao = (ena_object*) malloc(sizeof(ena_object));
  if (new_enao == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  new_enao->name = (char*) malloc(sizeof(char*) * (strlen(name) +1));
  if (new_enao->name == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  strcpy(new_enao->name, name);
  new_enao->is_dir = is_dir;
  kv_init(new_enao->sub_objects);
  kv_init(new_enao->studies);
  return new_enao;
}

/*
 * Recursively destroys all sub_objects and the object itself
*/
void destroy_ena_object(ena_object* old_enao) {
  while (kv_size(old_enao->sub_objects) > 0)
    destroy_ena_object(kv_pop(old_enao->sub_objects));
  kv_destroy(old_enao->sub_objects);
  kv_destroy(old_enao->studies);
  free(old_enao->name);
  free(old_enao);
  return;
}

/*
 * finds the sub_object with the right name
 * does not search recursively
 * returns pointer to the sub_object
 * returns NULL if it does not find the sub_object
*/
static ena_object* find_object_by_name(ena_object* parent_object, const char* const name) {
  int i;
  for (i=0; i<kv_size(parent_object->sub_objects); i++) {
    ena_object *test_object = kv_A(parent_object->sub_objects, i);
    if (strcmp(test_object->name, name) == 0)
      return test_object;
  }
  return NULL;
}

/*
 * searches sub_objects recursively to find the one with the right path
 * returns pointer to the sub_object
 * returns NULL if it does not find the sub_object
*/
ena_object* find_object_by_path(ena_object* parent_object, const char* const path) {
  ena_object* sub_object = NULL;
  int i;
  char* ptr = strchr(path, '/');
  size_t name_length = (ptr == NULL) ? strlen(path) : ptr - path;

  if (ptr == NULL)
    return find_object_by_name(parent_object, path);

  for (i=0; i<kv_size(parent_object->sub_objects); i++) {
    ena_object *test_object = kv_A(parent_object->sub_objects, i);
    if (strlen(test_object->name) == name_length &&
        memcmp(test_object->name, path, name_length) == 0) {
      sub_object = test_object;
      break;
    }
  }

  if (sub_object == NULL)
    return NULL;
  ptr++;
  return find_object_by_path(sub_object, ptr);
}

/*
 * returns true only if study matches one of the studies listed for the object
*/
bool check_object_matches_study(ena_object* enao, const char* study) {
  int i;
  for (i=0; i<kv_size(enao->studies); i++)
    if (strcmp( kv_A(enao->studies, i), study) == 0)
      return true;
  return false;
}


/*
 * Associate a study with the object (if it is not there already)
 * returns -1 on error, otherwise 0
*/
static int add_study(ena_object* enao, const char* study) {
  if (check_object_matches_study(enao, study))
    return 0;
  kv_push(const char*, enao->studies, study);
  if (enao->studies.a == NULL) {fputs ("Memory error\n",stderr); return -1;}
  return 0;
}


/*
 * First checks if parent_object already has a sub_object with the correct name
 * Creates a new sub_object if it is needed
 * Returns pointer to the sub_object, or NULL if there is an error
*/
static ena_object* add_sub_object(ena_object* parent_object, char* name, bool is_dir) {
  ena_object* sub_object = find_object_by_name(parent_object, name);
  if (sub_object == NULL) {
    sub_object = init_ena_object(name, is_dir);
    if (sub_object == NULL) return NULL;
    kv_push(ena_object*, parent_object->sub_objects, sub_object);
    if (parent_object->sub_objects.a == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  }
  return sub_object;
}

/*
 * Recursively finds the correct directory for storing a new object
 * Objects only created if they don't exist already
 * All directories in the path get associated with the study
 * Returns -1 error, otherwise 0
*/
int add_file_by_path(ena_object* ena_dir, char* path, const char* study) {
  char* ptr = strchr(path, '/');
  ena_object* sub_object;
  if (ptr == NULL) {
    sub_object = add_sub_object(ena_dir, path, false);
    if (sub_object == NULL) return -1;
    add_study(ena_dir, study);
    add_study(sub_object, study);
  }
  else {
    *ptr = '\0';
    ptr++;
    sub_object = add_sub_object(ena_dir, path, true);
    if (sub_object == NULL) return -1;
    add_study(ena_dir, study);
    add_file_by_path(sub_object, ptr, study);
  }
  return 0;
}
