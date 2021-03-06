/*
    Source of directory traversal: https://stackoverflow.com/questions/29401653/printing-all-files-and-folders-in-a-path-in-linux-recursively-in-c
*/

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <limits.h>
// #include <ctime.h>

typedef void *hashtable;
// typedef dirs dirs;
// typedef files files;

typedef struct {
    char last_change[20];
    unsigned long size;
    char name[256];
    char path[1024];
    int checked;
} file_data;

typedef struct {
  char *item[1024];
  int checked;
} item;

typedef struct {
  item *path[100000];
  int num;
} paths;

typedef struct {
    char *key;
    file_data *udata;
} hash_bucket;

typedef struct {
    unsigned long bits, size, mask;
    unsigned long num_entries, prevval;
    hash_bucket *table;
} hash_table_int;

hashtable
hash_init(int bits)
{
  hash_table_int *ht;

  ht = (hash_table_int *)calloc(1, sizeof(hash_table_int));
  if (ht == NULL)
    return(NULL);
  ht->bits = bits;
  ht->size = (1L << bits);
  ht->mask = ht->size - 1;
  ht->num_entries = 0;
  ht->prevval = 0xdeadbeef; /* an arbitrary initial value :) */
  ht->table = (hash_bucket *)calloc(ht->size, sizeof(hash_bucket));
  if (ht->table == NULL)
   return(NULL);
  return(ht);
}

#define MIX(a, b, c)\
{\
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12); \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b<<5); \
  a -= b; a -= c; a ^= (c>>3); \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

static unsigned long
hash(hash_table_int *ht, const char *p)
{
  unsigned long a, b, c, len, length;

  length = len = strlen(p);
  a = b = 0x9e3779b9;        /* golden ratio suggested by Jenkins */
  c = ht->prevval;           /* previous value */
  while (len >= 12)
  {
    a += (p[0] + (((unsigned long)p[1])<<8) + (((unsigned long)p[2])<<16) +
	  (((unsigned long)p[3])<<24));
    b += (p[4] + (((unsigned long)p[5])<<8) + (((unsigned long)p[6])<<16) +
	  (((unsigned long)p[7])<<24));
    c += (p[8] + (((unsigned long)p[9])<<8) + (((unsigned long)p[10])<<16) +
	  (((unsigned long)p[11])<<24));
    MIX(a, b, c);
    p += 12;
    len -= 12;
  }
  c += length;
  switch(len) {
  case 11:
    c+=((unsigned int)p[10]<<24);
  case 10:
    c+=((unsigned int)p[9]<<16);
  case 9:
    c+=((unsigned int)p[8]<<8);
  case 8:
    b+=((unsigned int)p[7]<<24);
  case 7:
    b+=((unsigned int)p[6]<<16);
  case 6:
    b+=((unsigned int)p[5]<<8);
  case 5:
    b+=((unsigned int)p[4]);
  case 4:
    a+=((unsigned int)p[3]<<24);
  case 3:
    a+=((unsigned int)p[2]<<16);
  case 2:
    a+=((unsigned int)p[1]<<8);
  case 1:
    a+=((unsigned int)p[0]);
  }
  MIX(a, b, c);
  return(c);
}

void *
hash_lookup(hashtable ht2, const char *s)
{
  unsigned long hashval;
  int i;
  hash_table_int *ht;

  ht = (hash_table_int *)ht2;

  hashval = (ht->mask) & hash(ht, s);

  for (i=0; i<ht->num_entries; i++) {
    if (ht->table[hashval].udata == NULL)
      return(NULL);
    if (strcmp(ht->table[hashval].key, s) == 0) {
        return(ht->table[hashval].udata);
    }
    hashval = (hashval+1)&(ht->mask);
  }
  return(NULL);
} 

int
hash_install(hashtable ht2, char *s, file_data *udata)
{
  unsigned long hashval, indx;
  int i;
  hash_table_int *ht;

  ht = (hash_table_int *)ht2;
  hashval = (ht->mask)&hash(ht, s);

  for (i=0; i<ht->size; i++) {
    indx = (hashval+i) & (ht->mask);
    if (ht->table[indx].udata == NULL) {  /* unused entry found */
      ht->table[indx].udata = udata;
      ht->table[indx].key = s;
      ht->num_entries++;
      return(0);
    }
  }
  return(1);
}

int is_dot_or_dot_dot(char const* name)
{
   return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 );
}

void listdir(char const* dirname, hashtable *ht, paths *paths)
{
   char* subdir;
   DIR* dirp = opendir(dirname);
   struct dirent *curr_ent;

   struct stat stbuf;
   char last_change[20];
   char tmp[1024];
   char path_buf[1024];  

   if ( dirp == NULL )
   {
      return;
   }

   while ( (curr_ent = readdir(dirp)) != NULL )
   { 
        if (curr_ent->d_type == DT_REG) {
            stat(curr_ent->d_name, &stbuf);
            if (!S_ISLNK(stbuf.st_mode)) {
                file_data *fd = (file_data*)malloc(sizeof(file_data));
                strftime(last_change, 20, "%Y-%m-%d %H:%M", localtime(&stbuf.st_mtime));
                if (strlen(dirname) > 2) {
                    strcpy(tmp, dirname);
                    strcpy(path_buf, &tmp[2]);
                } else {
                    strcpy(path_buf, dirname);
                }
                if(strcmp(dirname, ".") != 0) {
                    strcat(path_buf, "/");
                    strcat(path_buf, curr_ent->d_name);
                    strcpy(fd->last_change, last_change);
                    fd->size = stbuf.st_size;
                    strcpy(fd->name, curr_ent->d_name);
                    strcpy(fd->path, path_buf);
                    fd->checked = 0;
                    // printf("=== %s %lu %s\n%s\n\n", last_change, stbuf.st_size, curr_ent->d_name, path_buf);
                } else {
                    strcpy(fd->last_change, last_change);
                    fd->size = stbuf.st_size;
                    strcpy(fd->name, curr_ent->d_name);
                    strcpy(fd->path, curr_ent->d_name);
                    strcpy(path_buf, curr_ent->d_name);
                    fd->checked = 0;
                }
                hash_install(ht, fd->path, fd);
                strcpy(paths->path[paths->num]->item, fd->path);
                paths->num++;

            }
        }    

        // Traverse sub-directories excluding . and ..
        // Ignore . and ..
        if ( curr_ent->d_type == DT_DIR && ! (is_dot_or_dot_dot(curr_ent->d_name)) )
        {
            // Allocate memory for the subdirectory.
            // 1 additional for the '/' and the second additional for '\0'.
            subdir = malloc(strlen(dirname) + strlen(curr_ent->d_name) + 2);

            // Flesh out the subdirectory name.
            strcpy(subdir, dirname);
            strcat(subdir, "/");
            strcat(subdir, curr_ent->d_name);

            // List the contents of the subdirectory.
            listdir(subdir, ht, paths);

            // Free the allocated memory.
            free(subdir);
        }
   }

   // Close the directory
   closedir(dirp);
}

int main(int argc, char *argv[]) {
    hashtable *ht = hash_init(8);

    paths *path = (paths *)malloc(sizeof(paths));
    path->num = 0;
    for (int i = 0; i < 100000; i++) {
      path->path[i] = (item *) malloc (sizeof(item));
      // path->path[i]->item = (char *) malloc (sizeof(char) * 1024);
      strcpy(path->path[i]->item, "\n");
      path->path[i]->checked = 0;
    }

    listdir(".", ht, path);
    // hash_table_int *tmp = (hash_table_int*)ht;

    file_data *fd = (file_data*)malloc(sizeof(file_data));
    file_data *tmp = (file_data*)malloc(sizeof(file_data));
    int found = 0;
    for (int i = 0; i <= path->num; i++) {
      if (strcmp(path->path[i]->item, "\n") == 0) {
        break;
      } else {
        fd = hash_lookup(ht, path->path[i]->item);
        if (fd && !path->path[i]->checked) {
          path->path[i]->checked = 1;
          found = 0;
          for (int j = i+1; j <= path->num; j++) {
            tmp = hash_lookup(ht, path->path[j]->item);
            if (tmp) {
              if(strcmp(fd->name, tmp->name) == 0) {
                if (fd->size == tmp->size) {
                  if (!found) {
                    printf("=== %s %lu %s\n%s\n", fd->last_change, fd->size, fd->name, fd->path);
                    printf("%s\n", tmp->path);
                    path->path[j]->checked = 1;
                    found = 1;
                  } else {
                    printf("%s\n", tmp->path);
                    path->path[j]->checked = 1;
                  }
                }
              }
            }
          }
          if(found) {
            printf("\n");
            found = 0;
          }
        }
      }
    }

    return 0;
}