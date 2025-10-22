//
// Created by maks on 24.09.2022.
//

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "environ.h"
#define TAG __FILE_NAME__

struct pojav_environ_s *pojav_environ;
__attribute__((constructor)) void env_init() {
    char* strptr_env = getenv("POJAV_ENVIRON");
    if(strptr_env == NULL) {
        printf("No environ found\n");
        return;
    }else{
        printf("Found existing environ: %s\n", strptr_env);
        pojav_environ = (void*) strtoul(strptr_env, NULL, 0x10);
    }
    printf("%p\n", pojav_environ);
}