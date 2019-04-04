# Yet Another Divide and Conquer Framework

This framework can be used to parallelize a Divide and Conquer (DAC) algorithm. It is a C++14 reimplementation of the one described in [1]. It relies on a scheduler which adopts a *dynamic load-balancing* based on Pearson's *chi-squared test* (χ²). More details can be found in [this short report](https://github.com/flandolfi/SPM-project/raw/master/report/main.pdf).

This was presented as a final project of "Parallel and Distributed Systems: Paradigms and Models" (SPM), a course at University of Pisa taught by [Marco Danelutto](http://www.di.unipi.it/~marcod).


## References ##
[1] Danelutto, M., De Matteis, T., Mencagli, G., & Torquati, M. (2016, October). A divide-and-conquer parallel pattern implementation for multicores. In Proceedings of the 3rd International Workshop on Software Engineering for Parallel Systems (pp. 10-19). ACM.
