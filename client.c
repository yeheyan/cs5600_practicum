#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "operations.h"
#include "config.h"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    fprintf(stderr, "Usage: %s <OPERATION> <args...>\n", argv[0]);
    fprintf(stderr, "Operations:\n");
    fprintf(stderr, "  WRITE <local_file> [remote_file]\n");
    fprintf(stderr, "  GET <remote_file> [local_file]\n");
    fprintf(stderr, "  RM <remote_file>\n");
    fprintf(stderr, "  LS <path>\n");
    fprintf(stderr, "  STOP\n");
    fprintf(stderr, "\nServer: %s:%d (configured in config.h)\n",
            SERVER_IP, SERVER_PORT);
    return 1;
  }

  Operation op = parse_operation(argv[1]);

  switch (op)
  {
  case OP_WRITE:
    if (argc < 3)
    {
      fprintf(stderr, "Usage: %s WRITE <local_file> [remote_file]\n", argv[0]);
      return 1;
    }
    write_file(argv[2], argc >= 4 ? argv[3] : NULL);
    break;

  case OP_GET:
    if (argc < 3)
    {
      fprintf(stderr, "Usage: %s GET <remote_file> [local_file]\n", argv[0]);
      return 1;
    }
    get_file(argv[2], argc >= 4 ? argv[3] : NULL);
    break;

  case OP_RM:
    if (argc != 3)
    {
      fprintf(stderr, "Usage: %s RM <remote_file>\n", argv[0]);
      return 1;
    }
    remove_file(argv[2]);
    break;

  case OP_LS:
    if (argc != 3)
    {
      fprintf(stderr, "Usage: %s LS <path>\n", argv[0]);
      return 1;
    }
    list_directory(argv[2]);
    break;

  case OP_STOP:
    stop_server();
    break;

  case OP_UNKNOWN:
  default:
    fprintf(stderr, "Unknown operation: %s\n", argv[1]);
    return 1;
  }

  return 0;
}