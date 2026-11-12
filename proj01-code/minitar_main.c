#include <stdio.h>
#include <string.h>

#include "file_list.h"
#include "minitar.h"

int main(int argc, char **argv) {
    int o = 0;
    if (argc < 4) {
        printf("Usage: %s -c|a|t|u|x -f ARCHIVE [FILE...]\n", argv[0]);
        return 0;
    }

    file_list_t files;
    file_list_init(&files);

    // TODO: Parse command-line arguments and invoke functions from 'minitar.h'
    // to execute archive operations
    char* op = argv[1];     //operation to be done
    char* archname = argv[3];              //archive name to be used

    for(int i = 4; i < argc; i++){      //add files from input into file list
        file_list_add(&files, argv[i]);
    }
    files.size = argc - 3;      //initialize size of file list
    if(strcmp(argv[2],"-f")){       //make sure user input the -f
        printf("Invalid input. Please include: -f\n");
        file_list_clear(&files);
        return 1;
    }
    if(strcmp("-c",op)==0){     //create new archive
        o = create_archive(archname, &files);
        if(o !=0){
            printf("Error creating new archive\n");
            file_list_clear(&files);
            return 1;
        }
    }
    else if(strcmp("-a",op)==0){        //append to archive
        if(files.size < 1){     //checks that there are files to append
            printf("Please provide at least one file to append\n");
            file_list_clear(&files);
            return 1;
        }
        else{
            o = append_files_to_archive(archname,&files);
            if(o !=0){      //error check for successful appending
                printf("Error appending new files to archive\n");
                file_list_clear(&files);
                return 1;
            }
        }
    }
    else if(strcmp("-t",op)==0){        //lists files in the archive
        file_list_clear(&files);    //make sure the files list is empty
        file_list_init(&files);
        o = get_archive_file_list(archname, &files);
        if(o != 0){            //checks that files were successfully retreived from archive
            printf("Error getting files archive list\n");
            file_list_clear(&files);
            return 1;
        }
        node_t *node = files.head;
        while(node != NULL){        //print all the files present in the list
            printf("%s\n", node->name);
            node = node->next;
        }
    }
    else if(strcmp("-x",op)==0){        //extracts file from archive
        o = extract_files_from_archive(archname);
        if(o != 0){     //check that files were successfully extracted
            printf("Error extracting files from archive list\n");
            file_list_clear(&files);
            return 1;
        }
    }
    else if(strcmp("-u",op)==0){        //updates archive with new files
        if(files.size < 1){     //checks that user input files
            printf("Please provide at least one file to append\n");
            file_list_clear(&files);
            return 1;
        }
        else{
            file_list_t filesl;     //initialize file list that stores which files are already in the archive
            file_list_init(&filesl);
            o = get_archive_file_list(archname, &filesl);
            if(o !=0){      //checks for errors filling file list filesl with files from archive
                printf("Error getting file list of archive\n");
                file_list_clear(&files);
                file_list_clear(&filesl);
                return 1;
            }
            o = file_list_is_subset(&files, &filesl);
            if(o !=1){      //checks if new files provided are truely already in the archive
                printf("Error: One or more of the specified files is not already present in archive\n");
                file_list_clear(&files);
                file_list_clear(&filesl);
                return 1;

            }
            else{       //adds the files to the end of the archive if they are
                o = append_files_to_archive(archname,&files);
                if(o !=0){      //checks that appending to archive was successful
                    printf("Error appending new files to archive\n");
                    file_list_clear(&files);
                    return 1;
                }
            }
            file_list_clear(&filesl);
        }
        
    }
    else{
        printf("Please provide a valid archive operation\n");
        file_list_clear(&files);
        return 1;
    }

    file_list_clear(&files);
    return 0;
}
