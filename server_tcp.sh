#! /bin/sh
set -e
if [ "$#" -ne 3 ] || ! [ -f "$2" ] ; then
    echo "Usage: $0 PORT WELCOMEFILE OUTPUTFILE" >&2
    exit 1
fi
if ! [ -f "server_tcp" ] ; then
    g++ -g -lstdc++ -o server_tcp server_cpp_tcp.cpp
fi

#Start the server with the port ($1), pipe in the welcome ($2)
#Send output to $3
./server_tcp $1 < $2 > $3 2>&1 &

echo $!
#Save its PID so we can kill it after we run the server.

