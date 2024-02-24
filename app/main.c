#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "decoder.h"

int
sha1digest(uint8_t *digest, char *hexdigest, const uint8_t *data, size_t databytes);


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
        char * tracker_url = search_dict(decoded_values, ANNOUNCE)->str_value->value;
        struct bencode* info = search_dict(decoded_values, INFO);
        char *encoded_info = malloc(2048);
        int len=0;
        encode_bencode(info, encoded_info, &len);
        long length = search_dict( info, LENGTH)->int_value;
        long piece_length = search_dict( info, "piece length")->int_value;

        unsigned char sha_value[20];
        unsigned char* uencoded_info = to_unsigned_char(encoded_info, len);

//        FILE * wptr = fopen("out1.txt", "wb");
//        fwrite(uencoded_info, 1, len, wptr);
//        fclose(wptr);

        sha1digest(sha_value, NULL, uencoded_info, len);
        free(encoded_info);
        free(uencoded_info);

        printf("Tracker URL: %s\n", tracker_url);
        printf("Length: %ld\n", length);
        printf("Info Hash: ");
        print2Hex(sha_value, 20);
        printf("Piece Length: %ld\n", piece_length);
        printf("Piece Hashes:\n");

        struct bencode* piece_hashes = search_dict( info, "pieces");
        unsigned char *uhash;
        for(int i=0; i<piece_hashes->str_value->length; i+=20){
            uhash = to_unsigned_char(piece_hashes->str_value->value+i, 20);
            print2Hex(uhash, 20);
            free(uhash);
        }
        free(decoded_values);
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
