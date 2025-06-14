#include "txttt.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>

int ultimoIntervalo=-1;
char* rutaTxt=NULL;

void inicializaRutaTxt(){
    //obteniendo el root
    struct passwd *pw = getpwuid(getuid());
    const char *homedir = pw->pw_dir;
    const char* nombreArchivo="/31703888_31307754_31708119.txt";
    rutaTxt = malloc(strlen(homedir) + strlen(nombreArchivo) + 2);
    //concatenando con la ruta
    sprintf(rutaTxt, "%s/%s", homedir, nombreArchivo);
}

void regPadre() {
    if (!rutaTxt) inicializaRutaTxt();
    //abrimos el archivo y escribimos el pid ddel padre
    FILE *fp = fopen(rutaTxt, "w");
    fprintf(fp, "PID Padre: %d\n", getpid());
    fclose(fp);
}
void regPid(pid_t pid, int intervalo) {
    if (!rutaTxt) return;

    FILE *archivo = fopen(rutaTxt, "a");
    if (!archivo) {
        return;
    }
    //si estamos en un nuevo intervalo, escribirlo en el archivo
    if (intervalo != ultimoIntervalo) {
        fprintf(archivo, "\nIntervalo %d\n", intervalo);
        ultimoIntervalo = intervalo;
    }
    //escribir pid del hijo en el archivo
    fprintf(archivo, "PID %s: %d\n", pid);
    fclose(archivo);
}

void endReg() {
    if (rutaTxt) {
        free(rutaTxt);
        rutaTxt = NULL;
    }
    ultimoIntervalo = -1;
}