#! /bin/bash
### BEGIN INIT INFO
# Provides: excetpd
# Required-Start: $network $syslog $mysql
# Required-Stop: $network $syslog $mysql
# Default-Start: 2 3 4 5
# Default-Stop: 0 1 6
# Short-Description: A java exception report daemon.
### END INIT INFO

# Sustituya "proy1" por el nombre del ejecutable de su proyecto.
SERVNAME=fileList

start() {
    PID=`pidof $SERVNAME`
    if [ $? -eq 0 ]
    then
	echo "$SERVNAME already started."
	return
    else
	echo "Starting $SERVNAME."
	$SERVNAME &
	echo "Done."
    fi
}

stop() {
    PID=`pidof $SERVNAME`
    if [ $? -eq 0 ]
    then
	echo "Stopping $SERVNAME."
        kill -2 `pidof $SERVNAME`
        echo "Done."
    else
	echo "$SERVNAME already stopped."
    fi
}

case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    status)
	PID=`pidof $SERVNAME`
	if [ $? -eq 0 ]
	then
	    echo "$SERVNAME is running."
	else
	    echo "$SERVNAME is down."
	fi
        ;;
    restart)
        stop
        start
        ;;
    *)
        echo "Usage:  {start|stop|restart|status}"
        exit 1
        ;;
esac
exit $?
