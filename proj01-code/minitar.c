#include <fcntl.h>
#include <grp.h>
#include <math.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <unistd.h>

#include "minitar.h"

#define NUM_TRAILING_BLOCKS 2
#define MAX_MSG_LEN 512

/*
 * Helper function to compute the checksum of a tar header block
 * Performs a simple sum over all bytes in the header in accordance with POSIX
 * standard for tar file structure.
 */
void compute_checksum(tar_header *header) {
    // Have to initially set header's checksum to "all blanks"
    memset(header->chksum, ' ', 8);
    unsigned sum = 0;
    char *bytes = (char *)header;
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

    strncpy(header->name, file_name, 100); // Name of the file, null-terminated string
    snprintf(header->mode, 8, "%07o", stat_buf.st_mode & 07777); // Permissions for file, 0-padded octal

    snprintf(header->uid, 8, "%07o", stat_buf.st_uid); // Owner ID of the file, 0-padded octal
    struct passwd *pwd = getpwuid(stat_buf.st_uid); // Look up name corresponding to owner ID
    if (pwd == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up owner name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->uname, pwd->pw_name, 32); // Owner  name of the file, null-terminated string

    snprintf(header->gid, 8, "%07o", stat_buf.st_gid); // Group ID of the file, 0-padded octal
    struct group *grp = getgrgid(stat_buf.st_gid); // Look up name corresponding to group ID
    if (grp == NULL) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to look up group name of file %s", file_name);
        perror(err_msg);
        return -1;
    }
    strncpy(header->gname, grp->gr_name, 32); // Group name of the file, null-terminated string

    snprintf(header->size, 12, "%011o", (unsigned)stat_buf.st_size); // File size, 0-padded octal
    snprintf(header->mtime, 12, "%011o", (unsigned)stat_buf.st_mtime); // Modification time, 0-padded octal
    header->typeflag = REGTYPE; // File type, always regular file in this project
    strncpy(header->magic, MAGIC, 6); // Special, standardized sequence of bytes
    memcpy(header->version, "00", 2); // A bit weird, sidesteps null termination
    snprintf(header->devmajor, 8, "%07o", major(stat_buf.st_dev)); // Major device number, 0-padded octal
    snprintf(header->devminor, 8, "%07o", minor(stat_buf.st_dev)); // Minor device number, 0-padded octal

    compute_checksum(header);
    return 0;
}

/*
 * Removes 'nbytes' bytes from the file identified by 'file_name'
 * Returns 0 upon success, -1 upon error
 * Note: This function uses lower-level I/O syscalls (not stdio), which we'll learn about later
 */
int remove_trailing_bytes(const char *file_name, size_t nbytes) {
    char err_msg[MAX_MSG_LEN];
    // Note: ftruncate does not work with O_APPEND
    int fd = open(file_name, O_WRONLY);
    if (fd == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to open file %s", file_name);
        perror(err_msg);
        return -1;
    }
    //  Seek to end of file - nbytes
    off_t current_pos = lseek(fd, -1 * nbytes, SEEK_END);
    if (current_pos == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to seek in file %s", file_name);
        perror(err_msg);
        close(fd);
        return -1;
    }
    // Remove all contents of file past current position
    if (ftruncate(fd, current_pos) == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to truncate file %s", file_name);
        perror(err_msg);
        close(fd);
        return -1;
    }
    if (close(fd) == -1) {
        snprintf(err_msg, MAX_MSG_LEN, "Failed to close file %s", file_name);
        perror(err_msg);
        return -1;
    }
    return 0;
}




int write_file(FILE* out, node_t *node){     //helper function that writes filename in node to output file out. out is used both as an output file in extract, and as an archive in create archive
    const char* filename = node->name;      //file to read from
    char buffer[MAX_MSG_LEN];       //buffer for transfering data from file filename to file out
    int o = 0;      //used for checking errors
    tar_header head;        //header struct used for filling the header of output file
    if(filename == NULL){       //check if node's filename is valid
        printf("Non-valid filename provided: %s\n", filename);
        return -1;
    }
    
    o = fill_tar_header(&head, filename);
    if(o == -1){        //check if fill_tar_header was successful
        printf("Failed to fill tarheader for %s\nFile possibly does not exist\n", filename);
        return -1;
    }

    FILE* tmp = fopen(filename,"r");        //open filename as tmp
    if(tmp == NULL){        //double check that file was able to be opened
        printf("File %s does not exist\n", filename);
        perror("Failed to open file");
        if(fclose(tmp)){        //check that file was closed correctly
            printf("Failed to close %s\n", filename);
        }
        return -1;
    }
    o = fwrite(&head, 1, sizeof(tar_header),out);
    if(o != sizeof(tar_header)){        //check if number of bytes written matches size of a header
        printf("Wrote incorrect number of bytes to output file\n");
        if(fclose(tmp)){        //check that file was closed correctly
            printf("Failed to close %s\n", filename);
        }
        return -1;
    }

    
    int s = MAX_MSG_LEN;
    while(s == MAX_MSG_LEN){        //iterate until read through all of tmp
        memset(buffer,0,MAX_MSG_LEN);       //sets all memory to 0 first to account for blocks that may have trailing zeros since data read in doesn't fill a whole block
        s = fread(buffer, 1, MAX_MSG_LEN, tmp);
        if(ferror(tmp)){        //check if reading from tmp was successful
            printf("Error reading from %s\n", filename);
            if(fclose(tmp)){        //check that file was closed correctly
                printf("Failed to close %s\n", filename);
            }
            return -1;
        }
        if(s==0){       //check if an empty file got passed in. in this case don't write anything
            break;
        }
        o = fwrite(buffer, 1, MAX_MSG_LEN, out);
        if(o != MAX_MSG_LEN){        //check if writing to out was successful
            printf("Error in writing to output file\n");
            if(fclose(tmp)){        //check that file was closed correctly
                printf("Failed to close %s\n", filename);
            }
            return -1;
        }
    }
    if(fclose(tmp)){        //check that file was closed correctly
        printf("Failed to close %s\n", filename);
        return -1;
    }
    return 0;
}



int add_files_to_archive(FILE* out, const file_list_t *files){      //helper function that adds specified files in file_list to already opened file archive out
    node_t *node = files->head;     //initialize node used to parse through file list
    if(node == NULL){       //check that the passed in file list is pointing somewhere
        printf("Invalid file list passed in\n");
        return -1;
    }
    int o = 0;      //used for error checking
    char buffer[MAX_MSG_LEN];       //used for adding footer blocks
    while(node!=NULL){      //iterate through whole list to pass into helper function node by node
        o = write_file(out, node);
        if(o == -1){        //check if helper function was successful
            printf("Error adding file %s to archive\n", node->name);
            return -1;
        }
        node = node->next;
    }
    memset(buffer,0,MAX_MSG_LEN);       //add footer to end of archive
    fwrite(buffer,1,MAX_MSG_LEN,out);
    fwrite(buffer,1,MAX_MSG_LEN,out);
    return 0;
}





int create_archive(const char *archive_name, const file_list_t *files) {        //creates an archive called archive_name filled with given files in file_list_t files
    // TODO: Not yet implemented
    FILE* out = fopen(archive_name, "w");       //open file to create archive in  
    if(out == NULL){        //make sure archive pointer is pointing at something
        printf("Error creating archive %s\n", archive_name);
        perror("Failed to open file");
        if(fclose(out)){
            printf("Failed to close archive %s\n", archive_name);
        }
        return -1;
    }
    int o = 0;      //used for error checking
    o = add_files_to_archive(out, files);
    if(o != 0){     //check if files were successfully added to archive out
        printf("Error adding to archive %s\n", archive_name);
        if(fclose(out)){        //check that file was closed correctly
            printf("Failed to close archive %s\n", archive_name);
        }
        return -1;
    }
    if(fclose(out)){        //check that file was closed correctly
        printf("Failed to close archive %s\n", archive_name);
        return -1;
    }
    return 0;
}

int append_files_to_archive(const char *archive_name, const file_list_t *files) {       //appends new files in file list to given archive
    // TODO: Not yet implemented
    int o = 0;      //used for error checking
    o = remove_trailing_bytes(archive_name, 2*MAX_MSG_LEN);     //first remove footer
    if(o == -1){        //check that footer was removed successfully
        printf("Error removing footer from archive\n%s potentially does not exist\n",archive_name);
        return -1;
    }
    FILE* out = fopen(archive_name, "a+");       //make sure archive pointer is pointing at something
    if(out == NULL){
        printf("Error opening %s\n",archive_name);
        perror("Failed to open file");
        if(fclose(out)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }
    o = add_files_to_archive(out, files);       //add new files to archive using helper function
    if(o != 0){     //check for any errors in helper function
        printf("Error adding files to archive %s\n",archive_name);
        if(fclose(out)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }

    if(fclose(out)){        //check that file was closed correctly
        printf("Failed to close archive %s\n",archive_name);
        return -1;
    }
    return 0;
    
}

int get_archive_file_list(const char *archive_name, file_list_t *files) {       //populates given file list with filenames present in archive archive_name
    // TODO: Not yet implemented
    int size = 1;       //used to store how many bytes long each file is
    int s = 0;      //used to store how many bytes were read
    int mult = 0;       //used to calculate how many chunks to skip through to get to next file in archive
    tar_header head;        //used as a buffer to get file information
    
    FILE* out = fopen(archive_name, "r");       
    if(out == NULL){        //make sure archive pointer is pointing at something
        printf("Error opening %s\n",archive_name);
        perror("Failed to open file");
        if(fclose(out)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }
    fseek(out, 0, SEEK_END);        //get the total number of bytes in the archive
    int length = ftell(out);
    fseek(out, 0, SEEK_SET);

    if(length%MAX_MSG_LEN != 0){        //double check that input file is of correct format(number of bytes equals multiple of MAX_MSG_LEN)
        printf("Archive %s does not have the correct format\nArchive length is not compatible\n",archive_name);
        if(fclose(out)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }
    int pos = 0;        //used to see where in the file the file reader is
    while(pos < length - 2*MAX_MSG_LEN){        //iterate through archive making sure not to step outside of allocated memory. checks for reaching footer by location of the pos variable relative to the end of the archive
        size = 0;
        s = fread(&head, 1,sizeof(tar_header), out);
        if(s!=sizeof(tar_header) ){     //check that the header was read in correctly
            printf("Wrong number of bytes read from file header\n");
            if(fclose(out)){        //check that file was closed correctly
                printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        pos += sizeof(tar_header);      //augment position if header correctly read in
        if(ferror(out)){        //check for any errors from reading
            printf("Error reading in file header\n");
            if(fclose(out)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        if(head.name == NULL){      //double check file's name field is valid
            printf("Invalid filename found in archive %s\n",archive_name);
            if(fclose(out)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        if(file_list_add(files, head.name)){        //add file's name to file list and checks if it was successful
            printf("Failed to add %s to the file list\n", head.name);
            if(fclose(out)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }

        for(int i =10; i >=0; i--){     //convert files size from char octal to int
            size += round(pow(8,10-i)*(head.size[i]-'0'));
        }
        if(size < 0 || pos+size>length){       //make sure size was correctly read
            printf("Invalid file size read from file in archive\n");
            if(fclose(out)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        if(size%MAX_MSG_LEN==0){       //calculate the numer of chunks needed to skip passed to get to next header
            mult = size/MAX_MSG_LEN;
        }
        else{
            mult = size/MAX_MSG_LEN+1;
        }
        fseek(out, mult*MAX_MSG_LEN, SEEK_CUR);     //move to next file in archive
        pos += mult*MAX_MSG_LEN;           //update poition in file accordingly

    }


    if(fclose(out)){        //check that file was closed correctly
        printf("Failed to close archive %s\n",archive_name);
        return -1;
    }
    return 0;
}

int extract_files_from_archive(const char *archive_name) {
    // TODO: Not yet implemented
    int o = 0;      //used for error checking
    int s = 0;      //used for checking number of bytes read
    int size = 1;       //used for storing size of each file
    int mult = 0;       //used for calculating number of blocks needed to skip to get to next file in archive
    file_list_t files;      //used to store the names of files in archive to avoid repeated extracted files
    file_list_init(&files);
    char buffer[MAX_MSG_LEN];       //buffer for transfering data from file filename to file out
    
    o = get_archive_file_list(archive_name, &files);        //fill files with filenames of files in archive
    if(o!=0){       //check that file list got filled correctly
        printf("Error filling list to determine repeated files\n");
        return -1;
    }
    FILE* fp = fopen(archive_name, "r");       
    if(fp == NULL){        //make sure archive pointer is pointing at something
        printf("Error opening %s\n",archive_name);
        perror("Failed to open file");
        if(fclose(fp)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }
    fseek(fp, 0, SEEK_END);        //get the total number of bytes in the archive
    int length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if(length%MAX_MSG_LEN != 0){        //double check that input file is of correct format(number of bytes equals multiple of MAX_MSG_LEN)
        printf("Archive %s does not have the correct format\nArchive length is not compatible\n",archive_name);
        if(fclose(fp)){        //check that file was closed correctly
            printf("Failed to close archive %s\n",archive_name);
        }
        return -1;
    }
    int pos = 0;


    node_t *currnode = files.head;      //node used to represent current file being read in archive
    node_t *iter = files.head;      //node used to check if the current node appears later on in the files list
    tar_header head;        //used as a buffer to get data about files in archive
    while(currnode != NULL && pos < length - 2*MAX_MSG_LEN){        //iterate through archive while there are still files in it. code until 454 is copied from previous function
        size = 0;
        s = fread(&head, 1,sizeof(tar_header), fp);
        if(s!=sizeof(tar_header) ){     //check that the header was read in correctly
            printf("Wrong number of bytes read from file header\n");
            if(fclose(fp)){        //check that file was closed correctly
                printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        pos += sizeof(tar_header);      //augment position if header correctly read in
        if(ferror(fp)){        //check for any errors from reading
            printf("Error reading in file header\n");
            if(fclose(fp)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        if(head.name == NULL){      //double check file's name field is valid
            printf("Invalid filename found in archive %s\n",archive_name);
            if(fclose(fp)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }

        for(int i =10; i >=0; i--){     //convert files size from char octal to int
            size += round(pow(8,10-i)*(head.size[i]-'0'));
        }
        if(size < 0 || pos+size>length){       //make sure size was correctly read
            printf("Invalid file size read from file in archive\n");
            if(fclose(fp)){        //check that file was closed correctly
               printf("Failed to close archive %s\n",archive_name);
            }
            return -1;
        }
        if(size%MAX_MSG_LEN==0){       //calculate the numer of chunks needed to skip passed to get to next header
            mult = size/MAX_MSG_LEN;
        }
        else{
            mult = size/MAX_MSG_LEN+1;
        }
        //end of copied code


        iter = currnode->next;      //iterate through files after currnode because we only want to check if currnode is the last instance of a file in the list
        o = 0;      //use o to check if currenode's filename pops up again later in the list;
        while(iter != NULL){        //check that there are no files after this one
            if(strcmp(iter->name,currnode->name)==0){       //set o to -1 if this file appears again later in the list
                o = -1;
            }
            iter = iter->next;
        }
        if(o == 0){         //only extract if file is unique or the last instance in the list
            FILE* out = fopen(currnode->name, "w");       //make sure archive pointer is pointing at something
            if(out == NULL){        //check that file was opened correctly
                printf("Failed to extract %s", currnode->name);
                perror("Failed to open file");
                if(fclose(out)){        //check that file was closed correctly
                    printf("Failed to close file %s\n", currnode->name);
                }
                if(fclose(fp)){        //check that file was closed correctly
                    printf("Failed to close file\n");
                    return -1;
                }
                return -1;
            }
            
            
            int s = MAX_MSG_LEN;        //s will denote the number of bytes read in from the archive
            int p = 0;      //denotes position within current file being read from archive
            while(s >0 && p < size ){        //iterate to write all of file
                if(size-p < MAX_MSG_LEN){       //only read as much as needed if at the end of the file
                    s = fread(buffer, 1, size-p, fp);
                }
                else{       //read max amount if there is still more to go afterwards
                    s = fread(buffer, 1, MAX_MSG_LEN, fp);
                }
                p += s;
                if(ferror(fp)){        //check if reading from tmp was successful
                    printf("Error reading from %s\n", currnode->name);
                    if(fclose(out)){        //check that file was closed correctly
                        printf("Failed to close %s\n", currnode->name);
                    }
                    if(fclose(fp)){        //check that file was closed correctly
                        printf("Failed to close %s\n", currnode->name);
                    }
                    return -1;
                }
                if(s==0){       //check if an empty file got passed in. in this case don't write anything
                    break;
                }
                o = fwrite(buffer, 1,s, out);
                if(o != s){        //check if writing to out was successful
                    printf("Error in writing to output file\n");
                    if(fclose(out)){        //check that file was closed correctly
                        printf("Failed to close %s\n", currnode->name);
                    }
                    if(fclose(fp)){        //check that file was closed correctly
                        printf("Failed to close %s\n", currnode->name);
                    }
                    return -1;
                }
            }

            if(fclose(out)){        //check that file was closed correctly
                printf("Failed to close file\n");
                if(fclose(fp)){        //check that file was closed correctly
                    printf("Failed to close file\n");
                }
                return -1;
            }
            if(size%MAX_MSG_LEN!=0){       //check if bytes need to be skipped
                fseek(fp, MAX_MSG_LEN - size%MAX_MSG_LEN, SEEK_CUR);        //skip forward to next header
            }

        }
        else{
            fseek(fp, mult*MAX_MSG_LEN, SEEK_CUR);      //update location in archive if fread didn't do so while reading the file for extraction
        }
        currnode = currnode->next;      //iterate to next node in file list
        pos += mult*MAX_MSG_LEN;           //update poition in file accordingly
    }
    file_list_clear(&files);        //free file list
    if(fclose(fp)){        //check that file was closed correctly
        printf("Failed to close file\n");
        return -1;
    }
    return 0;
}
