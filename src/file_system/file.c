#include "file.h"
#include "../kernel/config.h"
#include "../libc/stdio/stdio.h"
#include "../libc/string/string.h"
#include "../libc/stdlib/stdlib.h"
#include "../drivers/screen/vga.h"
#include "../drivers/screen/terminal.h"
#include "fat/fat16.h"

#include <stddef.h>

struct file_system *file_systems[MAX_FILE_SYSTEMS];

struct file_desc *file_descs[MAX_FILE_DESCS];

// iterate over file system table and find a free slot
static struct file_system **find_free_fs(void){
    for(size_t i = 0; i < MAX_FILE_SYSTEMS; i++){
       if(file_systems[i] == 0){
            return &file_systems[i];
        } 
    }
    return 0;
}

void add_file_system(struct file_system *fs){
   struct file_system **fs_slot = find_free_fs();
    if(!fs || !fs_slot){
        //TODO: kernel panic
        printf("%CPANIC",red);
        while(1){}
    }
    *fs_slot = fs;
}

static void load_present_file_systems(void){
    add_file_system(fat16_init());   
}

void file_system_init(uint8_t verbose){
    memset(file_descs, 0, sizeof(file_descs)); 
    memset(file_systems, 0, sizeof(file_systems));
    load_present_file_systems();    
    if(verbose){
        printf("file system init");
        init_OK();
    }
}

int32_t file_desc_init(struct file_desc **desc_out){
    int32_t rs = -FILE_HANDLER_OVERFLOW;
    // iterate through file descriptors(open file) array and return first free one
    for(size_t i = 0; i< MAX_FILE_DESCS; i++){
        if(file_descs[i] == 0){
            struct file_desc *desc = calloc(sizeof(struct file_desc)); 
            desc->idx = i+1; //descriptor indexes start at 1
            file_descs[i] = desc;
            *desc_out = desc;
            rs = OK;
            break;
        }
    }
    return rs;
}

static struct file_desc *get_desc(uint32_t id){
    if(id <= 0 || id > MAX_FILE_DESCS){
        return 0;
    }
    return file_descs[id - 1];
}

struct file_system *fs_resolve(struct disk_t *disk){
    struct file_system *fs = 0;
    for(size_t i = 0; i < MAX_FILE_SYSTEMS; i++){
        // printf("%d\t",file_systems[i]->resolve_fn(disk));
       if(file_systems[i] != 0 && file_systems[i]->resolve_fn(disk) == 0){
            // check if one of the kernel's file system can resolve the given disk
            // if one can bind them together
            fs = file_systems[i];
            break;
        } 
    }
    return fs;
}
