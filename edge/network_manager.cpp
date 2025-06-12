#include "network_manager.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <assert.h>

#include "opcode.h"
using namespace std;

// Default constructor
NetworkManager::NetworkManager() 
{
  this->sock = -1; // Initializing socket
  this->addr = NULL; // Initializing address pointer
  this->port = -1; // Initializing port
}

// Constructor (receiving address & port)
NetworkManager::NetworkManager(const char *addr, int port)
{
  this->sock = -1; // Initializing socket
  this->addr = addr; // Setting server address
  this->port = port; // Setting server port
}

// Setting server address
void NetworkManager::setAddress(const char *addr)
{
  this->addr = addr;
}

// Returning server address
const char *NetworkManager::getAddress()
{
  return this->addr;
}

// Setting port
void NetworkManager::setPort(int port)
{
  this->port = port;
}

// Returning port
int NetworkManager::getPort()
{
  return this->port;
}

// Initializing socket & connecting to server
int NetworkManager::init()
{
	struct sockaddr_in serv_addr; // Structing server address

    // Creating TCP socket
	this->sock = socket(PF_INET, SOCK_STREAM, 0);
	if (this->sock == FAILURE) // FAILURE assumed to be -1
  {
    cout << "[*] Error: socket() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1); // Terminate program when failure
  }

    // Initializing address structure with 0
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET; // IPv4
	serv_addr.sin_addr.s_addr = inet_addr(this->addr); // Converting string IP to binary
	serv_addr.sin_port = htons(this->port); // Converting port to network byte order

    // Connect to the server
	if (connect(this->sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == FAILURE)
  {
    cout << "[*] Error: connect() error" << endl;
    cout << "[*] Please try again" << endl;
    exit(1); // Terminate program when failure
  }

  // Print connection success message
  cout << "[*] Connected to " << this->addr << ":" << this->port << endl;

  return sock; // Return connected socket
}

// Send data to the server with vector_id and opcode
int NetworkManager::sendData(uint8_t *data, int dlen, uint8_t vector_id)
{
  int sock = this->sock;

  // Calculating total packet size
  int total_len = 1 + 1 + dlen;  // opcode + vector_id + data
  uint8_t *buf = new uint8_t[total_len]; // Allocating buffer for full packet

  buf[0] = OPCODE_DATA;     // 1 byte
  buf[1] = vector_id;       // 1 byte
  memcpy(buf + 2, data, dlen);  // dlen: 8, 12, or 20

  int offset = 0;
  // Write loop to send full buffer
  while (offset < total_len) {
    int sent = write(sock, buf + offset, total_len - offset); // Try to send remaining bytes
    if (sent > 0)
      offset += sent; // Advance offset by amount actually sent
  }

  assert(offset == total_len); // Ensuring full packet being sent
  delete[] buf; // Free allocated memory
  return 0; // Return if success
}

// TODO: Please revise or implement this function as you want. You can also remove this function if it is not needed
// Receive command opcode from server
uint8_t NetworkManager::receiveCommand() 
{
  int sock;
  uint8_t opcode;
  uint8_t *p;

  sock = this->sock;
  opcode = OPCODE_WAIT; // Initialize with wait opcode

  // Keep reading until valid command is received
  while (opcode == OPCODE_WAIT)
    read(sock, &opcode, 1); // Read one byte

  // Ensure opcode is one of the expected values
  assert(opcode == OPCODE_DONE || opcode == OPCODE_QUIT) ;

  return opcode; // Return received opcode
}
