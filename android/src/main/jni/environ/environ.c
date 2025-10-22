//
// Created by maks on 24.09.2022.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "environ.h"

struct pojav_environ_s *pojav_environ;
__attribute__((constructor)) void env_init() {
    char* strptr_env = getenv("POJAV_ENVIRON");
    if(strptr_env == NULL) {
        printf("[AutoKeyboard/Environ] No environ found\n");
        return;
    }else{
        printf("[AutoKeyboard/Environ] Found existing environ: %s\n", strptr_env);
        pojav_environ = (void*) strtoul(strptr_env, NULL, 0x10);
    }
    printf("[AutoKeyboard/Environ] %p\n", pojav_environ);
}