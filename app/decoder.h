//
// Created by Aquib Nawaz on 22/02/24.
//

#ifndef CODECRAFTERS_BITTORRENT_C_DECODER_H
#define CODECRAFTERS_BITTORRENT_C_DECODER_H

#include <stdbool.h>

#define ANNOUNCE "announce"
#define INFO "info"
#define LENGTH "length"

enum bencode_type{
    STRING,
    INT,
    LIST,
    DICT
};

struct dict;
struct list;

struct bencode{
    enum bencode_type type;
    union{
        char* str_value;
        long int_value;
        struct list* list_value;
        struct dict* dict_value;
    };
};

struct dict{
  char **keys;
  struct bencode** values;
  int length;
};

struct list{
    struct bencode** values;
    int length;
};

void free_bencode(struct bencode*);
struct bencode* decode_string(const char** );
struct bencode* decode_int(const char** );
struct bencode* decode_list(const char** );
struct bencode* decode_dict(const char** );
struct bencode* decode_bencode(const char** );

void print_bencode(struct bencode*);

struct bencode* search_dict(struct bencode*, const char*);

bool is_digit(char c);

#endif //CODECRAFTERS_BITTORRENT_C_DECODER_H
