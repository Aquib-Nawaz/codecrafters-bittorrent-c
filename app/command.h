//
// Created by Aquib Nawaz on 24/02/24.
//

#ifndef CODECRAFTERS_BITTORRENT_C_COMMAND_H
#define CODECRAFTERS_BITTORRENT_C_COMMAND_H

void decode_command(const char*);
void info_command(char*);
void peers_command(char* filename);

struct MemoryStruct {
    char *memory;
    size_t size;
};

size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

#endif //CODECRAFTERS_BITTORRENT_C_COMMAND_H
