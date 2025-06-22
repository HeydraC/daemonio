#include "daemonizar.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void daemonizar()
{
    pid_t pid;

    // Fork el proceso padre
    pid = fork();
    if (pid < 0)
    {

        exit(-1);
    }
    // El padre termina para devolver el control a la terminal.
    if (pid > 0)
    {
        exit(0);
    }

    // Despues de aqui, solo lo ejecuta el hijo

    // Crear una nueva sesión para desvincularse de la terminal.
    if (setsid() < 0)
    {
        exit(-1);
    }

    // Por si acaso otro fork, es buena practica
    pid = fork();
    if (pid < 0)
    {
        exit(-1);
    }
    if (pid > 0)
    {
        exit(0);
    }

    // Aqui viene el nieto xd

    // Cambiar el directorio de trabajo a la raíz.
    if (chdir("/") < 0)
    {
        exit(-1);
    }

    //  Limpiar la máscara de permisos de archivos.
    umask(0);

    //  Cerrar los descriptores de archivo estándar.
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}