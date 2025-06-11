#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <getopt.h>
#include <ctime>
#include <cstdlib>

#include "edge.h"
#include "data/data.h"
#include "setting.h"

void usage(uint8_t *pname)
{
  printf(">> Usage: %s [options]\n", pname);
  printf("Options\n");
  printf("  -a, --addr       Server's address\n");
  printf("  -p, --port       Server's port\n");
  printf("  -v, --vector     Vector type (0 = 2D, 1 = 3D, 2 = 5D)\n");
  exit(0);
}

int main(int argc, char *argv[])
{
	int c, tmp, port, vector_id = 2; // default = 5D
  uint8_t *pname, *addr;
  uint8_t eflag = 0;
  Edge *edge;

  pname = (uint8_t *)argv[0];
  addr = NULL;
  port = -1;

  while (1)
  {
    int option_index = 0;
    static struct option long_options[] = {
      {"addr", required_argument, 0, 'a'},
      {"port", required_argument, 0, 'p'},
      {"vector", required_argument, 0, 'v'},
      {0, 0, 0, 0}
    };

    const char *opt = "a:p:v:";

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

      case 'v':
        vector_id = atoi(optarg);
        if (vector_id < 0 || vector_id > 2) {
          printf("[!] Invalid vector ID. Use 0 (2D), 1 (3D), or 2 (5D)\n");
          exit(1);
        }
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

  // edge 생성 및 vector ID 설정
  edge = new Edge((const char *)addr, port);
  edge->init();
  edge->setVectorID(vector_id); // ✅ vector 설정
  edge->run();

	return 0;
}