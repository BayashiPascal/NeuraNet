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

// https://archive.ics.uci.edu/ml/datasets/Breast+Cancer+Wisconsin+%28Diagnostic%29

// Nb of step between each save of the GenAlg
// Saving it allows to restart a stop learning process but is 
// very time consuming if there are many input/hidden/output
// If 0 never save
#define SAVE_GA_EVERY 100
// Nb input and output of the NeuraNet
#define NB_INPUT 30
#define NB_OUTPUT 2
// Nb max of hidden values, links and base functions
#define NB_MAXHIDDEN 0
#define NB_MAXLINK 300
#define NB_MAXBASE NB_MAXLINK
// Size of the gene pool and elite pool
#define ADN_SIZE_POOL 100
#define ADN_SIZE_ELITE 20
// Initial best value during learning, must be lower than any
// possible value returned by Evaluate()
#define INIT_BEST_VAL -1000.0
// Value of the NeuraNet above which the learning process stops
#define STOP_LEARNING_AT_VAL 0.999
// Number of epoch above which the learning process stops
#define STOP_LEARNING_AT_EPOCH 2000
// Save NeuraNet in compact format
#define COMPACT true

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

const char* catNames[NB_OUTPUT] = {
  "Malignant",
  "Benign"
};
  
// Structure for the data set

typedef struct Sample {
  float _props[NB_INPUT];
  int _cat;
  int _id;
} Sample;

typedef struct DataSet {
  // Category of the data set
  DataSetCat _cat;
  // Number of sample
  int _nbSample;
  // Samples
  Sample* _samples;
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

// Load the data set of category 'cat' in the DataSet 'that'
// Return true on success, else false
bool DataSetLoad(DataSet* const that, const DataSetCat cat) {
  // Set the category
  that->_cat = cat;
  
  // Load the data according to 'cat'
  FILE* f = fopen("./wdbc.data", "r");
  if (f == NULL) {
    printf("Couldn't open the data set file\n");
    return false;
  }
  int ret = 0;
  if (cat == datalearn) {
    that->_nbSample = 400;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Sample) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%d,", &(that->_samples[iSample]._id));
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      char cat;
      ret = fscanf(f, "%c,", &cat);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      if (cat == 'M')
        that->_samples[iSample]._cat = 0;
      else if (cat == 'B')
        that->_samples[iSample]._cat = 1;
      else {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      for (int iProp = 0; iProp < NB_INPUT; ++iProp) {
        ret = fscanf(f, "%f,", 
          that->_samples[iSample]._props + iProp);
        if (ret == EOF) {
          printf("Couldn't read the dataset\n");
          fclose(f);
          return false;
        }
      }
    }
  } else if (cat == datatest) {
    char buffer[1000];
    for (int iSample = 0; iSample < 400; ++iSample) {
      ret = fscanf(f, "%s", buffer);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
    }
    that->_nbSample = 169;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Sample) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%d,", &(that->_samples[iSample]._id));
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      char cat;
      ret = fscanf(f, "%c,", &cat);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      if (cat == 'M')
        that->_samples[iSample]._cat = 0;
      else if (cat == 'B')
        that->_samples[iSample]._cat = 1;
      else {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      for (int iProp = 0; iProp < NB_INPUT; ++iProp) {
        ret = fscanf(f, "%f,", 
          that->_samples[iSample]._props + iProp);
        if (ret == EOF) {
          printf("Couldn't read the dataset\n");
          fclose(f);
          return false;
        }
      }
    }
  } else if (cat == dataall) {
    that->_nbSample = 569;
    that->_samples = 
      PBErrMalloc(NeuraNetErr, sizeof(Sample) * that->_nbSample);
    for (int iSample = 0; iSample < that->_nbSample; ++iSample) {
      ret = fscanf(f, "%d,", &(that->_samples[iSample]._id));
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      char cat;
      ret = fscanf(f, "%c,", &cat);
      if (ret == EOF) {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      if (cat == 'M')
        that->_samples[iSample]._cat = 0;
      else if (cat == 'B')
        that->_samples[iSample]._cat = 1;
      else {
        printf("Couldn't read the dataset\n");
        fclose(f);
        return false;
      }
      for (int iProp = 0; iProp < NB_INPUT; ++iProp) {
        ret = fscanf(f, "%f,", 
          that->_samples[iSample]._props + iProp);
        if (ret == EOF) {
          printf("Couldn't read the dataset\n");
          fclose(f);
          return false;
        }
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
  
  free(*that);
  *that = NULL;
}

// Evalutation function for the NeuraNet 'that' on the DataSet 'dataset'
// Return the value of the NeuraNet, the bigger the better
float Evaluate(const NeuraNet* const that, 
  const DataSet* const dataset) {
  // Declare 2 vectors to memorize the input and output values
  VecFloat* input = VecFloatCreate(NNGetNbInput(that));
  VecFloat* output = VecFloatCreate(NNGetNbOutput(that));
  // Declare a variable to memorize the value
  float val = 0.0;
  
  // Evaluate

  int countCat[NB_OUTPUT] = {0};
  int countOk[NB_OUTPUT] = {0};
  int countNg[NB_OUTPUT] = {0};
  for (int iSample = dataset->_nbSample; iSample--;) {
    for (int iInp = 0; iInp < NNGetNbInput(that); ++iInp) {
      VecSet(input, iInp,
        dataset->_samples[iSample]._props[iInp]);
    }
    NNEval(that, input, output);
    int pred = VecGetIMaxVal(output);
    ++(countCat[dataset->_samples[iSample]._cat]);
    if (pred == dataset->_samples[iSample]._cat) {
      ++(countOk[dataset->_samples[iSample]._cat]);
    } else if (dataset->_cat == datalearn) {
      ++(countNg[dataset->_samples[iSample]._cat]);
    }
    /*if (dataset->_cat != datalearn) {
      printf("%010d %10s ", dataset->_samples[iSample]._id, 
        catNames[pred]);
      if (pred == dataset->_samples[iSample]._cat)
        printf("OK");
      else
        printf("NG");
      printf("\n");
    }*/
  }
  int nbCat = 0;
  for (int iCat = 0; iCat < NB_OUTPUT; ++iCat) {
    if (countCat[iCat] > 0) {
      ++nbCat;
      float perc = 0.0;
      if (dataset->_cat != datalearn) {
        perc = (float)(countOk[iCat]) / (float)(countCat[iCat]);
        printf("%10s (%3d): %f\n", catNames[iCat], countCat[iCat], perc);
        val += countOk[iCat];
      } else {
        perc = (float)(countOk[iCat] - countNg[iCat]) / 
          (float)(countCat[iCat]);
        val += perc;
      }
    }
  }
  if (dataset->_cat != datalearn)
    val /= (float)(dataset->_nbSample);
  else
    val /= (float)nbCat;
  
  // Free memory
  VecFree(&input);
  VecFree(&output);
  // Return the result of the evaluation
  return val;
}

// Create the NeuraNet
NeuraNet* createNN(void) {
  // Create the NeuraNet
  int nbIn = NB_INPUT;
  int nbOut = NB_OUTPUT;
  int nbMaxHid = NB_MAXHIDDEN;
  int nbMaxLink = NB_MAXLINK;
  int nbMaxBase = NB_MAXBASE;
  NeuraNet* nn = 
    NeuraNetCreate(nbIn, nbOut, nbMaxHid, nbMaxBase, nbMaxLink);

  // Set the links
  
  //VecShort* links = VecShortCreate(nbMaxLink * NN_NBPARAMLINK);
  //for (int iLink = VecGetDim(links); iLink--;) {
  //  VecSet(links, iLink * NN_NBPARAMLINK, base index 
  //    -1 means inactive );
  //  VecSet(links, iLink * NN_NBPARAMLINK + 1, input index );
  //  VecSet(links, iLink * NN_NBPARAMLINK + 2, output index );
  //}
  //NNSetLinks(nn, links);
  //VecFree(&links);
  
  // Set the bases
  
  //VecFloat* bases = VecFloatCreate(nbMaxBase * NN_NBPARAMBASE);
  //for (int iBase = VecGetDim(bases); iBases--;) {
  //  tan(param[0]*NN_THETA)*(x+param[1])+param[2]
  //  param[] in [-1,1]
  //  VecSet(bases, iBase * NN_NBPARAMBASE,  );
  //  VecSet(bases, iBase * NN_NBPARAMBASE + 1,  );
  //  VecSet(bases, iBase * NN_NBPARAMBASE + 2,  );
  //}
  //NNSetBases(nn, bases);
  //VecFree(&bases);
  
  // Return the NeuraNet
  return nn;
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
  NeuraNet* nn = createNN();
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
      bestVal = Evaluate(nn, dataset);
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
    // Must be declared as a GenAlg applied to a NeuraNet with 
    // convolution
    GASetTypeNeuraNet(ga, NB_INPUT, NB_MAXHIDDEN, NB_OUTPUT);
    GAInit(ga);
  }
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
      bestVal = Evaluate(nn, dataset);
      printf("Starting with best at %f.\n", bestVal);
      GenAlgAdn* adn = GAAdn(ga, 0);
      if (adn->_adnF)
        VecCopy(adn->_adnF, nn->_bases);
      if (adn->_adnI)
        VecCopy(adn->_adnI, nn->_links);
    }
    fclose(fd);
  }
  // Start learning process
  printf("Learning...\n");
  printf("Will stop when curEpoch >= %lu or bestVal >= %f\n",
    limitEpoch, STOP_LEARNING_AT_VAL);
  printf("Will save the best NeuraNet in ./bestnn.txt at each improvement\n");
  fflush(stdout);
  // Declare a variable to memorize the best value in the current epoch
  float curBest = 0.0;
  float curWorst = 0.0;
  // Declare a variable to manage the save of GenAlg
  int delaySave = 0;
  // Learning loop
  while (bestVal < STOP_LEARNING_AT_VAL && 
    GAGetCurEpoch(ga) < limitEpoch) {
    curWorst = curBest;
    curBest = INIT_BEST_VAL;
    int curBestI = 0;
    unsigned long int ageBest = 0;
    // For each adn in the GenAlg
    //for (int iEnt = GAGetNbAdns(ga); iEnt--;) {
    for (int iEnt = 0; iEnt < GAGetNbAdns(ga); ++iEnt) {
      // Get the adn
      GenAlgAdn* adn = GAAdn(ga, iEnt);
      // Set the links and base functions of the NeuraNet according
      // to this adn
      if (GABestAdnF(ga) != NULL)
        NNSetBases(nn, GAAdnAdnF(adn));
      if (GABestAdnI(ga) != NULL)
        NNSetLinks(nn, GAAdnAdnI(adn));
      // Evaluate the NeuraNet
      float value = Evaluate(nn, dataset);
      // Update the value of this adn
      GASetAdnValue(ga, adn, value);
      // Update the best value in the current epoch
      if (value > curBest) {
        curBest = value;
        curBestI = iEnt;
        ageBest = GAAdnGetAge(adn);
      }
      if (value < curWorst)
        curWorst = value;
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
    // If there has been improvement during this epoch
    if (curBest > bestVal) {
      bestVal = curBest;
      // Display info about the improvment
      printf("Improvement at epoch %05lu: %f(%03d) (in %02d:%02d:%02d:%02ds)       \n", 
        GAGetCurEpoch(ga), bestVal, curBestI, day, hour, min, sec);
      fflush(stdout);
      // Set the links and base functions of the NeuraNet according
      // to the best adn
      GenAlgAdn* bestAdn = GAAdn(ga, curBestI);
      if (GAAdnAdnF(bestAdn) != NULL)
        NNSetBases(nn, GAAdnAdnF(bestAdn));
      if (GAAdnAdnI(bestAdn) != NULL)
        NNSetLinks(nn, GAAdnAdnI(bestAdn));
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
        "Epoch %05lu: v%f a%03lu(%03d) kt%03lu ", 
        GAGetCurEpoch(ga), curBest, ageBest, curBestI, 
        GAGetNbKTEvent(ga));
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
void Check(const NeuraNet* const that, const DataSetCat cat) {
  // Load the DataSet
  DataSet* dataset = PBErrMalloc(NeuraNetErr, sizeof(DataSet));
  bool ret = DataSetLoad(dataset, cat);
  if (!ret) {
    printf("Couldn't load the data\n");
    return;
  }
  // Evaluate the NeuraNet
  float value = Evaluate(that, dataset);
  // Display the result
  printf("Value: %.6f\n", value);
  // Free memory
  DataSetFree(&dataset);
}

// Predict using the NeuraNet 'that' on 'inputs' (given as an array of 
// 'nbInp' char*)
void Predict(const NeuraNet* const that, const int nbInp, 
  char** const inputs) {
  // Start measuring time
  clock_t clockStart = clock();
  // Check the number of inputs
  if (nbInp != NNGetNbInput(that)) {
    printf("Wrong number of inputs, there should %d, there was %d\n",
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

  // End measuring time
  clock_t clockEnd = clock();
  double timeUsed = 
    ((double)(clockEnd - clockStart)) / (CLOCKS_PER_SEC * 0.001) ;
  // If the clock has been reset meanwhile
  if (timeUsed < 0.0)
    timeUsed = 0.0;
  //if (VecGet(output, 0) == ...)
  //  printf("...(in %fms)", timeUsed);
  
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
    Check(nn, cat);
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
  \"dataSet\": \"Wisconsin diagnostic breast cancer dataset\",\n \
  \"dataSetType\": \"0\",\n \
  \"desc\": \"Wisconsin diagnostic breast cancer dataset, 30 inputs, 2 output\",\n \
  \"dim\": {\n \
    \"_dim\":\"1\",\n \
    \"_val\":[\"32\"]\n \
  },\n \
  \"nbSample\": \"%d\",\n \
  \"samples\": [\n", dataset->_nbSample);
    for (int iSample = 0;
      iSample < dataset->_nbSample;
      ++iSample) {
        fprintf(fp,
"    {\n \
      \"_dim\":\"32\",\n \
      \"_val\":[\"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \
      \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \
      \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \
      \"%f\", \"%f\", \"%f\", \"%f\", \"%f\", \"%f\"]\n \
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
      dataset->_samples[iSample]._props[10],
      dataset->_samples[iSample]._props[11],
      dataset->_samples[iSample]._props[12],
      dataset->_samples[iSample]._props[13],
      dataset->_samples[iSample]._props[14],
      dataset->_samples[iSample]._props[15],
      dataset->_samples[iSample]._props[16],
      dataset->_samples[iSample]._props[17],
      dataset->_samples[iSample]._props[18],
      dataset->_samples[iSample]._props[19],
      dataset->_samples[iSample]._props[20],
      dataset->_samples[iSample]._props[21],
      dataset->_samples[iSample]._props[22],
      dataset->_samples[iSample]._props[23],
      dataset->_samples[iSample]._props[24],
      dataset->_samples[iSample]._props[25],
      dataset->_samples[iSample]._props[26],
      dataset->_samples[iSample]._props[27],
      dataset->_samples[iSample]._props[28],
      dataset->_samples[iSample]._props[29],
      (dataset->_samples[iSample]._cat == 0 ? 1.0 : -1.0),
      (dataset->_samples[iSample]._cat == 1 ? 1.0 : -1.0));

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

