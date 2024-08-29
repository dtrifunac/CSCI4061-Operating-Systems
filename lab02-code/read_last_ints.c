#include <stdio.h>
#include <stdlib.h>

/*
 * Read the last integers from a binary file
 *   'num_ints': The number of integers to read
 *   'file_name': The name of the file to read from
 * Returns 0 on success and -1 on error
 */
int read_last_ints(const char *file_name, int num_ints) {
    // TODO Not yet implemented
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL){
        fclose(fp);
        return -1;
    }
    fseek(fp, -num_ints*sizeof(int), SEEK_END);
    int buffer = 0;
    for(int i = 0; i < num_ints; i++){
        fread(&buffer,1,sizeof(int), fp);
        printf("%d\n", buffer);
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <file_name> <num_ints>\n", argv[0]);
        return 1;
    }

    const char *file_name = argv[1];
    int num_ints = atoi(argv[2]);
    if (read_last_ints(file_name, num_ints) != 0) {
        printf("Failed to read last %d ints from file %s\n", num_ints, file_name);
        return 1;
    }
    return 0;
}
