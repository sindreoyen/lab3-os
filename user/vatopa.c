#include "kernel/types.h"
#include "user/user.h"

void main(int argc, char *argv[]) {
  // Check if the user has provided the correct number of arguments
  // and print the usage if not.
  switch (argc) {
    // If the user has provided only the virtual address, then
    // we will use the current process's page table.
    case 2:
      printf("0x%x\n", va2pa(atoi(argv[1]), -1));
      break;
    // If the user has provided the virtual address and the pid,
    // then we will use the page table of the process with the
    // given pid.
    case 3:
      printf("0x%x\n", va2pa(atoi(argv[1]), atoi(argv[2])));
      break;
    // If the user has provided an incorrect number of arguments,
    // then we will print the usage.
    default:
      printf("Usage: vatopa virtual_address [pid]\n");
  }
}