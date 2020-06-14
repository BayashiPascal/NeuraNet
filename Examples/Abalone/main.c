#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include "pberr.h"
#include "genalg.h"
#include "neuranet.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

// http://www.cs.toronto.edu/~delve/data/abalone/desc.html
// Results for comparison available in 
// https://eprints.utas.edu.au/21965/1/whole_WaughSamuelGeorge1997_thesis.pdf

// Nb of step between each save of the GenAlg
// Saving it allows to restart a stop learning process but is 
// very time consuming if there are many input/hidden/output
// If 0 never save
#define SAVE_GA_EVERY 0
// Nb input and output of the NeuraNet
#define NB_INPUT 10
#define NB_OUTPUT 1
// Nb max of hidden values, links and base functions
#define NB_MAXHIDDEN (NB_INPUT * 2)
#define NB_MAXLINK (NB_MAXHIDDEN * 10)
#define NB_MAXBASE NB_MAXLINK
// Size of the gene pool and elite pool
#define ADN_SIZE_POOL 100
#define ADN_SIZE_ELITE 20
// Diversity threshold for KT event in GenAlg
#define DIVERSITY_THRESHOLD 0.001 //0.1
// Initial best value during learning, must be lower than any
// possible value returned by Evaluate()
#define INIT_BEST_VAL -10000.0
// Value of the NeuraNet above which the learning process stops
#define STOP_LEARNING_AT_VAL -1.0
// Number of epoch above which the learning process stops
#define STOP_LEARNING_AT_EPOCH 100000
// Save NeuraNet in compact format
#define COMPACT true
// Switch between mutable links and immutable links
#define MUTABLE_LINK 1
// Number of threads used during learning
#define NB_THREAD 10

// Categories of data sets

typedef enum DataSetCat {
  unknownDataSet,
  datalearn,
  datatest,
  dataall
} DataSetCat;
#define NB_DATASET 4
const char* dataSetNames[NB_DATASET] = {
  "unknownDataSet", "datalearn", "datatest", "dataall"
  };
  
// Structure for the data set

typedef struct Abalone {
  float _props[10];
  float _age;
} Abalone;

typedef struct DataSet {
  // Category of the data set
  DataSetCat _cat;
  // Number of sample
  int _nbSample;
  // Samples
  Abalone* _samples;
} DataSet;

// Get the DataSetCat from its 'name'
DataSetCat GetCategoryFromName(const char* const name) {
  // Declare a variable to memorize the DataSetCat
  DataSetCat cat = unknownDataSet;
  // Search the dataset
  for (int iSet = NB_DATASET; iSet--;)
    if (strcmp(name, dataSetNames[iSet]) == 0)
      cat = iSet;
  // Return the category
  return cat;
}

// Structure for multithreading
typedef struct ThreadData {
    // The adn to be evaluate
    GenAlgAdn* adn;
    // The index of the adn
    int iAdn;
    // Declare the pipe file descriptors
    int pipeChildToParent[2];
    int pipeParentToChild[2];
    // PID
    pid_t pid;
} ThreadData;

// Load the data set of category 'cat' in the DataSet 'that'
// Return true on success, else false
bool DataSetLoad(DataSet* const that, const DataSetCat cat) {
  // Set the category
  that->_cat = cat;
  
  // Load the data according to 'cat'
  FILE* f = fopen("./Prototask.data", "r");
  if (f == NULL) {
    printf("Couldn't open the data set file\n");
    return false;
  }
  char sex;
  int age;
  int ret = 0;
  if (cat == datalearn) {
    that->_nbSample = 3000;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Abalone) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%c %f %f %f %f %f %f %f %d\n", 
        &sex,
        that->_samples[iSample]._props + 3,
        that->_samples[iSample]._props + 4,
        that->_samples[iSample]._props + 5,
        that->_samples[iSample]._props + 6,
        that->_samples[iSample]._props + 7,
        that->_samples[iSample]._props + 8,
        that->_samples[iSample]._props + 9,
        &age);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      that->_samples[iSample]._age = (float)age;
      if (sex == 'M') {
        that->_samples[iSample]._props[0] = 1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'F') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = 1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'I') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = 1.0;
      }
    }
  } else if (cat == datatest) {
    for (int iSample = 0; iSample < 3000; ++iSample) {
      float dummy;
      ret = fscanf(f, "%c %f %f %f %f %f %f %f %d\n", 
        &sex,
        &dummy,
        &dummy,
        &dummy,
        &dummy,
        &dummy,
        &dummy,
        &dummy,
        &age);
      (void)dummy;
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
    }
    that->_nbSample = 1177;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Abalone) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%c %f %f %f %f %f %f %f %d\n", 
        &sex,
        that->_samples[iSample]._props + 3,
        that->_samples[iSample]._props + 4,
        that->_samples[iSample]._props + 5,
        that->_samples[iSample]._props + 6,
        that->_samples[iSample]._props + 7,
        that->_samples[iSample]._props + 8,
        that->_samples[iSample]._props + 9,
        &age);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      that->_samples[iSample]._age = (float)age;
      if (sex == 'M') {
        that->_samples[iSample]._props[0] = 1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'F') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = 1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'I') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = 1.0;
      }
    }
  } else if (cat == dataall) {
    that->_nbSample = 4177;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Abalone) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%c %f %f %f %f %f %f %f %d\n", 
        &sex,
        that->_samples[iSample]._props + 3,
        that->_samples[iSample]._props + 4,
        that->_samples[iSample]._props + 5,
        that->_samples[iSample]._props + 6,
        that->_samples[iSample]._props + 7,
        that->_samples[iSample]._props + 8,
        that->_samples[iSample]._props + 9,
        &age);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      that->_samples[iSample]._age = (float)age;
      if (sex == 'M') {
        that->_samples[iSample]._props[0] = 1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'F') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = 1.0;
        that->_samples[iSample]._props[2] = -1.0;
      } else if (sex == 'I') {
        that->_samples[iSample]._props[0] = -1.0;
        that->_samples[iSample]._props[1] = -1.0;
        that->_samples[iSample]._props[2] = 1.0;
      }
    }
  } else {
    printf("Invalid dataset\n");
    fclose(f);
    return false;
  }
  fclose(f);

  // Return success code
  return true;
} 

// Free memory for the DataSet 'that'
void DataSetFree(DataSet** that) {
  if (*that == NULL) return;
  // Free the memory
  free((*that)->_samples);
  free(*that);
  *that = NULL;
}

// Evalutation function for the NeuraNet 'that' on the DataSet 'dataset'
// Return the value of the NeuraNet, the bigger the better
float Evaluate(const NeuraNet* const that, 
  const DataSet* const dataset,
  float thresholdVal) {
  // Declare 2 vectors to memorize the input and output values
  VecFloat* input = VecFloatCreate(NNGetNbInput(that));
  VecFloat* output = VecFloatCreate(NNGetNbOutput(that));
  // Declare a variable to memorize the value
  float val = 0.0;
  
  // Evaluate
  for (int iSample = dataset->_nbSample; iSample--;) {
    for (int iInp = 0; iInp < NNGetNbInput(that); ++iInp) {
      VecSet(input, iInp,
        dataset->_samples[iSample]._props[iInp]);
    }
    NNEval(that, input, output);
    
    float pred = VecGet(output, 0);
    float age = dataset->_samples[iSample]._age + 0.5;
    float v = fabs(pred - age);
    val -= v;
    if (dataset->_cat == datalearn) {
      float predVal = val / (float)(dataset->_nbSample);
      if (predVal < thresholdVal) {
        val = val / (float)iSample * (float)(dataset->_nbSample);
        iSample = 0;
      }
    }

  }
  val /= (float)(dataset->_nbSample);

  // Free memory
  VecFree(&input);
  VecFree(&output);
  // Return the result of the evaluation
  return val;
}

// Create the NeuraNet
NeuraNet* CreateNN(void) {
#if MUTABLE_LINK == 0
  // Create the NeuraNet
  int nbIn = NB_INPUT;
  int nbOut = NB_OUTPUT;
  VecLong* layers = VecLongCreate(2);
  VecSet(layers, 0, NB_INPUT);
  VecSet(layers, 1, NB_INPUT);
  NeuraNet* nn = 
    NeuraNetCreateFullyConnected(nbIn, nbOut, layers);
  VecFree(&layers);
  // Return the NeuraNet
  return nn;
#else
  // Create the NeuraNet
  int nbIn = NB_INPUT;
  int nbOut = NB_OUTPUT;
  int nbMaxHid = NB_MAXHIDDEN;
  int nbMaxLink = NB_MAXLINK;
  int nbMaxBase = NB_MAXBASE;
  NeuraNet* nn = 
    NeuraNetCreate(nbIn, nbOut, nbMaxHid, nbMaxBase, nbMaxLink);

  // Return the NeuraNet
  return nn;
#endif
}

// Learn based on the DataSetCat 'cat'
void Learn(DataSetCat cat) {
  // Init the random generator
  srandom(time(NULL));
  // Declare variables to measure time
  struct timespec start, stop;
  // Start measuring time
  clock_gettime(CLOCK_REALTIME, &start);
  // Load the DataSet
  DataSet* dataset = PBErrMalloc(NeuraNetErr, sizeof(DataSet));
  bool ret = DataSetLoad(dataset, cat);
  if (!ret) {
    printf("Couldn't load the data\n");
    return;
  }
  // Create the NeuraNet
  NeuraNet* nn = CreateNN();
  // Declare a variable to memorize the best value
  float bestVal = INIT_BEST_VAL;
  // Declare a variable to memorize the limit in term of epoch
  unsigned long int limitEpoch = STOP_LEARNING_AT_EPOCH;
  // Create the GenAlg used for learning
  // If previous weights are available in "./bestga.txt" reload them
  GenAlg* ga = NULL;
  FILE* fd = fopen("./bestga.txt", "r");
  if (fd) {
    printf("Reloading previous GenAlg...\n");
    if (!GALoad(&ga, fd)) {
      printf("Failed to reload the GenAlg.\n");
      NeuraNetFree(&nn);
      DataSetFree(&dataset);
      return;
    } else {
      printf("Previous GenAlg reloaded.\n");
      if (GABestAdnF(ga) != NULL)
        NNSetBases(nn, GABestAdnF(ga));
      if (GABestAdnI(ga) != NULL)
        NNSetLinks(nn, GABestAdnI(ga));
      bestVal = Evaluate(nn, dataset, INIT_BEST_VAL);
      printf("Starting with best at %f.\n", bestVal);
      limitEpoch += GAGetCurEpoch(ga);
    }
    fclose(fd);
  } else {
    printf("Creating new GenAlg...\n");
    fflush(stdout);
    ga = GenAlgCreate(ADN_SIZE_POOL, ADN_SIZE_ELITE, 
      NNGetGAAdnFloatLength(nn), NNGetGAAdnIntLength(nn));
    NNSetGABoundsBases(nn, ga);
    NNSetGABoundsLinks(nn, ga);
    // Must be declared as a GenAlg applied to a NeuraNet
    GASetTypeNeuraNet(ga, NB_INPUT, NB_MAXHIDDEN, NB_OUTPUT);
#if MUTABLE_LINK == 0
    GASetNeuraNetLinkMutability(ga, false);
#else
    GASetNeuraNetLinkMutability(ga, true);
#endif
    GAInit(ga);
  }
  // Set the diveristy
  GASetDiversityThreshold(ga, DIVERSITY_THRESHOLD);

  // Turn on the TextOMeter
  GASetTextOMeterFlag(ga, true);
  
  // If there is a NeuraNet available, reload it into the GenAlg
  fd = fopen("./bestnn.txt", "r");
  if (fd) {
    printf("Reloading previous NeuraNet...\n");
    if (!NNLoad(&nn, fd)) {
      printf("Failed to reload the NeuraNet.\n");
      NeuraNetFree(&nn);
      DataSetFree(&dataset);
      return;
    } else {
      printf("Previous NeuraNet reloaded.\n");
      bestVal = Evaluate(nn, dataset, INIT_BEST_VAL);
      printf("Starting with best at %f.\n", bestVal);
      GenAlgAdn* adn = GAAdn(ga, 0);
      if (adn->_adnF)
        VecCopy(adn->_adnF, nn->_bases);
#if MUTABLE_LINK == 1
      if (adn->_adnI)
        VecCopy(adn->_adnI, nn->_links);
#endif
    }
    fclose(fd);
  }
  // Start learning process
  printf("Learning...\n");
  printf("Will stop when curEpoch >= %lu or bestVal >= %f\n",
    limitEpoch, STOP_LEARNING_AT_VAL);
  printf("Will save the best NeuraNet in ./bestnn.txt at each improvement\n");
  printf("Will use %d thread(s)\n", NB_THREAD);
  fflush(stdout);
  // Declare a variable to memorize the best value in the current epoch
  float curBest = 0.0;
  float curWorst = 0.0;
  float curWorstElite = 0.0;
  // Declare a variable to manage the save of GenAlg
  int delaySave = 0;
  // Learning loop
  while (bestVal < STOP_LEARNING_AT_VAL && 
    GAGetCurEpoch(ga) < limitEpoch) {
    curWorst = curBest;
    curBest = INIT_BEST_VAL;
    curWorstElite = INIT_BEST_VAL;
    int curBestI = 0;
#if NB_THREAD == 1
    // For each adn in the GenAlg
    for (int iEnt = 0; iEnt < GAGetNbAdns(ga); ++iEnt) {
      // Get the adn
      GenAlgAdn* adn = GAAdn(ga, iEnt);
      // If this adn is new
      if (GAAdnIsNew(adn) == true) {
        // Set the links and base functions of the NeuraNet according
        // to this adn
        if (GABestAdnF(ga) != NULL)
          NNSetBases(nn, GAAdnAdnF(adn));
#if MUTABLE_LINK == 1
        if (GABestAdnI(ga) != NULL)
          NNSetLinks(nn, GAAdnAdnI(adn));
#endif
        // Evaluate the NeuraNet
        float value = Evaluate(nn, dataset, curWorstElite);
        // Depreciate entites identical to the current best
        if (fabs(value - curBest) < PBMATH_EPSILON)
          value -= 1000.0;
        // Update the value of this adn
        GASetAdnValue(ga, adn, value);
        // Update the best value in the current epoch
        if (value > curBest) {
          curBest = value;
          curBestI = iEnt;
        }
        if (value < curWorst)
          curWorst = value;
      }
    }
#else
    // Declare the set of entities to evaluate
    GSet entToEval = GSetCreateStatic();
    // For each adn in the GenAlg
    for (int iEnt = 0; iEnt < GAGetNbAdns(ga); ++iEnt) {
      // Get the adn
      GenAlgAdn* adn = GAAdn(ga, iEnt);
      // If this adn is new
      if (GAAdnIsNew(adn) == true) {
        // Add it to the set of entities to evaluate
        ThreadData* data = PBErrMalloc(NeuraNetErr, sizeof(ThreadData));
        data->adn = adn;
        data->iAdn = iEnt;
        GSetAppend(&entToEval, data);
      }
    }
    // Declare the set of entities currently evaluated
    GSet entUnderEval = GSetCreateStatic();
    // While there are entities to evaluate or still entities
    // under evaluation
    while (GSetNbElem(&entToEval) > 0 || GSetNbElem(&entUnderEval) > 0) {
      // While there are thread available and entity to evaluate
      while (GSetNbElem(&entUnderEval) < NB_THREAD &&
        GSetNbElem(&entToEval) > 0) {
        // Get one entity to evaluate
        ThreadData* data = GSetPop(&entToEval);
        // Add it to the entity under evaluation
        GSetAppend(&entUnderEval, data);
        // Create the pipe to receive the reply form the child process
        if (pipe(data->pipeChildToParent) == -1) {
          fprintf(stderr,"ref: pipe() failed!\n%s\n", strerror(errno));
          exit(0);
        };
        if (pipe(data->pipeParentToChild) == -1) {
          fprintf(stderr,"ref: pipe() failed!\n%s\n", strerror(errno));
          exit(0);
        };
        // Fork to evaluate this entity in its own thread
        int pid = fork();
        if (pid == 0) { // Child thread
          // Set the links and base functions of the NeuraNet according
          // to this adn
          if (GABestAdnF(ga) != NULL)
            NNSetBases(nn, GAAdnAdnF(data->adn));
#if MUTABLE_LINK == 1
          if (GABestAdnI(ga) != NULL)
            NNSetLinks(nn, GAAdnAdnI(data->adn));
#endif
          // Evaluate the NeuraNet
          float value = Evaluate(nn, dataset, curWorstElite);
          // Return the value to the parent
          ssize_t ret = write(data->pipeChildToParent[1], &value, sizeof(float));
          if (ret == -1) {
            fprintf(stderr,"ref: write() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          // Read ack from the parent
          char ack = 0;
          while (ack != 1) {
            ret = read(data->pipeParentToChild[0], &ack, 1);
          }
          // Close the pipe
          int retClose;
          retClose = close(data->pipeParentToChild[0]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeParentToChild[1]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeChildToParent[0]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeChildToParent[1]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          // Nothing to do any more
          exit(0);
        } else { // Parent thread
          // Memorize the child pid
          data->pid = pid;
          // Set the reading pipe in non blocking mode
          int ret = fcntl(
            data->pipeChildToParent[0], F_SETFL, 
            fcntl(data->pipeChildToParent[0], F_GETFL) | O_NONBLOCK);
          if (ret == -1) {
            fprintf(stderr,"ref: fcntl() failed!\n%s\n", strerror(errno));
            exit(0);
          }
        }
      }
      // Declare a variable to manage the iterator step
      bool step;
      // Listen to the result of the current child threads
      GSetIterForward iter = GSetIterForwardCreateStatic(&entUnderEval);
      do {
        // Get the thread data
        ThreadData* data = GSetIterGet(&iter);
        // Non blocking read of the reply
        float value;
        ssize_t ret = read(data->pipeChildToParent[0], &value, sizeof(float));
        // If we've received something make sure we received everything
        while (ret > 0 && ret < (ssize_t)sizeof(float)) {
          ssize_t retNext = read(data->pipeChildToParent[0], &value + ret, sizeof(float) - ret);
          ret += retNext;
        }
        // If we could read the value
        if (ret > 0) {
          // Send ack to child
          char ack = 1;
          ret = write(data->pipeParentToChild[1], &ack, 1);
          if (ret == -1) {
            fprintf(stderr,"ref: write() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          // Wait for the child to die
          waitpid(data->pid, NULL, 0);
          // Get the adn
          GenAlgAdn* adn = GAAdn(ga, data->iAdn);
          // Depreciate entites identical to the current best
          if (fabs(value - curBest) < PBMATH_EPSILON)
            value -= 1000.0;
          // Update the value of this adn
          GASetAdnValue(ga, adn, value);
          // Update the best value in the current epoch
          if (value > curBest) {
            curBest = value;
            curBestI = data->iAdn;
          }
          if (value < curWorst)
            curWorst = value;
          // Close the pipe
          int retClose = close(data->pipeParentToChild[0]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeParentToChild[1]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeChildToParent[0]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          retClose = close(data->pipeChildToParent[1]);
          if (retClose == -1) {
            fprintf(stderr,"ref: close() failed!\n%s\n", strerror(errno));
            exit(0);
          }
          // Free memory
          free(data);
          // Remove the adn from the list of adn under evaluation
          step = GSetIterRemoveElem(&iter);
        // Else, simply step to the next thread
        } else {
          step = GSetIterStep(&iter);
        }
      } while (step);
    }
#endif
    // Memorize the current value of the worst elite
    curWorstElite = GAAdnGetVal(GAAdn(ga, GAGetNbElites(ga) - 1));
    // Measure time
    clock_gettime(CLOCK_REALTIME, &stop);
    float elapsed = stop.tv_sec - start.tv_sec;
    int day = (int)floor(elapsed / 86400);
    elapsed -= (float)(day * 86400);
    int hour = (int)floor(elapsed / 3600);
    elapsed -= (float)(hour * 3600);
    int min = (int)floor(elapsed / 60);
    elapsed -= (float)(min * 60);
    int sec = (int)floor(elapsed);
    // If there has been improvement during this epoch
    if (curBest > bestVal) {
      bestVal = curBest;
      GenAlgAdn* bestAdn = GAAdn(ga, curBestI);
      // Display info about the improvment
      printf("Improvement at epoch %05lu: %f(%03d,%08ld->%08ld) (in %02d:%02d:%02d:%02ds)       \n", 
        GAGetCurEpoch(ga), bestVal, curBestI, bestAdn->_idParents[0], bestAdn->_id, day, hour, min, sec);
      fflush(stdout);
      // Set the links and base functions of the NeuraNet according
      // to the best adn
      if (GAAdnAdnF(bestAdn) != NULL)
        NNSetBases(nn, GAAdnAdnF(bestAdn));
#if MUTABLE_LINK == 1
      if (GAAdnAdnI(bestAdn) != NULL)
        NNSetLinks(nn, GAAdnAdnI(bestAdn));
#endif
      // Save the best NeuraNet
      fd = fopen("./bestnn.txt", "w");
      if (!NNSave(nn, fd, COMPACT)) {
        printf("Couldn't save the NeuraNet\n");
        NeuraNetFree(&nn);
        GenAlgFree(&ga);
        DataSetFree(&dataset);
        return;
      }
      fclose(fd);
    } else {
      fprintf(stderr, 
        "Epoch %05lu: v%f a%03lu kt%03lu ", 
        GAGetCurEpoch(ga), GAAdnGetVal(GAAdn(ga, 0)), 
        GAAdnGetAge(GAAdn(ga, 0)), GAGetNbKTEvent(ga));
      fprintf(stderr, "(in %02d:%02d:%02d:%02ds)  \r", 
        day, hour, min, sec);
      fflush(stderr);
    }
    ++delaySave;
    if (SAVE_GA_EVERY != 0 && delaySave >= SAVE_GA_EVERY) {
      delaySave = 0;
      // Save the adns of the GenAlg, use a temporary file to avoid
      // loosing the previous one if something goes wrong during
      // writing, then replace the previous file with the temporary one
      fd = fopen("./bestga.tmp", "w");
      if (!GASave(ga, fd, COMPACT)) {
        printf("Couldn't save the GenAlg\n");
        NeuraNetFree(&nn);
        GenAlgFree(&ga);
        DataSetFree(&dataset);
        return;
      }
      fclose(fd);
      int ret = system("mv ./bestga.tmp ./bestga.txt");
      (void)ret;
    }
    // Step the GenAlg
    GAStep(ga);
  }
  // Measure time
  clock_gettime(CLOCK_REALTIME, &stop);
  float elapsed = stop.tv_sec - start.tv_sec;
  int day = (int)floor(elapsed / 86400);
  elapsed -= (float)(day * 86400);
  int hour = (int)floor(elapsed / 3600);
  elapsed -= (float)(hour * 3600);
  int min = (int)floor(elapsed / 60);
  elapsed -= (float)(min * 60);
  int sec = (int)floor(elapsed);
  printf("\nLearning complete (in %d:%d:%d:%ds)\n", 
    day, hour, min, sec);
  fflush(stdout);
  // Free memory
  NeuraNetFree(&nn);
  GenAlgFree(&ga);
  DataSetFree(&dataset);
}

// Check the NeuraNet 'that' on the DataSetCat 'cat'
void Validate(const NeuraNet* const that, const DataSetCat cat) {
  // Load the DataSet
  DataSet* dataset = PBErrMalloc(NeuraNetErr, sizeof(DataSet));
  bool ret = DataSetLoad(dataset, cat);
  if (!ret) {
    printf("Couldn't load the data\n");
    return;
  }

  // Declare an array to memorize the error in prediction per
  // predicted number of layers
  unsigned int confusion[30][30];
  unsigned int errors[30];
  for (int i = 0; i < 30; ++i) {
    errors[i] = 0;
    for (int j = 0; j < 30; ++j)
      confusion[i][j] = 0;
  }

  // Declare 2 vectors to memorize the input and output values
  VecFloat* input = VecFloatCreate(NNGetNbInput(that));
  VecFloat* output = VecFloatCreate(NNGetNbOutput(that));
  // Declare a variable to memorize the value
  float val = 0.0;
  
  // Evaluate on the validation dataset
  
  for (int iSample = dataset->_nbSample; iSample--;) {
    for (int iInp = 0; iInp < NNGetNbInput(that); ++iInp) {
      VecSet(input, iInp,
        dataset->_samples[iSample]._props[iInp]);
    }
    NNEval(that, input, output);
    
    float pred = VecGet(output, 0);
    float age = dataset->_samples[iSample]._age + 0.5;
    float v = fabs(pred - age);
    int error = (int)v;
    if (error >= 30) 
      error = 29;
    (errors[error])++;
    val -= v;
    if (pred >= 0 && pred <= 29)
      ++(confusion[(int)floor(dataset->_samples[iSample]._age + 0.5)][(int)floor(pred)]);

  }
  val /= (float)(dataset->_nbSample);
  
  // Display the result per predicted nb of layer
  float cumul = 0.0;
  printf("age_err\tcount\tcumul_perc\n");
  for (int i = 0; i < 30; ++i) {
    float perc = (float)(errors[i]) / (float)(dataset->_nbSample);
    cumul += perc;
    printf("%u\t%u\t%f\n", i, errors[i], cumul);
  }
  for (int i = 0; i < 30; ++i) {
    for (int j = 0; j < 30; ++j)
    if (confusion[i][j] > 0)
      printf("%03d ", confusion[i][j]);
    else
      printf("... ");
    printf("\n");
  }

  // Free memory
  VecFree(&input);
  VecFree(&output);


  // Display the result
  printf("Value: %.6f\n", val);


  // Free memory
  DataSetFree(&dataset);
}

// Predict using the NeuraNet 'that' on 'inputs' (given as an array of 
// 'nbInp' char*)
void Predict(const NeuraNet* const that, const int nbInp, 
  char** const inputs) {
  // Check the number of inputs
  if (nbInp != NNGetNbInput(that)) {
    printf("Wrong number of inputs, there should be %d, there was %d\n",
      NNGetNbInput(that), nbInp);
    return;
  }
  // Declare 2 vectors to memorize the input and output values
  VecFloat* input = VecFloatCreate(NNGetNbInput(that));
  VecFloat* output = VecFloatCreate(NNGetNbOutput(that));
  // Set the input
  for (int iInp = 0; iInp < nbInp; ++iInp) {
    float v = 0.0;
    sscanf(inputs[iInp], "%f", &v);
    VecSet(input, iInp, v);
  }
  // Predict
  NNEval(that, input, output);
  printf("Prediction: %f rings\n", VecGet(output, 0));
  // Free memory
  VecFree(&input);
  VecFree(&output);
}

int main(int argc, char** argv) {
  // Declare a variable to memorize the mode (learning/checking)
  int mode = -1;
  // Declare a variable to memorize the dataset used
  DataSetCat cat = unknownDataSet;
  // Decode mode from arguments
  if (argc >= 3) {
    if (strcmp(argv[1], "-learn") == 0) {
      mode = 0;
      cat = GetCategoryFromName(argv[2]);
    } else if (strcmp(argv[1], "-check") == 0) {
      mode = 1;
      cat = GetCategoryFromName(argv[2]);
    } else if (strcmp(argv[1], "-predict") == 0) {
      mode = 2;
    }
  } else if (argc >= 2) {
    if (strcmp(argv[1], "-convert") == 0) {
      mode = 3;
    }
  }
  // If the mode is invalid print some help
  if (mode == -1) {
    printf("Select a mode from:\n");
    printf("-learn <dataset name>\n");
    printf("-check <dataset name>\n");
    printf("-predict <input values>\n");
    printf("-convert\n");
    return 0;
  }
  if (mode == 0) {
    Learn(cat);
  } else if (mode == 1) {
    NeuraNet* nn = NULL;
    FILE* fd = fopen("./bestnn.txt", "r");
    if (!NNLoad(&nn, fd)) {
      printf("Couldn't load the best NeuraNet\n");
      return 0;
    }
    fclose(fd);
    Validate(nn, cat);
    NeuraNetFree(&nn);
  } else if (mode == 2) {
    NeuraNet* nn = NULL;
    FILE* fd = fopen("./bestnn.txt", "r");
    if (!NNLoad(&nn, fd)) {
      printf("Couldn't load the best NeuraNet\n");
      return 0;
    }
    fclose(fd);
    Predict(nn, argc - 2, argv + 2);
    NeuraNetFree(&nn);
  } else if (mode == 3) {
    DataSet* dataset = PBErrMalloc(NeuraNetErr, sizeof(DataSet));
    bool ret = DataSetLoad(dataset, dataall);
    if (!ret) {
      printf("Couldn't load the data\n");
      return 1;
    }
    FILE* fp = fopen("./Prototask.json", "w");
    fprintf(fp,
"{\n \
  \"dataSet\": \"Abalone dataset\",\n \
  \"dataSetType\": \"0\",\n \
  \"desc\": \"Abalone dataset, 10 inputs, 1 output\",\n \
  \"dim\": {\n \
    \"_dim\":\"1\",\n \
    \"_val\":[\"11\"]\n \
  },\n \
  \"nbSample\": \"%d\",\n \
  \"samples\": [\n", dataset->_nbSample);
    for (int iSample = 0;
      iSample < dataset->_nbSample;
      ++iSample) {
        fprintf(fp,
"    {\n \
      \"_dim\":\"11\",\n \
      \"_val\":[\"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \
      \"%f\", \"%f\", \"%f\", \"%f\", \"%f\"]\n \
   }",
      dataset->_samples[iSample]._props[0],
      dataset->_samples[iSample]._props[1],
      dataset->_samples[iSample]._props[2],
      dataset->_samples[iSample]._props[3],
      dataset->_samples[iSample]._props[4],
      dataset->_samples[iSample]._props[5],
      dataset->_samples[iSample]._props[6],
      dataset->_samples[iSample]._props[7],
      dataset->_samples[iSample]._props[8],
      dataset->_samples[iSample]._props[9],
      dataset->_samples[iSample]._age);

      if (iSample < dataset->_nbSample - 1)
        fprintf(fp, ",");
      fprintf(fp, "\n");
    }
    fprintf(fp,
"  ]\n" \
"}\n");
    fclose(fp);
    DataSetFree(&dataset);
  }
  // Return success code
  return 0;
}

