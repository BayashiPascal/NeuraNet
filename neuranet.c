// ============ NEURANET.C ================

// ================= Include =================

#include "neuranet.h"
#if BUILDMODE == 0
#include "neuranet-inline.c"
#endif

// ----- NeuraNet

// ================ Functions implementation ====================

// Create a new NeuraNet with 'nbInput' input values, 'nbOutput' 
// output values, 'nbMaxHidden' hidden values, 'nbMaxBases' base 
// functions, 'nbMaxLinks' links
NeuraNet* NeuraNetCreate(int nbInput, int nbOutput, int nbMaxHidden, 
  int nbMaxBases, int nbMaxLinks) {
#if BUILDMODE == 0
  if (nbInput <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbInput' is invalid (0<%d)", nbInput);
    PBErrCatch(NeuraNetErr);
  }
  if (nbOutput <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbOutput' is invalid (0<%d)", nbOutput);
    PBErrCatch(NeuraNetErr);
  }
  if (nbMaxHidden < 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbMaxHidden' is invalid (0<=%d)", 
      nbMaxHidden);
    PBErrCatch(NeuraNetErr);
  }
  if (nbMaxBases <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbMaxBases' is invalid (0<%d)", 
      nbMaxBases);
    PBErrCatch(NeuraNetErr);
  }
  if (nbMaxLinks <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbMaxLinks' is invalid (0<%d)", 
      nbMaxLinks);
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Declare the new NeuraNet
  NeuraNet* that = PBErrMalloc(NeuraNetErr, sizeof(NeuraNet));
  // Set properties
  that->_nbInputVal = nbInput;
  that->_nbOutputVal = nbOutput;
  that->_nbMaxHidVal = nbMaxHidden;
  that->_nbMaxBases = nbMaxBases;
  that->_nbMaxLinks = nbMaxLinks;
  that->_bases = VecFloatCreate(nbMaxBases * NN_NBPARAMBASE);
  that->_links = VecShortCreate(nbMaxLinks * NN_NBPARAMLINK);
  if (nbMaxHidden > 0)
    that->_hidVal = VecFloatCreate(nbMaxHidden);
  else
    that->_hidVal = NULL;
  // Return the new NeuraNet
  return that;  
}

// Free the memory used by the NeuraNet 'that'
void NeuraNetFree(NeuraNet** that) {
  // Check argument
  if (that == NULL || *that == NULL)
    // Nothing to do
    return;
  // Free memory
  VecFree(&((*that)->_bases));
  VecFree(&((*that)->_links));
  VecFree(&((*that)->_hidVal));
  free(*that);
  *that = NULL;
}

// Calculate the output values for the input values 'input' for the 
// NeuraNet 'that' and memorize the result in 'output'
// input values in [-1,1] and output values in [-1,1]
// All values of 'output' are set to 0.0 before evaluating
// Links which refer to values out of bounds of 'input' or 'output'
// are ignored
void NNEval(NeuraNet* that, VecFloat* input, VecFloat* output) {
#if BUILDMODE == 0
  if (that == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'that' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (input == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'input' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (output == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'output' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (VecGetDim(input) != that->_nbInputVal) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, 
      "'input' 's dimension is invalid (%d!=%d)", 
      VecGetDim(input), that->_nbInputVal);
    PBErrCatch(NeuraNetErr);
  }
  if (VecGetDim(output) != that->_nbOutputVal) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, 
      "'output' 's dimension is invalid (%d!=%d)", 
      VecGetDim(output), that->_nbOutputVal);
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Declare a vector to memorize the nb of links in input of each 
  // hidden values and output values
  VecShort* nbIn = 
    VecShortCreate(NNGetNbMaxHidden(that) + NNGetNbOutput(that));
  // Reset the hidden values and output
  if (NNGetNbMaxHidden(that) > 0)
    VecSetNull(NNHiddenValues(that));
  VecSetNull(output);
  // Declare two variables to memorize the starting index of hidden 
  // values and output values in the link definition
  int startHid = NNGetNbInput(that);
  int startOut = NNGetNbMaxHidden(that) + NNGetNbInput(that);
  // Declare a variable to memorize the previous link
  int prevLink[2] = {-1, -1};
  // Declare a variable to memorize the previous output value
  float prevOut = 1.0;
  // Loop on links
  int iLink = 0;
  while (iLink < NNGetNbMaxLinks(that) && 
    VecGet(that->_links, NN_NBPARAMLINK * iLink) != -1) {
    // Declare a variable for optimization
    int jLink = NN_NBPARAMLINK * iLink;
    // If this link has different input or output than previous link
    // and we are not on the first link
    if (iLink != 0 && 
      (VecGet(that->_links, jLink + 1) != prevLink[0] ||
      VecGet(that->_links, jLink + 2) != prevLink[1])) {
      // Add the previous output value to the output of the previous link
      if (prevLink[1] < startOut)
        VecSet(that->_hidVal, prevLink[1] - startHid,
          VecGet(that->_hidVal, prevLink[1] - startHid) + prevOut);
      else 
        VecSet(output, prevLink[1] - startOut,
          VecGet(output, prevLink[1] - startOut) + prevOut);
      // Increment the nb of input on this output
      VecSet(nbIn, prevLink[1] - startHid, 
        VecGet(nbIn, prevLink[1] - startHid) + 1);
      // Reset the previous output 
      prevOut = 1.0;
    }
    // Update the previous link
    prevLink[0] = VecGet(that->_links, jLink + 1);
    prevLink[1] = VecGet(that->_links, jLink + 2);
    // Multiply the previous output by the evaluation of the current link
    // With the base function of the link and the normalised input value
    float* param = that->_bases->_val + 
      VecGet(that->_links, jLink) * NN_NBPARAMBASE;
    float x = 0.0;
    if (prevLink[0] < startHid)
      x = VecGet(input, prevLink[0]);
    else {
      int n = VecGet(nbIn, prevLink[0] - startHid);
      if (n > 0)
        x = NNGetHiddenValue(that, prevLink[0] - startHid) / 
          (float)(VecGet(nbIn, prevLink[0] - startHid));
      else
        x = NNGetHiddenValue(that, prevLink[0] - startHid);
    }
    prevOut *= NNBaseFun(param, x);
    // Move to the next link
    ++iLink;
  }
  // Update the output of the last link
  if (prevLink[1] < startOut)
    VecSet(that->_hidVal, prevLink[1] - startHid,
      VecGet(that->_hidVal, prevLink[1] - startHid) + prevOut);
  else 
    VecSet(output, prevLink[1] - startOut,
      VecGet(output, prevLink[1] - startOut) + prevOut);
  // Normalise output
  for (int iVal = VecGetDim(output); iVal--;) {
    int n = VecGet(nbIn, NNGetNbMaxHidden(that) + iVal);
    if (n > 0)
      VecSet(output, iVal, VecGet(output, iVal) / (float)(n));
  }
  // Free memory
  VecFree(&nbIn);
}

// Save the NeuraNet 'that' to the stream 'stream'
// Return true if the NeuraNet could be saved, false else
bool NNSave(NeuraNet* that, FILE* stream) {
#if BUILDMODE == 0
  if (that == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'that' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (stream == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'stream' is null");
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Save properties
  int ret = fprintf(stream, "%d %d %d %d %d\n", 
    that->_nbInputVal, that->_nbOutputVal, that->_nbMaxHidVal, 
    that->_nbMaxBases, that->_nbMaxLinks);
  if (ret < 0)
    return false;
  // Save the bases
  if (!VecSave(that->_bases, stream))
    return false;
  // Save the links
  if (!VecSave(that->_links, stream))
    return false;
  // Return the successful code
  return true;
}

// Load the NeuraNet 'that' from the stream 'stream'
// If 'that' is not null the memory is first freed 
// Return true if the NeuraNet could be loaded, false else
bool NNLoad(NeuraNet** that, FILE* stream) {
#if BUILDMODE == 0
  if (that == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'that' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (stream == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'stream' is null");
    PBErrCatch(NeuraNetErr);
  }
#endif
  // If 'that' is already allocated
  if (*that != NULL)
    // Free memory
    NeuraNetFree(that);
  // Read the properties
  int nbInputVal;
  int nbOutputVal;
  int nbMaxHidVal;
  int nbMaxBases;
  int nbMaxLinks;
  int ret = fscanf(stream, "%d %d %d %d %d", &nbInputVal, &nbOutputVal, 
    &nbMaxHidVal, &nbMaxBases, &nbMaxLinks);
  if (ret == EOF)
    return false;
  // Declare the loaded NeuraNet
  *that = NeuraNetCreate(nbInputVal, nbOutputVal, nbMaxHidVal, 
    nbMaxBases, nbMaxLinks);
  // Load the bases
  if (!VecLoad(&((*that)->_bases), stream))
    return false;
  // Load the links
  if (!VecLoad(&((*that)->_links), stream))
    return false;
  // Return the successful code
  return true;
}

// Print the NeuraNet 'that' to the stream 'stream'
void NNPrintln(NeuraNet* that, FILE* stream) {
#if BUILDMODE == 0
  if (that == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'that' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (stream == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'stream' is null");
    PBErrCatch(NeuraNetErr);
  }
#endif
  fprintf(stream, "nbInput: %d\n", that->_nbInputVal);
  fprintf(stream, "nbOutput: %d\n", that->_nbOutputVal);
  fprintf(stream, "nbHidden: %d\n", that->_nbMaxHidVal);
  fprintf(stream, "nbMaxBases: %d\n", that->_nbMaxBases);
  fprintf(stream, "nbMaxLinks: %d\n", that->_nbMaxLinks);
  fprintf(stream, "bases: ");
  VecPrint(that->_bases, stream);
  fprintf(stream, "\n");
  fprintf(stream, "links: ");
  VecPrint(that->_links, stream);
  fprintf(stream, "\n");
  fprintf(stream, "hidden values: ");
  VecPrint(that->_hidVal, stream);
  fprintf(stream, "\n");
}

// Set the links description of the NeuraNet 'that' to a copy of 'links'
// Links with a base function equals to -1 are ignored
// If the input id is higher than the output id they are swap
// The links description in the NeuraNet are ordered in increasing 
// value of input id and output id
void NNSetLinks(NeuraNet* that, VecShort* links) {
#if BUILDMODE == 0
  if (that == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'that' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (links == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'links' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (VecGetDim(links) != that->_nbMaxLinks * NN_NBPARAMLINK) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, 
      "'links' 's dimension is invalid (%d!=%d)", 
      VecGetDim(links), that->_nbMaxLinks);
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Declare a GSet to sort the links
  GSet set = GSetCreateStatic();
  // Declare a variable to memorize the maximum id
  int maxId = NNGetNbInput(that) + NNGetNbMaxHidden(that) + 
    NNGetNbOutput(that);
  // Loop on links
  for (int iLink = 0; iLink < NNGetNbMaxLinks(that) * NN_NBPARAMLINK; 
    iLink += NN_NBPARAMLINK) {
    // If this link is active
    if (VecGet(links, iLink) != -1) {
      // Declare two variable to memorize the effective input and output
      int in = VecGet(links, iLink + 1);
      int out = VecGet(links, iLink + 2);
      // If the input is greater than the output
      if (in > out) {
        // Swap the input and output
        int tmp = in;
        in = out;
        out = tmp;
      }
      // Add the link to the set, sorting on input and ouput
      float sortVal = (float)(in * maxId + out);
      GSetAddSort(&set, links->_val + iLink, sortVal);
    }
  }
  // Declare a variable to memorize the number of active links
  int nbLink = GSetNbElem(&set);
  // If there are active links
  if (nbLink > 0) {
    // loop on active sorted links
    GSetIterForward iter = GSetIterForwardCreateStatic(&set);
    int iLink = 0;
    do {
      short *link = GSetIterGet(&iter);
      VecSet(NNLinks(that), iLink * NN_NBPARAMLINK, link[0]);
      if (link[1] <= link[2]) {
        VecSet(NNLinks(that), iLink * NN_NBPARAMLINK + 1, link[1]);
        VecSet(NNLinks(that), iLink * NN_NBPARAMLINK + 2, link[2]);
      } else {
        VecSet(NNLinks(that), iLink * NN_NBPARAMLINK + 1, link[2]);
        VecSet(NNLinks(that), iLink * NN_NBPARAMLINK + 2, link[1]);
      }
      ++iLink;
    } while (GSetIterStep(&iter));
  }
  // Reset the inactive links
  for (int iLink = nbLink; iLink < NNGetNbMaxLinks(that); ++iLink)
    VecSet(NNLinks(that), iLink * NN_NBPARAMLINK, -1);
  // Free the memory
  GSetFlush(&set);
}


