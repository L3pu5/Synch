# Todo

* Write Protocol Header
* Write the Server
* Write command line argument handler

## Server

* Server binds to a port.
* Server uses SSL to wrap the port
* Server requires a folder heirachy to synchronise
* Server performs an inventory of the folder

* Server accepts a client
* Server accepts an inventory
* If the inventory of the client does not match, server sends overwrites
* Server accepts a new inventory
* Server closes the connection 

## Client

* Client connects to a port
* Client performs an inventory of their version of the folder
* Client sends inventory
* Client accepts overwrites
* Client performs and sents a new inventory