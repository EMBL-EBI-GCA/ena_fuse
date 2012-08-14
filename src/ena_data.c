#include "ena_data.h"

static int _read_to_buffer(char** buffer, const char* filename, long* file_size)
{
  FILE *fh;
  size_t result;

  fh = fopen(filename, "r");
  if (fh==NULL) {fputs("File error\n",stderr); return -1;}

  fseek(fh, 0, SEEK_END);
  *file_size = ftell(fh);
  *buffer = (char*) malloc((*file_size+1) * sizeof(char));
  if (*buffer == NULL) {fputs ("Memory error\n",stderr); return -1;}

  rewind(fh);
  result = fread(*buffer, 1, *file_size, fh);
  if ((long) result != *file_size) {fputs ("Reading error\n",stderr); return -1;}
  (*buffer)[*file_size] = '\0';

  fclose(fh);
  return 0;
}

ena_data* init_ena_data(const char *rootdir, const char* permissions_file, const char* paths_file) {
  ena_data* new_enad;
  ena_object *dirstruct = init_ena_object(rootdir, true);
  if (dirstruct == NULL) {return NULL;}
  new_enad = (ena_data*) malloc(sizeof(ena_data));
  if (new_enad == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  new_enad->dirstruct = dirstruct;
  kv_init(new_enad->permissions);
  kv_init(new_enad->studies);
  new_enad->refresh_num = 0;
  new_enad->permissions_file = permissions_file;
  new_enad->paths_file = paths_file;
  new_enad->has_data = false;
  new_enad->rootdir = rootdir;
  return new_enad;
}

void destroy_ena_data(ena_data* enad) {
  while (kv_size(enad->permissions) > 0)
    destroy_ena_permission(kv_pop(enad->permissions));
  while (kv_size(enad->studies) > 0)
    free(kv_pop(enad->studies));
  kv_destroy(enad->permissions);
  kv_destroy(enad->studies);
  destroy_ena_object(enad->dirstruct);
  free(enad);
  return;
}

ena_dir_list* init_ena_dir_list(const int refresh_num) {
  ena_dir_list* new_enadl = (ena_dir_list*) malloc(sizeof(ena_dir_list));
  if (new_enadl == NULL) {fputs ("Memory error\n",stderr); return NULL;}
  kv_init(new_enadl->contents);
  new_enadl->refresh_num = refresh_num;
  return new_enadl;
}
void destroy_ena_dir_list(ena_dir_list* enadl) {
  kv_destroy(enadl->contents);
  free(enadl);
  return;
}


/*
 * file contains three columns, separated by space / tab / comma
 * first column is user
 * second column is study
 * third column is a hash of user study and password
 * e.g: streeter ERS000001 streeterERS000001password
 * returns 0 if file is read successfully and dirstruct is created
*/
int add_permissions_from_file(ena_data* my_ena_data)
{
  long file_size;
  char *buffer = NULL;
  char *ptr, *line_end;
  int res;

  res = _read_to_buffer(&buffer, my_ena_data->permissions_file, &file_size);
  if (res != 0) return res;

  ptr = buffer;
  while ((line_end = strchr(ptr, '\n')) != NULL) {
    char *user, *study, *hash;
    ena_permission *enap;
    *line_end = '\0';
    user = strtok(ptr, " ,\t");
    study = strtok(NULL, " ,\t");
    hash = strtok(NULL, " ,\t");
    ptr = line_end + 1;
    enap = init_ena_permission(user, study, hash);
    if (enap == NULL) {free(buffer); return -1;}
    kv_push(ena_permission*, my_ena_data->permissions, enap);
    if (my_ena_data->permissions.a == NULL) {free(buffer); fputs ("Memory error\n",stderr); return -1;}
  }
  free(buffer);

  return 0;
}


/*
 * file contains two columns, separated by space / tab / comma
 * first column is study, second column is path to file within the root directory
 * e.g: ERS000001 subdir/file1.fastq.gz
 * returns 0 if file is read successfully and dirstruct is created
*/
int add_dirstruct_from_file(ena_data* my_ena_data) {
  long file_size;
  char *line_start, *line_end;
  char *buffer = NULL;
  int res;

  res = _read_to_buffer(&buffer, my_ena_data->paths_file, &file_size);
  if (res != 0) return res;

  line_start = buffer;
  while ((line_end = strchr(line_start, '\n')) != NULL) {
    char *study, *filepath;
    char *data_study = NULL;
    int i;

    *line_end = '\0';
    study = strtok(line_start, " ,\t");
    filepath = strtok(NULL, " ,\t");
    line_start = line_end + 1;

    for (i=0; i<kv_size(my_ena_data->studies); i++)
      if (strcmp(kv_A(my_ena_data->studies, i), study) == 0) {
        data_study = kv_A(my_ena_data->studies, i);
        break;
      }
    if (data_study == NULL) {
      data_study = (char*) malloc(sizeof(char*) * (strlen(study) +1));
      if (data_study==NULL) {fputs("Memory error\n",stderr); return -1;}
      strcpy(data_study, study);
      kv_push(char*, my_ena_data->studies, data_study);
      if (my_ena_data->studies.a == NULL) {free(buffer); fputs ("Memory error\n",stderr); return -1;}
    }

    res = add_file_by_path(my_ena_data->dirstruct, filepath, data_study);
    if (res != 0) return res;
  }

  free(buffer);
  return 0;
}

/*
 * on success, .has_data is set to true and return value is 0
 * on failure, .has_data is set to false and return value is -1
*/
int refresh_ena_data(ena_data* enad) {
  enad->has_data = false;
  enad->refresh_num ++;
  while (kv_size(enad->permissions) > 0)
    destroy_ena_permission(kv_pop(enad->permissions));
  while (kv_size(enad->studies) > 0)
    free(kv_pop(enad->studies));
  destroy_ena_object(enad->dirstruct);
  
  ena_object *dirstruct = init_ena_object(enad->rootdir, true);
  if (dirstruct == NULL) {return -1;}
  enad->dirstruct = dirstruct;

  if (add_permissions_from_file(enad) != 0)
    return -1;
  if (add_dirstruct_from_file(enad) != 0)
    return -1;
  enad->has_data = true;
  return 0;
}


/*
 * e.g. when parsing /user/study/hash/part1/part2/part3
 * study will contain the study
 * filepath will contain part1/part2/part3
 * point to strings stored somewhere else in memory (i.e. no need to free)
*/
struct parsed_path {
  const char* study;
  const char* filepath;
};

/*
 * client_path is the path typed in by the user
 * client_path is parsed and results put in pp
 * return value is the number of parts found in client_path
 *     e.g. parsing /user returns 1
 *     e.g. parsing /user/study/hash returns 3
 *     e.g. parsing /user/study/hash/filepath returns 4
 * returns -1 if the permission hash in client_path is invalid
 */
static int parse_client_path (const char *client_path, ena_data* my_ena_data, struct parsed_path *pp) {
  const char *ptr[4];
  size_t lengths[3];
  int i;
  int ret;
  ptr[0] = client_path;
  for (i=0; i<3; i++) {
    if (*ptr[i] == '/')
      ptr[i] ++;
    if (*ptr[i] == '\0') {
      return i;
    }
    lengths[i] = strcspn(ptr[i], "/\0");
    ptr[i+1] = ptr[i] + lengths[i];
  }

  pp->filepath = ptr[3];
  if (*pp->filepath == '/')
    pp->filepath ++;
  ret = (*pp->filepath == '\0') ? 3 : 4;


  for (i=0; i < kv_size(my_ena_data->permissions); i++) {
    ena_permission *perm = kv_A(my_ena_data->permissions, i);
    if (memcmp(ptr[0], perm->user, lengths[0]) == 0
        && memcmp(ptr[1], perm->study, lengths[1]) == 0
        && memcmp(ptr[2], perm->hash, lengths[2]) == 0) {
      pp->study = perm->study;
      return ret;
    }
  }
  return -1;
}


/*
 * client_path is the path typed in by the user
 * host_path should be an empty string of length PATH_MAX
 * host_path will be filled with the path of the file / directory on the server
 * returns -1 if the permission hash in client_path is invalid
 * returns -1 if the study in client_path does not match the study of the file
 * returns -1 if the file does not exist in the dirstruct object
 * otherwise returns 0 (success)
 */
int get_host_path(ena_data* my_ena_data, const char* client_path, char* host_path) {
  ena_object* enao;
  struct parsed_path pp;
  switch(parse_client_path(client_path, my_ena_data, &pp))
  {
    case -1:
      return -1;
    case 0:
    case 1:
    case 2:
    case 3:
      strcpy(host_path, my_ena_data->dirstruct->name);
      return 0;
    case 4:
      enao = find_object_by_path(my_ena_data->dirstruct, pp.filepath);
      if (enao == NULL)
        return -1;
      if (check_object_matches_study(enao, pp.study) == false)
        return -1;
  }
  sprintf(host_path, "%s/%s", my_ena_data->dirstruct->name, pp.filepath);
  return 0;
}

/*
 * client_path is the path typed in by the user
 * dir_list should be initialised and empty
 * file names are added to dir_list only if they exist in the dirstruct object AND they can be found using stat
 * returns -1 if the permission hash in client_path is invalid
 * returns -1 if the study in client_path does not match the study of the directory
 * returns -1 if the directory does not exist in the dirstruct object
 * returns -2 if there is a memory error
 * otherwise returns 0 (success)
 */
int fill_dir_list(ena_data* my_ena_data, const char* client_path, ena_dir_list *dir_list) {
  ena_object* directory;
  int i,j;
  struct parsed_path pp;
  struct stat buf;
  char host_path[PATH_MAX];
  switch(parse_client_path(client_path, my_ena_data, &pp))
  {
    case -1:
      return -1;
    case 0:
    case 1:
    case 2:
      return 0;
    case 3:
      directory = my_ena_data->dirstruct;
      break;
    case 4:
      directory = find_object_by_path(my_ena_data->dirstruct, pp.filepath);
      if (directory == NULL)
        return -1;
      if (check_object_matches_study(directory, pp.study) == false)
        return -1;
      break;
  }
  for (i=0, j=0; i < kv_size(directory->sub_objects); i++) {
    ena_object* sub_object = kv_A(directory->sub_objects, i);
    if (check_object_matches_study(sub_object, pp.study) == true) {
      sprintf(host_path, "%s/%s/%s", my_ena_data->dirstruct->name, pp.filepath, sub_object->name);
      if (stat(host_path, &buf) == 0) {
          kv_a(const char*, dir_list->contents, j) = sub_object->name;
          if (dir_list->contents.a == NULL) {fputs ("Memory error\n",stderr); return -2;}
          j++;
      }
    }
  }
  return 0;
}













