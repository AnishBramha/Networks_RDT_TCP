#include <iostream>
#include <cstdint>
#include <unordered_map>
#include <stack>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <ctime>
#include <cstdlib>

#define BOLD "\033[1m"
#define YELLOW "\033[33m"
#define PINK "\033[95m"
#define RED "\033[31m"
#define BLUE "\033[34m"
#define GREEN "\033[32m"
#define ORANGE "\033[38;2;255;165;0m"
#define RESET "\033[0m"

using namespace std;

#define GAP this_thread::sleep_for(chrono::milliseconds(700))
#define PAUSE this_thread::sleep_for(chrono::milliseconds(1500))
#define DROP this_thread::sleep_for(chrono::milliseconds(5000))
#define TIMEOUT 3000
#define MAX_MESSAGES 15

vector<char> decoding = {'0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

class Packet {

public:

    int seqNo;
    string payload;
    uint16_t checksum;

    Packet() {}

    Packet(int seqNo, string payload) : seqNo(seqNo), payload(payload) {

        this->checksum = this->makeChecksum();
    }

    bool hasChecksumError(void);

    string intToHex(uint16_t checksum);

private:

    unordered_map<char, string> encoding = {
        {'0', "0000"}, {'1', "0001"}, {'2', "0010"}, {'3', "0011"},
        {'4', "0100"}, {'5', "0101"}, {'6', "0110"}, {'7', "0111"},
        {'8', "1000"}, {'9', "1001"}, {'A', "1010"}, {'B', "1011"},
        {'C', "1100"}, {'D', "1101"}, {'E', "1110"}, {'F', "1111"},
        {'a', "1010"}, {'b', "1011"}, {'c', "1100"}, {'d', "1101"},
        {'e', "1110"}, {'f', "1111"}
    };

    vector<char> decoding = {'0', '1', '2', '3', '4', '5', '6', '7',
                    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    string truncate(string payload, size_t len);

    uint16_t make16Bit(int seqNo);
    uint16_t make16Bit(string payload);

    string hexToBin(string payload);

    uint16_t sum(void);

    uint16_t makeChecksum(void);
};

class Sender {

private:

    Packet pkt;

    int lastSeq;

    chrono::time_point<chrono::steady_clock> lastSentTime;
    bool timeoutTriggered;

    Packet make_pkt(int seqNo, string payload);
    void udt_send(Packet snd_pkt);

public:

    enum State {WAIT_CALL0, WAIT_ACK0, WAIT_CALL1, WAIT_ACK1};

    State state;

    Sender() : state(WAIT_CALL0), timeoutTriggered(false) {}

    Packet rdt_send(string payload);
    void rdt_rcv(Packet rcv_pkt);

    bool hasTimeout(void);
};

class Receiver {

private:

    enum State {WAIT_0, WAIT_1};

    State state;
    int last_ack;

    Packet make_pkt(int seqNo);
    void udt_send(Packet snd_pkt);
    void extract_deliver_data(Packet rcv_pkt);

public:

    Receiver() : state(WAIT_0), last_ack(-1) {}

    Packet rdt_rcv(Packet rcv_pkt);
};

int randomInRange(int a, int b) {

    return min(a, b) + (rand() % (max(a, b) - min(a, b) + 1));
}

void simulation(void) {

    Sender sender;
    Receiver receiver;

    cout << endl << BOLD << YELLOW << "-------------------SIMULATION---------------------" << RESET << endl << endl;
    PAUSE;

    int SIZE = randomInRange(1, MAX_MESSAGES); // random no. of messages
    vector<string> messages; // hold messages

    for (int i = 0; i < SIZE; i++) {

        string hex = "0x";
        for (int j = 0; j < 4; j++)
            hex.push_back(decoding[randomInRange(0, 15)]); // generate random 16-bit messages

        if (hex != "0x0000" && hex != "0xFFFF") // reserved for ACK's
            messages.push_back(hex);

        else
            i--; // try again if reserved payloads are generated
    }

    for (int i = 0; i < messages.size(); i++) {

        cout << endl << BOLD << ORANGE << "---------------SIMULATION (" << YELLOW << messages[i] << ORANGE << ")----------------" << RESET << endl << endl;
        GAP;

        bool done = false; // flag used to move from payload to payload

        Packet snd_pkt = sender.rdt_send(messages[i]);
        GAP;

        while (!done) { // until transmission complete

            if (randomInRange(0, 10000) >= 7500 && i) { // 25% probability that packet gets dropped

                DROP;
    
                Packet fake_pkt; // placeholder to trigger tiemout
                sender.rdt_rcv(fake_pkt);

                continue;
            }

            Packet corrupt_snd_pkt(snd_pkt.seqNo, snd_pkt.payload); // send through corrupt channel

            if (randomInRange(0, 10000) >= 5000 && i) { // 50% probability of packet corruption

                int pos = randomInRange(2, 5);
                int x = randomInRange(0, 15);
                corrupt_snd_pkt.payload[pos] = decoding[x]; // flip a random bit
            }

            Packet ack_pkt = receiver.rdt_rcv(corrupt_snd_pkt); // receive corrupt packet and make ACK
            GAP;

            Packet corrupt_ack_pkt(ack_pkt.seqNo, ack_pkt.payload); // ACK may get corrupted in channel

            if (randomInRange(0, 10000) >= 7500 && i) { // 25% probability that ACK gets dropped

                DROP;

                Packet fake_pkt; // placeholder to trigger timeout
                sender.rdt_rcv(fake_pkt);

                continue;
            }

            if (i) {

                corrupt_ack_pkt.seqNo = (randomInRange(0, 10000) <= 1000) ? (1 - corrupt_ack_pkt.seqNo) : corrupt_ack_pkt.seqNo; // 10% probability that sequence no. gets flipped
                int pos = randomInRange(2, 5);
                int x = randomInRange(0, 15);
                corrupt_ack_pkt.payload[pos] = (randomInRange(0, 10000) <= 3000) ? decoding[x] : corrupt_ack_pkt.payload[pos]; // 30% probability that random payload bit gets flipped
            }

            sender.rdt_rcv(corrupt_ack_pkt); // receive corrupt packet  and analyse
            GAP;

            done = sender.state == sender.WAIT_CALL0 || sender.state == sender.WAIT_CALL1;
        }

        PAUSE;
    }

    cout << endl << BOLD << GREEN << "----------------SIMULATION COMPLETE---------------" << RESET << endl << endl;

    return;
}

int main(void) {

    cout << endl;

    srand(time(0));

    simulation();

    return 0;
}


// *************************** PACKET ********************************

string Packet::truncate(string payload, size_t len) {

    if (payload.length() == len) // if sufficient length
        return payload;

    if (payload.length() > len) // if length exceeded
        payload = payload.substr(0, len); // chop off extra bits

    else { // if too short

        while (payload.length() != 16) // until length reached
            payload += '0'; // padding
    }

    return payload;
}

uint16_t Packet::make16Bit(int seqNo) {

    return seqNo ? 0x0001 : 0x0000; // extend 1-bit to 16-bit
}

uint16_t Packet::make16Bit(string payload) {

    uint16_t bin = 0x0000;

    payload = this->truncate(payload, 16); // make 16 bit

    for (int i = 0; i < 16; i++) {

        if (payload[i] == '1')
            bin |= (1 << (15 - i)); // set corresponding bit
    }

    return bin;
}

string Packet::hexToBin(string payload) {

    if (payload.empty())
        return "000000000000000";

    string bin;

    this->truncate(payload, 6); // make sure it is of the form 0xabcd

    payload = payload.substr(2); // remove the characters '0x'

    for (int i = 0; i < 4; i++)
        bin += this->encoding[payload[i]]; // get corresponding hexadecimal digit

    return bin;
}

string Packet::intToHex(uint16_t checksum) {

    stack<char> s; // store in reverse order

    while (checksum) {

        s.push(decoding[checksum & 0x000F]); // get last bit

        checksum >>= 4; // chop off last bit
    }

    string hex;

    while (!s.empty()) {

        hex += s.top();
        s.pop();
    }

    return hex;
}

uint16_t Packet::sum(void) {

    string payload = this->hexToBin(this->payload);

    uint32_t sum = this->make16Bit(this->seqNo) + this->make16Bit(payload); // 16-bit extended seq no. + 16-bit payload

    sum = (sum >> 16) + (sum & 0xFFFF); // carry-over bits

    sum += (sum >> 16); // overflow bits

    return static_cast<uint16_t>(sum);
}

uint16_t Packet::makeChecksum(void) {

    return ~this->sum(); // 1's complement
}

bool Packet::hasChecksumError(void) {

    return this->makeChecksum() != this->checksum; // if calculated checksum is not equal to transmitted checksum
}

// *******************************************************************

// ****************************SENDER*********************************

Packet Sender::make_pkt(int seqNo, string payload) {

    return Packet(seqNo, payload);
}

void Sender::udt_send(Packet snd_pkt) {

    string msg = string(PINK) + "(SENDER)   -> Sent packet with seq. no.: " + string(YELLOW) + to_string(snd_pkt.seqNo);

    cout << BOLD << msg << RESET << endl;

    this->lastSentTime = chrono::steady_clock::now(); // start timer
    this->timeoutTriggered = false; // reset flag

    return;
}

Packet Sender::rdt_send(string payload) {

    Packet snd_pkt;

    switch(this->state) {

        case WAIT_CALL0:

            snd_pkt = this->make_pkt(0, payload);
            this->state = WAIT_ACK0;
            break;

        case WAIT_CALL1:
            
            snd_pkt = this->make_pkt(1, payload);
            this->state = WAIT_ACK1;
            break;
    }

    GAP;
    this->udt_send(snd_pkt);
    this->pkt = snd_pkt;

    return snd_pkt;
}

void Sender::rdt_rcv(Packet rcv_pkt) {

    if (this->hasTimeout()) { // if there is a timeout

        cout << BOLD << RED << "(SENDER)   -> " << "Timeout: Packet/ACK Lost/Corrupted: Resending Packet" << RESET << endl;
        
        GAP;
        this->udt_send(this->pkt); // retransmit

        return;
    }

    bool corrupt = rcv_pkt.hasChecksumError();
    
    bool isACK0 = !corrupt && !rcv_pkt.seqNo;
    bool isACK1 = !corrupt && rcv_pkt.seqNo;

    switch(this->state) {

        case WAIT_ACK0:

            if (corrupt) {

                cout << BOLD << RED << "(SENDER)   -> " << "Corrupt ACK" << RESET << endl;
                DROP; // wait for correct ACK

                Packet fake_pkt;
                this->rdt_rcv(fake_pkt); // trigger timeout

                return;

            } else if (isACK1) {

                cout << BOLD << RED << "(SENDER)   -> " << "Packet Corrupted in Channel" << RESET << endl;
                DROP;

                Packet fake_pkt;
                this->rdt_rcv(fake_pkt);

                return;
            
            } else if (isACK0) {

                cout << BOLD << PINK << "(SENDER)   -> " << "Packet with seq. no.: " << YELLOW << this->pkt.seqNo << PINK << " ACK\'ed correctly" << RESET << endl;
                GAP;
                state = WAIT_CALL1;

                cout << endl << BOLD << GREEN << "--------------TRANSMISSION COMPLETE---------------" << RESET << endl << endl;
            }

            break;

        case WAIT_ACK1:

            if (corrupt) {

                cout << BOLD << RED << "(SENDER)   -> " << "Corrupt ACK" << RESET << endl;
                DROP;

                Packet fake_pkt;
                this->rdt_rcv(fake_pkt);

                return;

            } else if (isACK0) {

                cout << BOLD << RED << "(SENDER)   -> " << "Packet Corrupted in Channel" << RESET << endl;
                DROP;

                Packet fake_pkt;
                this->rdt_rcv(fake_pkt);
            
                return;

            } else if (isACK1) {

                cout << BOLD << PINK << "(SENDER)   -> " << "Packet with seq. no.: " << YELLOW << this->pkt.seqNo << PINK << " ACK\'ed correctly" << RESET << endl;
                GAP;
                state = WAIT_CALL0;
                
                cout << endl << BOLD << GREEN << "--------------TRANSMISSION COMPLETE---------------" << RESET << endl << endl;
            }

        break;
    }
    
    return;
}

bool Sender::hasTimeout(void) {

    if (this->timeoutTriggered) // if timeout
        return true;

    if (chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - this->lastSentTime).count() > TIMEOUT) { // if time_now - time_start > threshold

        this->timeoutTriggered = true;
        return true;
    }

    return false; // no timeout
}

// ********************************************************************

// ****************************RECEIVER********************************

Packet Receiver::make_pkt(int seqNo) {

    return Packet(seqNo, (seqNo ? "0xFFFF" : "0x0000"));
}

void Receiver::udt_send(Packet snd_pkt) {

    string msg = string(BLUE) + "(RECEIVER) -> Sending ACK for seq. no.: " + string(YELLOW) + to_string(this->last_ack == -1 ? 0 : this->last_ack);
    GAP;
    cout << BOLD << msg << RESET << endl;

    return;
}

void Receiver::extract_deliver_data(Packet rcv_pkt) {

    string msg = string(BLUE) + "(RECEIVER) -> Packet with seq no.: " + string(YELLOW) + to_string(rcv_pkt.seqNo) + string(BLUE) + " received: Delivering data from payload: " + string(YELLOW) + rcv_pkt.payload;
    GAP;
    cout << BOLD << msg << RESET << endl;

    return;
}

Packet Receiver::rdt_rcv(Packet rcv_pkt) {

    bool corrupt = rcv_pkt.hasChecksumError();

    bool isDuplicate = !corrupt && rcv_pkt.seqNo == this->last_ack;

    Packet snd_pkt;

    switch(this->state) {

        case WAIT_0:

            if (corrupt) {

                cout << BOLD << RED << "(RECEIVER) -> " << "Corrupt Packet: Sending cumulative ACK" << RESET << endl;

                snd_pkt = make_pkt(this->last_ack == -1 ? 0 : this->last_ack);

            } else if (isDuplicate) {

                cout << BOLD << RED << "(RECEIVER) -> " << "Duplicate Packet: Sending cumulative ACK" << RESET << endl;

                snd_pkt = make_pkt(this->last_ack == -1 ? 0 : this->last_ack);
            
            } else {

                this->extract_deliver_data(rcv_pkt);
                this->last_ack = 0;
                this->state = WAIT_1;

                snd_pkt = make_pkt(0);
            }

            break;

        case WAIT_1:

            if (corrupt) {

                snd_pkt = make_pkt(this->last_ack == -1 ? 1 : this->last_ack);

                cout << BOLD << RED << "(RECEIVER) -> " << "Corrupt Packet: Sending cumulative ACK" << RESET << endl;

            } else if (isDuplicate) {

                cout << BOLD << RED << "(RECEIVER) -> " << "Duplicate Packet: Sending cumulative ACK" << RESET << endl;

                snd_pkt = make_pkt(this->last_ack == -1 ? 0 : this->last_ack);
                
            } else {

                this->extract_deliver_data(rcv_pkt);
                this->last_ack = 1;
                this->state = WAIT_0;

                snd_pkt = make_pkt(1);
            }

            break;
    }

    this->udt_send(snd_pkt);

    return snd_pkt;
}

// *********************************************************************
