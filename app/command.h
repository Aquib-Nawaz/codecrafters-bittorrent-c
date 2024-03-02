//
// Created by Aquib Nawaz on 24/02/24.
//

#ifndef CODECRAFTERS_BITTORRENT_C_COMMAND_H
#define CODECRAFTERS_BITTORRENT_C_COMMAND_H

#include <stdint.h>
#include <curl/curl.h>
#include <stdbool.h>

#define PEER_ID "22101999935711102001"
#define PROTOCOL "BitTorrent protocol"
#define PORT 6881
#define PIECE_BLOCK_SIZE 16384

struct MemoryStruct {
    char *memory;
    size_t size;
};

struct MetaInfo{
    char* tracker_url;
    struct bencode* decoded_value;
    unsigned char* info_hash;
};

struct MetaInfo decode_meta_info(char* filename);
void decode_command(const char*);
void info_command(struct MetaInfo);
unsigned char * peers_command( struct MetaInfo);
CURL* handshake_command( char*, struct MetaInfo);
bool download_piece_command(char*,unsigned char *, struct MetaInfo, int, uint8_t );
void download_command(char*, unsigned char*, struct MetaInfo);

struct __attribute__((__packed__)) HandShake{
    char protocol_len;
    char protocol[19];
    uint64_t reserved;
    char info_hash[20];
    char peer_id[20];
};


struct PieceInfo {
    int piece_length;
    int cur_piece_index;
    int cur_piece_offset;
    int cur_piece_length;
    uint8_t file_mode;
    FILE* fp;

} ;

enum MESSAGE_TYPE{
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 3,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL=8
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);
int WaitOnSocket(curl_socket_t sockfd, int for_recv, long timeout_ms);

#endif //CODECRAFTERS_BITTORRENT_C_COMMAND_H
