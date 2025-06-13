#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdint.h>

#include "../edge/byte_op.h"

#define BUFLEN 1024
#define OPCODE_SUM 1
#define OPCODE_REPLY 2

void protocol_execution(int sock);
void error_handling(const char *message);

void usage(const char *pname)
{
  printf(">> Usage: %s [options]\n", pname);
  printf("Options\n");
  printf("  -a, --addr       Server's address\n");
  printf("  -p, --port       Server's port\n");
  exit(0);
}

int main(int argc, char *argv[])
{
  int sock;
  struct sockaddr_in serv_addr;
  char msg[] = "Hello, World!\n";
  char message[30] = {
      0,
  };
  int c, port, tmp, str_len;
  char *pname;
  uint8_t *addr;
  uint8_t eflag;

  pname = argv[0];
  addr = NULL;
  port = -1;
  eflag = 0;

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = {
        {"addr", required_argument, 0, 'a'},
        {"port", required_argument, 0, 'p'},
        {0, 0, 0, 0}};

    const char *opt = "a:p:0";

    c = getopt_long(argc, argv, opt, long_options, &option_index);

    if (c == -1)
      break;

    switch (c)
    {
    case 'a':
      tmp = strlen(optarg);
      addr = (uint8_t *)malloc(tmp);
      memcpy(addr, optarg, tmp);
      break;

    case 'p':
      port = atoi(optarg);
      break;

    default:
      usage(pname);
    }
  }

  if (!addr)
  {
    printf("[*] Please specify the server's address to connect\n");
    eflag = 1;
  }

  if (port < 0)
  {
    printf("[*] Please specify the server's port to connect\n");
    eflag = 1;
  }

  if (eflag)
  {
    usage(pname);
    exit(0);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
    error_handling("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr((const char *)addr);
  serv_addr.sin_port = htons(port);

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    error_handling("connect() error");
  printf("[*] Connected to %s:%d\n", addr, port);

  protocol_execution(sock);

  close(sock);
  return 0;
}

void protocol_execution(int sock)
{
  // Initialize message and buffer for communication
  char msg[] = "Alice";
  char buf[BUFLEN];
  int tbs, sent, tbr, rcvd, offset;
  int len;

  // Step 1: Send name length and name to Bob
  // Convert string length to network byte order (big-endian)
  len = strlen(msg);
  printf("[*] Length information to be sent: %d\n", len);

  // Convert length to network byte order for transmission
  len = htonl(len);
  tbs = 4; // 4 bytes for length
  offset = 0;

  // Send length information (4 bytes)
  while (offset < tbs)
  {
    sent = write(sock, &len + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // Convert length back to host byte order for sending the actual message
  tbs = ntohl(len);
  offset = 0;

  // Send the actual name "Alice"
  printf("[*] Name to be sent: %s\n", msg);
  while (offset < tbs)
  {
    sent = write(sock, msg + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // Step 2: Receive name length and name from Bob
  // Receive length information (4 bytes)
  tbr = 4;
  offset = 0;

  // Read length information from socket
  while (offset < tbr)
  {
    rcvd = read(sock, &len + offset, tbr - offset);
    if (rcvd > 0)
      offset += rcvd;
  }
  len = ntohl(len); // Convert received length to host byte order
  printf("[*] Length received: %d\n", len);

  // Receive the actual name from Bob
  tbr = len;
  offset = 0;

  // Read the name into buffer
  while (offset < tbr)
  {
    rcvd = read(sock, buf + offset, tbr - offset);
    if (rcvd > 0)
      offset += rcvd;
  }

  printf("[*] Name received: %s \n", buf);

  // Step 3: Send summation request to Bob
  // Prepare buffer for sending opcode and arguments
  char *p;
  int i, arg1, arg2;

  // Clear buffer and set up pointer
  memset(buf, 0, BUFLEN);
  p = buf;
  arg1 = 2;
  arg2 = 5;

  // Pack data into buffer: opcode(1 byte) + arg1(4 bytes) + arg2(4 bytes)
  VAR_TO_MEM_1BYTE_BIG_ENDIAN(OPCODE_SUM, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg1, p);
  VAR_TO_MEM_4BYTES_BIG_ENDIAN(arg2, p);
  tbs = p - buf; // Calculate total bytes to send
  offset = 0;

  // Print debug information about the data to be sent
  printf("[*] # of bytes to be sent: %d\n", tbs);
  printf("[*] The following bytes will be sent\n");
  for (i = 0; i < tbs; i++)
    printf("%02x ", buf[i]);
  printf("\n");

  // Send the packed data to Bob
  while (offset < tbs)
  {
    sent = write(sock, buf + offset, tbs - offset);
    if (sent > 0)
      offset += sent;
  }

  // Step 4: Receive result from Bob
  // Prepare to receive opcode(4 bytes) and result(4 bytes)
  int opcode, result;

  tbr = 8; // Total bytes to receive
  offset = 0;
  memset(buf, 0, BUFLEN);

  // Receive the response from Bob
  printf("[*] # of bytes to be received: %d\n", tbr);
  while (offset < tbr)
  {
    rcvd = read(sock, buf + offset, tbs - offset);
    if (rcvd > 0)
      offset += rcvd;
  }

  // Print received data in hexadecimal format
  printf("[*] The following bytes is received\n");
  for (i = 0; i < tbr; i++)
    printf("%02x ", buf[i]);
  printf("\n");

  // Unpack received data: opcode and result
  p = buf;
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, opcode);
  printf("[*] Opcode: %d\n", opcode);
  MEM_TO_VAR_4BYTES_BIG_ENDIAN(p, result);
  printf("[*] Result: %d\n", result);
}

void error_handling(const char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}
