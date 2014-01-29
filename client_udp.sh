#! /bin/sh
set -e
if [ "$#" -ne 4 ] || ! [ -f "$3" ] ; then
    echo "Usage: $0 PORT USERNAME CLIENTINPUT OUTPUTFILE" >&2
    exit 1
fi
if ! [ -f "client_udp" ] ; then
    g++ -g -lstdc++ -o client_udp client_cpp_udp.cpp
fi

#Start the client with the port ($1), username ($2) and pipe in the commands ($3)
#Send output to $4
./client_udp localhost $1 $2 < $3 > $4 2>&1

