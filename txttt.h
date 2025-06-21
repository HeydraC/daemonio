#ifndef TXTTT_H
#define TXTTT_H

#include <sys/types.h>
#include <unistd.h>

extern int ultimoIntervalo;
extern char rutaTxt[250];

void inicializaRutaTxt();


void regPadre();


void regPid(pid_t pid, int intervalo);


#endif
