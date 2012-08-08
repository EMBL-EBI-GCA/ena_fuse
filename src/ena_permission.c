#include "ena_permission.h"

/*
 * allocates memory for a new ena_permission
 * ena_object must be destroyed when no longer needed
 * Returns pointer to the object, or NULL if there is an error
*/
ena_permission* init_ena_permission(const char* user, const char* study, const char* hash) {
  ena_permission* new_enap = (ena_permission*) malloc(sizeof(ena_permission));
  if (new_enap == NULL) {fputs ("Memory error\n",stderr); return NULL;}

  new_enap->user = (char*) malloc(sizeof(char*) * (strlen(user) +1));
  if (new_enap->user == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  strcpy(new_enap->user, user);

  new_enap->study = (char*) malloc(sizeof(char*) * (strlen(study) +1));
  if (new_enap->study == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  strcpy(new_enap->study, study);

  new_enap->hash = (char*) malloc(sizeof(char*) * (strlen(hash) +1));
  if (new_enap->hash == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  strcpy(new_enap->hash, hash);

  return new_enap;
}

void destroy_ena_permission(ena_permission* perm) {
  free(perm->user);
  free(perm->study);
  free(perm->hash);
}
