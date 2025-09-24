#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
    return 0;
  }

  file_list_t files;
  file_list_init(&files);

  if (strcmp("-c", argv[1]) == 0) {
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    if (create_archive(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }

  } else if (strcmp("-a", argv[1]) == 0) {
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    if (append_files_to_archive(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }

  } else if (strcmp("-t", argv[1]) == 0) {
    if (get_archive_file_list(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }
    node_t *current = files.head;

    for (int i = 0; i < files.size; i++) {
      printf("%s\n", current->name);
      current = current->next;
    }

  } else if (strcmp("-u", argv[1]) == 0) {
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    if (update_files_in_archive(argv[3], &files)) {
      file_list_clear(&files);
      return 1;
    }
  }

  file_list_clear(&files);
  return 0;
}

/*oopsie*/
