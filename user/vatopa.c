#include "kernel/types.h"
#include "user/user.h"

void main(int argc, char *argv[]) {
  if (argc == 2) {
    printf("0x%x\n", va2pa(atoi(argv[1]), -1));
  } else if (argc == 3) {
    printf("0x%x\n", va2pa(atoi(argv[1]), atoi(argv[2])));
  } else {
    printf("Usage: vatopa virtual_address [pid]\n");
  }
}