//
// Created by Aquib Nawaz on 22/02/24.
//

#include "decoder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

char* decode_string(const char** bencoded_value) {
    if (is_digit(*bencoded_value[0])) {
        int length = atoi(*bencoded_value);
        const char *colon_index = strchr(*bencoded_value, ':');
        if (colon_index != NULL) {
            const char *start = colon_index + 1;
            char *decoded_str = (char *) malloc(length + 3);
            snprintf(decoded_str, length + 3, "\"%.*s\"", length, start);
            decoded_str[length + 2] = '\0';
            *bencoded_value = start+length;
            return decoded_str;
        } else {
            fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
            exit(1);
        }
    }
    return NULL;
}

char* decode_int(const char** bencoded_value) {
    if (*bencoded_value[0] == 'i') {
        const char *end = strchr(*bencoded_value, 'e');
        if (end != NULL) {
            int length = end - *bencoded_value - 1;
            char *decoded_str = (char *) malloc(length + 1);
            snprintf(decoded_str, length + 1, "%.*s", length, *bencoded_value + 1);
            decoded_str[length] = '\0';
            *bencoded_value = end+1;
            return decoded_str;
        } else {
            fprintf(stderr, "Invalid encoded value: %s\n", *bencoded_value);
            exit(1);
        }
    }
    return NULL;
}

char *decode_list(const char ** bencoded_value) {
    (*bencoded_value)++;
    char * list_values[100];
    int i=0;
    int length = 3;
    while (i<100&&*bencoded_value[0] != 'e') {
        list_values[i] = decode_bencode(bencoded_value);
        length += strlen(list_values[i]);
        i++;
    }
    if(i>0)
        length+=i-1;

    if (*bencoded_value[0] == 'e') {
        (*bencoded_value)++;
        char *ret = malloc(length);
        int j = 0;
        strcat(ret, "[");
        for (; j < i-1; j++) {
            strcat(ret, list_values[j]);
            free(list_values[j]);
            strcat(ret, ",");
        }
        if(i>0) {
            strcat(ret, list_values[j]);
            free(list_values[j]);
        }
        strcat(ret, "]");
        ret[length - 1] = '\0';
        return ret;
    }
    return NULL;
}

char* decode_bencode(const char** bencoded_value) {
    if (is_digit(*bencoded_value[0])) {
        return decode_string(bencoded_value);
    }
    else if(*bencoded_value[0]=='i'){
        return decode_int(bencoded_value);
    }
    else if(*bencoded_value[0]=='l'){
        return decode_list(bencoded_value);
    }
    else {
        fprintf(stderr, "Only strings are supported at the moment\n");
        exit(1);
    }
}