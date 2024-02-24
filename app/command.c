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

void char_to_ip(unsigned char *ip){

    printf("%u.%u.%u.%u:%u", ip[0], ip[1], ip[2], ip[3], ip[4]*256+ip[5]);
}


void decode_command(const char* encoded_str){
    struct bencode* decoded_values = decode_bencode(&encoded_str);
    print_bencode(decoded_values);
    printf("\n");
    free(decoded_values);
}

void info_command(char* filename){
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
    char * tracker_url = search_dict(decoded_values, ANNOUNCE)->str_value->value;
    struct bencode* info = search_dict(decoded_values, INFO);
    struct bencode* piece_hashes = search_dict( info, "pieces");

    char *encoded_info = malloc(2048);
    int len=0;
    encode_bencode(info, encoded_info, &len);
    long length = search_dict( info, LENGTH)->int_value;
    long piece_length = search_dict( info, "piece length")->int_value;

    unsigned char sha_value[20];
    unsigned char* uencoded_info = to_unsigned_char(encoded_info, len);


    sha1digest(sha_value, NULL, uencoded_info, len);

    free(encoded_info);
    free(uencoded_info);

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
    free(decoded_values);
}

void peers_command(char* filename){
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
    char * tracker_url = search_dict(decoded_values, ANNOUNCE)->str_value->value;
    struct bencode* info = search_dict(decoded_values, INFO);
    struct bencode* piece_hashes = search_dict( info, "pieces");

    char *encoded_info = malloc(2048);
    int len=0;
    encode_bencode(info, encoded_info, &len);
    long length = search_dict( info, LENGTH)->int_value;
    long piece_length = search_dict( info, "piece length")->int_value;

    unsigned char sha_value[20];
    unsigned char* uencoded_info = to_unsigned_char(encoded_info, len);
    sha1digest(sha_value,NULL, uencoded_info, len);

    free(encoded_info);
    free(uencoded_info);

    CURL *curl_handle = curl_easy_init();
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    char* url_encoded_hash = curl_easy_escape(curl_handle, sha_value, 20);

    char url_with_parameters[1024];
    char peer_id[] = "22101999935711102001";
    sprintf(url_with_parameters, "%s?info_hash=%s&peer_id=%s"
                                               "&port=6881&uploaded=0&downloaded=0&left=%ld"
                                               "&compact=1", tracker_url, url_encoded_hash, peer_id, length);
    free(url_encoded_hash);

    if(curl_handle) {
        curl_easy_setopt(curl_handle, CURLOPT_URL, url_with_parameters);
        curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl_handle);

        if(res != CURLE_OK) {
            fprintf(stderr, "error: %s\n", curl_easy_strerror(res));
        } else {
            struct bencode* decoded_response = decode_bencode(&chunk.memory);

            struct bencode* peers = search_dict(decoded_response, "peers");
            unsigned char* peers_ip = to_unsigned_char(peers->str_value->value, peers->str_value->length);
            for(int i=0; i<peers->str_value->length; i+=6){
                char_to_ip(peers_ip+i);
                printf("\n");
            }
            free(peers_ip);
            free(decoded_response);
        }
        curl_easy_cleanup(curl_handle);
//        free(chunk.memory);
    }
    free(decoded_values);
}