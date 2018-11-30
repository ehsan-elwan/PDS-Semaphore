/*
 * Producer-consommer, base without synchronisation
 * */

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define TRACE

#define NB_TIMES_PROD 2
#define NB_TIMES_CONS 2

#define NB_PROD_MAX 20
#define NB_CONS_MAX 20
#define NB_TYPES_MAX 10

#define NB_POSITIONS 20 // Maximum size of the buffer

pthread_mutex_t mutex_display = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t *empty;
sem_t *full;
sem_t *namedSem[NB_TYPES_MAX];

typedef struct {
  char info[80];
  int typeOfMessage;
  int producerNumber;
} TypeMessage;

typedef struct {
  TypeMessage buffer[NB_POSITIONS]; // Buffer
  int posW;                         // Index next put
  int posR;                         // Index next get
} CriticalResource;

// Shared Variables
CriticalResource resCritiques; // Modifications so possible conflicts
int nbPositions;               // Size of the buffer

typedef struct { // Parameters of the threads
  int threadNumber;
  int typeOfMessage;
} Parameters;

/*---------------------------------------------------------------------*/
/* codeErr : code returned by a primitive
 * msgErr  : error message, personalised
 * valErr  : returned value by the thread
 */
void thdErreur(int codeErr, char *msgErr, int valeurErr) {
  int *retour = malloc(sizeof(int));
  *retour = valeurErr;
  fprintf(stderr, "%s: %d (%s) \n", msgErr, codeErr, strerror(codeErr));
  pthread_exit(retour);
}

/*--------------------------------------------------*/
void initializeSharedVariables(void) {
  int i;
  /* The buffer, its indices and the number of full positions */
  resCritiques.posW = 0;
  resCritiques.posR = 0;
  for (i = 0; i < nbPositions; i++) {
    strcpy(resCritiques.buffer[i].info, "Empty");
    resCritiques.buffer[i].typeOfMessage = -1;
    resCritiques.buffer[i].producerNumber = -1;
  }
}

/*--------------------------------------------------*/
void showBuffer(void) {
  int i;

  printf("[ ");
  for (i = 0; i < nbPositions; i++) {
    if (resCritiques.buffer[i].producerNumber == -1)
      printf("(Empty)");
    else
      printf("(type: %d, %s, %d)", resCritiques.buffer[i].typeOfMessage,
             resCritiques.buffer[i].info,
             resCritiques.buffer[i].producerNumber);
  }
  printf("]\n");
}

/*--------------------------------------------------*/
void makePut(TypeMessage theMessage) {

  strcpy(resCritiques.buffer[resCritiques.posW].info, theMessage.info);
  resCritiques.buffer[resCritiques.posW].typeOfMessage =
      theMessage.typeOfMessage;
  resCritiques.buffer[resCritiques.posW].producerNumber =
      theMessage.producerNumber;
  resCritiques.posW = (resCritiques.posW + 1) % nbPositions;
}

/*--------------------------------------------------*/
void makeGet(TypeMessage *theMessage) {

  strcpy(theMessage->info, resCritiques.buffer[resCritiques.posR].info);
  theMessage->typeOfMessage =
      resCritiques.buffer[resCritiques.posR].typeOfMessage;
  theMessage->producerNumber =
      resCritiques.buffer[resCritiques.posR].producerNumber;

  resCritiques.buffer[resCritiques.posR].producerNumber = -1;
  resCritiques.buffer[resCritiques.posR].typeOfMessage = -1;
  strcpy(resCritiques.buffer[resCritiques.posR].info, "Empty");

  resCritiques.posR = (resCritiques.posR + 1) % nbPositions;
}

/*--------------------------------------------------*/
void *producer(void *arg) {
  int i;
  TypeMessage theMessage;
  Parameters *param = (Parameters *)arg;

  sleep(1); // to make sure that all consumers and producers have been
  // created
  // before starting --> only for display reasons, so that there is no
  // interleaving...

  for (i = 0; i < NB_TIMES_PROD; i++) {
    sem_wait(empty);
    // pthread_mutex_lock(&mutex);
    sprintf(theMessage.info, "%s n°%d", "Hello", i);
    theMessage.typeOfMessage = param->typeOfMessage;
    theMessage.producerNumber = param->threadNumber;

    pthread_mutex_lock(&mutex_display);
    printf("\tProducer %d of type: %d with Message = (%d, %s, %d)\n",
           param->threadNumber, param->typeOfMessage, theMessage.typeOfMessage,
           theMessage.info, theMessage.producerNumber);

    makePut(theMessage);
#ifdef TRACE
    showBuffer();
#endif
    pthread_mutex_unlock(&mutex_display);
    // pthread_mutex_unlock(&mutex);
    sem_post(full);
    if (resCritiques.posR + (resCritiques.posW - 1) == 0) {
      // sleep(2);
      printf("posting on type: %d\n", param->typeOfMessage);
      sem_post(namedSem[param->typeOfMessage]);
    }

    // usleep(rand()%(100 * param->rang + 100));
  }
  return NULL;
}

/*--------------------------------------------------*/
void *consumer(void *arg) {
  int i;
  TypeMessage theMessage;
  Parameters *param = (Parameters *)arg;
  sleep(2); // to make sure that all consumers and producers have been
  // created
  // before starting --> only for display reasons, so that there is no
  // interleaving...

  for (i = 0; i < NB_TIMES_CONS; i++) {
    sem_wait(namedSem[param->typeOfMessage]);

    if (resCritiques.buffer[resCritiques.posR].typeOfMessage !=
        param->typeOfMessage) {
      printf("cons %d type: %d |  msg type: %d\n", param->threadNumber,
             param->typeOfMessage,
             resCritiques.buffer[resCritiques.posR].typeOfMessage);

      sem_wait(namedSem[param->typeOfMessage]);
    }

    sem_wait(full);
    pthread_mutex_lock(&mutex);
    pthread_mutex_lock(&mutex_display);

    makeGet(&theMessage);
    printf("\t\tConso %d of type: %d with Message = (msg type: %d, %s, %d)\n",
           param->threadNumber, param->typeOfMessage, theMessage.typeOfMessage,
           theMessage.info, theMessage.producerNumber);
#ifdef TRACE
    showBuffer();
#endif
    pthread_mutex_unlock(&mutex_display);
    if (resCritiques.buffer[resCritiques.posR].typeOfMessage != -1) {
      printf("Consume n°%d have to wait -> next type: %d\n",
             param->threadNumber,
             resCritiques.buffer[resCritiques.posR].typeOfMessage);
      sem_post(namedSem[resCritiques.buffer[resCritiques.posR].typeOfMessage]);
      // sleep(1);
    }
    pthread_mutex_unlock(&mutex);

    sem_post(empty);
    // usleep(rand()%(100 * param->rang + 100));
  }
  return NULL;
}

/*--------------------------------------------------*/
int main(int argc, char *argv[]) {
  int i, etat;
  int nbThds, nbProd, nbCons, nbTypes;
  Parameters paramThds[NB_PROD_MAX + NB_CONS_MAX];
  pthread_t idThdProd[NB_PROD_MAX], idThdConso[NB_CONS_MAX];

  if (argc < 4) {
    printf("Usage: %s <Nb Producers <= %d> <Nb Consumers <= %d> <Nb of types "
           "<= %d> <Nb Positions "
           "<= %d> \n",
           argv[0], NB_PROD_MAX, NB_CONS_MAX, NB_TYPES_MAX, NB_POSITIONS);
    exit(2);
  }
  pthread_mutex_init(&mutex_display, NULL);
  //	srand((int)pthread_self());

  nbProd = atoi(argv[1]);
  if (nbProd > NB_PROD_MAX)
    nbProd = NB_PROD_MAX;
  nbCons = atoi(argv[2]);
  if (nbCons > NB_CONS_MAX)
    nbCons = NB_CONS_MAX;
  nbThds = nbProd + nbCons;
  nbTypes = atoi(argv[3]);
  if (nbTypes > NB_TYPES_MAX) {
    printf("Number of types should be <= 10");
    exit(2);
  }
  nbPositions = atoi(argv[4]);
  if (nbPositions > NB_POSITIONS)
    nbPositions = NB_POSITIONS;

  initializeSharedVariables();
  pthread_mutex_init(&mutex, NULL);

  if ((empty = sem_open("/empty", O_CREAT, 0644, nbPositions)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  if ((full = sem_open("/full", O_CREAT, 0644, 0)) == SEM_FAILED) {
    perror("sem_open");
    exit(EXIT_FAILURE);
  }

  const char *semNames[NB_TYPES_MAX] = {"/1", "/2", "/3", "/4", "/5",
                                        "/6", "/7", "/8", "/9", "/10"};
  const int semValues[NB_TYPES_MAX] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  for (int i = 0; i < nbTypes; i++) {
    if ((namedSem[i] = sem_open(semNames[i], O_CREAT, 0644, semValues[i])) ==
        SEM_FAILED) {

      perror("sem_open:semaphore already exist\n");
      exit(EXIT_FAILURE);
    }
  }

  /* Creation of nbProd producers and nbConso consumers */
  for (i = 0; i < nbThds; i++) {
    if (i < nbProd) {
      paramThds[i].typeOfMessage = i % nbTypes;
      paramThds[i].threadNumber = i;
      if ((etat = pthread_create(&idThdProd[i], NULL, producer,
                                 &paramThds[i])) != 0)
        thdErreur(etat, "Creation producer", etat);
#ifdef TRACE
      printf("Creation thread producer n°%d -> %d/%d type: %d\n", i,
             paramThds[i].threadNumber, nbProd - 1, paramThds[i].typeOfMessage);
#endif
    } else {
      paramThds[i].typeOfMessage = i % nbTypes;
      paramThds[i].threadNumber = i - nbProd;
      if ((etat = pthread_create(&idThdConso[i - nbProd], NULL, consumer,
                                 &paramThds[i])) != 0)
        thdErreur(etat, "Creation consumer", etat);
#ifdef TRACE
      printf("Creation thread consumer n°%d -> %d/%d type: %d\n", i - nbProd,
             paramThds[i].threadNumber, nbCons - 1, paramThds[i].typeOfMessage);
#endif
    }
  }

  /* Wait the end of threads */
  for (i = 0; i < nbProd; i++) {
    if ((etat = pthread_join(idThdProd[i], NULL)) != 0)
      thdErreur(etat, "Join threads producers", etat);
#ifdef TRACE
    pthread_mutex_lock(&mutex_display);
    printf("End thread producer n°%d\n", i);
    pthread_mutex_unlock(&mutex_display);

#endif
  }

  for (i = 0; i < nbCons; i++) {
    if ((etat = pthread_join(idThdConso[i], NULL)) != 0)
      thdErreur(etat, "Join threads consumers", etat);
#ifdef TRACE
    pthread_mutex_lock(&mutex_display);
    printf("End thread consumer n°%d\n", i);
    pthread_mutex_unlock(&mutex_display);
#endif
  }

#ifdef TRACE
  printf("\nEnd of main \n");
  showBuffer();
#endif
  if (sem_close(empty) == -1) {
    perror("sem_close");
    exit(EXIT_FAILURE);
  }
  if (sem_unlink("/empty") == -1) {
    perror("sem_unlink");
    exit(EXIT_FAILURE);
  }

  if (sem_close(full) == -1) {
    perror("sem_close");
    exit(EXIT_FAILURE);
  }
  if (sem_unlink("/full") == -1) {
    perror("sem_unlink");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < nbTypes; i++) {

    if (sem_close(namedSem[i]) == -1) {
      perror("sem_close");
      exit(EXIT_FAILURE);
    }

    if (sem_unlink(semNames[i]) == -1) {
      perror("sem_unlink");
      exit(EXIT_FAILURE);
    }
  }
  pthread_mutex_destroy(&mutex);
  pthread_mutex_destroy(&mutex_display);
  return 0;
}
