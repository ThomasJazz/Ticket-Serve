#include "Plane.h"

Plane::Plane(int row, int col) {
    numRows = row;
    numCols = col;

    // made numSeats and seatsTaken so that I don't have to manually check the
    // array of the plane in every iteration. Should be much more efficient
    numSeats = row * col;
    seatsTaken = 0;

    SeatMatrix = new char*[numRows];

    for (int i = 0; i < numRows; i++) {
        SeatMatrix[i] = new char[numCols];
        for (int j = 0; j < numCols; j++) {
            // fill our plane with empty seats
            SeatMatrix[i][j] = '.';
        }
    }
}

bool Plane::buyTicket(int row, int col) {
    if (isAvailable(row,col)) {
        seatsTaken++;
        SeatMatrix[row][col] = 'X';
        return true;
    } else {
        return false;
    }
}

bool Plane::isAvailable(int row, int col) const {
    if (row < numRows && row >= 0 && col < numCols && col >= 0)
        return (SeatMatrix[row][col] == '.');
    else
        return false;
}

bool Plane::isFull() const {
    return (seatsTaken >= numSeats);
}

string Plane::matrixToString() const {
    char** tempMatrix = SeatMatrix;

    string seats = "";
    for (int i = 0; i < numRows; i++) {
        seats += tempMatrix[i];
        seats += "\n";
    }
    
    return seats;
}

char** Plane::getSeatMatrix() const {
    return SeatMatrix;
}

int Plane::getNumRows() const {
    return numRows;
}

int Plane::getNumCols() const {
    return numCols;
}
// () operator override
char Plane::operator()(int row, int col) const {
    return SeatMatrix[row][col];
}