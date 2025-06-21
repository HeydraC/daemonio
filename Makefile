# Este Makefile se provee como ejemplo para su modificacion. Puede utilizarlo
# en su proyecto de forma directa llenando los campos faltantes o puede
# extenderlo segun las necesidades de su solucion.
#
# GDSO 2-2014
# Coloque aqui los nombres de todos los archivos compilados con extension .o
# necesarios para su proyecto.
CFLAGS =

# Punto de entrada para make si se ejecuta sin parametros.
all: fileList

fileList: fileList.o txttt.o
	gcc -o fileList fileList.o txttt.o

%.o: %.c $(wildcard %.h)
	gcc -c $<

# Esta regla sustituye las banderas que se pasan al compilador por banderas
# utiles para depuracion.
debug: CFLAGS = -g -Wall -D_DEBUG
debug: all

# Esta regla permite instalar el proyecto como un servicio de sistema asumiendo
# que el archivo init.sh tenga el formato correcto. Debe ser ejecutada como
# usuario root.
install: fileList
	cp fileList /sbin/fileList
	cp init.sh /etc/init.d/fileList
	chmod 755 /etc/init.d/fileList
	update-rc.d exceptd defaults

# Esta regla desinstala el proyecto. Debe ser ejecutada como usuario root
# despues de haber ejecutado la regla install.
uninstall:
	update-rc.d -f exceptd remove
	$(RM) /sbin/fileList /etc/init.d/fileList

# Esta regla limpia los archivos creados por la compilacion.
clean:
	$(RM) fileList *.o
