#include <errno.h>
#include <semaphore.h>
#include <stdio.h>

const char *semNames[12] = {"/empty", "/full", "/1", "/2", "/3", "/4",
                            "/5",     "/6",    "/7", "/8", "/9", "/10"};

int main(int argc, char *argv[]) {
  for (int i = 0; i < 12; i++) {

    if (sem_unlink(semNames[i]) != 0) {
      fprintf(stderr, "%s: ", semNames[i]);
      perror("");
    } else {
      printf("semaphore %s unlinked. \n", semNames[i]);
    }
  }
}
