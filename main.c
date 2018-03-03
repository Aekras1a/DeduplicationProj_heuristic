#include <stdio.h>

#include "TextParsingUtilities.h"
#define  OUTPUT_TYPE_CHAR_LOC 15
#define  OUTPUT_NUM_FILE_OBJECTS_LOC 13
#define  OUTPUT_NUM_DIR_OBJECTS_LOC 19
#define  OUTPUT_NUM_BLOC_OBJECTS_LOC 14

int main() {
    /* ------------------------------------------ Define Variables ----------------------------------------- */
    char dedup_type[2];
    dedup_type[1] = '\0';
    char* input_file_path;
    char line[1024];
    int num_roots = 0;
    unsigned long num_file_objects = 0 , num_dir_objects = 0 , num_block_objects = 0, num_phys_file_objects = 0;
    unsigned long file_objects_cnt = 0 , dir_objects_cnt = 0 , root_objects_cnt = 0, block_objects_cnt = 0, phys_file_objects_cnt = 0;
    File* files_array = NULL;
    File* physical_files_array = NULL;
    Dir* dirs_array = NULL;
    Dir* roots_array = NULL;
    Block* blocks_array = NULL;

    int goal_depth = 2;
    /* ------------------------------------------ Define Variables ---------------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* -------------------------------------- Read Global Parameters -------------------------------------- */
    ErrorCode err = readInputParams(&input_file_path);
    if(err != SUCCESS){
        return 0;
    }

    //Open the Input File
    FILE* input_file = fopen(input_file_path , "r");
    if(input_file == NULL){
        printf("-----> Can't open input file/s =[ \n");
        return 0;
    }

    //Read the Output type
    fgets(line , 1024 , input_file);
    if(line[OUTPUT_TYPE_CHAR_LOC] == 'b'){
        dedup_type[0] = 'B';
    } else if (line[OUTPUT_TYPE_CHAR_LOC] == 'f'){
        dedup_type[0] = 'F';
    }

    //Get the number of files that were read - use the input file names line
    fgets(line , 1024 , input_file);
    num_roots = countRootsInInput(line);
    printf("%d\n" , num_roots);

    //Get the number of File objects in the input file
    fgets(line , 1024 , input_file);
    num_file_objects = line[OUTPUT_NUM_FILE_OBJECTS_LOC] - '0';

    //Get the number of Directory objects in the input file
    fgets(line , 1024 , input_file);
    num_dir_objects = line[OUTPUT_NUM_DIR_OBJECTS_LOC] - '0';

    //Get the number of Blocks/Physical Files objects in the input file
    fgets(line , 1024 , input_file);
    if(dedup_type[0] == 'B'){
        num_block_objects = line[OUTPUT_NUM_BLOC_OBJECTS_LOC] - '0';
    }else{
        num_phys_file_objects = line[OUTPUT_NUM_BLOC_OBJECTS_LOC] - '0';
    }

    //Allocate Arrays For Files, Block/Physical Files , Directories and roots
    files_array = malloc(num_file_objects * sizeof(*files_array));
    dirs_array = malloc(num_dir_objects * sizeof(*dirs_array));
    roots_array = malloc(num_roots * sizeof(*roots_array));
    blocks_array = malloc(num_block_objects * sizeof(*blocks_array));
    if(files_array == NULL || dirs_array == NULL || roots_array == NULL || blocks_array == NULL){
        return 0;
    }

    if(dedup_type[0] == 'F'){
        physical_files_array = malloc(num_phys_file_objects * sizeof(*physical_files_array));
        if(physical_files_array == NULL){
            free(files_array);
            free(dirs_array);
            free(roots_array);
            free(blocks_array);
            return 0;
        }
    }

    /* -------------------------------------- Read Global Parameters -------------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* ----------------------------------------- Read Data Objects ---------------------------------------- */
    Dir res_dir = NULL;
    File res_file = NULL;
    Block res_block = NULL;
    fgets(line , 1024 , input_file);
    while(!feof(input_file)) {
        printf("%s" , line);
        switch(line[0]){
            case 'F':
                res_file = readFileLine(line , dedup_type);
                files_array[res_file->file_sn] = res_file;
                file_objects_cnt++;
                break;
            case 'B':
                res_block = readBlockLine(line);
                blocks_array[res_block->block_sn] = res_block;
                block_objects_cnt++;
                break;
            case 'P':
                res_file = readFileLine(line , dedup_type);
                physical_files_array[res_file->file_sn] = res_file;
                phys_file_objects_cnt++;
                break;
            case 'R':
                res_dir = readRootDirLine(line , 'R');
                roots_array[root_objects_cnt] = res_dir;
                dirs_array[res_dir->dir_sn] = res_dir;
                root_objects_cnt++;
                dir_objects_cnt++;
                break;
            case 'D':
                res_dir = readRootDirLine(line , 'D');
                dirs_array[res_dir->dir_sn] = res_dir;
                dir_objects_cnt++;
                break;
            default:
                break;
        }
        fgets(line , 1024 , input_file);
    }

    //TODO Remove this prints later
    printf(" #-#-# The Files array #-#-# \n");
    for( int i=0 ; i<num_file_objects ; i++){
        print_file((files_array[i]));
    }

    printf(" #-#-# The Directories array #-#-# \n");
    for( int i=0 ; i<num_dir_objects ; i++){
        print_dir((dirs_array[i]));
    }

    printf(" #-#-# The Blocks array #-#-# \n");
    for( int i=0 ; i<num_block_objects ; i++){
        print_block((blocks_array[i]));
    }

    //TODO Build the tree hierarchy of the file systems
    if(dedup_type[0] == 'B'){ //Block Level Deduplication
        calculateDepthAndMergeFiles(roots_array, num_roots,
                                    dirs_array, num_dir_objects,
                                    files_array, num_file_objects,
                                    blocks_array, num_block_objects,
                                    physical_files_array, num_phys_file_objects,
                                    'B' , goal_depth);
    } else {//File Level Deduplication
        calculateDepthAndMergeFiles(roots_array, num_roots,
                                    dirs_array, num_dir_objects,
                                    files_array, num_file_objects,
                                    blocks_array, num_block_objects,
                                    physical_files_array, num_phys_file_objects ,
                                    'F' , goal_depth);
    }

    //TODO Do The Heuristic Part


    //TODO Save Output to File

    /* ----------------------------------------- Read Data Objects ---------------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* ---------------------------------------------------------------------------------------------------- */
    /* -------------------------------------- Free all allocated Data ------------------------------------- */
    free(input_file_path);
    fclose(input_file);
    err = freeStructuresArrays(files_array , physical_files_array , dirs_array , blocks_array,
                               num_file_objects , num_dir_objects , num_block_objects,
                               num_phys_file_objects ,dedup_type);
    if(err != SUCCESS){
        return 0;
    }
    //TODO free the blocks/physical files array
//    free(blocks_array);
//    if(dedup_type[0] == 'F'){
//        free(physical_files_array);
//    }
//    //TODO free the Files array
//    free(files_array);
//    //TODO free the Directories array
//    free(dirs_array);
    /* ------------------------------------- Free all allocated Data ------------------------------------ */
    return 0;
}