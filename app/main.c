#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
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
         struct bencode* decoded_values = decode_bencode(&encoded_str);
         print_bencode(decoded_values);
         printf("\n");
         free(decoded_values);
    }
    else if(strcmp(command, "info")==0){

        FILE* fptr = fopen(argv[2], "r");
        if (fptr == NULL) {
            printf("Could not open file %s", argv[2]);
            return 1;
        }
        const char* encoded_str = malloc(2048);
        size_t read_len = fread((void*)encoded_str,1, 2048, fptr);
        assert(read_len < 2048);
        fclose(fptr);
        struct bencode* decoded_values = decode_bencode(&encoded_str);
        char * tracker_url = search_dict(decoded_values, ANNOUNCE)->str_value;
        long length = search_dict( search_dict(decoded_values, INFO), LENGTH)->int_value;
        printf("Tracker URL: %s\n", tracker_url);
        printf("Length: %ld\n", length);
        free(decoded_values);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
