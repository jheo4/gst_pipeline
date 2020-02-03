#include <iostream>
#include <string>
#include <debug.h>

#ifndef __CODEC_PAY__
#define __CODEC_PAY__

using namespace std;

// All the codec-payload mapping should be specified here...

string get_pay_type(string codec)
{
  if(codec.compare("x264enc") == 0)
    return "rtph264pay";
  else {
    DEBUG("Invalid codec type");
    return NULL;
  }
}


string get_depay_type(string codec)
{
  if(codec.compare("avdec_h264") == 0)
    return "rtph264depay";
  else {
    DEBUG("Invalid codec type");
    return NULL;
  }
}

#endif

