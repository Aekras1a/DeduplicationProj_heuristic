#include "TextParsingUtilities.h"
#define  OUTPUT_TYPE_CHAR_LOC 15

int main(int argc , char** argv){
    /* ---------------------------------------------- Define Variables ---------------------------------------------- */
    PMemory_pool mem_pool = calloc(1 , sizeof(Memory_pool));
    memory_pool_init(mem_pool);

    char dedup_type[2];
    dedup_type[1] = '\0';
    int goal_depth = 0;

    char* input_file_path;
    char* line = (char*)memory_pool_alloc(mem_pool , (MAX_LINE_LEN*sizeof(char)));
    int num_roots = 0;
    unsigned long num_file_objects = 0 , num_dir_objects = 0 , num_base_objects = 0;
    unsigned long file_objects_cnt = 0 , dir_objects_cnt = 0 , root_objects_cnt = 0, base_objects_cnt = 0;
    File* files_array = NULL;
    Dir* dirs_array = NULL;
    Dir* roots_array = NULL;
    Base_Object* base_objects_arr = NULL;
    char input_files_list[MAX_LINE_LEN];
    char* tok = NULL;
    char sep[2] = ":";

    //We Will Determine the hastable size of the merged file blocks based on the total amount of blocks in the system
    int merged_file_ht_size = 0;

    bool is_input_file_heuristic = false;
    /* ---------------------------------------------- Define Variables ---------------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* ------------------------------------------- Read Global Parameters ------------------------------------------- */
    if(argc == 1){
        printf("No Extra Command Line Argument Passed Other Than Program Name\n");
        return 0;
    }

    goal_depth = atoi(argv[1]);
    int input_file_path_len = (int)strlen(argv[2]) + 1;
    input_file_path = (char*)memory_pool_alloc(mem_pool , (input_file_path_len * sizeof(char)));
    strcpy(input_file_path, argv[2]);

    //Open the Input File
    FILE* input_file = fopen(input_file_path , "r");
    if(input_file == NULL){
        printf("-----> Can't open input file/s =[ \n");
        return 0;
    }

    //Read the Output type
    fgets(line , MAX_LINE_LEN , input_file);
    if(line[OUTPUT_TYPE_CHAR_LOC] == 'b'){
        dedup_type[0] = 'B';
    } else if (line[OUTPUT_TYPE_CHAR_LOC] == 'f'){
        dedup_type[0] = 'F';
    }

    //Get the number of files that were read - use the input file names line
    fgets(input_files_list , MAX_LINE_LEN , input_file);
    num_roots = countRootsInInput(input_files_list);

    //Get the number of File objects in the input file
    fgets(line , MAX_LINE_LEN , input_file);
    // Check if  this is a heuristic Output File:
    //                - It its heuristic it will be :      # Target depth: 5
    //                - Otherwise it will be :             # Num files: 36
    if(line[2] == 'T'){
        is_input_file_heuristic = true;
        printf("---> For your INFORMATION : You are executing the heuristic code with heuristic input .....  \n" );
        fgets(line , MAX_LINE_LEN , input_file); //read next relevant line
    }

    tok = strtok(line , sep);
    tok = strtok(NULL , sep); //get the number with leading space
    num_file_objects = (unsigned long)atol(tok);

    //Get the number of Directory objects in the input file
    fgets(line , MAX_LINE_LEN , input_file);
    tok = strtok(line , sep);
    tok = strtok(NULL , sep); //get the number with leading space
    num_dir_objects = (unsigned long)atol(tok);

    //Get the number of Blocks/Physical Files objects in the input file
    fgets(line , MAX_LINE_LEN , input_file);
    tok = strtok(line , sep);
    tok = strtok(NULL , sep);
    num_base_objects = (unsigned long)atol(tok);

    //Allocate Arrays For Files, Base objects , Directories and Roots
    files_array = memory_pool_alloc(mem_pool , (num_file_objects * sizeof(*files_array)));
    dirs_array = memory_pool_alloc(mem_pool , (num_dir_objects * sizeof(*dirs_array)));
    roots_array = memory_pool_alloc(mem_pool , (num_roots * sizeof(*roots_array)));
    base_objects_arr = memory_pool_alloc(mem_pool , (num_base_objects * sizeof(*base_objects_arr)));
    if(files_array == NULL || dirs_array == NULL || roots_array == NULL || base_objects_arr == NULL){
        printf("----------> Fail to allocate arrays for objects from input file\n");
        memory_pool_destroy(mem_pool);
        free(mem_pool);
        return 0;
    }
    for(int i = 0; i < num_base_objects ; i++){
        base_objects_arr[i] = NULL;
    }

    //Determine Merged File HashTable Size
    merged_file_ht_size = determine_Merged_File_Base_Object_HT_Size(num_base_objects , dedup_type[0] , goal_depth);
    printf("---> Merged File Hashtable size is: %d \n" , merged_file_ht_size);
    printf("---> Number of Base Objects  is: %lu \n" , num_base_objects);
    /* ------------------------------------------- Read Global Parameters ------------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* --------------------------------------------- Read Data Objects ---------------------------------------------- */
    Dir dir = NULL;
    File file = NULL;
    Base_Object base_object = NULL;
    fgets(line , MAX_LINE_LEN , input_file);
    while(!feof(input_file)) {
        switch(line[0]){
            case 'F': //This Lines can be too long for the buffer
                file = readFileLine(input_file, line, mem_pool, base_objects_arr , merged_file_ht_size);
                files_array[file->sn] = file;
                file_objects_cnt++;
                break;
            case 'B':
            case 'P': //This Lines Shouldn't be extremely long
                base_object = readBaseObjectLine(line, mem_pool, base_objects_arr);
                base_objects_arr[base_object->sn] = base_object;
                base_objects_cnt++;
                break;
            case 'R':
                dir = readDirLine(input_file, line, mem_pool);
                roots_array[root_objects_cnt] = dir;
                dirs_array[dir->sn] = dir;
                root_objects_cnt++;
                dir_objects_cnt++;
                break;
            case 'D':
                dir = readDirLine(input_file, line, mem_pool);
                dirs_array[dir->sn] = dir;
                dir_objects_cnt++;
                break;
            default:
                break;
        }
        fgets(line , MAX_LINE_LEN , input_file);
    }

    /* --------------------------------------------- Read Data Objects ---------------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* --------------------------------- Define Output Directories and Files arrays --------------------------------- */

    File* output_files_array = memory_pool_alloc(mem_pool , (num_file_objects * sizeof(*output_files_array)));
    if(output_files_array == NULL){
        return 0;
    }
    Dir* output_dirs_array = memory_pool_alloc(mem_pool , (num_dir_objects * sizeof(*output_dirs_array)));
    if(output_dirs_array == NULL){
        free(output_files_array);
        return 0;
    }

    unsigned long output_files_idx = 0 , output_dirs_idx = 0;

    /* --------------------------------- Define Output Directories and Files arrays --------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* ------------------------------------- Implement Heuristic on Input Data -------------------------------------- */

    //Build the tree hierarchy of the file systems
    calculate_depth_and_merge_files(roots_array, num_roots, dirs_array, num_dir_objects, files_array,  num_file_objects,
                                    base_objects_arr, num_base_objects, goal_depth, output_files_array,
                                    &output_files_idx, output_dirs_array, &output_dirs_idx , merged_file_ht_size , mem_pool);

    /* ------------------------------------- Implement Heuristic on Input Data -------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* --------------------------------------- Create Output File Name String --------------------------------------- */
    //The format of the File Name will be : P_heuristic_depth3_118_120.csv
    char* temp_output_line = (char*)memory_pool_alloc(mem_pool , (MAX_LINE_LEN*sizeof(char)));
    char* input_file_name = (strrchr(input_file_path , '\\') + 1);
    // char* input_file_name = (strrchr(input_file_path , '/') + 1);

    FILE *results_file = NULL;
    char depth_to_output[8];
    sprintf(depth_to_output, "_depth%d", goal_depth);
    char* output_file_name = calloc(777 , sizeof(char));
    strncpy(output_file_name , input_file_name , 2);
    strcat(output_file_name , "heuristic");
    strcat(output_file_name , depth_to_output);
    if(is_input_file_heuristic == true){ //The input was heuristic output
        input_file_name += 18;
    } else { //The input was not heuristic output
        input_file_name += 7;
    }
    strcpy(output_file_name + 11 + (strlen(depth_to_output)) , input_file_name);
    /* --------------------------------------- Create Output File Name String --------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* ---------------------------------------- Print Objects to Output File ---------------------------------------- */
    // Open the output file
    results_file = fopen(output_file_name , "w+");
    print_output_csv_header(results_file ,dedup_type[0] , input_files_list , goal_depth , output_files_idx ,
                            output_dirs_idx , num_base_objects);
    printf(" #-#-# The OUTPUT Files array #-#-# \n");
    for( int i = 0 ; i < output_files_idx ; i++){ //Print Files to Output CSV
        print_file_to_csv(output_files_array[i] , temp_output_line , results_file);
    }

    printf(" #-#-# The OUTPUT Blocks array #-#-# \n");
    for(int i = 0 ; i < num_base_objects; i++){ //Print Base_object (physichal_file or block) output CSV
        if(dedup_type[0] == 'B'){
            print_base_object_to_csv(base_objects_arr[i] , temp_output_line, 'B' , results_file);
        } else{
            print_base_object_to_csv(base_objects_arr[i] , temp_output_line, 'P' , results_file);

        }
    }

    printf(" #-#-# The OUTPUT Directories array #-#-# \n");
    for( int i = 0 ; i < output_dirs_idx ; i++){ //Print Directories to output CSV
        print_dir_to_csv(output_dirs_array[i], temp_output_line , results_file);
    }

    /* ---------------------------------------- Print Objects to Output File ---------------------------------------- */
    /* -------------------------------------------------------------------------------------------------------------- */
    /* ------------------------------------------- Free all allocated Data ------------------------------------------ */
    fclose(input_file);
    fclose(results_file);
    free(output_file_name);

    //freeing all allocations
    memory_pool_destroy(mem_pool);
    free(mem_pool);

    /* ------------------------------------------- Free all allocated Data ------------------------------------------ */
    return 0;
}
