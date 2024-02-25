#include <stdio.h>
#include <string.h>
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
    else if(strcmp(command, "info")==0){
        info_command(argv[2]);
    }
    else if(strcmp(command, "peers")==0){
        peers_command(argv[2]);
    }
    else if(strcmp(command, "handshake")==0){
        if (argc < 4) {
            fprintf(stderr, "Usage: your_bittorrent.sh <command> <args> <peer_ip>:<peer_port>\n");
            return 1;
        }
        handshake_command(argv[2], argv[3]);
    }

    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        return 1;
    }

    return 0;
}
