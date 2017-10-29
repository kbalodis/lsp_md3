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

int is_dot_or_dot_dot(char const* name)
{
   return (strcmp(name, ".") == 0 || strcmp(name, "..") == 0 );
}

void listdir(char const* dirname)
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
                    printf("=== %s %lu %s\n%s\n\n", last_change, stbuf.st_size, curr_ent->d_name, path_buf);
                } else {
                    printf("=== %s %lu %s\n%s\n\n", last_change, stbuf.st_size, curr_ent->d_name, curr_ent->d_name);
                }
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
            listdir(subdir);

            // Free the allocated memory.
            free(subdir);
        }
   }

   // Close the directory
   closedir(dirp);
}

int main(int argc, int *argv[]) {
    listdir(".");;
    return 0;
}