#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

// return the base name of path
// ex: ./a/b -> b
char *basename(char *path) {
    char* name = path;

    for(char* p = path; *p != '\0'; p++) {
        if(*p == '/') 
            name = p + 1;
    }

    return name;
}

void find(char* path, char* target) {
    int fd;
    struct stat st;
    struct dirent de;
    char child[521];

    fd = open(path, O_RDONLY);

    if(fd < 0) {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if((st.type == T_FILE || st.type == T_DIR) && strcmp(basename(path), target) == 0) {
        printf("%s\n", path);
    }

    if(st.type == T_DIR) {
        int length = strlen(path);

        if(length + 1 + DIRSIZ + 1 > sizeof(child)) {
            fprintf(2, "find: path too long\n");
            close(fd);
            return;
        }

        strcpy(child, path);
        child[length] = '/';

        int n;

        while((n = read(fd, &de, sizeof(de))) ==  sizeof(de)) {
            if(de.inum == 0) {
                continue;
            }

            char name[DIRSIZ + 1];
            memmove(name, de.name, DIRSIZ);
            name[DIRSIZ] = '\0';

            if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            strcpy(child + length + 1, name);  // Append entry name after '/'

            find(child, target);
        }

        if(n != 0) {
            fprintf(2, "find: directory read error\n");
        }
    }

    close(fd);
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        fprintf(2, "Usage: find <path> <target>\n");
        exit(1);
    }

    int length = strlen(argv[1]);

    while(length > 1 && argv[1][length - 1] == '/') {
        argv[1][--length] = '\0';
    }

    find(argv[1], argv[2]);

    exit(0);
}