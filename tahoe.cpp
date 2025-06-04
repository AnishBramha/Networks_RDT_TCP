#include <iostream>
#include <random>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <vector>
#include <thread>
#include <cstdlib>
#include <cstdint>
#include <chrono>

using namespace std;

#define GAP this_thread::sleep_for(chrono::seconds(2))

int randomInRange(int a, int b) {

    return min(a, b) + (rand() % (max(a, b) - min(a, b) + 1));
}

void CSVCreate(void) {

    ofstream fout("tahoe.csv", ios::trunc);

    fout << "TR,CWND" << endl;

    fout.close();

    return;
}

void CSVWrite(int rtt, int cwnd) {

    ofstream fout("tahoe.csv", ios::app);

    fout << rtt << "," << cwnd << endl;

    fout.close();

    return;
}

void simulation(void) {

    int cwnd = 1; // initialise congestion window size

    int ssthresh = randomInRange(10, 15); // start with a random threshold

    vector<int> loss_pt = {8, 16, 22}; // predefined loss intervals

    for (int rtt = 1; rtt <= 30; rtt++) { // run for at most 30 transmission rounds

        cout << "SSTHRESH: " << ssthresh << " | CWND: " << cwnd << endl; // log output

        CSVWrite(rtt, cwnd); // log values

        if (find(loss_pt.begin(), loss_pt.end(), rtt) != loss_pt.end()) { // if predefined interval reached

            cout << endl << "PACKET LOST" << endl << endl;

            GAP;

            cwnd = 1; // drop congestion window size to 1
            ssthresh /= 2; // halve the threshold

            if (ssthresh <= 1) // if threshold goes below 1
                break; // stop transmitting
        
        } else if (cwnd < ssthresh) // slow start phase
            cwnd *= 2;
        
        else // congestion avoidance phase
            cwnd++;
    }

    return;
}

int main(void) {

    CSVCreate();

    simulation();

    return 0;
}
