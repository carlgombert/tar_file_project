#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
    return 0;
  }

  file_list_t files;      // all files!
  file_list_init(&files); // init

  if (strcmp("-c", argv[1]) == 0) {  // create archive handling
    for (int i = 4; i < argc; i++) { // grabs all files
      file_list_add(&files, argv[i]);
    }
    if (create_archive(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }

  } else if (strcmp("-a", argv[1]) == 0) { // append file handling
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    if (append_files_to_archive(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }

  } else if (strcmp("-t", argv[1]) == 0) { // file name list handling
    if (get_archive_file_list(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }
    node_t *current = files.head;

    for (int i = 0; i < files.size; i++) { // prints all files names
      printf("%s\n", current->name);
      current = current->next;
    }

  } else if (strcmp("-u", argv[1]) == 0) { // update archive handling
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    file_list_t file_list;
    file_list_init(&file_list);

    get_archive_file_list(argv[3], &file_list);
    if (file_list_is_subset(&files, &file_list) == 0) {
      printf("Error: One or more of the specified files is not already present "
             "in archive");
      file_list_clear(&file_list);
      file_list_clear(&files);
      return 1;
    }
    append_files_to_archive(argv[3], &files); // adds files to archive
    file_list_clear(&file_list);
  }

  file_list_clear(&files); // clears file list and exits
  return 0;
}

/*oopsie*/
