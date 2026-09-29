#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void remote_add(const char* name, const char* url) {
    FILE* config = fopen(".pit/config", "a");
    if (!config) {
        fprintf(stderr, "error: could not open .pit/config\n");
        return;
    }

    fprintf(config, "remote.%s=%s\n", name, url);
    fclose(config);
    printf("Added remote '%s' -> %s\n", name, url);
}

void remote_list() {
    FILE* config = fopen(".pit/config", "r");

    if (!config) {
        fprintf(stderr, "error: could not open .pit/config\n");
        return;
    }

    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), config)) {
        if (strncmp(line, "remote.", 7) == 0) {
            // print "name -> url"
            char* equals = strchr(line, '=');
            *equals = '\0';
            char* remote_name = line + 7;  // skip "remote."
            char* url = equals + 1;
            url[strcspn(url, "\n")] = '\0';
            printf("%s -> %s\n", remote_name, url);
            found = 1;
        }
    }
    if (!found) printf("no remotes configured\n");
    fclose(config);
}


// returns the URL for a given remote name, caller must free
char* remote_get_url(const char* name) {
    FILE* config = fopen(".pit/config", "r");
    if (!config) return NULL;

    char line[512];
    char key[256];
    snprintf(key, sizeof(key), "remote.%s=", name);

    while (fgets(line, sizeof(line), config)) {
        if (strncmp(line, key, strlen(key)) == 0) {
            char* url = strdup(line + strlen(key));
            url[strcspn(url, "\n")] = '\0';
            fclose(config);
            return url;
        }
    }
    fclose(config);
    return NULL;
}

void pit_remote(int argc, char** argv) {
    if (argc < 2) {
        printf("usage: pit remote <add|list> [name] [url]\n");
        return;
    }

    if (strcmp(argv[1], "add") == 0) {
        if (argc < 4) {
            printf("usage: pit remote add <name> <url>\n");
            return;
        }
        remote_add(argv[2], argv[3]);

    } else if (strcmp(argv[1], "list") == 0) {
        remote_list();

    } else {
        printf("unknown subcommand '%s'\n", argv[1]);
    }
}