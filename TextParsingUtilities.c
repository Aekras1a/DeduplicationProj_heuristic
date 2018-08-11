//
// Created by mihush on 17/03/2018.
//

#include "TextParsingUtilities.h"

/*****************************************~ Implementation ~*****************************************/

int countRootsInInput(char* line){
    int roots = 1 , idx = 0;
    if(line == NULL){
        return 0;
    }

    while(line[idx] !='\n'){
        if(line[idx] == ','){
            roots++;
        }
        idx++;
    }

    return roots;
}

File readFileLine(char* line, PMemory_pool memory_pool, Base_Object* base_objects_arr){
    File file = NULL;
    char* file_id;
    unsigned long file_sn = 0;
    unsigned long parent_dir_sn = 0;
    unsigned long num_base_objects = 0;

    char* tok = NULL;
    char sep[2] = ",";

    tok = strtok(line , sep); //reading the flag ("F")
    tok = strtok(NULL , sep); // reading the file_sn
    file_sn = atol(tok);
    file_id = strtok(NULL , sep); // reading the file_id

    //Block Level: F - file_sn     - file_id     - dir_sn    - num_blocks - block1_sn - block1_size - block2_sn - block2_size ....
    //File Level: F - file_sn     - file_id     - dir_sn    - num_blocks - physical1_sn - physical1_size

    // reading parent_sir_sn
    tok = strtok(NULL , sep);
    parent_dir_sn = atol(tok);
    //reading num_base_objects
    tok = strtok(NULL , sep);
    num_base_objects = atol(tok);

    file = file_create(file_sn, file_id, parent_dir_sn,
                       num_base_objects, 0, false, memory_pool);

    unsigned long base_object_sn = 0;
    unsigned int base_object_size = 0;
    for(int i = 0 ; i < num_base_objects ; i++){
        tok = strtok(NULL , sep);
        base_object_sn = atol(tok);
        tok = strtok(NULL , sep);
        base_object_size = atoi(tok);

        // In case we have files which share the same base_object don't need to create him again
        if(base_objects_arr[base_object_sn] == NULL){
            base_objects_arr[base_object_sn] = base_object_create(base_object_sn, base_object_size, memory_pool);
        }
        file->base_objects_arr[i] = base_objects_arr[base_object_sn];
        file->size += base_object_size;
    }

    return file;
}

Base_Object readBaseObjectLine(char *line, File *files_array, PMemory_pool memory_pool , Base_Object* base_objects_arr){
    char* base_object_id = NULL;
    unsigned long base_object_sn = 0;
    unsigned short shared_by_num_files = 0;
    char* tok = NULL;
    char sep[2] = ",";

    tok = strtok(line , sep); //reading the flag ("B/P")
    tok = strtok(NULL , sep); //reading base_object_sn
    base_object_sn = atol(tok);
    base_object_id = strtok(NULL , sep); //reading Block_id

    tok = strtok(NULL , sep); //reading num_of_files
    shared_by_num_files = atoi(tok);

    Base_Object base_object = base_object_update(base_objects_arr[base_object_sn], base_object_id,
                                            shared_by_num_files, memory_pool);
    return base_object;
}

Dir readDirLine(char* line , PMemory_pool memory_pool){
    char* dir_id = NULL;
    unsigned long dir_sn = 0;
    unsigned long parent_dir_sn = 0;
    unsigned long num_of_sub_dirs = 0;
    unsigned long num_of_files = 0;
    char* tok = NULL;
    char sep[2] = ",";

    tok = strtok(line , sep); //reading the flag ("D/R")
    //reading dir_sn
    tok = strtok(NULL , sep);
    dir_sn = atol(tok);

    //reading dir_id
    dir_id = strtok(NULL , sep);

    //reading parent_dir_sn
    tok = strtok(NULL , sep);
    parent_dir_sn = atol(tok);

    //reading num_of_sub_dirs
    tok = strtok(NULL , sep);
    num_of_sub_dirs = atol(tok);

    //reading num_of_files
    tok = strtok(NULL , sep);
    num_of_files = atol(tok);
    Dir directory = dir_create(dir_id , dir_sn , parent_dir_sn , num_of_files , num_of_sub_dirs , memory_pool);

    //reading dir_sn
    unsigned long sub_dir_sn = 0;
    for(int i = 0 ; i < num_of_sub_dirs ; i++){
        tok = strtok(NULL , sep);
        sub_dir_sn = atol(tok);
        //add sub_dir to directory
        add_sub_dir_sn_to_dir(directory, sub_dir_sn, i);
    }

    //reading file_sn
    unsigned long sub_file_sn = 0;
    for(int i = 0 ; i < num_of_files  ; i++){
        tok = strtok(NULL , sep);
        sub_file_sn = atol(tok);
        //add file to directory
        add_file_sn_to_dir(directory, sub_file_sn, i);
    }

    return directory;
}

void add_base_object_to_merge_file(File merged_file, File file_to_insert){
    Base_Object base_object = NULL;
    for(int i = 0 ; i < file_to_insert->num_base_objects ; i++){
        bool object_exists = false, is_first_object = false;
        base_object = (file_to_insert->base_objects_arr)[i];
        if(merged_file->objects_bin_array[(base_object->sn)]){
            object_exists = true;
        }
        if(object_exists == false){ //Check if block exists already - do not increase counter
            // Update correspondingly file_sn at each block contain this file.
            (base_object->files_array_updated)[(base_object->output_updated_idx)] = merged_file->sn;
            (base_object->output_updated_idx)++;

            (merged_file->base_objects_arr)[merged_file->base_object_arr_idx] = base_object;
            if(merged_file->base_object_arr_idx == 0){
                is_first_object = true;
            }
            merged_file->base_object_arr_idx = (merged_file->base_object_arr_idx) + 1;
            merged_file->objects_bin_array[base_object->sn] = true;
        }
        //Check if first physical file and changed id
        if(is_first_object){
            char new_file_id[FILE_ID_LEN];
            sprintf(new_file_id , "MF_");
            strcat(new_file_id , file_to_insert->id);
            strcpy(merged_file->id , new_file_id);
        }
    }
}

void move_files_to_output_array(Dir current_dir , File* files_array , File* output_files_array , unsigned long* output_files_idx){
    // add all of the files to output_files_array
    for(int f = 0 ; f < current_dir->num_of_files  ; f++){
        //Update the sn of the inserted file .
        File curr_file = files_array[(current_dir->files_array)[f]]; // Get file ptr from files array
        curr_file->dir_sn = current_dir->sn;
        // Update file sn with the global output index
        curr_file->sn = *output_files_idx;
        output_files_array[*output_files_idx] = curr_file;
        (*output_files_idx)++;

        //update parent directory with new file sn
        (current_dir->upd_files_array)[current_dir->upd_files_array_idx] = curr_file->sn;
        (current_dir->upd_files_array_idx)++;

        // Update file_sn at each base_object containing this file
        for(int j = 0 ; j < curr_file->num_base_objects ; j++){
            Base_Object curr_object = curr_file->base_objects_arr[j];
            curr_object->files_array_updated[(curr_object->output_updated_idx)] = curr_file->sn;
            (curr_object->output_updated_idx)++;
        }
    }
}

void update_dir_values(Dir current_dir , int goal_depth,
                       Dir* dirs_array, unsigned long num_dirs,
                       File* files_array,  unsigned long num_files,
                       Base_Object* base_object_array, unsigned long num_base_object,
                       File* output_files_array , unsigned long* output_files_idx,
                       Dir* output_dirs_array , unsigned long* output_dirs_idx, int parent_depth,
                       PMemory_pool memory_pool){
    int current_depth = 0;
    int new_sub_dir_sn = 0;
    if(current_dir == NULL){
        return;
    }
    // Check if current dir is a root directory or not 
    if(current_dir->sn == current_dir->parent_dir_sn){
        current_depth = current_dir->depth;
    } else {
        current_dir->depth = (parent_depth + 1);
        current_depth = current_dir->depth;
    }

    // STOP CONDITIONS - stop if you have reached the leaves meaning a folder with no subdirs or files
    if(current_dir->num_of_subdirs == 0){ 
        if(current_dir->num_of_files == 0){
            return;
        } else { //There are still some files in the directory
            if(current_depth <= (goal_depth - 1)){
                move_files_to_output_array(current_dir, files_array, output_files_array, output_files_idx);
            }
            else { // current_depth > (goal_depth -1)
                // Add all the file blocks to the merged file - meaning current_dir->mergedFile
                assert(current_dir->merged_file);
                for(int f = 0 ; f < current_dir->num_of_files  ; f++) {
                    //merge all blocks of files_array[(current_dir->files_array)[f]] to current_dir->merged_file
                    add_base_object_to_merge_file(current_dir->merged_file, files_array[(current_dir->files_array)[f]]);
                }
            }
            return;
        }
    }

    // CALCULATE RECURSION - Get Here ONLY if we have dirs to process !!!!!!!!!
    if(current_depth < (goal_depth - 1)){ //we HAVEN'T Reached the desired depth
        move_files_to_output_array(current_dir, files_array, output_files_array, output_files_idx);
        for(int d = 0 ; d < current_dir->num_of_subdirs ; d++){
            // root dir contains its own sn in the subdirs array
            if((dirs_array[(current_dir->dirs_array)[d]])->sn == (dirs_array[(current_dir->dirs_array)[d]])->parent_dir_sn){
                continue;
            }
            // Add current subdir to output_dirs_array
            output_dirs_array[*output_dirs_idx] = dirs_array[(current_dir->dirs_array)[d]];
            output_dirs_array[*output_dirs_idx]->sn = *output_dirs_idx;
            new_sub_dir_sn = *output_dirs_idx;

            // Update the sn of the subdir in the parent subdir array
            (current_dir->dirs_array)[d] = *output_dirs_idx;
            output_dirs_array[*output_dirs_idx]->parent_dir_sn = current_dir->sn;
            (*output_dirs_idx)++;

            //Update parent directory with the subdirectories new sn
            (current_dir->upd_subdirs_array)[current_dir->upd_subdirs_array_idx] = new_sub_dir_sn;
            (current_dir->upd_subdirs_array_idx)++;

            //For each directory - call update_dir_values
            update_dir_values(output_dirs_array[(current_dir->dirs_array)[d]] ,goal_depth,
                              dirs_array, num_dirs, files_array, num_files,
                              base_object_array, num_base_object, output_files_array, output_files_idx,
                              output_dirs_array, output_dirs_idx, current_depth, memory_pool);

        }
    } else {//current_depth >= (goal_depth - 1) : we have Reached the desired depth
        if (current_depth == (goal_depth - 1)){
            //create new merged file and save it to output_files_array
            current_dir->merged_file = file_create(*output_files_idx , "Sarit_Hadad_12345678912345678", current_dir->sn ,
                                                    num_base_object , 0 , true , memory_pool);
            output_files_array[*output_files_idx] = current_dir->merged_file;
            (*output_files_idx)++;

            //update parent directory with new file sn
            (current_dir->upd_files_array)[current_dir->upd_files_array_idx] = (current_dir->merged_file)->sn;
            (current_dir->upd_files_array_idx)++;
        }

        //merge all child file blocks to the merged file of the parent directory
        assert(current_dir->merged_file);
        for(int f = 0 ; f < current_dir->num_of_files  ; f++){
            //merge all blocks of files_array[(current_dir->files_array)[f]] to current_dir->merged_file
            add_base_object_to_merge_file(current_dir->merged_file, files_array[(current_dir->files_array)[f]]);
        }

        //Update all directory depths : for each directory - call update_dir_values
        for(int j = 0 ; j < current_dir->num_of_subdirs ; j++){
            // In case this is a root directory - don't need to update
            if((dirs_array[(current_dir->dirs_array)[j]])->sn == (dirs_array[(current_dir->dirs_array)[j]])->parent_dir_sn){
                continue;
            }

            dirs_array[(current_dir->dirs_array)[j]]->depth = current_dir->depth + 1; //update dir depth
            dirs_array[(current_dir->dirs_array)[j]]->merged_file = current_dir->merged_file; //update dir merged file - taken from parent
            dirs_array[(current_dir->dirs_array)[j]]->parent_dir_sn = current_dir->sn; //update the directory's parent dir sn

            //For each directory - call update_dir_values
            update_dir_values(dirs_array[(current_dir->dirs_array)[j]] ,goal_depth,
                              dirs_array, num_dirs, files_array, num_files,
                              base_object_array, num_base_object, output_files_array,
                              output_files_idx, output_dirs_array , output_dirs_idx, current_depth, memory_pool);

        }
    }
};

void calculate_depth_and_merge_files(Dir* roots_array, int num_roots,
                                 Dir* dirs_array, unsigned long num_dirs,
                                 File* files_array,  unsigned long num_files,
                                 Base_Object * base_object_array, unsigned long num_base_object, int goal_depth,
                                 File* output_files_array , unsigned long* output_files_idx ,
                                 Dir* output_dirs_array , unsigned long* output_dirs_idx , PMemory_pool memory_pool){

    for(int r = 0 ; r < num_roots ; r++){
        //Set each roots depth to be 0
        roots_array[r]->depth = 0;

        // Add root to output_dirs_array - update the dir_sn from a global param
        output_dirs_array[*output_dirs_idx] = roots_array[r];
        roots_array[r]->sn = *output_dirs_idx;
        roots_array[r]->parent_dir_sn = *output_dirs_idx;
        (*output_dirs_idx)++;

        //add the root to be his own subdirectory
        (roots_array[r]->upd_subdirs_array)[roots_array[r]->upd_subdirs_array_idx] = roots_array[r]->sn;
        (roots_array[r]->upd_subdirs_array_idx)++;

        update_dir_values(roots_array[r] , goal_depth, dirs_array, num_dirs,
                          files_array, num_files, base_object_array, num_base_object,
                          output_files_array , output_files_idx,
                          output_dirs_array , output_dirs_idx, 0, memory_pool);
    }
}

void print_output_csv_header(FILE* results_file , char dedup_type , char* input_files_list, int goal_depth,
                             unsigned long num_files_input , unsigned long num_files_output,
                             unsigned long num_dirs_input , unsigned long num_dirs_output){
    if(dedup_type == 'B'){
        fprintf(results_file ,"# Output type: block-level\n");
    } else if(dedup_type == 'F'){
        fprintf(results_file ,"# Output type: file-level\n");
    }
    //TODO - check the requirements for the header titles
    fprintf(results_file ,"%s" , input_files_list);
    fprintf(results_file ,"# Target Level: %d\n" , goal_depth);
    fprintf(results_file ,"# Run Time: %d\n", -173);
    fprintf(results_file ,"# Memory Usage: ... \n"); //TODO find a way
    fprintf(results_file ,"# Num files in input: %lu\n",num_files_input);
    fprintf(results_file ,"# Num files in output: %lu\n",num_files_output);
    fprintf(results_file ,"# Num directories in input: %lu\n",num_dirs_input);
    fprintf(results_file ,"# Num directories in output: %lu\n",num_dirs_output);
}