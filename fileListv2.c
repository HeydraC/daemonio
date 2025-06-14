#include<stdio.h>
#include<dirent.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<stdbool.h>
#include<time.h>
#include<stdint.h>

#include "txttt.h"

#define __STDC_FORMAT_MACROS
#include<inttypes.h>

uint32_t hashing(char* string){
	uint32_t hash = 0;

	for(int i = 0; string[i] != '\0'; ++i){
		hash = hash*31 + string[i];
	}

	return hash;
}

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

void commands(struct dirent** files, int dirSize, char checksums[1024][1024], bool* changed, int intervalo){
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
			regPid(pid, intervalo);//Registrar el hijo en el txt
	        close(pipefd[1]); // Close write end
        	while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
           	buffer[bytesRead] = '\0';
			if (strcmp(buffer, checksums[hashing(files[i]->d_name) % 1024])) changed[i] = true;

			strcpy(checksums[hashing(files[i]->d_name) % 1024], buffer);
        }
        close(pipefd[0]); // Close read end
        wait(NULL); // Wait for child process
    	}		
	}
}

void initSum(char checksums[1024][1024], int dirSize){
	for (int i = 0; i < dirSize; ++i){
		strcpy(checksums[i], "0");
	}
}

void unchange(int dirSize, bool* changed){
	for (int i = 0; i < dirSize; ++i){
		changed[i] = false;
	}
}

void gziping(char *fileName){
	pid_t pid;

	pid = fork();
   	if (pid == -1) {
       	perror("fork");
       	exit(EXIT_FAILURE);
   	}

   	if (pid == 0) { // Child process
   		// Execute command
   		execl("/bin/gzip", "gzip", fileName, NULL); // Example command
   		perror("exec");
   	    exit(EXIT_FAILURE);
   	}
}

void paking(struct dirent** files, int dirSize, bool* changed){
	time_t t;
	struct tm *tm_info;
	char hour[32], date[32], fileName[32], dir[64], *buffer, pakdir[40] = "folder/";
	uint64_t fileSize;
	FILE *pak, *content;

	time(&t);

	tm_info = localtime(&t);

	strftime(hour, sizeof(hour), "%H:%M:%S", tm_info);
	strftime(date, sizeof(date), "%d-%m-%Y-", tm_info);
	
	strcat(date, hour);
	
	strcat(date, ".pak");
	strcat(pakdir, date);

	pak = fopen(pakdir, "w+");

	for (int i = 0; i < dirSize; ++i){
		if (!changed[i]) continue;

		strcpy(dir, "/var/log/");
		strcat(dir, files[i]->d_name);
		
		content = fopen(dir, "rb");
		if (content == NULL){
			perror("Error opening file");
			exit(0);
		}
		
		fseek(content, 0, SEEK_END);
   		fileSize = ftell(content);
   		rewind(content);

		buffer = (char*)malloc(sizeof(char) * fileSize);

		fread(buffer, 1, fileSize, content);

		fclose(content);

		strncpy(fileName, files[i]->d_name, 32);
		
		fprintf(pak, "%s%"PRIu64"%s\n", fileName, fileSize, buffer);
		
		free(buffer);
	}

	fprintf(pak, "%s%"PRIu64, "FIN", (uint64_t)0);

	fclose(pak);

	gziping(pakdir);
}

int main(){
	struct dirent** files;
	int dirSize;
	char checksums[1024][1024];
	bool* changed;
	int intervalo=0;
	inicializaRutaTxt(); 
    regPadre();
	initSum(checksums, dirSize);

	while (1){
		dirSize = scandir("/var/log", &files, nodirNogz, alphasort);
		if (dirSize < 0){
			puts("Error reading from /var/log");
		}

		//for (int i = 0; i < dirSize; ++i) printf("%"PRIu32"\n", hashing(files[i]->d_name) % 1024);

		changed = (bool*) malloc(sizeof(bool) * dirSize);

		unchange(dirSize, changed);

		commands(files, dirSize, checksums, changed, intervalo);

		paking(files, dirSize, changed);

		free(changed);
		//Incrementar intervalo para el registro en el txt
		intervalo++;
		sleep(60);
	}
	endReg();
	return 0;
}
