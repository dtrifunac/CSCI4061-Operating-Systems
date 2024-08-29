#include <stdio.h>

#define BUF_SIZE 4096

/*
 * Copy the contents of one file into another file
 *   source_file: Name of the source file to copy from
 *   dest_file: Name of the destination file to copy to
 * The destination file is overwritten if it already exists
 * Returns 0 on success and -1 on error
 */
int copy_file(const char *source_file, const char *dest_file) {
    // TODO Not yet implemented
    FILE* fp = fopen(source_file, "r");
    if (fp == NULL){
        fclose(fp);
        return -1;
    }
    FILE* out = fopen(dest_file, "w");
    char buffer[BUF_SIZE];
    int s = BUF_SIZE;
    int l = 0;
    while(s > 0){
        s = fread(buffer,1, BUF_SIZE, fp);
        if(ferror(fp)){
            fclose(fp);
            fclose(out);
            return -1;
        }
        l = fwrite(buffer, 1, s, out);
        if(ferror(out)){
            fclose(fp);
            fclose(out);
            return -1;
        }
        if(s != l){
            fclose(fp);
            fclose(out);
            return -1;
        }
    }

    fclose(fp);
    fclose(out);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }

    // copy_file already prints out any errors
    if (copy_file(argv[1], argv[2]) != 0) {
        return 1;
    }
    return 0;
}
