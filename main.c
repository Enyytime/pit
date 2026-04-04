#include <stdio.h>
#include <string.h> 
#include <sys/stat.h>
#include "include/hash_object.h"
#include "include/cat_file.h"
#include "include/command_handler.h"


/**
 * @brief Entry point for the pit CLI.
 *
 * Delegates all command parsing to parse_command.
 *
 * @param argc  Argument count
 * @param argv  Argument vector
 * @return      Exit status
 */
int main(int argc, char** argv){

    return parse_command(argc, argv);
    
}