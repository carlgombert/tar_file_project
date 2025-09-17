#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
  if (argc < 4) {
    printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
    return 0;
  }

  if (strcmp("-c", argv[1]) == 0) {
    file_list_t files;
    file_list_init(&files);
    for (int i = 4; i < argc; i++) {
      file_list_add(&files, argv[i]);
    }
    create_archive(argv[3], &files);
    file_list_clear(&files);
  } else if (strcmp("-a", argv[1]) == 0) {
    return;
  }

  return 0;
}

/*oopsie*/
