#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <stdlib.h>

#define currentTime chrono::high_resolution_clock::now
#define timeStamp chrono::high_resolution_clock::time_point
#define elapsedTime(end, start) chrono::duration_cast<chrono::milliseconds>(end - start).count()

#endif
