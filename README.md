# Socket_in_C

In this repository you will find an architecture client-server based, in which a client is requesting some expenses to the server, which has a txt file with a list of expenses.
The client is implemented in Java.
The server behavior is implemented in C.

File you will find here:

-rxb (.c, .h) -> Server needs them to allocate, de-allocate, read, write using buffers
-utils(.c, .h) -> Server will use this to send files in a non-standard way.

-main_server.c -> logic of the server

-clientjava.java -> client implemented in java

expenses/cancelleria.txt -> the expenses in the file txt (emulating file system of server)

Furthermore, you will find a lot of pdf files with other interactions client server, and the client will be implemented here in both JAVA and C .
