#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "pberr.h"
#include "neuranet.h"

int main(int argc, char **argv) {
  char* NNUrl = NULL;
  // Decode arguments
  bool flagPrune = false;
  for (int iArg = 0; iArg < argc; ++iArg) {
    if (strcmp(argv[iArg] , "-nn") == 0 && iArg + 1 < argc) {
      NNUrl = argv[iArg + 1];
      ++iArg;
    } else if (strcmp(argv[iArg] , "-prune") == 0) {
      flagPrune = true;
    } else if (strcmp(argv[iArg] , "-help") == 0) {
      printf("arguments : -nn <NeuraNet file url>\n");
      // Stop here
      return 0;
    }
  }
  if (NNUrl == NULL)
    return 1;
  FILE* fd = fopen(NNUrl, "r");
  NeuraNet* nn = NULL;
  if (NNLoad(&nn, fd) == false) {
    fprintf(stderr, "Failed to open the NeuraNet %s\n", NNUrl);
  }
  fclose(fd);
  if (flagPrune)
    NNPrune(nn);
  char* cloudUrl = "./cloud.txt";
  if (NNSaveLinkAsCloudGraph(nn, cloudUrl) == false) {
    fprintf(stderr, "Failed to save the CloudGraph %s\n", cloudUrl);
  }
  NeuraNetFree(&nn);
  // Return success code
  return 0;
}

