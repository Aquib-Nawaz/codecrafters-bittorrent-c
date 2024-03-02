//
// Created by Aquib Nawaz on 24/02/24.
//

#include <stdlib.h>
#include <stdio.h>
#include "command.h"
#include "decoder.h"
#include <assert.h>
#include <curl/curl.h>
#include <string.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

int
sha1digest(uint8_t *digest, char *hexdigest, const uint8_t *data, size_t databytes);

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if(ptr == NULL) {
        printf("error: not enough memory\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int WaitOnSocket(curl_socket_t sockfd, int for_recv, long timeout_ms) {
    struct timeval tv;
    fd_set infd, outfd, errfd;
    int res;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (int)(timeout_ms % 1000) * 1000;
    FD_ZERO(&infd);
    FD_ZERO(&outfd);
    FD_ZERO(&errfd);
    FD_SET(sockfd, &errfd); /* always check for error */
    if (for_recv) {
        FD_SET(sockfd, &infd);
    } else {
        FD_SET(sockfd, &outfd);
    }
    /* select() returns the number of signalled sockets or -1 */
    res = select((int)sockfd + 1, &infd, &outfd, &errfd, &tv);
    return res;
}

void char_to_ip(unsigned char *ip){

    printf("%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], ip[4]*256+ip[5]);
}


void decode_command(const char* encoded_str){
    struct bencode* decoded_values = decode_bencode(&encoded_str);
    print_bencode(decoded_values);
    printf("\n");
    free(decoded_values);
}

struct MetaInfo decode_meta_info(char* filename){

    FILE* fptr = fopen(filename, "r");
    if (fptr == NULL) {
        printf("Could not open file %s", filename);
        exit(1);
    }

    const char* encoded_str = malloc(2048);
    size_t read_len = fread((void*)encoded_str,1, 2048, fptr);
    assert(read_len < 2048);
    fclose(fptr);

    struct bencode* decoded_values = decode_bencode(&encoded_str);
    struct MetaInfo meta_info;

    meta_info.decoded_value = decoded_values;

    struct bencode* info = search_dict(decoded_values, INFO);
    meta_info.tracker_url = search_dict(decoded_values, ANNOUNCE)->str_value->value;

    char *encoded_info = malloc(2048);

    int len=0;
    encode_bencode(info, encoded_info, &len);
    unsigned char *sha_value = malloc(20);
    unsigned char* uencoded_info = to_unsigned_char(encoded_info, len);

    sha1digest(sha_value, NULL, uencoded_info, len);
    meta_info.info_hash = sha_value;

    free(encoded_info);
    free(uencoded_info);

    free((char *)encoded_str-read_len);
    return meta_info;
}

void info_command(struct MetaInfo meta_info){

//    struct MetaInfo meta_info = decode_meta_info(filename);
    struct bencode* decoded_values = meta_info.decoded_value;
    char * tracker_url = meta_info.tracker_url;

    struct bencode* info = search_dict(decoded_values, INFO);
    struct bencode* piece_hashes = search_dict( info, "pieces");

    long length = search_dict( info, LENGTH)->int_value;
    long piece_length = search_dict( info, "piece length")->int_value;

    unsigned char *sha_value = meta_info.info_hash;

    printf("Tracker URL: %s\n", tracker_url);
    printf("Length: %ld\n", length);
    printf("Info Hash: ");
    print2Hex(sha_value, 20);
    printf("Piece Length: %ld\n", piece_length);
    printf("Piece Hashes:\n");

    unsigned char *uhash;
    for(int i=0; i<piece_hashes->str_value->length; i+=20){
        uhash = to_unsigned_char(piece_hashes->str_value->value+i, 20);
        print2Hex(uhash, 20);
        free(uhash);
    }
}

unsigned char * peers_command(struct MetaInfo meta_info){

//    struct MetaInfo meta_info = decode_meta_info(filename);
    struct bencode* decoded_values = meta_info.decoded_value;
    char * tracker_url = meta_info.tracker_url;
    struct bencode* info = search_dict(decoded_values, INFO);

    long length = search_dict( info, LENGTH)->int_value;

    CURL *curl_handle = curl_easy_init();
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    char* url_encoded_hash = curl_easy_escape(curl_handle, (char *)meta_info.info_hash, 20);

    char url_with_parameters[1024];
    sprintf(url_with_parameters, "%s?info_hash=%s&peer_id=%s"
                                               "&port=%d&uploaded=0&downloaded=0&left=%ld"
                                               "&compact=1", tracker_url, url_encoded_hash, PEER_ID, PORT, length);
    free(url_encoded_hash);
    unsigned char* peers_ip=NULL;
    if(curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url_with_parameters);
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl_handle);

        if(res != CURLE_OK) {
            fprintf(stderr, "error: %s\n", curl_easy_strerror(res));
        } else {
            struct bencode* decoded_response = decode_bencode((const char **) &chunk.memory);

            struct bencode* peers = search_dict(decoded_response, "peers");
            peers_ip = to_unsigned_char(peers->str_value->value, peers->str_value->length);
            for(int i=0; i<peers->str_value->length; i+=6){
                char_to_ip(peers_ip+i);
                printf("\n");
            }
            free(decoded_response);
        }
        curl_easy_cleanup(curl_handle);
//        free(chunk.memory);
    }
    return peers_ip;
}

CURL* handshake_command(char* peer_ip_port, struct MetaInfo info){
    CURL *curl_handle = curl_easy_init();
    CURLcode res;

    int curl_socket;
    if(curl_handle==NULL){
        fprintf(stderr, "curl_easy_init() failed\n");
        exit(1);
    }
    curl_easy_setopt(curl_handle, CURLOPT_URL, peer_ip_port);
    curl_easy_setopt(curl_handle, CURLOPT_CONNECT_ONLY, 1L);
    res = curl_easy_perform(curl_handle);
    if(res != CURLE_OK){
        fprintf(stderr, "Couldn't connect to peer %s\n",
                curl_easy_strerror(res));
        exit(1);
    }
    res = curl_easy_getinfo(curl_handle, CURLINFO_ACTIVESOCKET, &curl_socket);
    if (res != CURLE_OK) {
        fprintf(stderr, "[ERROR] Could not get socket from peer\n");
        fprintf(stderr, "[ERROR]: %s\n", curl_easy_strerror(res));
        exit(1);
    }
    struct HandShake message;
    message.protocol_len=19;
    memcpy(message.protocol, PROTOCOL, 19);
    message.reserved=0;
    memcpy(message.info_hash , info.info_hash, 20);
    memcpy(message.peer_id , PEER_ID, 20);

    size_t NSentTotal = 0;
    size_t NSend;
    do {
        NSend = 0;
        res =
                curl_easy_send(curl_handle, (uint8_t *)&message + NSentTotal,
                               sizeof(message) - NSentTotal, &NSend);
        NSentTotal += NSend;
        if (res == CURLE_AGAIN && !WaitOnSocket(curl_socket, 0, 60000L)) {
            fprintf(stderr, "[ERROR] Could not send data to peer \n");
            exit( 1);
        }
    } while (res == CURLE_AGAIN);
    size_t NRead;
    struct HandShake Ack;
    do {
        NRead = 0;
        res = curl_easy_recv(curl_handle, &Ack, sizeof(Ack), &NRead);
        if (res == CURLE_AGAIN && !WaitOnSocket(curl_socket, 1, 60000L)) {
            fprintf(stderr, "[ERROR] TIMEOUT: Could not receive data from peer"
            );
            fprintf(stderr, "[ERROR]: %s\n", curl_easy_strerror(res));
            exit(1);
        }
    } while (res == CURLE_AGAIN);
    if (res != CURLE_OK) {
        fprintf(stderr, "[ERROR] Could not receive data from peer");
        fprintf(stderr, "[ERROR]: %s\n", curl_easy_strerror(res));
        exit(1);
    }
    if (NRead != sizeof Ack)
        perror("ERROR reading from socket");
    printf("Peer ID: ");
    print2Hex( Ack.peer_id, 20);
    return curl_handle;
}

void send_message(CURL *curl_handle,char message_type, int curl_socket, struct MemoryStruct* message, struct PieceInfo* piece_info){
    uint32_t payload_len = message->size+1;
    uint32_t n = htonl( payload_len );
    char send_message[payload_len+5];
    memcpy(send_message, &n, 4);
    send_message[4] = message_type;
    memcpy(send_message+5, message->memory, message->size);
    printf("SENDING: ");
    print2Hex((unsigned char *)send_message, 4+payload_len);
    CURLcode res;
    size_t nsent_total = 0;
    do{
        size_t n_sent = 0;
        res =
                curl_easy_send(curl_handle, send_message + nsent_total,
                               payload_len+4 - nsent_total, &n_sent);
        nsent_total += n_sent;
        if (res == CURLE_AGAIN && !WaitOnSocket(curl_socket, 0, 60000L)) {
            fprintf(stderr, "[ERROR] Could not send data to peer \n");
            exit( 1);
        }
    }
    while(res==CURLE_AGAIN);
    if(res!=CURLE_OK){
        fprintf(stderr, "[ERROR] Could not send data to peer \n");
        exit( 1);
    }

}
void receive_message(CURL* curl_handle, int curl_socket ,struct MemoryStruct* chunk, struct PieceInfo* piece_info);

void handle_received_message(CURL *curl_handle, int curl_socket, struct MemoryStruct *input_message, struct PieceInfo* piece_info){
    switch(*input_message->memory){
        case PIECE:{
            uint32_t index,piece_offset;
            memcpy(&index, input_message->memory+1,4);
            index = ntohl(index);
            assert(index==piece_info->cur_piece_index);

            memcpy(&piece_offset, input_message->memory+5,4);
            piece_offset = ntohl(piece_offset);
            if(piece_info->file_mode){
                piece_offset+=piece_info->cur_piece_index*piece_info->piece_length;
            }
            fseek(piece_info->fp, piece_offset, SEEK_SET);
            fwrite(input_message->memory+9,1, input_message->size-9,piece_info->fp);
            piece_info->cur_piece_offset+=input_message->size-9;
        }
        //unchoke message recieved
        case UNCHOKE:{
            if(piece_info->cur_piece_offset==piece_info->cur_piece_length)
                break;
            char message[12];
            uint32_t digit;
            digit = htonl(piece_info->cur_piece_index);
            memcpy(message, &digit, 4);
            digit = htonl(piece_info->cur_piece_offset);
            memcpy(message+4, &digit, 4);
            int block_length = PIECE_BLOCK_SIZE;
            if(PIECE_BLOCK_SIZE>piece_info->cur_piece_length-piece_info->cur_piece_offset){
                block_length = piece_info->cur_piece_length-piece_info->cur_piece_offset;
            }
            digit = htonl(block_length);
            memcpy(message+8, &digit, 4);
            struct MemoryStruct chunk;
            chunk.memory = malloc(12);
            memcpy(chunk.memory, message, 12);
            chunk.size = 12;
            send_message(curl_handle, REQUEST, curl_socket, &chunk, piece_info);
            free(chunk.memory);
            receive_message(curl_handle, curl_socket, input_message, piece_info);
            break;

        }
        //bitfield message recieved
        case BITFIELD:{
            struct MemoryStruct _chunk;
            _chunk.memory = NULL;
            _chunk.size = 0;
            //send interested message
            send_message(curl_handle, INTERESTED, curl_socket, &_chunk, piece_info);
            receive_message(curl_handle, curl_socket, input_message, piece_info);
            break;
        }

    }

}



void receive_message(CURL* curl_handle, int curl_socket ,struct MemoryStruct* chunk, struct PieceInfo* piece_info){
    CURLcode res;
    uint32_t message_len;
    size_t nread;
    do {
        do {
            res = curl_easy_recv(curl_handle, &message_len, 4, &nread);
            if (nread) {
                if (nread != 4) {
                    fprintf(stderr, "[ERROR] Could not receive message length correctly \n");
                    exit(1);
                }
                break;
            }
        } while (res == CURLE_AGAIN && !WaitOnSocket(curl_socket, 1, 60000L));
    }while(nread==0);
    message_len = ntohl( message_len );
    chunk->size = message_len;
    chunk->memory = realloc(chunk->memory, message_len+1);
    if(!chunk->memory){
        fprintf(stderr, "[ERROR] Could not allocate memory for chunk \n");
        exit( 1);
    }
    chunk->memory[message_len]='\0';
    size_t n_read_total=0;
//    res=CURLE_AGAIN;
    do {
        res = curl_easy_recv(curl_handle, chunk->memory + n_read_total, message_len-n_read_total, &nread);
        n_read_total+=nread;

        if (res == CURLE_AGAIN && !WaitOnSocket(curl_socket, 1, 60000L)) {
            fprintf(stderr, "[ERROR] TIMEOUT: Could not receive data from peer"
            );
            fprintf(stderr, "[ERROR]: %s\n", curl_easy_strerror(res));
            exit(1);
        }
    } while (n_read_total<message_len);

    assert(res==CURLE_OK);
    printf("RECEIVED: ");
//    print2Hex(chunk->memory, chunk->size);
    handle_received_message(curl_handle, curl_socket, chunk, piece_info);

}

bool download_piece_command(char* out_filename, unsigned char * peers, struct MetaInfo meta_info, int piece_num, uint8_t file_mode){

    CURL *curl_handle;
    CURLcode res;
    char ip[30];
    sprintf(ip, "%u.%u.%u.%u:%u", peers[0], peers[1], peers[2], peers[3], peers[4]*256+peers[5]);
    int curl_socket;
    curl_handle = handshake_command(ip, meta_info);
    res = curl_easy_getinfo(curl_handle, CURLINFO_ACTIVESOCKET, &curl_socket);

    if (res != CURLE_OK) {
        fprintf(stderr, "[ERROR] Could not get socket from peer\n");
        fprintf(stderr, "[ERROR]: %s\n", curl_easy_strerror(res));
        exit(1);
    }

    FILE* fptr = fopen(out_filename, "ab");
    if(!fptr){
        char *err_message;
        err_message = strerror(errno);
        fprintf(stderr, "[ERROR] Could not open file for writing :- %s\n", err_message);
        exit( 1);
    }

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.memory[0] = '\0';

    struct bencode* info = search_dict(meta_info.decoded_value, INFO);
    int file_length = search_dict( info, LENGTH)->int_value;
    int piece_length = search_dict(info, "piece length")->int_value;
    int cur_piece_length = piece_length;
    struct bencode* piece_hashes = search_dict(info, "pieces");
    int num_pieces = piece_hashes->str_value->length/20;

    if(piece_num==num_pieces-1){
        cur_piece_length =file_length-piece_num*piece_length;
    }

    struct PieceInfo pieceInfo = {
            .piece_length = piece_length,
            .cur_piece_index = piece_num,
            .cur_piece_offset = 0,
            .cur_piece_length =cur_piece_length,
            .fp = fptr,
            .file_mode = file_mode
    };

    receive_message(curl_handle, curl_socket, &chunk, &pieceInfo);
    free(chunk.memory);
    assert(pieceInfo.cur_piece_offset == pieceInfo.cur_piece_length);
    /**
     * SHA1 Checksum
    */
    int fileoffset = piece_num*piece_length*file_mode;
    char *buffer = malloc(pieceInfo.cur_piece_length);

    fptr = fopen(out_filename, "rb");
    if(!fptr){
        char *err_message;
        err_message = strerror(errno);
        fprintf(stderr, "[ERROR] Could not open file for reading :- %s\n", err_message);
        exit( 1);
    }

    fseek(fptr, fileoffset, SEEK_SET);
    fread(buffer, 1, pieceInfo.cur_piece_length, fptr);
    unsigned char *sha_value = malloc(20);
    sha1digest(sha_value, NULL, (unsigned char *)buffer, pieceInfo.cur_piece_length);
    free(buffer);

    bool ret;
    ret = memcmp(sha_value, piece_hashes->str_value->value+20*piece_num, 20) ==0;

    free(sha_value);
    curl_easy_cleanup(curl_handle);
    fclose(fptr);

    return ret;
    //    printf("Data: size:-%zu %s\n",chunk.size, chunk.memory);

}

void download_command(char* out_filename, unsigned char* peers, struct MetaInfo meta_info) {
    struct bencode *info = search_dict(meta_info.decoded_value, INFO);
    int total_piece = search_dict(info, "pieces")->str_value->length / 20;
    bool piece_downloaded;
    for (int i = 0; i < total_piece; i++) {
        do{
            printf("[INFO] Attempting piece download: %d\n", i);
            piece_downloaded = download_piece_command(out_filename, peers, meta_info, i, 1);
            break;
        }while(!piece_downloaded);
    }
}
