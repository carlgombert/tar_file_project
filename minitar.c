#include "minitar.h"

#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 128
#define BLOCK_SIZE 512

// Constants for tar compatibility information
#define MAGIC "ustar"

// Constants to represent different file types
// We'll only use regular files in this project
#define REGTYPE '0'
#define DIRTYPE '5'

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *) header;
    for (int i = 0; i < sizeof(tar_header); i++) {
        sum += bytes[i];
    }
    snprintf(header->chksum, 8, "%07o", sum);
}

/*
 * Populates a tar header block pointed to by 'header' with metadata about
 * the file identified by 'file_name'.
 * Returns 0 on success or -1 if an error occurs
 */
int fill_tar_header(tar_header *header, const char *file_name) {
    memset(header, 0, sizeof(tar_header));
    char err_msg[MAX_MSG_LEN];
    struct stat stat_buf;
    // stat is a system call to inspect file metadata
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    strncpy(header->name, file_name,
            100);    // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%07o",
             stat_buf.st_mode & 07777);    // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%07o",
             stat_buf.st_uid);                         // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid);    // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->uname, pwd->pw_name,
            32);    // Owner name of the file, null-terminated string

    snprintf(header->gid, 8, "%07o",
             stat_buf.st_gid);                        // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid);    // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->gname, grp->gr_name,
            32);    // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%011o",
             (unsigned) stat_buf.st_size);    // File size, 0-padded octal
    snprintf(header->mtime, 12, "%011o",
             (unsigned) stat_buf.st_mtime);    // Modification time, 0-padded octal
    header->typeflag = REGTYPE;                // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6);          // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2);          // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%07o",
             major(stat_buf.st_dev));    // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%07o",
             minor(stat_buf.st_dev));    // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll
 * learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];

    struct stat stat_buf;
    if (stat(file_name, &stat_buf) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to stat file %s", file_name);
        perror(err_msg);
        return -1;
    }

    off_t file_size = stat_buf.st_size;
    if (nbytes > file_size) {
        file_size = 0;
    } else {
        file_size -= nbytes;
    }

    if (truncate(file_name, file_size) != 0) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        return -1;
    }
    return 0;
}

/*
 * creates archive "archive name" with files in files
 * returns 0 upon success and 1 upon failure
 * creates archive object
 */
int create_archive(const char *archive_name, const file_list_t *files) {
    FILE *arc_fp = fopen(archive_name, "wb");
    if (arc_fp == NULL) {
        perror("Error opening archive file");
        return 1;
    }
    char BUFFER[BLOCK_SIZE];

    node_t *current = files->head;
    while (current != NULL) {    // goes file by file
        const char *file_name = current->name;
        tar_header header;

        FILE *input_fp = fopen(file_name, "rb");
        if (input_fp == NULL) {
            perror("Error opening input file");
            fclose(arc_fp);
            return 1;
        }

        fill_tar_header(&header, file_name);    // writes header for file

        if (fwrite(&header, BLOCK_SIZE, 1, arc_fp) != 1) {
            perror("Error writing header");
            fclose(arc_fp);
            fclose(input_fp);
            return 1;
        }

        ssize_t bytes_read;
        size_t total_bytes = 0;

        while ((bytes_read = fread(BUFFER, 1, BLOCK_SIZE, input_fp)) > 0) {
            if (fwrite(BUFFER, bytes_read, 1, arc_fp) ==
                0) {    // loops through block size and copies charecters
                perror("Error writing data");
                fclose(arc_fp);
                fclose(input_fp);
                return 1;
            }
            total_bytes += bytes_read;
        }
        if (ferror(input_fp)) {
            perror("Error reading from input file");
            fclose(arc_fp);
            fclose(input_fp);
            return 1;
        }

        size_t pad = (BLOCK_SIZE - (total_bytes % BLOCK_SIZE)) % BLOCK_SIZE;
        if (pad > 0) {    // writes pad to archive
            memset(BUFFER, 0, BLOCK_SIZE);
            if (fwrite(BUFFER, pad, 1, arc_fp) != 1) {
                perror("Error writing padding to archive");
                fclose(arc_fp);
                fclose(input_fp);
                return 1;
            }
        }
        fclose(input_fp);
        current = current->next;
    }

    memset(BUFFER, 0, BLOCK_SIZE);    // writes end blocks to archive
    if (fwrite(BUFFER, BLOCK_SIZE, 1, arc_fp) != 1 || fwrite(BUFFER, BLOCK_SIZE, 1, arc_fp) != 1) {
        perror("Error writing final null blocks");
        fclose(arc_fp);
        return 1;
    }

    fclose(arc_fp);
    return 0;
}

/*
 * appends files to archive
 * opens archive, appends all files regardless of contained or not
 * returns 0 upon success, 1 upon failure.
 */
int append_files_to_archive(const char *archive_name, const file_list_t *files) {
    int fd = open(archive_name, O_WRONLY | O_APPEND, 0666);
    char BUFFER[BLOCK_SIZE];
    if (!fd) {
        close(fd);
        return -1;
    }

    remove_trailing_bytes(archive_name, 2 * BLOCK_SIZE);

    node_t *current = files->head;
    while (current != NULL) {
        const char *file_name = current->name;
        tar_header header;

        int input_fd = open(file_name, O_RDONLY);
        if (!fd) {
            close(fd);
            return -1;
        }

        fill_tar_header(&header, file_name);
        write(fd, &header, BLOCK_SIZE);
        ssize_t bytes_read;
        size_t total_bytes = 0;

        while ((bytes_read = read(input_fd, BUFFER, BLOCK_SIZE)) > 0) {
            write(fd, BUFFER, bytes_read);
            total_bytes += bytes_read;
        }

        size_t pad = (BLOCK_SIZE - (total_bytes % BLOCK_SIZE)) % BLOCK_SIZE;
        if (pad > 0) {
            memset(BUFFER, 0, BLOCK_SIZE);
            write(fd, BUFFER, pad);
        }

        current = current->next;
    }
    memset(BUFFER, 0, BLOCK_SIZE);
    write(fd, BUFFER, BLOCK_SIZE);
    write(fd, BUFFER, BLOCK_SIZE);

    close(fd);
    return 0;
}

/*
 * creates a list of all files contained in the archive
 * returns 0 upon success and 1 upon failure
 * takes archive name and empty files list, add to files list
 */
int get_archive_file_list(const char *archive_name, file_list_t *files) {
    FILE *fd = fopen(archive_name, "rb");
    if (fd == NULL) {
        perror("Error opening archive file");
        return 1;
    }

    char BUFFER[BLOCK_SIZE];
    tar_header header;

    fseek(fd, BLOCK_SIZE * 2 * -1,
          SEEK_END);    // finds end of file then steps back to last block of data
    off_t end = ftell(fd);
    fseek(fd, 0, SEEK_SET);    // resets to front

    while (ftell(fd) < end) {
        ssize_t bytes_read = fread(BUFFER, 1, BLOCK_SIZE, fd);
        if (bytes_read == 0) {
            perror("Error reading tar header from archive");
            fclose(fd);
            return 1;
        }
        memcpy(&header, BUFFER, sizeof(tar_header));    // copies header to local
                                                        // struct
        if (file_list_contains(files, header.name) == 0) {
            file_list_add(files, header.name);    // adds headers name from local struct
        }

        long file_size = strtol(header.size, NULL, 8);
        long data_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        fseek(fd, (data_blocks) *BLOCK_SIZE,
              SEEK_CUR);    // skips to start of next struct
    }
    fclose(fd);
    return 0;
}

int extract_files_from_archive(const char *archive_name) {
    FILE *fd = fopen(archive_name, "rb");
    if (fd == NULL) {
        perror("Error opening archive file");
        return 1;
    }

    char BUFFER[BLOCK_SIZE];
    tar_header header;

    fseek(fd, BLOCK_SIZE * 2 * -1,
          SEEK_END);    // finds end of file then steps back to last block of data
    off_t end = ftell(fd);
    fseek(fd, 0, SEEK_SET);    // resets to front

    while (ftell(fd) < end) {
        ssize_t bytes_read = fread(BUFFER, 1, BLOCK_SIZE, fd);
        if (bytes_read == 0) {
            perror("Error reading tar header from archive");
            fclose(fd);
            return 1;
        }
        memcpy(&header, BUFFER, sizeof(tar_header));

        long file_size = strtol(header.size, NULL, 8);
        long data_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

        FILE *output_fd = fopen(header.name, "wb");
        if (output_fd == NULL) {
            perror("Error creating output file");
            fclose(fd);
            return 1;
        }

        long bytes_written = 0;
        for (long i = 0; i < data_blocks; i++) {
            fread(BUFFER, 1, BLOCK_SIZE, fd);
            size_t bytes_to_write = BLOCK_SIZE;
            if (bytes_written + BLOCK_SIZE > file_size) {
                bytes_to_write = file_size - bytes_written;
            }
            fwrite(BUFFER, 1, bytes_to_write, output_fd);
            bytes_written += bytes_to_write;
        }
        fclose(output_fd);
    }
    fclose(fd);
    return 0;
}

int update_files_in_archive(const char *archive_name, const file_list_t *files) {
    file_list_t file_list;
    file_list_init(&file_list);

    get_archive_file_list(archive_name, &file_list);

    /////////////////////////////////////////////////////////////////////////////
    /*
      for (node_t *cur = files->head; cur != NULL; cur = cur->next) {
        printf("%s\n", cur->name);
      }

      for (node_t *cur = file_list.head; cur != NULL; cur = cur->next) {
        printf("%s\n", cur->name);
      }
    */
    /////////////////////////////////////////////////////////////////////////////

    if (file_list_is_subset(files, &file_list) == 0) {    // checks all files are present
        printf(
            "Error: One or more of the specified files is not already present "
            "in archive");
        file_list_clear(&file_list);
        return -1;
    }

    append_files_to_archive(archive_name, files);    // adds files to archive
    file_list_clear(&file_list);
    return 0;
}
