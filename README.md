# NeuraNet
NeuraNet is a C library providing structures and functions to implement a neural network.

The neural network implemented in NeuraNet consists of a layer of input values, a layer of output values, a layer of hidden values, a set of generic base functions and a set of links. Each base function has 3 parameters (detailed below) and each links has 3 parameters: the base function index and the indices of two values. A NeuraNet is defined by the parameters' values of its generic base functions and links, and the number of input, output and hidden values.

The evaluation of the NeuraNet consists of taking each link, ordered on index of values, and apply the generic base function on the first value and store the result in the second value. If several links has the same second value index, the average value of all these links is used. However if several links have same first and second value, these values are multiplied instead of average (but they can still be part of an average value due to other links having same second value).

The generic base functions is a linear function. However by using several links with same first and second value it is possible to simulate any polynomial function. Also, there is no concept of layer inside hidden values, but the first value index is constrained to be lower than the second one. So, the links can be arranged to form layers of subset of hidden values, while still allowing any other type of arrangement inisde hidden values. Also, a link can be inactivated by setting its base function index to -1. Finally, all values are constrained to [-1.0,1.0].

NeuraNet provides functions to easily use the library GenAlg to search the values of base functions and links' parameters.
