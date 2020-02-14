#include <thread>
#include <gst/gst.h>

#ifndef __BINDER__
#define __BINDER__

using namespace std;

typedef struct _PipeThread_t
{
  thread trd;
  guint id;
} PipeThread_t;

class Binder
{
private:

public:
};
#endif
