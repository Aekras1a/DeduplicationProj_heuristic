
/********************** ~Includes~ **********************/

#include "ObjectStructures.h"

///* ****************** START ****************** Block STRUCT Functions ****************** START ****************** */

/*
 *  blockCreate - Creates a new Block with:
 *           - a given serial number
 *           - a hashed id
 *           - creates an empty files list
 *           - zeros the counter that contains the amount of files sharing this block
 */
Block block_create(char* block_id , unsigned long block_sn , unsigned int block_size ,
                   unsigned short shared_by_num_files){
    Block block = malloc(sizeof(*block)); //create a block
    if(block == NULL){ //Check memory allocation was successful
        return NULL;
    }

    block->block_id = malloc(sizeof(char)*(BLOCK_ID_LEN + 1)); //allocate string for block_id
    if(block->block_id == NULL){ //check successful allocation
        free(block);
        return NULL;
    }
    block->block_id = strcpy(block->block_id , block_id);

    block->block_sn = block_sn;
    block->shared_by_num_files = 0;
    block->block_size = block_size;

    block->files_array = calloc(shared_by_num_files , sizeof(unsigned long));
    if(block->files_array == NULL){
        free(block->block_id);
        free(block);
        return NULL;
    }
    return block;
}

/*
 *  block_destroy - Destroys and frees space of a block structure
 */
void block_destroy(Block block){
    assert(block);
    free(block->block_id);
    free(block->files_array);
    free(block);
}

/*
 *  block_get_SN - returns the SN of the block
 */
long block_get_SN(Block block){
    assert(block);
    return block->block_sn;
}

/*
 *  block_get_ID - Returns the hashed id of the block
 */
char* block_get_ID(Block block){
    assert(block);
    return block->block_id;
}

/*
 *  block_add_file - adds the file containing the block to the files list saved in the block
 */
ErrorCode block_add_file(Block block , unsigned long file_sn){
    if(block == NULL){ //Check input is valid
        return INVALID_INPUT;
    }

    (block->files_array)[block->shared_by_num_files] = file_sn;
    (block->shared_by_num_files)++;

    return SUCCESS;
}

void print_block(Block block){
    assert(block);
    printf("## Block : %lu\n" , block->block_sn);
    printf("        id : %s\n" , block->block_id);
    printf("      size : %d\n" , block->block_size);
    printf(" num_files : %d\n" , block->shared_by_num_files);
    for(int i = 0 ; i < block->shared_by_num_files ; i++){
        if( i == ((block->shared_by_num_files)-1)){
            printf("%lu\n", block->files_array[i]);
        } else {
            printf("%lu - ", block->files_array[i]);
        }
    }
}
/* *************** START ************** File STRUCT Functions *************** START **************** */
/*
 *  file_create - Creates a new file object with:
 *                      - file id - a hashed id as appears in the input file
 *                      - depth
 *                      -file sn - running index on all files in the filesystem
 *                      - dir sn
 *
 */
File file_create(char* file_id , unsigned long file_sn ,unsigned long parent_dir_sn,
                 unsigned long num_of_blocks , unsigned long num_of_files,
                 unsigned int size , unsigned long physical_sn ,
                 char* dedup_type , char* file_type){
    File file = malloc(sizeof(*file));
    if(file == NULL){
        return NULL;
    }
    file->depth = -1;

    file->file_id = malloc(sizeof(char)* (FILE_ID_LEN + 1));
    if(file->file_id == NULL){
        free(file);
        return NULL;
    }
    file->file_id = strcpy(file->file_id , file_id);
    file->file_sn = file_sn;

    file->ht_blocks = ht_createF();
    if(file->ht_blocks == NULL){
        free(file->file_id);
        free(file);
        return NULL;
    }
    if(strcmp(dedup_type , "B") == 0) { //Block level deduplication
        file->dir_sn = parent_dir_sn;
        file->num_blocks = num_of_blocks;
        file->flag = 'F';
        file->blocks_array = malloc(num_of_blocks*sizeof(struct block_info));
        if(file->blocks_array == NULL){
            //TODO Destroy ht_blocks
            free(file->file_id);
            free(file);
            return NULL;
        }
    }else{
        if(strcmp(file_type , "P") == 0) { //Physical File
            file->shared_by_num_files = num_of_files;
            file->flag = 'P';
            file->files_array = calloc(num_of_files , sizeof(unsigned long));
            if(file->files_array == NULL){
                //TODO Destroy ht_blocks
                free(file->file_id);
                free(file);
                return NULL;
            }
        }else{
            file->dir_sn = parent_dir_sn;
            file->num_blocks = num_of_blocks;
            file->physical_sn = physical_sn;
            file->file_size = size;
            file->flag = 'L';
        }
    }

    return file;
}

/*
 *  file_destroy - Destroys and frees space of a file structure
 */
void file_destroy(File file){
    assert(file);
    free(file->file_id);
    if(file->flag == 'P'){
        free(file->files_array);
    }else if(file->flag == 'F'){
        free(file->blocks_array);
    }
    free(file);
}

/*
 *  file_get_SN - returns the SN of the file
 */
unsigned long file_get_SN(File file){
    assert(file);
    return file->file_sn;
}

/*
 * file_get_ID - returns the hashed ID of the file
 */
char* file_get_ID(File file){
    assert(file);
    return file->file_id;
}

/*
 *  file_get_depth - returns the depth of the file in the hierarchy
 */
int file_get_depth(File file){
    assert(file);
    return file->depth;
}

void file_set_depth(File file, int depth){
    assert(file);
    file->depth = depth;
}

/*
 *  file_get_num_blocks - returns the number of blocks the file contains
 */
int file_get_num_blocks(File file){
    assert(file);
    return file->num_blocks;
}

/*
 *
 */
ErrorCode file_add_block(File file , unsigned long block_sn , int block_size){
    if(file == NULL || block_size < 0){
        return INVALID_INPUT;
    }

    Block_Info bi = malloc(sizeof(*bi));
    if(bi == NULL){
        return OUT_OF_MEMORY;
    }

    bi->block_sn =  block_sn;
    bi->size = block_size;

    (file->blocks_array)[file->num_blocks] = bi;
    (file->num_blocks)++;

    return SUCCESS;
}

/*
 *
 */
ErrorCode file_add_logical_file(File file , unsigned long logical_files_sn){
    if(file == NULL){
        return INVALID_INPUT;
    }

    (file->files_array)[file->shared_by_num_files] = logical_files_sn;
    (file->shared_by_num_files)++;

    return SUCCESS;
}

/*
 *
 */
void file_add_merged_block(File file , Block_Info bi){
    assert(file);

    ht_setF(file->ht_blocks , bi);
    (file->num_blocks)++;
    return;
}

/*
 *
 */
void print_file(File file){
    assert(file);
    if(file->flag == 'F'){
        printf("## File : %lu\n" , file->file_sn);
    } else if (file->flag == 'P'){
        printf("## Physical File : %lu \n", file->file_sn);
    } else if(file->flag == 'L'){
        printf("## Logical File : %lu \n", file->file_sn);
    }
}
/* *************** START *************** Directory STRUCT Functions *************** START *************** */
/*
 * dir_create - Creates a new Directory object with:
 *                      - dir_id
 *                      - dir_sn
 *                      - depth
 */
Dir dir_create(char* dir_id , unsigned long dir_sn, unsigned long parent_dir_sn ,
               unsigned long num_of_files , unsigned long num_of_sub_dirs){
    Dir dir = malloc(sizeof(*dir));
    if(dir == NULL){
        return NULL;
    }
    dir->depth = -1;

    dir->dir_id = malloc((sizeof(char)*DIR_NAME_LEN));
    if(!(dir->dir_id)){
        free(dir);
        return NULL;
    }
    dir->dir_id = strcpy(dir->dir_id , dir_id);

    dir->dir_sn = dir_sn;
    dir->num_of_files = num_of_files;
    dir->num_of_subdirs = num_of_sub_dirs;
    dir->parent_dir_sn = parent_dir_sn;

    dir->files_array = calloc(num_of_files , sizeof(unsigned long));
    if(dir->files_array== NULL){
        free(dir->dir_id);
        free(dir);
        return NULL;
    }
    dir->dirs_array = calloc(num_of_sub_dirs , sizeof(unsigned long));
    if(dir->dirs_array == NULL){
        free(dir->files_array);
        free(dir->dir_id);
        free(dir);
        return NULL;
    }

    dir->merged_file = NULL;

    return dir;
}


/*
 * dir_destroy - Destroy struct of Directory
 */
void dir_destroy(Dir dir){
    assert(dir);
    free(dir->dir_id);
    free(dir->dirs_array);
    free(dir->files_array);
    free(dir);
}

/*
 * dir_get_SN - Return the sn of directory
 */
unsigned long dir_get_SN(Dir dir){
    assert(dir);
    return dir->dir_sn;
}

/*
 * dir_get_ID - Return the ID of directory
 */
char* dir_get_ID(Dir dir){
    assert(dir);
    return dir->dir_id;
}

/*
 * dir_get_depth - Return the depth of the directory
 */
unsigned int dir_get_depth(Dir dir){
    assert(dir);
    return dir->depth;
}

/*
 * dir_set_depth - updates the depth of the directory
 */
void dir_set_depth(Dir dir , int depth){
    assert(dir);
    dir->depth = depth;
}

/* Adding file into the directory */
ErrorCode dir_add_file(Dir dir , unsigned long file_sn){
    if(dir == NULL || file_sn < 0){
        return INVALID_INPUT;
    }
    (dir->files_array)[dir->num_of_files] = file_sn;
    (dir->num_of_files)++;

    return SUCCESS;
}

/* Adding sub_dir into the directory */
ErrorCode dir_add_sub_dir(Dir dir , unsigned long dir_sn){
    if(dir == NULL || dir_sn < 0){
        return INVALID_INPUT;
    }
    (dir->files_array)[dir->num_of_subdirs] = dir_sn;
    (dir->num_of_subdirs)++;

    return SUCCESS;
}

void print_dir(Dir dir){
    assert(dir);
    printf("## Directory : %lu \n" , dir->dir_sn);
}
/* **************** END **************** Directory STRUCT Functions **************** END **************** */
/* ****************************************** Function Declarations ******************************************** */

//#endif //DEDUPLICATIONPROJ_HEURISTIC_OBJECTSTRUCTURES_H