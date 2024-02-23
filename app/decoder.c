//
// Created by Aquib Nawaz on 22/02/24.
//

#include "decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

struct bencode* decode_string(const char** bencoded_value) {
    if (is_digit(*bencoded_value[0])) {
        int length = atoi(*bencoded_value);
        const char *colon_index = strchr(*bencoded_value, ':');
        if (colon_index != NULL) {
            const char *start = colon_index + 1;
            char *decoded_str = (char *) malloc(length + 1);
            strncpy(decoded_str, start, length);
            decoded_str[length ] = '\0';
            *bencoded_value = start+length;
            struct bencode* ret_value = malloc(sizeof(struct bencode)); // return value
            ret_value->type = STRING;
            ret_value->str_value = decoded_str;
            return ret_value;
        } else {
            fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
            exit(1);
        }
    }
    return NULL;
}

struct bencode* decode_int(const char** bencoded_value) {
    if (*bencoded_value[0] == 'i') {
        const char *end = strchr(*bencoded_value, 'e');
        if (end != NULL) {
            int length = end - *bencoded_value - 1;
            char *decoded_str = (char *) malloc(length + 1);
            snprintf(decoded_str, length + 1, "%.*s", length, *bencoded_value + 1);
            decoded_str[length] = '\0';
            *bencoded_value = end+1;
            struct bencode* ret_value = malloc(sizeof(struct bencode)); // return value
            ret_value->type = INT;
            ret_value->int_value = strtol(decoded_str, NULL,10);
            return ret_value;
        } else {
            fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
            exit(1);
        }
    }
    return 0;
}

struct bencode *decode_list(const char ** bencoded_value) {
    (*bencoded_value)++;
    struct bencode* ret_value = malloc(sizeof (struct bencode));
    ret_value->type =LIST;
    ret_value->list_value = malloc(sizeof (struct list));
    struct bencode* list_values[100];
    int i=0;
    while (i<100&&*bencoded_value[0] != 'e') {
        list_values[i] = decode_bencode(bencoded_value);
        i++;
    }
    ret_value->list_value->length=i;
    ret_value->list_value->values= calloc(i, sizeof(struct bencode*));

    for (int j = 0; j < i; ++j) {
        ret_value->list_value->values[j] = list_values[j];
    }

    if (*bencoded_value[0] == 'e') {
        (*bencoded_value)++;
        return ret_value;
    }
    return NULL;
}

struct bencode* decode_dict(const char** bencoded_value){
    (*bencoded_value)++;
    struct bencode* keys[100];
    struct bencode* values[100];
    int i=0;//, length=3;
    while (i<100&&*bencoded_value[0] != 'e') {
        keys[i] = decode_bencode(bencoded_value);
        values[i] = decode_bencode(bencoded_value);
        i++;
    }
    if(*bencoded_value[0] == 'e') {
        (*bencoded_value)++;
        struct bencode* ret_value = malloc(sizeof (struct bencode));
        ret_value->type =DICT;
        ret_value->dict_value = malloc(sizeof (struct dict));
        ret_value->dict_value->length=i;
        ret_value->dict_value->keys= calloc(i, sizeof(char*));
        ret_value->dict_value->values= calloc(i, sizeof(struct bencode*));
        for (int j = 0; j < i; ++j) {
            assert(keys[j]->type == STRING );
            ret_value->dict_value->keys[j] = keys[j]->str_value;
            ret_value->dict_value->values[j] = values[j];
        }
        return ret_value;
    }
    return NULL;
}

struct bencode* decode_bencode(const char** bencoded_value) {
    if (is_digit(*bencoded_value[0])) {
        return decode_string(bencoded_value);
    }
    else if(*bencoded_value[0]=='i'){
        return decode_int(bencoded_value);
    }
    else if(*bencoded_value[0]=='l'){
        return decode_list(bencoded_value);
    }
    else if(*bencoded_value[0]=='d'){
        return decode_dict(bencoded_value);
    }
    else {
        fprintf(stderr, "Only strings, int, list and dict are supported at the moment\n");
        exit(1);
    }
}

void print_bencode(struct bencode* bencode) {
    switch (bencode->type) {
        case STRING:
            printf("\"%s\"", bencode->str_value);
            break;
        case INT:
            printf("%ld", bencode->int_value);
            break;
        case LIST:
            printf("[");
            for (int i = 0; i < bencode->list_value->length; ++i) {
                print_bencode(bencode->list_value->values[i]);
                if (i < bencode->list_value->length - 1) {
                    printf(",");
                }
            }
            printf("]");
            break;
        case DICT:
            printf("{");
            for (int i = 0; i < bencode->dict_value->length; ++i) {
                printf("\"%s\":", bencode->dict_value->keys[i]);
                print_bencode(bencode->dict_value->values[i]);
                if (i < bencode->dict_value->length - 1) {
                    printf(",");
                }
            }
            printf("}");
            break;
    }
}

void free_bencode(struct bencode* val){
    if(val==NULL){
        return;
    }
    switch (val->type) {
        case STRING:
            free(val->str_value);
            break;
        case INT:
            break;
        case LIST:
            for (int i = 0; i < val->list_value->length; ++i) {
                free_bencode(val->list_value->values[i]);
            }
            free(val->list_value->values);
            free(val->list_value);
            break;
        case DICT:
            for (int i = 0; i < val->dict_value->length; ++i) {
                free(val->dict_value->keys[i]);
                free_bencode(val->dict_value->values[i]);
            }
            free(val->dict_value->keys);
            free(val->dict_value->values);
            free(val->dict_value);
            break;
    }
    free(val);
}

struct bencode* search_dict(struct bencode* dict, const char* value){
    assert(dict->type==DICT);
    for(int i=0; i<dict->dict_value->length; i++){
        if(strcmp(value, dict->dict_value->keys[i])==0)
            return dict->dict_value->values[i];
    }
    return NULL;
}
