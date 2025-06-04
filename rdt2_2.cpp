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

    Packet make_pkt(int seqNo, string payload);
    void udt_send(Packet snd_pkt);

public:

    enum State {WAIT_CALL0, WAIT_ACK0, WAIT_CALL1, WAIT_ACK1};

    State state;

    Sender() : state(WAIT_CALL0) {}

    Packet rdt_send(string payload);
    void rdt_rcv(Packet rcv_pkt);
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

// DEBUGGING PURPOSES ONLY
// ************************
void testPacket(void) {

    string str = "0x1F23";

    Packet pkt(1, str);

    cout << "Seq. No.: " << pkt.seqNo << endl;
    cout << "Payload: " << pkt.payload << endl;
    cout << "Checksum: 0x" << pkt.intToHex(pkt.checksum) << endl;

    cout << pkt.hasChecksumError() << endl;

    return;
}

void simulation1(void) {

    Sender sender;
    Receiver receiver;

    string message = "0xFA45";

    cout << BOLD << RED << endl << "-----------SIMULATION---------" << RESET << endl << endl;
    PAUSE;

    Packet snd_pkt = sender.rdt_send(message);
    
    Packet ack_pkt = receiver.rdt_rcv(snd_pkt);
    GAP;

    sender.rdt_rcv(ack_pkt);

    PAUSE;

    cout << endl << endl;

    cout << BOLD << RED << "-----------SIMULATION COMPLETE-----------" << RESET << endl << endl;

    return;
}

void simulation2(void) {

    Sender sender;
    Receiver receiver;

    vector<string> messages = {"0xFFF45", "0xED52", "0x112A", "0x78BE"};

    for (auto& message : messages) {

        cout << endl << BOLD << RED << "-----------SIMULATION FOR (" << message << ")-----------" << RESET << endl << endl;
        PAUSE;

        Packet snd_pkt = sender.rdt_send(message);
        GAP;

        Packet ack_pkt = receiver.rdt_rcv(snd_pkt);
        GAP;

        sender.rdt_rcv(ack_pkt);
        
        PAUSE;

        cout << endl << endl;

        cout << BOLD << RED << "-----------SIMULATION FOR (" << message << ") COMPLETE-----------" << RESET << endl << endl;
        PAUSE;
    }

    return;
}

void simulation3(void) {

    Sender sender;
    Receiver receiver;
    
    Packet snd_pkt = sender.rdt_send("0xEB11");
    GAP;

    Packet ack_pkt = receiver.rdt_rcv(snd_pkt);
    GAP;

    sender.rdt_rcv(ack_pkt);
    GAP;

    snd_pkt = sender.rdt_send("0xEA07");

    Packet corrupt_pkt = snd_pkt;
    int pos = randomInRange(2, 5);
    int x = randomInRange(0, 15);
    corrupt_pkt.payload[pos] = decoding[x];

    ack_pkt = receiver.rdt_rcv(corrupt_pkt);
    GAP;

    sender.rdt_rcv(ack_pkt);
    GAP;

    ack_pkt = receiver.rdt_rcv(snd_pkt);
    GAP;

    sender.rdt_rcv(ack_pkt);
    GAP;
}

void simulation4(void) {

    Sender sender;
    Receiver receiver;

    cout << endl << BOLD << ORANGE << "---------------SIMULATION---------------" << RESET << endl << endl; 

    int SIZE = randomInRange(0, 15);
    vector<string> messages;

    for (int i = 0; i < SIZE; i++) {

        string hex = "0x";
        for (int j = 0; j < 4; j++)
            hex.push_back(decoding[randomInRange(0, 15)]);

        if (hex != "0x0000" && hex != "0xFFFF")
            messages.push_back(hex);

        else
            i--;
    }

    for (int i = 0; i < messages.size(); i++) {

        cout << endl << BOLD << ORANGE << "---------------SIMULATION (" << messages[i] << ")---------------" << RESET << endl << endl;

        bool done = false;

        Packet snd_pkt = sender.rdt_send(messages[i]);
        GAP;

        while (!done) {

            Packet corrupt_pkt(snd_pkt.seqNo, snd_pkt.payload);

            int wantError = randomInRange(0, 1);

            if (wantError && i) {

                int pos = randomInRange(2, 5);
                int x = randomInRange(0, 15);
                corrupt_pkt.payload[pos] = decoding[x];
            }

            Packet ack_pkt = receiver.rdt_rcv(corrupt_pkt);
            GAP;

            sender.rdt_rcv(ack_pkt);
            GAP;

            done = sender.state == sender.WAIT_CALL0 || sender.state == sender.WAIT_CALL1;
        }

        PAUSE;
    }

    cout << endl << BOLD << ORANGE << "-----------SIMULATION COMPLETE----------" << RESET << endl << endl;

    return;
}
// *************************

// ACTUAL SIMULATION
void simulation5(void) {

    Sender sender;
    Receiver receiver;

    cout << endl << BOLD << YELLOW << "-------------------SIMULATION---------------------" << RESET << endl << endl;
    PAUSE;

    int SIZE = randomInRange(1, MAX_MESSAGES); // random no. of messages
    vector<string> messages; // hold messages

    for (int i = 0; i < SIZE; i++) {

        string hex = "0x";
        for (int j = 0; j < 4; j++)
            hex.push_back(decoding[randomInRange(0, 15)]); // generate 4 random hexadecimal digits

        if (hex != "0x0000" && hex != "0xFFFF") // reserved for ACK's
            messages.push_back(hex);

        else
            i--; // try again if reserved payloads are generated
    }

    for (int i = 0; i < messages.size(); i++) {

        cout << endl << BOLD << ORANGE << "---------------SIMULATION (" << YELLOW << messages[i] << ORANGE << ")----------------" << RESET << endl << endl;
        GAP;

        bool done = false; // flag used to move from payload to payload

        Packet snd_pkt = sender.rdt_send(messages[i]); // packet at sender's side
        GAP;

        while (!done) { // until transmission complete

            Packet corrupt_snd_pkt(snd_pkt.seqNo, snd_pkt.payload); // send through corrupt channel

            if (randomInRange(0, 10000) >= 5000 && i) { // 50% probability of packet corruption

                int pos = randomInRange(2, 5);
                int x = randomInRange(0, 15);
                corrupt_snd_pkt.payload[pos] = decoding[x]; // flip a random bit
            }

            Packet ack_pkt = receiver.rdt_rcv(corrupt_snd_pkt); // receive corrupt packet and make ACK
            GAP;

            Packet corrupt_ack_pkt(ack_pkt.seqNo, ack_pkt.payload); // ACK may get corrupted in channel

            if (i) {

                corrupt_ack_pkt.seqNo = (randomInRange(0, 10000) <= 1000) ? (1 - corrupt_ack_pkt.seqNo) : corrupt_ack_pkt.seqNo; // 10% chance of sequence number flip
                int pos = randomInRange(2, 5);
                int x = randomInRange(0, 15);
                corrupt_ack_pkt.payload[pos] = (randomInRange(0, 10000) <= 3000) ? decoding[x] : corrupt_ack_pkt.payload[pos]; // 30% chance of payload bit flip
            }

            sender.rdt_rcv(corrupt_ack_pkt); // receive corrupt packet and analyse
            GAP;

            done = sender.state == sender.WAIT_CALL0 || sender.state == sender.WAIT_CALL1; // if moved to next state
        }

        PAUSE;
    }

    cout << endl << BOLD << GREEN << "----------------SIMULATION COMPLETE---------------" << RESET << endl << endl;

    return;
}

int main(void) {

    cout << endl;

    srand(time(0));

    // simulation1();
    // simulation2();
    // simulation3();
    // simulation4();
    simulation5();

    return 0;
}


// *************************** PACKET ********************************

string Packet::truncate(string payload, size_t len) {

    if (payload.length() == len) // sufficient length
        return payload;

    if (payload.length() > len) // length exceeding
        payload = payload.substr(0, len); // chop off the extra bits

    else { // if too short

        while (payload.length() != 16) // until sufficient length
            payload += '0'; // padding
    }

    return payload;
}

uint16_t Packet::make16Bit(int seqNo) {

    return seqNo ? 0x0001 : 0x0000; // extend 1-bit to 16-bit
}

uint16_t Packet::make16Bit(string payload) {

    uint16_t bin = 0x0000;

    payload = this->truncate(payload, 16); // get 16 bits

    for (int i = 0; i < 16; i++) {

        if (payload[i] == '1')
            bin |= (1 << (15 - i)); // set corresponding bit to 1
    }

    return bin;
}

string Packet::hexToBin(string payload) {

    if (payload.empty())
        return "000000000000000";

    string bin;

    this->truncate(payload, 6); // make sure it is of the form 0xabcd

    payload = payload.substr(2); // remove the first two characters

    for (int i = 0; i < 4; i++)
        bin += this->encoding[payload[i]]; // get hexadecimal digit

    return bin;
}

string Packet::intToHex(uint16_t checksum) {

    stack<char> s; // store in reverse order

    while (checksum) {

        s.push(decoding[checksum & 0x000F]); // get last bit

        checksum >>= 4; // remove last bit
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

    sum = (sum >> 16) + (sum & 0xFFFF); // carry over bits

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

    this->udt_send(snd_pkt);
    this->pkt = snd_pkt;

    return snd_pkt;
}

void Sender::rdt_rcv(Packet rcv_pkt) {

    bool corrupt = rcv_pkt.hasChecksumError();
    
    bool isACK0 = !corrupt && !rcv_pkt.seqNo;
    bool isACK1 = !corrupt && rcv_pkt.seqNo;

    switch(this->state) {

        case WAIT_ACK0:

            if (corrupt) {

                cout << BOLD << RED << "(SENDER)   -> Corrupt ACK: Packet Resent" << RESET << endl;
                GAP;
                this->udt_send(this->pkt);

            } else if (isACK1) {

                cout << BOLD << RED << "(SENDER)   -> Packet Corrupted in Channel: Resent" << RESET << endl;
                GAP;
                this->udt_send(this->pkt);
            
            } else if (isACK0) {

                cout << BOLD << PINK << "(SENDER)   -> Packet with seq. no.: " << YELLOW << this->pkt.seqNo << PINK << " ACK\'ed correctly" << RESET << endl;
                GAP;
                state = WAIT_CALL1;

                cout << endl << BOLD << GREEN << "--------------TRANSMISSION COMPLETE---------------" << RESET << endl << endl;
            }

            break;

        case WAIT_ACK1:

            if (corrupt) {

                cout << BOLD << RED << "(SENDER)   -> Corrupt ACK: Packet Resent" << RESET << endl;
                GAP;
                this->udt_send(this->pkt);

            } else if (isACK0) {

                cout << BOLD << RED << "(SENDER)   -> Packet Corrupted in Channel: Resent" << RESET << endl;
                GAP;
                this->udt_send(this->pkt);
            
            } else if (isACK1) {

                cout << BOLD << PINK << "(SENDER)   -> Packet with seq. no.: " << YELLOW << this->pkt.seqNo << PINK << " ACK\'ed correctly" << RESET << endl;
                GAP;
                state = WAIT_CALL0;
                
                cout << endl << BOLD << GREEN << "--------------TRANSMISSION COMPLETE---------------" << RESET << endl << endl;
            }

            break;
    }
    
    return;
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

                cout << BOLD << RED << "(RECEIVER) -> Corrupt Packet: Sending cumulative ACK" << RESET << endl;

                snd_pkt = make_pkt(this->last_ack == -1 ? 0 : this->last_ack);

            } else if (isDuplicate) {

                cout << BOLD << RED << "(RECEIVER) -> Duplicate Packet: Sending cumulative ACK" << RESET << endl;

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

                cout << BOLD << RED << "(RECEIVER) -> Corrupt Packet: Sending cumulative ACK" << RESET << endl;

            } else if (isDuplicate) {

                cout << BOLD << RED << "(RECEIVER) -> Duplicate Packet: Sending cumulative ACK" << RESET << endl;

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
