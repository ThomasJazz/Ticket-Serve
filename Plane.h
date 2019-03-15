#ifndef Plane_h
#define Plane_h

#include <string>
using namespace std;
class Plane {
public:
    Plane(int, int);
    bool buyTicket(int, int);
    bool isAvailable(int, int) const;
    bool isFull() const;
    char operator()(int,int) const;
    char** getSeatMatrix() const;
    std::string matrixToString() const;
    int getNumRows() const;
    int getNumCols() const;
private:
    int numRows, numCols, numSeats, seatsTaken;
    char** SeatMatrix;
};
#endif