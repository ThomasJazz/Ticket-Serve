// Built on MacOS Mojave by Thomas Jaszczult
// Compile with: Client.cpp -o Client
// Run with: ./Client <mode> <Server-Info-File>.ini
// The Port my Server.cpp is using is 54000, so be sure to add that
// to your ini file
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <time.h>

using namespace std;

struct serverInfo {
    string ipAddr;
    int portNum;
    int timeout;
};

int getRand(int max) {
    return rand() % max;
}

bool getPlaneInfo(string line, int& rows, int& cols) {
    stringstream ss;
    ss << line;
    try {
        ss >> rows >> cols;
        return true;
    } catch (exception e) {
        cout << "Critical error\n";
        return false;
    }
}

void getServerInfo(ifstream &serverCfg, serverInfo &conn_serv) {
    // variables that we'll read into
    string label, val, eq;
    int i = 0;
    try { // for conversion errors
        while (serverCfg >> label >> eq >> val) {
            if (i == 0)
                conn_serv.ipAddr = val;
            else if (i == 1)
                conn_serv.portNum = stoi(val);
            else if (i == 2)
                conn_serv.timeout = stoi(val);
            else
                break;
            i++;
        }
    } catch (exception e) {
        e.what();
    }
}

// arguments being sent in should be 'automatic' or 'manual' for method of purchasing
// followed by the .ini file containing the server connection info.
int main(int argc, char* argv[]) {
    srand(time(NULL)); // seed random with time
    // we get these int variables from the first server response
    int rows, cols;

    bool AUTOMATIC = false;

    // make sure arguments are present and valid
    if (argc != 3) {
        cout << "Invalid number of arguments. Exiting...\n";
    }
    if (strncmp(argv[1],"automatic", 9) != 0 && strncmp(argv[1],"manual", 6) != 0) {
        cout << "Invlaid arguments! Please use 'manual' or 'automatic'. Exiting...\n";
        return -1;
    }

    // check to see if they want automatic ticket purchasing
    if (strncmp(argv[1], "automatic", 9) == 0) {
        AUTOMATIC = true;
    }

    // Handle file processing in getServerInfo function
    string fileName = argv[2];
    ifstream SERVER_CFG;
    SERVER_CFG.open(fileName);

    // store values from file in conn_info
    serverInfo conn_info;
    if(SERVER_CFG) {
        getServerInfo(SERVER_CFG, conn_info);
    } else {
        cout << "Invalid filename. Exiting...\n";
        return -2;
    }
    SERVER_CFG.close();

    // create socket
    int conn_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (conn_sock < 0) {
        cout << "\nFailed to Create Socket. Exiting...\n";
        return -3;
    }

    // get port and ipAddr information that we read from file
    int port = conn_info.portNum;
    string ipAddr = conn_info.ipAddr;
    
    // make hint
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipAddr.c_str(), &hint.sin_addr);
    
    // try to connect to server socket i times where i is conn_info.timeout
    for (int i = 0; i < conn_info.timeout; i++) {
        int connectVal = connect(conn_sock, (sockaddr*) &hint, sizeof(sockaddr_in));
        if (connectVal < 0 && i >= conn_info.timeout-1) {
            cout << "Failed to connect (" << (i+1) << ")\n";
            cout << "Failed to connect after " << (i+1) << " attempts. Exiting.\n";
            return -4;
        } else if (connectVal == 0) {
            break;
        }
        cout << "Failed to connect (" << (i+1) << ")\n";
    }
    
    // buff will contain server responses
    // userInput contains data we're sending
    char buff[4096];
    string userInput;

    // Send initial greeting message to server
    if (true) {
        userInput = "Greeting the server";
        send(conn_sock, userInput.c_str(), userInput.size() + 1, 0);

        // wait for response
        memset(buff, 0, 4096);
        int bytesReceived = recv(conn_sock, buff, 4096, 0);
        
        // store data from response
        string planeInf(string(buff,bytesReceived));
        if (getPlaneInfo(planeInf, rows, cols)) {
            cout << "Rows: " << rows << ", Columns: " << cols << "\n";
        } else {
            return -5;
        }
    }
    
    do {
        userInput = "";
        
        int sendResult;

        if (AUTOMATIC) {
            int row = getRand(rows);
            int col = getRand(cols);
            userInput = string("buy ") + to_string(row) + " " + to_string(col);
            cout << "Sending request to buy seat " << row << " " << col << "\n";
        } else { // get input if its manual
            cout << "> ";
            getline(cin, userInput);
        }

        // send to server
        sendResult = send(conn_sock, userInput.c_str(), userInput.size() + 1, 0);

        // check if sent successfully
        if (sendResult < 0) { // connection error
            cout << "Failed to send to server\r\n";
            continue;
        }
        // wait for response
        memset(buff, 0, 4096);
        int bytesReceived = recv(conn_sock, buff, 4096, 0);

        // print response
        cout << "Server> " << string(buff, bytesReceived) << "\r\n";


        sleep(1); // REMOVE IF YOU DON'T WANT IT TO TAKE FOREVER FOR PLANE TO FILL

    } while (true);

    // closing socket
    close(conn_sock);
    return 0;
}