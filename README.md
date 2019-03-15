# Ticket-Serve
Client-server application to simulate purchasing airplane tickets. Made for my operating systems class.

Built on MacOS Mojave by Thomas Jaszczult
### Server Application
Compile with: ````g++ -o Server Server.cpp Plane.cpp -lpthread````

Run with: ````./Server <row> <col>````

### Client Application
Compile with: ````Client.cpp -o Client````

Run with: ````./Client <mode> <Server-Info-File>.ini````

The port that my Server.cpp is using is 54000, so be sure to add that to your ini file or use the one provided in this repository
