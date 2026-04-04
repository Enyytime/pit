#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
#include "include/hash_object.h"
#include "include/cat_file.h"
#include "include/command_handler.h"


int main(int argc, char** argv){

    return parse_command(argc, argv);
    
}