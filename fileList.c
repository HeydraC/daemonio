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

uint32_t hashing(char *string){
  uint32_t hash = 0; //Variable para guardar el hash resultante

  for (int i = 0; string[i] != '\0'; ++i){
    hash = hash * 31 + string[i];
  }

  return hash;
}

char *ext(const char *filename){
  char *ext = strrchr(filename, '.'); //Puntero a la última aparición del caracter .
  if (ext == NULL || ext == filename){ //Si no hay puntos o están al principio del nombre
    return "";
  }
  return ext + 1; //se retorna el resto del string (la extensión)
}

int nodirNogz(const struct dirent *file)
{
  if (file->d_type == DT_DIR) //Si es un directorio
    return 0;

  return strcmp(ext(file->d_name), "gz"); //Retorna diferente de 0 (true) si tiene extensión diferente a gz
}

void commands(struct dirent **files, int dirSize, char checksums[1024][1024], bool *changed, int intervalo){
  int pipefd[2]; //File descriptor para los pipes
  pid_t pid; //Variable para guardar el pid del hijo creado
  char buffer[1024]; //Buffer para guardar el resultado del comando
  size_t bytesRead; //Números de bytes resultantes

  for (int i = 0; i < dirSize; ++i){ //Se itera por todos los archivos de var/log/
    char dest[1024] = "/var/log/";
    if (pipe(pipefd) == -1){ //Por cada archivo se crea un pipe
      syslog(LOG_ERR, "Error creando el pipe");

      exit(EXIT_FAILURE);
    }

    pid = fork(); //Por cada archivo se crea un proceso hijo
    if (pid == -1){
      syslog(LOG_ERR, "Error creando el fork");
      exit(EXIT_FAILURE);
    }

    if (pid == 0){ //Se identifica al hijo
      close(pipefd[0]); //Se cierra la lectura del pipe
      dup2(pipefd[1], STDOUT_FILENO); //Se redirige la salida estandar al lado de escritura del pipe
      close(pipefd[1]); //Se cierra la escritura original del pipe
      syslog(LOG_INFO, "Ejecutando el comando md5sum para el archivo: %s", files[i]->d_name);
      execl("/bin/md5sum", "md5sum", strcat(dest, files[i]->d_name), NULL); //Se ejecuta el comando con el nombre del archivo
      syslog(LOG_ERR, "Error llamando al execl");
      exit(EXIT_FAILURE);
    }
    else{ //Se identifica al padre
      regPid(pid, intervalo); //Se registra el hijo en el txt
      close(pipefd[1]); //Se cierra la escritura al pipe
      while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0){ //Se lee el contenido del pipe
        buffer[bytesRead] = '\0';

        if (strcmp(buffer, checksums[hashing(files[i]->d_name) % 1024])) //Se accede al arreglo de checksums mediante el nombre del archivo
          changed[i] = true; //Si el checksum cambia con respecto a la última revisión se marca

        strcpy(checksums[hashing(files[i]->d_name) % 1024], buffer); //Se copia lo leido al arreglo de checksums para la próxima revisión
      }
      
      close(pipefd[0]); //Se cierra la lectura del pipe
      wait(NULL); //Se espera a que termine el proceso hijo
    }
  }
}

void initSum(char checksums[1024][1024]){
  for (int i = 0; i < 1024; ++i){
    strcpy(checksums[i], "0"); //Se inicializan todos los checksum en 0
  }
}

void unchange(int dirSize, bool *changed){
  for (int i = 0; i < dirSize; ++i){
    changed[i] = false;
  }
}

void gziping(char *fileName, int intervalo){
  pid_t pid; //Variable para guardar el pid del hijo

  pid = fork(); //Se crea el proceso hijo
  if (pid == -1)
  {
    syslog(LOG_ERR, "fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0){ //Se identifica al hijo
    execl("/bin/gzip", "gzip", fileName, NULL); //Se ejecuta el comando con el nombre del .pak como parámetro
    syslog(LOG_ERR, "exec");
    exit(EXIT_FAILURE);
  }
  else{
 	regPid(pid, intervalo); //Se registra el hijo en el txt
  	wait(NULL); //Se espera a que el hijo termine su ejecución
  }
}

void paking(struct dirent **files, int dirSize, bool *changed, int intervalo){
  time_t t;
  struct stat sb;
  struct tm *tm_info;
  char hour[32], date[32],fileName[32], dir[64], pakdir[64] = "/var/log/PROYECTO_SO_1";
  char *buffer, *toWrite;
  uint64_t fileSize;
  FILE *pak, *content;

  if (stat(pakdir, &sb) == -1){ //Se verifica si el directorio existe
  	if (mkdir(pakdir, 0700) < 0) //Si no existe se crea
  		syslog(LOG_ERR, "Error creando directorio"); 
  }

  strcat(pakdir, "/logs"); //Y se agrega el caracter para separar el nombre del archivo

  time(&t);

  tm_info = localtime(&t);

  strftime(hour, sizeof(hour), "%H:%M:%S", tm_info);
  strftime(date, sizeof(date), "%d-%m-%Y-", tm_info);

  strcat(date, hour); //Se obtiene y concadena la fecha y hora actual

  strcat(date, ".pak"); //Se concatena la extensión .pak al nombre
  strcat(pakdir, date); //Se concatena el nombre de el archivo al directorio

  pak = fopen(pakdir, "wb+"); //Se crea y se abre el archivo para escritura binaria
  if (pak == NULL){
  	syslog(LOG_ERR, "Error abriendo directorio");
    exit(0);
  }

  for (int i = 0; i < dirSize; ++i){
    if (!changed[i]) //Si no ha cambiado, no se guarda en el .pak
      continue;

    strcpy(dir, "/var/log/");
    strcat(dir, files[i]->d_name); //Se concatena el nombre del archivo al directorio

    content = fopen(dir, "rb"); //Se abre cada archivo para lectura binaria
    if (content == NULL){
      syslog(LOG_ERR, "Error abriendo archivo");
      exit(0);
    }

    fseek(content, 0, SEEK_END); //Se lleva el cursor de lectura al final del archivo
    fileSize = ftell(content); //Se guarda la posición del cursor, que corresponde al tamaño del archivo
    rewind(content); //Se regresa el cursor principio del archivo

    buffer = (char *)malloc(sizeof(char) * fileSize); //Se reserva memoria para el contenido en bytes del archivo
    toWrite = (char *)malloc(sizeof(char) * (fileSize + 96)); //Se reserva memoria para el contenido del archivo y los 96 caracteres de nombre y tamaño

    fread(buffer, 1, fileSize, content); //Se lee al buffer el contenido del archivo

    fclose(content); //Se cierra el archivo a guardar

    strncpy(fileName, files[i]->d_name, 32); //Se copia el nombre del archivo truncado a 32 caracteres

    snprintf(toWrite, fileSize + 96, "%s%"PRIu64"%s", fileName, fileSize, buffer); //Se concadena el nombre, tamaño y contenido
	fwrite(toWrite, 1, fileSize + 96, pak); //Se guarda el resultado en el .pak

    free(buffer);
	free(toWrite); //Se libera la memoria reservada
 }

  fprintf(pak, "%s%" PRIu64, "FIN", (uint64_t)0); //Se coloca la etiqueta de fin al final del .pak

  fclose(pak); //Se cierra el .pak

  gziping(pakdir, intervalo); //Se comprime el .pak
}

void readProyInit(char *logTag, int *interval, int sideOfLogTag){
  FILE *proyInit = fopen("/etc/proyecto_so_1/proy1.ini", "r"); //Se abre el archivo proy1.ini en modo de lectura
  if (proyInit == NULL){
    syslog(LOG_ERR, "Error abriendo archivo proy1.ini\n");
    exit(EXIT_FAILURE);
  }

  char line[256];

  int seccionConf = 0; //Para identificar si ya se vió [CONF]

  char clave[100], valor[100];
  while (fgets(line, sizeof(line), proyInit) != NULL){ //Se lee cada linea del archivo

    if (line[0] == ';' || line[0] == '\0' || line[0] == '\n') //En caso de estar vacía o contener un ; se ignora
      continue;

    line[strcspn(line, "\n")] = 0; //Se coloca el caracter de final del string
    if (strcmp(line, "[CONF]") == 0){ //Se identifica la linea de conf

      seccionConf = 1;
      continue;
    }
    if (seccionConf != 1){
      syslog(LOG_ERR, "Formato invalido en proy1.ini\n");
      fclose(proyInit);
      exit(EXIT_FAILURE);
    }
    if (sscanf(line, "%[^=] = %[^\n]", clave, valor) == 2){ //Si la linea tiene el formato apropiado
      if (strcmp(clave, "log_tag") == 0){ //Se identifica a qué atributo corresponde

        strncpy(logTag, valor, sideOfLogTag - 1);
        logTag[sideOfLogTag - 1] = '\0'; //Se guarda el logtag a usar
      }
      else if (strcmp(clave, "interval") == 0){
        *interval = atoi(valor); //Se identifica la duración de cada intervalo
      }
    }
  }
}

int main(){
  char logTag[100];
  int intervalInitProy = 0;

  readProyInit(logTag, &intervalInitProy, 100); //Se lee proy1.ini

  daemonizar(); //Se separa el ejecutable de la terminal
  
  openlog(logTag, LOG_PID | LOG_CONS, LOG_DAEMON); //Se abre el log con el logtag recibido

  syslog(LOG_INFO, "Empezando el proceso fileList con un intervalo de : %d segundos", intervalInitProy);

  struct dirent **files; //Arreglo de punteros a struct que contienen información de los archivos en el directorio
  int dirSize; //Número de archivos en el directorio
  char checksums[1024][1024];
  bool *changed;
  int intervalo = 0;

  syslog(LOG_INFO, "Inicialiazando lo necesario para el txt adicional");
  inicializaRutaTxt(); //Se obtiene la ubicación del txt
  regPadre(); //Se registra el pid del proceso padre

  initSum(checksums);

  while (1){ //Se repite mientras el proceso siga vivo
    syslog(LOG_INFO, "Escaneando el directorio /var/log");
    //Se obtienen los archivos del directorio ordenados por nombre, excluyendo subdirectorios y .gz
    dirSize = scandir("/var/log", &files, nodirNogz, alphasort);
    if (dirSize < 0){
      syslog(LOG_ERR, "Error reading from /var/log");
    }
    
    changed = (bool *)malloc(sizeof(bool) * dirSize); //Se inicializa un arreglo de booleandos para identificar archivos cambiados

    unchange(dirSize, changed); //Se inicializa el arreglo a false

    commands(files, dirSize, checksums, changed, intervalo); //Se hace el checksum para identificar archivos cambiados

    paking(files, dirSize, changed, intervalo); //Se empaquetan los archivos cambiados

    free(changed); //Se libera la memoria asignada al arreglo de bool

    intervalo++; //Se pasa al siguiente intervalo
    sleep(intervalInitProy); //Se espera el tiempo indicado en proy1.ini
  }
  
  return 0;
}
