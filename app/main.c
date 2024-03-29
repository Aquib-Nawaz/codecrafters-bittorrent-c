#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "command.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: your_bittorrent.sh <command> <args>\n");
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "decode") == 0) {
        decode_command(argv[2]);
    }
    else{
        struct MetaInfo metaInfo;

        if(strncmp(command, "download", 8)!=0)
             metaInfo = decode_meta_info(argv[2]);
        else
            metaInfo = decode_meta_info(argv[4]);

        if(strcmp(command, "info")==0){
            info_command(metaInfo);
        }
        else if(strcmp(command, "peers")==0){
            free(peers_command(metaInfo));
        }
        else if(strcmp(command, "handshake")==0){
            if (argc < 4) {
                fprintf(stderr, "Usage: your_bittorrent.sh <command> <args> <peer_ip>:<peer_port>\n");
                return 1;
            }
            handshake_command(argv[3], metaInfo);
        }
        else if(strcmp(command, "download_piece")==0){
            unsigned char * peers = peers_command(metaInfo);
            FILE* fptr = fopen(argv[3], "wb");
            if(!fptr){
                char *err_message;
                err_message = strerror(errno);
                fprintf(stderr, "[ERROR] Could not open file %s for writing :- %s\n",argv[3], err_message);
                exit( 1);
            }
            fclose(fptr);
            download_piece_command(argv[3], peers, metaInfo, atoi(argv[5]), 0);
            free(peers);
        }
        else if(strcmp(command, "download")==0){
            unsigned char * peers = peers_command(metaInfo);
            download_command(argv[3], peers, metaInfo);
            free(peers);
        }
        else {
            fprintf(stderr, "Unknown command: %s\n", command);
            return 1;
        }
        free(metaInfo.decoded_value);
    }

    return 0;
}
