Peter Bennion
CS176A: Intro Computer Networks
Project 1 : TCP/UDP client-server programs

Submission is designed to be run on the computers at
UCSB's CSIL tech lab. Makefile and library inclusions
are for a Linux machine. Test scripts were provided
by the TA and designed specifically for CSIL - mileage
with them may vary.

The first assignment for this course was to design and
implement a set of programs that could interact using
both transport layer protocols. These programs were 
built using C++. Programs were graded when run on the
same computer, so there was only limited testing for
LAN communication.

NOTE: The programs, as they currently stand, are highly
flawed due to the short timeframe and introductary nature
of the project. The TCP program adds unnecessary flags 
that are already present in the protocol. The UDP server 
is inconsistent when recieving packets. I plan to fix
both of these problems if/when I polish up this repo.
