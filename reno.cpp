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

    ofstream fout("reno.csv", ios::trunc);

    fout << "TR,CWND" << endl;

    fout.close();

    return;
}

void CSVWrite(int rtt, int cwnd) {

    ofstream fout("reno.csv", ios::app);

    fout << rtt << "," << cwnd << endl;

    fout.close();

    return;
}

void simulation(void) {

    int cwnd = 1; // initialise congestion window size

    int ssthresh = randomInRange(20, 25); // start with a random threshold

    vector<int> loss_pt = {4, 8, 22}; // predefined intervals for packet loss
    vector<int> ack_pt = {11, 17, 27}; // predefined intervals for duplicate ACK's

    bool hadAck = false; // check if duplicate ACK was received

    for (int rtt = 1; rtt <= 30; rtt++) { // run for at most 30 transmission rounds

        cout << "SSTHRESH: " << ssthresh << " | CWND: " << cwnd << endl; // log output

        CSVWrite(rtt, cwnd); // log values

        bool lost = find(loss_pt.begin(), loss_pt.end(), rtt) != loss_pt.end(); // check if loss interval is reached
        bool ack = find(ack_pt.begin(), ack_pt.end(), rtt) != ack_pt.end(); // check if duplicate ACK interval is reached

        if ((lost || ack) && (cwnd > ssthresh)) { // if any untoward incident

            cout << endl << (lost ? "PACKET LOST" : (ack ? "DUPLICATE ACKS" : "")) << endl << endl;
            GAP;

            hadAck = ack; // flag to check if duplicate ACK

            cwnd = lost ? 1 : (ack ? (ssthresh + 3) : cwnd); // reduce congestion window size to 1 if packet loss, else to threshold + 3
            ssthresh /= 2; // halve the threshold

            if (ssthresh <= 1) // if threshold goes below 1
                break; // stop transmitting
        
        } else if (cwnd < ssthresh && !hadAck) // slow start phase
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
