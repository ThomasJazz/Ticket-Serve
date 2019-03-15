// Built on MacOS Mojave by Thomas Jaszczult
// Compile with: g++ -o Server Server.cpp Plane.cpp -lpthread
// Run with: ./Server <row> <col>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <pthread.h>

#include "Plane.h"

using namespace std;
// could probably put this in the threadArgs struct as a reference
// variable so it isn't global
Plane* plane; 
pthread_mutex_t mutexL = PTHREAD_MUTEX_INITIALIZER;

void *connection_handling_thread(void*);

struct argList {
    string arg;
    int row, col;
};

struct threadArgs {
    int rows, cols, clientID, clientSocket;
};

bool argParser(string input, argList &argL) {
    stringstream ss;
    
    ss << input;
    try {
        ss >> argL.arg >> argL.row >> argL.col;
    } catch (exception e) {
        cout << "Invalid arguments\n";
        return false;
    }
    return true;
}

string purchaseTicket(int row, int col) {
    string output;
    
    // lock this section before we use shared resource
    pthread_mutex_lock(&mutexL);
    cout << "Mutex locked\n";
    if (plane->isAvailable(row, col)) {
        plane->buyTicket(row, col);

        output = "Successfully purchased ticket for row: " + to_string(row) + ", column: " + to_string(col) + "\n";
    } else {
        if (row > plane->getNumRows() || row < 0 || col > plane->getNumCols() || col < 0) {
            output = "Invalid seat location!\n";
        } else {
            output = "Seat unavailable!\n";
        }
    }
    pthread_mutex_unlock(&mutexL);
    cout << "Mutex unlocked\n";
    // unlock when we're done
    return output;
}

// arguments to run: column row
int main(int argc, char* argv[]) {
    // default plane sizing
    const int DEFAULT_ROWS = 26;
    const int DEFAULT_COLUMNS = 6;
    int rows, cols;
    int connections = 0;

    // array of threads (thread pool), Maximum number of threads is 20
    pthread_t threads[20];

    if (argc < 3) {
        rows = DEFAULT_ROWS;
        cols = DEFAULT_COLUMNS;
        plane = new Plane(rows, cols);
    } else if (argc == 3) {
        if (!isdigit(argv[1][0]) || !isdigit(argv[2][0])){
            cout << "Invalid arguments. Usage: ./Server <rows> <cols>\n";
            return -1;
        }
        rows = atoi(argv[1]);
        cols = atoi(argv[2]);
        plane = new Plane(rows, cols);
    } else {
        cout << "Only 3 arguments allowed. You entered [" << argc << "]\n";
        return -1;
    }

	// Create socket
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock == -1) {
		cerr << "Failed to create socket\n";
		return -1;		
	}
	
	// Socket hint stuff
	sockaddr_in hint;
	hint.sin_family = AF_INET;
    hint.sin_port = htons(54000);
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr);

    // Bind socket to IP and port
    if (bind(listen_sock, (sockaddr*)&hint, sizeof(hint)) < 0) {
        cerr << "Binding to IP/Port failed\n";
        return -2;
    }

	// Mark the socket for listening
    if (listen(listen_sock, SOMAXCONN) == -1) {
        cerr << "Can't listen";
        return -3;
    }
    
    char host[NI_MAXHOST];
    char service[NI_MAXSERV];

    int numThread = 0;
    while (!plane->isFull()) {
        cout << "Listening for connections...\n";

        sockaddr_in client;
        socklen_t clientSize = sizeof(client);
        // accept connections
        int clientSocket = accept(listen_sock, (sockaddr*)&client, &clientSize);

        // if connection failed
        if (clientSocket == -1) {
            cerr << "Failed to connect with client";
            return -4;
        } else {
            cout << "Connection successful\n";
            connections++;
        }

        // set up our arguments to be passed with the thread
        threadArgs tArgs;
        tArgs.rows = rows;
        tArgs.cols = cols;
        tArgs.clientID = connections;
        tArgs.clientSocket = clientSocket;
        pthread_create(&threads[numThread], NULL, connection_handling_thread, (void*) &tArgs);
        
        // 0 out used memory
        memset(host, 0, NI_MAXHOST);
        memset(service, 0, NI_MAXSERV);
        int result = getnameinfo((sockaddr*)&client, 
                                sizeof(client), 
                                host, 
                                NI_MAXHOST, 
                                service,
                                NI_MAXSERV,
                                0);    
        if (result) {
            cout << host << " connected on " << service << endl;
        } else {
            inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
            cout << host << " connected on " << ntohs(client.sin_port) << endl;
        }
        numThread++;
        
        //cout << "Do we reach here?\n";
        if (numThread >= 20) {
            cout << "Too many threads!\n";
            break;
        }
            
    }

    // join threads together
    for (int i = 0; i < numThread; i++) {
        cout << "Joining thread " << pthread_self() << "\n";
        pthread_join(threads[i], NULL);
    }

	return 0;
}

void *connection_handling_thread(void* arguments) {
    // Get all the data from our arguments struct we passed in
    struct threadArgs *args = (struct threadArgs *)arguments;
    int rows = args -> rows;
    int cols = args -> cols;
    int clientID = args -> clientID;
    int clientSocket = args -> clientSocket;

    cout << "Thread No: " << pthread_self() << "\n-----\n";

    // necessary variables for processing
    char buff[4096];
    string custMsg;
    
    custMsg += to_string(rows) + " " + to_string(cols) + "\n";
    int msgSize = strlen(custMsg.c_str())*sizeof(char);
    send(clientSocket, custMsg.c_str(), msgSize+1, 0);

    // Determine what we do when we receieve messages
    bool firstMsg = true;
    while (true) {
        memset(buff, 0, 4096);
        custMsg = "";

        
        int bytesRecv = recv(clientSocket, buff, 4096, 0);
        cout << "Now handling servicing client " << clientID << "\n";
        if (bytesRecv == -1) {
            pthread_mutex_lock(&mutexL); // lock so error messages don't get messed up
            cerr << "There was a connection issue (client " << clientID << ")\n";
            pthread_mutex_unlock(&mutexL);
            break;
        } else if (bytesRecv == 0) {
            pthread_mutex_lock(&mutexL);
            cout << "Client " << clientID << " disconnected" << endl;
            pthread_mutex_unlock(&mutexL);
            break;
        }
        if (bytesRecv > 0)
            cout << "Received: " << string(buff, 0, bytesRecv) << " (client " << clientID << ")\n";

        // do things based on user input
        string inputStr(buff);
        argList args;
        if (argParser(inputStr, args)) {
            if (args.arg == "buy") {
                string purchResult = purchaseTicket(args.row, args.col);
                custMsg += purchResult;
                cout << purchResult << "\n";
            } else {
                custMsg = "To buy a ticket, enter: 'buy <row> <col>'\n";
            }
        } else {
            custMsg = "Invalid argument list";
        }
        
        custMsg += plane->matrixToString();

        // check if our plane is full yet
        bool fullPlane = plane->isFull();
        if (fullPlane){
            custMsg += "Plane has no more empty seats! Exiting...\n";
            cout << "Plane is full. Closing socket.\n";
        }


        int msgSize = strlen(custMsg.c_str())*sizeof(char);
        

        send(clientSocket, custMsg.c_str(), msgSize+1, 0);

        // if the plane is full, we exit
        if (fullPlane) {
            break;
        }
    }
    // Close socket
    cout << "Closing socket " << clientSocket << "\n";
    close(clientSocket);
    return 0;
}