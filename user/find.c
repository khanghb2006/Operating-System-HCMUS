#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "user/user.h"

// Command to run for each match (argv of "-exec cmd args")
static char **exec_argv;
static int exec_argc;

// Return the last component of a path "a/b/c" -> "c"
static char*
basename(char *path) {
	int len = strlen(path);
	char *ptr;
	for (ptr = path + len; ptr >= path && *ptr != '/'; --ptr);
	return ptr + 1;
} 

// Run "cmd args ... path" in a child process and wait for it to finish
static void
run_exec(char *path) {
	char *argv[MAXARG];

	// find . wc -exec echo hi
	// build the argv like argv = ["echo" , "hi" , "./wc" , 0]
	int i , pid;
	for (i = 0; i < exec_argc; ++i)
		argv[i] = exec_argv[i];
	argv[i++] = path;
	argv[i] = 0;

	pid = fork();

	if (pid < 0) {
		fprintf(2 , "find : fork failed\n");
		return;
	}

	if (pid == 0) {
		exec(argv[0] , argv);
		
		// Reaching this line means exec failed
		fprintf(2 , "find : exec %s failed\n" , argv[0]);
		exit(1);
	}
	wait(0);
}

// Handle a path whose name matched : run the command it
static void 
found(char *path) {
	if (exec_argv)
		run_exec(path);
	else printf("%s\n" , path);
}

// Recursively walk the tree rooted at path , calling found() for every named
static void 
find (char *path , char *name) {
	char buffer[512] , *ptr;
	int fd;
	struct dirent de; // one directory entry 
	struct stat st; // file info : type , size

	if ((fd = open(path , O_RDONLY)) < 0) {
		fprintf(2 , "find : cannot open %s\n" , path);
		return;
	}
	if (fstat(fd , &st) < 0) {
		fprintf(2 , "find : cannot stat %s\n" , path);
		close(fd);
		return;
	}

	// Check the name before descending
	if (strcmp(basename(path) , name) == 0)
		found(path);

	if (st.type == T_DIR) {
		if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buffer)) {
			fprintf(2 , "find : path too long: %s\n" , path);
			close(fd);
			return;
		}

		strcpy(buffer , path);
		ptr = buffer + strlen(buffer);
		*(ptr++) = '/';

		while(read(fd , &de , sizeof(de)) == sizeof(de)) {
			if (de.inum == 0)
				continue;

			// Skip "." and ".." to avoid cycles
			if (strcmp(de.name , ".") == 0 || strcmp(de.name , "..") == 0)
				continue;

			memmove(ptr , de.name , DIRSIZ);
			ptr[DIRSIZ] = 0;
			find(buffer , name);
		}
	}
	close(fd);
}

int
main (int argc , char *argv[]) {
	// find [dir name] [-exec cmd [args...]]
	// [0]   [1]  [2]  [3]    [4] [5...]
	
	if (argc < 3) {
		fprintf(2 , "usage : find dir name [-exec cmd [args...]]\n");
		exit(1);
	}
	
	if (argc > 3) {
		if(strcmp(argv[3] , "-exec") != 0 || argc < 5) {
			fprintf(2 , "usage : find dir name [-exec cmd [args...]]\n");
			exit(1);
		}
		exec_argv = argv + 4; // sub-array "cmd [args...]"
		exec_argc = argc - 4; // number of elements in sub-array

		if (exec_argc + 1 >= MAXARG) {
			fprintf(2 , "find : too many arguments to exec\n");
			exit(1);
		}
	}
	find(argv[1] , argv[2]);
	exit(0);

}