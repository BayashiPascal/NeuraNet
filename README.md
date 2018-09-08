# NeuraNet
NeuraNet is a C library providing structures and functions to implement a neural network.

The neural network implemented in NeuraNet consists of a layer of input values, a layer of output values, a layer of hidden values, a set of generic base functions and a set of links. Each base function has 3 parameters (detailed below) and each links has 3 parameters: the base function index and the indices of input and output values. A NeuraNet is defined by the parameters' values of its generic base functions and links, and the number of input, output and hidden values.

The evaluation of the NeuraNet consists of taking each link, ordered on index of values, and apply the generic base function on the first value and store the result in the second value. If several links has the same second value index, the sum value of all these links is used. However if several links have same input and output values, the outputs of these links are multiplied instead of added (before being eventually added to other links having same output value but different input value).

The generic base functions is a linear function. However by using several links with same input and output values it is possible to simulate any polynomial function. Also, there is no concept of layer inside hidden values, but the input value index is constrained to be lower than the output one. So, the links can be arranged to form layers of subset of hidden values, while still allowing any other type of arrangement inside hidden values. Also, a link can be inactivated by setting its base function index to -1. Finally, the parameters of the base function are constrained to [-1.0,1.0].

NeuraNet provides functions to easily use the library GenAlg to search the values of base functions and links' parameters. An example is given in the unit tests (see below). It also provides functions to save and load the neural network (in JSON format).

NeuraNet has been validated on 
* the Iris data set: classification, 4 inputs, 3 outputs, 75 learning samples, 75 validation samples, https://archive.ics.uci.edu/ml/datasets/iris , 94.6% correct classification on validation data after learning during 0s 
* the Abalone data set : regression, 10 inputs, 1 output, 3000 learning samples, 1177 validation samples, http://www.cs.toronto.edu/~delve/data/abalone/desc.html , 25.7% correct regression, 66.5% +-1 error, 83.3% +-2, 91.5% +-3, 94.8% +-4, 96.9% +-5, 98.1% +-6, 99.2% +-7, 99.7% +-8, on validation data after learning during 19m:49s 
* the Arrhythmia data set: classification, 279 inputs, 16 outputs, 300 learning samples, 152 validation samples, https://archive.ics.uci.edu/ml/datasets/arrhythmia , 41.4% correct classification on validation data after learning during 02:04:49s (68.6% on the learning samples)
* the Wisconsin Diagnostic Breast Cancer data set: classification, 30 inputs, 2 outputs, 400 learning samples, 169 validation samples, https://archive.ics.uci.edu/ml/datasets/Breast+Cancer+Wisconsin+%28Diagnostic%29 , 98.2% correct classification in 01:42s
* the MNIST data set: classification, 784 inputs, 9 outputs, 50000 learning samples, 10000 validation samples,  http://yann.lecun.com/exdb/mnist/ , 81.1% correct classification on validation data after learning during 01:01:30:11s
* the ORHD data set: classification, 64 inputs, 9 outputs, 3823 learning samples, 1797 validation samples, https://archive.ics.uci.edu/ml/datasets/Optical+Recognition+of+Handwritten+Digits , 88.2% correct classification on validation data after learning during 12:05:51s
