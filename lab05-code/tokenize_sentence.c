#include <string.h>
#include <stdio.h>

#define BUF_LEN 512

void print_words(char *s) {
    // TODO Not yet implemented
    char *tok = strtok(s, " ");
    while(tok!=NULL){
        printf("%s\n",tok);
        tok = strtok(NULL, " ");
    }
    return;
}

int main(int argc, char **argv) {
    char sentence[BUF_LEN];

    printf("sentence> ");
    while(fgets(sentence, BUF_LEN, stdin) != NULL) {
        // Remove trailing newline from sentence
        int i = 0;
        while (sentence[i] != '\n') {
            i++;
        }
        sentence[i] = '\0';

        if (strcmp(sentence, "exit") == 0) {
            return 0;
        }

        print_words(sentence);

        printf("sentence> ");
    }

    printf("\n");
    return 0;
}
