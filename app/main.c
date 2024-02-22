#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "decoder.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
    	// You can use print statements as follows for debugging, they'll be visible when running tests.
//        printf("Logs from your program will appear here!\n");
            
        // Uncomment this block to pass the first stage
         const char* encoded_str = argv[2];
         char* decoded_str = decode_bencode(&encoded_str);
         printf("%s\n", decoded_str);
         free(decoded_str);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
