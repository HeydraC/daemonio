el proy1.ini hay que crearlo manualmente, ni idea por qué

es más fácil hacer las cosas si hacen sudo -s para usar una terminal de root
TODO (o bueno, casi todo) hay que hacerlo con permiso de root

Primero hay que hacer
cd /etc
mkdir proyecto_so_1
cd proyecto_so_1

Luego crean el archivo y le ponen los argumentos con cualquier editor de texto

luego, van a donde tengan los archivos del proyecto y hacen
make install
chmod +x init.sh
./init.sh start
(capaz les dice que ya está corriendo)

ahora, si van a /var/log/ deberia estar creado el directorio PROYECTO_1_SO con los .pak.gz dentro
