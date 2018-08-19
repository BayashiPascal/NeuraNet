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
NeuraNet* NeuraNetCreate(const int nbInput, const int nbOutput, 
  const int nbMaxHidden, const int nbMaxBases, const int nbMaxLinks) {
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
  *(int*)&(that->_nbInputVal) = nbInput;
  *(int*)&(that->_nbOutputVal) = nbOutput;
  *(int*)&(that->_nbMaxHidVal) = nbMaxHidden;
  *(int*)&(that->_nbMaxBases) = nbMaxBases;
  *(int*)&(that->_nbMaxLinks) = nbMaxLinks;
  *(int*)&(that->_nbBasesConv) = 0;
  *(int*)&(that->_nbBasesCellConv) = 0;
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

// Create a new NeuraNet with 'nbIn' innput values, 'nbOut' 
// output values and a set of hidden layers described by 
// 'hiddenLayers' as follow:
// The dimension of 'hiddenLayers' is the number of hidden layers
// and each component of 'hiddenLayers' is the number of hidden value 
// in the corresponding hidden layer
// For example, <3,4> means 2 hidden layers, the first one with 3 
// hidden values and the second one with 4 hidden values
// If 'hiddenValues' is null it means there is no hidden layers
// Then, links are automatically added between each input values 
// toward each hidden values in the first hidden layer, then from each 
// hidden values of the first hidden layer to each hidden value of the 
// 2nd hidden layer and so on until each values of the output
NeuraNet* NeuraNetCreateFullyConnected(const int nbIn, const int nbOut, 
  const VecShort* const hiddenLayers) {
#if BUILDMODE == 0
  if (nbIn <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbInput' is invalid (0<%d)", nbIn);
    PBErrCatch(NeuraNetErr);
  }
  if (nbOut <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbOutput' is invalid (0<%d)", nbOut);
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Declare variable to memorize the number of links, bases 
  // and hidden values
  int nbHiddenVal = 0;
  int nbBases = 0;
  int nbLinks = 0;
  int nbHiddenLayer = 0;
  // If there are hidden layers
  if (hiddenLayers != NULL) {
    // Get the number of hidden layers
    nbHiddenLayer = VecGetDim(hiddenLayers);
    // Declare two variables for computation
    int nIn = nbIn;
    int nOut = 0;
    // Calculate the nb of links and hidden values
    for (int iLayer = 0; iLayer < nbHiddenLayer; ++iLayer) {
      nOut = VecGet(hiddenLayers, iLayer);
      nbHiddenVal += nOut;
      nbLinks += nIn * nOut;
      nIn = nOut;
    }
    nbLinks += nIn * nbOut;
  // Else, there is no hidden layers
  } else {
    // Set the number of links
    nbLinks = nbIn * nbOut;
  }
  // There is one base function per link
  nbBases = nbLinks;
  // Create the NeuraNet
  NeuraNet* nn = 
    NeuraNetCreate(nbIn, nbOut, nbHiddenVal, nbBases, nbLinks);
  // Declare a variable to memorize the index of the link
  int iLink = 0;
  // Declare variables for computation
  int shiftIn = 0;
  int shiftOut = nbIn;
  int nIn = nbIn;
  int nOut = 0;
  // Loop on hidden layers
  for (int iLayer = 0; iLayer <= nbHiddenLayer; ++iLayer) {
    // Init the links
    if (iLayer < nbHiddenLayer)
      nOut = VecGet(hiddenLayers, iLayer);
    else
      nOut = nbOut;
    for (int iIn = 0; iIn < nIn; ++iIn) {
      for (int iOut = 0; iOut < nOut; ++iOut) {
        int jLink = NN_NBPARAMLINK * iLink;
        VecSet(nn->_links, jLink, iLink);
        VecSet(nn->_links, jLink + 1, iIn + shiftIn);
        VecSet(nn->_links, jLink + 2, iOut + shiftOut);
        ++iLink;
      }
    }
    shiftIn = shiftOut;
    shiftOut += nOut;
    nIn = nOut;
  }
  // Return the new NeuraNet
  return nn;
}

// Create a NeuraNet using convolution
// The input's dimension is equal to the dimension of 'dimIn', for 
// example if dimIn==<2,3> the input is a 2D array of width 2 and 
// height 3, input values are expected ordered by lines 
// The NeuraNet has 'nbOutput' outputs
// The dimension of each convolution cells is 'dimCell' 
// The maximum number of convolution (in depth) is 'depthConv'
// Each convolution layer has 'thickConv' convolutions in parallel
// The outputs are fully connected to the last layer of convolution cells
// For example, if the input is a 2D array of 4 cols and 3 rows, 2 
// outputs, 2x2 convolution cell, convolution depth of 2, and 
// convolution thickness of 2:
// index of values from input layer to ouput layer
// 00,01,02,03,
// 04,05,06,07,
// 08,09,10,11
//
// 12,13,14,  18,19,20,
// 15,16,17,  21,22,23,
// 
// 24,25  26,27
// 
// 28,29
//
// nbInput: 12
// nbOutput: 2
// nbHidden: 16
// nbMaxBases: 24
// nbMaxLinks: 72
// links:
//    0,0,12, 4,0,18, 1,1,12, 0,1,13, 5,1,18, 4,1,19, 1,2,13, 0,2,14,
//    5,2,19, 4,2,20, 1,3,14, 5,3,20, 2,4,12, 0,4,15, 6,4,18, 4,4,21,
//    3,5,12, 2,5,13, 1,5,15, 0,5,16, 7,5,18, 6,5,19, 5,5,21, 4,5,22,
//    3,6,13, 2,6,14, 1,6,16, 0,6,17, 7,6,19, 6,6,20, 5,6,22, 4,6,23,
//    3,7,14, 1,7,17, 7,7,20, 5,7,23, 2,8,15, 6,8,21, 3,9,15, 2,9,16,
//    7,9,21, 6,9,22, 3,10,16, 2,10,17, 7,10,22, 6,10,23, 3,11,17,
//    7,11,23, 8,12,24, 9,13,24, 8,13,25, 9,14,25, 10,15,24, 11,16,24,
//    10,16,25, 11,17,25, 12,18,26, 13,19,26, 12,19,27, 13,20,27, 
//    14,21,26, 15,22,26, 14,22,27, 15,23,27, 16,24,28, 17,24,29, 
//    18,25,28, 19,25,29, 20,26,28, 21,26,29, 22,27,28, 23,27,29
NeuraNet* NeuraNetCreateConvolution(const VecShort* const dimIn, 
  const int nbOutput, const VecShort* const dimCell, 
  const int depthConv, const int thickConv) {
#if BUILDMODE == 0
  if (dimIn == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'dimIn' is null");
    PBErrCatch(NeuraNetErr);
  }
  for (int iDim = VecGetDim(dimIn); iDim--;)
    if (VecGet(dimIn, iDim) <= 0) {
      NeuraNetErr->_type = PBErrTypeInvalidArg;
      sprintf(NeuraNetErr->_msg, "'dimIn' %dth dim is invalid (%d>0)",
        iDim, VecGet(dimIn, iDim));
      PBErrCatch(NeuraNetErr);
    }
  if (nbOutput <= 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'nbOutput' is invalid (0<%d)", nbOutput);
    PBErrCatch(NeuraNetErr);
  }
  if (dimCell == NULL) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'dimCell' is null");
    PBErrCatch(NeuraNetErr);
  }
  if (VecGetDim(dimCell) != VecGetDim(dimIn)) {
    NeuraNetErr->_type = PBErrTypeNullPointer;
    sprintf(NeuraNetErr->_msg, "'dimCell' 's dim is invalid (%d==%d)",
      VecGetDim(dimCell), VecGetDim(dimIn));
    PBErrCatch(NeuraNetErr);
  }
  for (int iDim = VecGetDim(dimCell); iDim--;)
    if (VecGet(dimCell, iDim) <= 0) {
      NeuraNetErr->_type = PBErrTypeInvalidArg;
      sprintf(NeuraNetErr->_msg, "'dimCell' %dth dim is invalid (%d>0)",
        iDim, VecGet(dimCell, iDim));
      PBErrCatch(NeuraNetErr);
    }
  if (depthConv < 0) {
    NeuraNetErr->_type = PBErrTypeInvalidArg;
    sprintf(NeuraNetErr->_msg, "'depthConv' is invalid (0<=%d)", 
      depthConv);
    PBErrCatch(NeuraNetErr);
  }
#endif
  // Declare a variable to memorize the nb of input, hidden values, 
  // bases and links
  int nbIn = 0;
  int nbHiddenVal = 0;
  int nbBases = 0;
  int nbLinks = 0;
  // Calculate the number of inputs
  nbIn = 1;
  for (int iDim = VecGetDim(dimIn); iDim--;)
    nbIn *= VecGet(dimIn, iDim);
  // Calculate the number of bases, links and hidden values
  // Declare a variable to memorize the number of links per cell
  int nbLinkPerCell = 1;
  for (int iDim = VecGetDim(dimCell); iDim--;)
    nbLinkPerCell *= VecGet(dimCell, iDim);
  // Declare a variable to memorize the position of the convolution
  // cell in the current convolution layer
  VecShort* pos = VecShortCreate(VecGetDim(dimIn));
  // Declare a variable to memorize the position in the convolution cell
  VecShort* posCell = VecShortCreate(VecGetDim(dimIn));
  // Declare variables to memorize the dimension and size of the input 
  // layer at current convolution level
  VecShort* curDimIn = VecClone(dimIn);
  int sizeLayerIn = 1;
  for (int iDim = VecGetDim(curDimIn); iDim--;) {
    sizeLayerIn *= VecGet(curDimIn, iDim);
  }
  // Declare variables to memorize the dimension and size of the 
  // output layer at current convolution level
  VecShort* curDimOut = VecClone(curDimIn);
  int sizeLayerOut = 1;
  for (int iDim = VecGetDim(curDimOut); iDim--;) {
    VecSetAdd(curDimOut, iDim, -1 * VecGet(dimCell, iDim) + 1);
    sizeLayerOut *= VecGet(curDimOut, iDim);
  }
  // Loop on convolution levels
  for (int iConv = 0; iConv < depthConv; ++iConv) {
    // Update the number of bases
    nbBases += nbLinkPerCell;
    // Update the number of hidden values
    nbHiddenVal += sizeLayerOut;
    // Update the number of links
    nbLinks += sizeLayerOut * nbLinkPerCell;
    // If we are not a the last convolution level
    if (iConv < depthConv - 1) {
      // Update input and output dimensions at next convolution level 
      VecCopy(curDimIn, curDimOut);
      sizeLayerIn = sizeLayerOut;
      sizeLayerOut = 1;
      for (int iDim = VecGetDim(curDimOut); iDim--;) {
        VecSetAdd(curDimOut, iDim, -1 * VecGet(dimCell, iDim) + 1);
        sizeLayerOut *= VecGet(curDimOut, iDim);
      }
    }
  }
  // Multiply by the number of convolution in parallel
  nbHiddenVal *= thickConv;
  nbBases *= thickConv;
  nbLinks *= thickConv;
  int nbBasesConv = nbBases;
  // Add the links and bases for the fully connected layer toward output
  nbBases += sizeLayerOut * thickConv * nbOutput;
  nbLinks += sizeLayerOut * thickConv * nbOutput;
  // Create the NeuraNet
  NeuraNet* nn = 
    NeuraNetCreate(nbIn, nbOutput, nbHiddenVal, nbBases, nbLinks);
  *(int*)&(nn->_nbBasesConv) = nbBasesConv;
  *(int*)&(nn->_nbBasesCellConv) = nbLinkPerCell;
  // Declare variables to create the links
  VecShort* links = VecShortCreate(nbLinks * NN_NBPARAMLINK);
  // Declare a variable to memorize the index of the currenlty 
  // created link
  int iLink = 0;
  // Reset the dimension and size of the input layer at current 
  // convolution level
  VecCopy(curDimIn, dimIn);
  sizeLayerIn = 1;
  for (int iDim = VecGetDim(curDimIn); iDim--;) {
    sizeLayerIn *= VecGet(curDimIn, iDim);
  }
  // Reset the dimension and size of the output layer at current 
  // convolution level
  VecCopy(curDimOut, dimIn);
  sizeLayerOut = 1;
  for (int iDim = VecGetDim(curDimOut); iDim--;) {
    VecSetAdd(curDimOut, iDim, -1 * VecGet(dimCell, iDim) + 1);
    sizeLayerOut *= VecGet(curDimOut, iDim);
  }
  // Declare variables to memorize the index of the beginning of the
  // input and output layer and base functions at current convolution 
  // level
  int* iStartBase = PBErrMalloc(NeuraNetErr, sizeof(int) * thickConv);
  int* iStartLayerIn = PBErrMalloc(NeuraNetErr, sizeof(int) * thickConv);
  int* iStartLayerOut = PBErrMalloc(NeuraNetErr, 
    sizeof(int) * thickConv);
  for (int iThick = 0; iThick < thickConv; ++iThick) {
    iStartLayerIn[iThick] = 0;
    iStartLayerOut[iThick] = sizeLayerIn + iThick * sizeLayerOut;
    iStartBase[iThick] = iThick * nbLinkPerCell;
  }
  // Loop on convolution levels
  for (int iConv = 0; iConv < depthConv; ++iConv) {
    // Reset the position of the convolution cell in the input layer
    VecSetNull(pos);
    // Loop on position of the convolution cell at the current 
    // convolution levels
    do {
      do {
        // Loop on convolution in parallel
        for (int iThick = 0; iThick < thickConv; ++iThick) {
          // Declare a variable to memorize the index of the input of the
          // current link
          int iInput = 0;
          for (int iDim = VecGetDim(curDimIn); iDim--;) {
            iInput *= VecGet(curDimIn, iDim);
            iInput += VecGet(posCell, iDim) + VecGet(pos, iDim);
          }
          iInput += iStartLayerIn[iThick];
          // Declare a variable to memorize the index of the output of 
          // the current link
          int iOutput = 0;
          for (int iDim = VecGetDim(curDimOut); iDim--;) {
            iOutput *= VecGet(curDimOut, iDim);
            iOutput += VecGet(pos, iDim);
          }
          iOutput += iStartLayerOut[iThick];
          // Declare a variable to memorize the index of the base of the
          // current link
          int iBase = 0;
          for (int iDim = VecGetDim(posCell); iDim--;) {
            iBase *= VecGet(dimCell, iDim);
            iBase += VecGet(posCell, iDim);
          }
          iBase += iStartBase[iThick];
          // Set the current link's parameters
          VecSet(links, iLink * NN_NBPARAMLINK, iBase);
          VecSet(links, iLink * NN_NBPARAMLINK + 1, iInput);
          VecSet(links, iLink * NN_NBPARAMLINK + 2, iOutput);
          // Increment the index of the current link
          ++iLink;
        }
      } while (VecPStep(posCell, dimCell));
    } while (VecPStep(pos, curDimOut));
    // If we are not at the last convolution level
    if (iConv < depthConv - 1) {
      // Update input and output dimensions at next convolution level 
      VecCopy(curDimIn, curDimOut);
      sizeLayerIn = sizeLayerOut;
      sizeLayerOut = 1;
      for (int iDim = VecGetDim(curDimOut); iDim--;) {
        VecSetAdd(curDimOut, iDim, -1 * VecGet(dimCell, iDim) + 1);
        sizeLayerOut *= VecGet(curDimOut, iDim);
      }
    }
    // Update the start index of input and output layers and bases
    // for each convolution in parallel
    for (int iThick = 0; iThick < thickConv; ++iThick) {
      iStartLayerIn[iThick] = iStartLayerOut[iThick];
      iStartLayerOut[iThick] = iStartLayerIn[0] + 
        thickConv * sizeLayerIn + iThick * sizeLayerOut;
      iStartBase[iThick] = 
        ((iConv + 1) * thickConv + iThick) * nbLinkPerCell;
    }
  }
  // Set the links of the last fully connected layer between last 
  // convolution and NeuraNet output
  // Declare a variable to remember the index of the base
  int iBase = iStartBase[0];
  // Loop on the last output of convolution layer
  for (int iLayerOut = 0; iLayerOut < sizeLayerOut; ++iLayerOut) {
    // Loop on parallel convolution
    for (int iThick = 0; iThick < thickConv; ++iThick) {
      // Loop on output of the NeuraNet
      for (int iOut = 0; iOut < nbOutput; ++iOut) {
        // Declare a variable to memorize the index of the input of 
        // the link
        int iInput = iStartLayerIn[0] + 
          iLayerOut * thickConv + iThick;
        // Declare a variable to memorize the index of the output of 
        // the link
        int iOutput = iOut + nbIn + nbHiddenVal;
        // Set the link's parameters
        VecSet(links, iLink * NN_NBPARAMLINK, iBase);
        VecSet(links, iLink * NN_NBPARAMLINK + 1, iInput);
        VecSet(links, iLink * NN_NBPARAMLINK + 2, iOutput);
        // Increment the link index
        ++iLink;
        // Increment the base function
        ++iBase;
      }
    }
  } 
  // Set up the links
  NNSetLinks(nn, links);
  // Free memory
  VecFree(&links);
  VecFree(&pos);
  VecFree(&posCell);
  VecFree(&curDimIn);
  VecFree(&curDimOut);
  free(iStartBase);
  free(iStartLayerIn);
  free(iStartLayerOut);
  // Return the new NeuraNet
  return nn;
}

// Calculate the output values for the input values 'input' for the 
// NeuraNet 'that' and memorize the result in 'output'
// input values in [-1,1] and output values in [-1,1]
// All values of 'output' are set to 0.0 before evaluating
// Links which refer to values out of bounds of 'input' or 'output'
// are ignored
void NNEval(const NeuraNet* const that, const VecFloat* const input, VecFloat* const output) {
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
  // Reset the hidden values and output
  if (NNGetNbMaxHidden(that) > 0)
    VecSetNull(that->_hidVal);
  VecSetNull(output);
  // If there are links in the network
  if (VecGet(that->_links, 0) != -1) {
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
        // Add the previous output value to the output of the previous 
        // link
        if (prevLink[1] < startOut) {
          int iVal = prevLink[1] - startHid;
          float nVal = MIN(1.0, MAX(-1.0, VecGet(that->_hidVal, iVal) + prevOut));
          VecSet(that->_hidVal, iVal, nVal);
        } else { 
          int iVal = prevLink[1] - startOut;
          float nVal = VecGet(output, iVal) + prevOut;
          VecSet(output, iVal, nVal);
        }
        // Reset the previous output 
        prevOut = 1.0;
      }
      // Update the previous link
      prevLink[0] = VecGet(that->_links, jLink + 1);
      prevLink[1] = VecGet(that->_links, jLink + 2);
      // Multiply the previous output by the evaluation of the current 
      // link with the base function of the link and the normalised 
      // input value
      float* param = that->_bases->_val + 
        VecGet(that->_links, jLink) * NN_NBPARAMBASE;
      float x = 0.0;
      if (prevLink[0] < startHid)
        x = VecGet(input, prevLink[0]);
      else
        x = NNGetHiddenValue(that, prevLink[0] - startHid);
      prevOut *= NNBaseFun(param, x);
      // Move to the next link
      ++iLink;
    }
    // Update the output of the last link
    if (prevLink[1] < startOut) {
      int iVal = prevLink[1] - startHid;
      float nVal = MIN(1.0, MAX(-1.0, VecGet(that->_hidVal, iVal) + prevOut));
      VecSet(that->_hidVal, iVal, nVal);
    } else { 
      int iVal = prevLink[1] - startOut;
      float nVal = VecGet(output, iVal) + prevOut;
      VecSet(output, iVal, nVal);
    }
  }
}

// Function which return the JSON encoding of 'that' 
JSONNode* NNEncodeAsJSON(const NeuraNet* const that) {
#if BUILDMODE == 0
  if (that == NULL) {
    PBMathErr->_type = PBErrTypeNullPointer;
    sprintf(PBMathErr->_msg, "'that' is null");
    PBErrCatch(PBMathErr);
  }
#endif
  // Create the JSON structure
  JSONNode* json = JSONCreate();
  // Declare a buffer to convert value into string
  char val[100];
  // Encode the nbInputVal
  sprintf(val, "%d", that->_nbInputVal);
  JSONAddProp(json, "_nbInputVal", val);
  // Encode the nbOutputVal
  sprintf(val, "%d", that->_nbOutputVal);
  JSONAddProp(json, "_nbOutputVal", val);
  // Encode the nbMaxHidVal
  sprintf(val, "%d", that->_nbMaxHidVal);
  JSONAddProp(json, "_nbMaxHidVal", val);
  // Encode the nbMaxBases
  sprintf(val, "%d", that->_nbMaxBases);
  JSONAddProp(json, "_nbMaxBases", val);
  // Encode the nbMaxLinks
  sprintf(val, "%d", that->_nbMaxLinks);
  JSONAddProp(json, "_nbMaxLinks", val);
  // Encode the bases
  JSONAddProp(json, "_bases", VecEncodeAsJSON(that->_bases));
  // Encode the links
  JSONAddProp(json, "_links", VecEncodeAsJSON(that->_links));
  // Return the created JSON 
  return json;
}

// Function which decode from JSON encoding 'json' to 'that'
bool NNDecodeAsJSON(NeuraNet** that, const JSONNode* const json) {
#if BUILDMODE == 0
  if (that == NULL) {
    PBMathErr->_type = PBErrTypeNullPointer;
    sprintf(PBMathErr->_msg, "'that' is null");
    PBErrCatch(PBMathErr);
  }
  if (json == NULL) {
    PBMathErr->_type = PBErrTypeNullPointer;
    sprintf(PBMathErr->_msg, "'json' is null");
    PBErrCatch(PBMathErr);
  }
#endif
  // If 'that' is already allocated
  if (*that != NULL)
    // Free memory
    NeuraNetFree(that);
  // Decode the nbInputVal
  JSONNode* prop = JSONProperty(json, "_nbInputVal");
  if (prop == NULL) {
    return false;
  }
  int nbInputVal = atoi(JSONLabel(JSONValue(prop, 0)));
  // Decode the nbOutputVal
  prop = JSONProperty(json, "_nbOutputVal");
  if (prop == NULL) {
    return false;
  }
  int nbOutputVal = atoi(JSONLabel(JSONValue(prop, 0)));
  // Decode the nbMaxHidVal
  prop = JSONProperty(json, "_nbMaxHidVal");
  if (prop == NULL) {
    return false;
  }
  int nbMaxHidVal = atoi(JSONLabel(JSONValue(prop, 0)));
  // Decode the nbMaxBases
  prop = JSONProperty(json, "_nbMaxBases");
  if (prop == NULL) {
    return false;
  }
  int nbMaxBases = atoi(JSONLabel(JSONValue(prop, 0)));
  // Decode the nbMaxLinks
  prop = JSONProperty(json, "_nbMaxLinks");
  if (prop == NULL) {
    return false;
  }
  int nbMaxLinks = atoi(JSONLabel(JSONValue(prop, 0)));
  // Allocate memory
  *that = NeuraNetCreate(nbInputVal, nbOutputVal, nbMaxHidVal, 
    nbMaxBases, nbMaxLinks);
  // Decode the bases
  prop = JSONProperty(json, "_bases");
  if (prop == NULL) {
    return false;
  }
  if (!VecDecodeAsJSON(&((*that)->_bases), prop)) {
    return false;
  }
  // Decode the links
  prop = JSONProperty(json, "_links");
  if (prop == NULL) {
    return false;
  }
  if (!VecDecodeAsJSON(&((*that)->_links), prop)) {
    return false;
  }
  // Return the success code
  return true;
}

// Save the NeuraNet 'that' to the stream 'stream'
// If 'compact' equals true it saves in compact form, else it saves in 
// readable form
// Return true if the NeuraNet could be saved, false else
bool NNSave(const NeuraNet* const that, FILE* const stream, const bool compact) {
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
  // Get the JSON encoding
  JSONNode* json = NNEncodeAsJSON(that);
  // Save the JSON
  if (!JSONSave(json, stream, compact)) {
    return false;
  }
  // Free memory
  JSONFree(&json);
  // Return success code
  return true;
}

// Load the NeuraNet 'that' from the stream 'stream'
// If 'that' is not null the memory is first freed 
// Return true if the NeuraNet could be loaded, false else
bool NNLoad(NeuraNet** that, FILE* const stream) {
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
  // Declare a json to load the encoded data
  JSONNode* json = JSONCreate();
  // Load the whole encoded data
  if (!JSONLoad(json, stream)) {
    return false;
  }
  // Decode the data from the JSON
  if (!NNDecodeAsJSON(that, json)) {
    return false;
  }
  // Free the memory used by the JSON
  JSONFree(&json);
  // Return the success code
  return true;
}

// Print the NeuraNet 'that' to the stream 'stream'
void NNPrintln(const NeuraNet* const that, FILE* const stream) {
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
// value of input id and output id, but 'links' doesn't have to be 
// sorted
// Each link is defined by (base index, input index, output index)
// If base index equals -1 it means the link is inactive
void NNSetLinks(NeuraNet* const that, VecShort* const links) {
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
      VecSet(that->_links, iLink * NN_NBPARAMLINK, link[0]);
      if (link[1] <= link[2]) {
        VecSet(that->_links, iLink * NN_NBPARAMLINK + 1, link[1]);
        VecSet(that->_links, iLink * NN_NBPARAMLINK + 2, link[2]);
      } else {
        VecSet(that->_links, iLink * NN_NBPARAMLINK + 1, link[2]);
        VecSet(that->_links, iLink * NN_NBPARAMLINK + 2, link[1]);
      }
      ++iLink;
    } while (GSetIterStep(&iter));
  }
  // Reset the inactive links
  for (int iLink = nbLink; iLink < NNGetNbMaxLinks(that); ++iLink)
    VecSet(that->_links, iLink * NN_NBPARAMLINK, -1);
  // Free the memory
  GSetFlush(&set);
}


