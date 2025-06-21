#include "txttt.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int ultimoIntervalo=-1;
char rutaTxt[250];

void inicializaRutaTxt(){
    //obteniendo el root
    
    const char *homedir = NULL;
    homedir=getenv("HOME");
    if (!homedir) {
        struct passwd *pw = getwuid(getuid());
        if (pw) {
            homedir = pw->pw_dir;
        }
    }
    const char* nombreArchivo="31703888_31307754_31708119.txt";
    //concatenando con la ruta
    sprintf(rutaTxt, "%s/%s", homedir, nombreArchivo);
}


void regPadre() {
    
    //abrimos el archivo y escribimos el pid ddel padre
    FILE *fp = fopen(rutaTxt, "w+");
    fprintf(fp, "PID Padre: %d\n", getpid());
    fclose(fp);
}
void regPid(pid_t pid, int intervalo) {
    

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
    fprintf(archivo, "PID: %d\n", pid);
    fclose(archivo);
}

