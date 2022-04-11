#include<string.h>
#include<ctype.h>
#include <iostream>
#include <iomanip>

#ifndef _UTIL_
#define _UTIL_

using namespace std;
bool IsNumber(char* str) {
  int length = strlen(str);
  for (int i = 0; i < length; ++i) {
    if (!isdigit(str[i]))
      return false;
    }
  return true;
}

class formatted_output
{
    private:
        int width;
        ostream& stream_obj;

    public:
        formatted_output(ostream& obj, int w): width(w), stream_obj(obj) {}

        template<typename T>
        formatted_output& operator<<(const T& output)
        {
            stream_obj << setw(width) << output;

            return *this;
        }

        formatted_output& operator<<(ostream& (*func)(ostream&))
        {
            func(stream_obj);
            return *this;
        }
};
#endif