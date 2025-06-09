#include<stdio.h>
#include<dirent.h>
#include<string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

char *ext(const char *filename) {
    char *ext = strrchr(filename, '.');
    if (ext == NULL || ext == filename) {
        return "";
    }
    return ext + 1;
}

int nodirNogz (const struct dirent *file){
	if (file->d_type == 4) return 0;

	return strcmp(ext(file->d_name), "gz");
}

void commands(struct dirent** files, int dirSize){
    int pipefd[2];
    pid_t pid;
    char buffer[1024];
    ssize_t bytesRead;

	for (int i = 0; i < dirSize; ++i){
		char dest[1024] = "/var/log/";
	    // Create pipe
	    if (pipe(pipefd) == -1) {
    	    perror("pipe");
	        exit(EXIT_FAILURE);
    	}
	
    	// Fork child process
	    pid = fork();
    	if (pid == -1) {
        	perror("fork");
        	exit(EXIT_FAILURE);
    	}
	
    	if (pid == 0) { // Child process
        	close(pipefd[0]); // Close read end
        	dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end
        	close(pipefd[1]); // Close original write end
	
        	// Execute command
        	execl("/bin/md5sum", "md5sum", strcat(dest, files[i]->d_name), NULL); // Example command
        	perror("exec");
        	exit(EXIT_FAILURE);
    	} else { // Parent process
	        close(pipefd[1]); // Close write end
        	while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
           	buffer[bytesRead] = '\0';
           	printf("Command %d output:\n%s", i, buffer);
        }
        close(pipefd[0]); // Close read end
        wait(NULL); // Wait for child process
    	}		
	}
}

int main(){
	struct dirent** files;
	int dirSize;

	dirSize = scandir("/var/log", &files, nodirNogz, alphasort);
	if (dirSize >= 0){
		for (int i = 0; i < dirSize; ++i){
			puts(files[i]->d_name);
		}
	}else{
		puts("Error reading from /var/log");
	}

	commands(files, dirSize);

	return 0;
}
