#ifndef LOG_H
#define LOG_H

#include <iostream>

#define LOG_INFO std::cout << "[" <<__FILE__ << " " << __FUNCTION__ << " " << __LINE__ << "]"

#endif // LOG_H