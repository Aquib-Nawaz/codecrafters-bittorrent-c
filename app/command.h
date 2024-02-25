//
// Created by Aquib Nawaz on 24/02/24.
//

#ifndef CODECRAFTERS_BITTORRENT_C_COMMAND_H
#define CODECRAFTERS_BITTORRENT_C_COMMAND_H

#include <stdint.h>

#define PEER_ID "22101999935711102001"
#define PROTOCOL "BitTorrent protocol"
#define PORT 6881

void decode_command(const char*);
void info_command(char*);
void peers_command(char* filename);
void handshake_command(char*, char*);

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct MetaInfo{
    char* tracker_url;
    struct bencode* decoded_value;
    unsigned char* info_hash;
};

struct __attribute__((__packed__)) HandShake{
    char protocol_len;
    char protocol[19];
    uint64_t reserved;
    char info_hash[20];
    char peer_id[20];
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

#endif //CODECRAFTERS_BITTORRENT_C_COMMAND_H
