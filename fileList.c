#include <dirent.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <syslog.h>

#include "txttt.h"
#include "daemonizar.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

uint32_t hashing(char *string)
{
  uint32_t hash = 0;

  for (int i = 0; string[i] != '\0'; ++i)
  {
    hash = hash * 31 + string[i];
  }

  return hash;
}

char *ext(const char *filename)
{
  char *ext = strrchr(filename, '.');
  if (ext == NULL || ext == filename)
  {
    return "";
  }
  return ext + 1;
}

int nodirNogz(const struct dirent *file)
{
  if (file->d_type == 4)
    return 0;

  return strcmp(ext(file->d_name), "gz");
}

void commands(struct dirent **files, int dirSize, char checksums[1024][1024],
              bool *changed, int intervalo)
{
  int pipefd[2];
  pid_t pid;
  char buffer[1024];
  ssize_t bytesRead;

  for (int i = 0; i < dirSize; ++i)
  {
    char dest[1024] = "/var/log/";
    // Create pipe
    if (pipe(pipefd) == -1)
    {
      syslog(LOG_ERR, "Error creado el pipe");

      exit(EXIT_FAILURE);
    }

    // Fork child process
    pid = fork();
    if (pid == -1)
    {
      syslog(LOG_ERR, "Error creado el fork");
      exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {                                 // Child process
      close(pipefd[0]);               // Close read end
      dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to write end
      close(pipefd[1]);               // Close original write end
      syslog(LOG_INFO, "Ejecutando el comando md5sum para el archivo: %s",
             files[i]->d_name);
      // Execute command
      execl("/bin/md5sum", "md5sum", strcat(dest, files[i]->d_name),
            NULL); // Example command
      syslog(LOG_ERR, "Error llamdo al execl");
      exit(EXIT_FAILURE);
    }
    else
    {                         // Parent process
      regPid(pid, intervalo); // Registrar el hijo en el txt
      close(pipefd[1]);       // Close write end
      while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
      {
        buffer[bytesRead] = '\0';
        if (strcmp(buffer, checksums[hashing(files[i]->d_name) % 1024]))
          changed[i] = true;

        strcpy(checksums[hashing(files[i]->d_name) % 1024], buffer);
      }
      close(pipefd[0]); // Close read end
      wait(NULL);
    }
  }
}

void initSum(char checksums[1024][1024], int dirSize)
{
  for (int i = 0; i < dirSize; ++i)
  {
    strcpy(checksums[i], "0");
  }
}

void unchange(int dirSize, bool *changed)
{
  for (int i = 0; i < dirSize; ++i)
  {
    changed[i] = false;
  }
}

void gziping(char *fileName, int intervalo)
{
  pid_t pid;

  pid = fork();
  if (pid == -1)
  {
    syslog(LOG_ERR, "fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0)
  { // Child process
    // Execute command
    execl("/bin/gzip", "gzip", fileName, NULL);
    syslog(LOG_ERR, "exec");
    exit(EXIT_FAILURE);
  }
  else{
 	regPid(pid, intervalo); // Registrar el hijo en el txt
  	wait(NULL);
  }
}

void paking(struct dirent **files, int dirSize, bool *changed, int intervalo)
{
  time_t t;
  struct stat sb;
  struct tm *tm_info;
  char hour[32], date[32], size[32],  dir[64], pakdir[64] = "/var/log/PROYECTO_SO_1";
  char *buffer, *toWrite;
  uint64_t fileSize;
  FILE *pak, *content;

  if (stat(pakdir, &sb) == -1){
  	if (mkdir(pakdir, 0700) < 0) syslog(LOG_ERR, "Error creando directorio"); 
  }

  strcat(pakdir, "/");

  time(&t);

  tm_info = localtime(&t);

  strftime(hour, sizeof(hour), "%H:%M:%S", tm_info);
  strftime(date, sizeof(date), "%d-%m-%Y-", tm_info);

  strcat(date, hour);

  strcat(date, ".pak");
  strcat(pakdir, date);

  pak = fopen(pakdir, "wb+");
  if (pak == NULL)
  {
  	syslog(LOG_ERR, "Error abriendo directorio");
    exit(0);
  }

  for (int i = 0; i < dirSize; ++i)
  {
    if (!changed[i])
      continue;

    strcpy(dir, "/var/log/");
    strcat(dir, files[i]->d_name);

    content = fopen(dir, "rb");
    if (content == NULL)
    {
      syslog(LOG_ERR, "Error abriendo archivo");
      exit(0);
    }

    fseek(content, 0, SEEK_END);
    fileSize = ftell(content);
    rewind(content);

    buffer = (char *)malloc(sizeof(char) * fileSize);
    toWrite = (char *)malloc(sizeof(char) * (fileSize + 96));

    fread(buffer, 1, fileSize, content);

    fclose(content);

    strncpy(toWrite, files[i]->d_name, 32);
    snprintf(size, sizeof(size), "%" PRIu64, fileSize);
    strncat(toWrite, size, 32);
    strncat(toWrite, buffer, fileSize);

    //fprintf(pak, "%s%" PRIu64 "%s\n", fileName, fileSize, buffer);
	fwrite(toWrite, 1, fileSize + 96, pak);

    free(buffer);
    free(toWrite);
  }

  fprintf(pak, "%s%" PRIu64, "FIN", (uint64_t)0);

  fclose(pak);

  gziping(pakdir, intervalo);
}

void readProyInit(char *logTag, int *interval, int sideOfLogTag)
{

  FILE *proyInit = fopen("/etc/proyecto_so_1/proy1.ini", "r");
  if (proyInit == NULL)
  {
    syslog(LOG_ERR, "Error abriendo archivo proy1.ini\n");
    exit(EXIT_FAILURE);
  }

  char line[256];

  int seccionConf = 0;

  char clave[100], valor[100];
  while (fgets(line, sizeof(line), proyInit) != NULL)
  {

    if (line[0] == ';' || line[0] == '\0' || line[0] == '\n')
      continue;

    line[strcspn(line, "\n")] = 0;
    if (strcmp(line, "[CONF]") == 0)
    {

      seccionConf = 1;
      continue;
    }
    if (seccionConf != 1)
    {
      syslog(LOG_ERR, "Formato invalido en proy1.ini\n");
      fclose(proyInit);
      exit(EXIT_FAILURE);
    }
    if (sscanf(line, "%[^=] = %[^\n]", clave, valor) == 2)
    {
      if (strcmp(clave, "log_tag") == 0)
      {

        strncpy(logTag, valor, sideOfLogTag - 1);
        logTag[sideOfLogTag - 1] = '\0';
      }
      else if (strcmp(clave, "interval") == 0)
      {

        *interval = atoi(valor);
      }
    }
  }
}

int main()
{
  char logTag[100];
  int intervalInitProy = 0;

  readProyInit(logTag, &intervalInitProy, 100);

  daemonizar();
  
  openlog(logTag, LOG_PID | LOG_CONS, LOG_DAEMON);

  syslog(LOG_INFO, "Empezando el proceso fileList con un intervalo de : %d segundos", intervalInitProy);

  struct dirent **files;
  int dirSize;
  char checksums[1024][1024];
  bool *changed;
  int intervalo = 0;

  syslog(LOG_INFO, "Inicialiazando lo necesario para el txt adicional");
  dirSize = scandir("/var/log", &files, nodirNogz, alphasort);
  inicializaRutaTxt();
  regPadre();

  initSum(checksums, dirSize);

  while (1)
  {
    syslog(LOG_INFO, "Escaneando el directorio /var/log");
    dirSize = scandir("/var/log", &files, nodirNogz, alphasort);
    if (dirSize < 0)
    {
      syslog(LOG_ERR, "Error reading from /var/log");
    }

    // for (int i = 0; i < dirSize; ++i) printf("%"PRIu32"\n",
    // hashing(files[i]->d_name) % 1024);

    changed = (bool *)malloc(sizeof(bool) * dirSize);

    unchange(dirSize, changed);

    commands(files, dirSize, checksums, changed, intervalo);

    paking(files, dirSize, changed, intervalo);

    free(changed);

    intervalo++;
    sleep(intervalInitProy);
  }
  endReg();
  return 0;
}
