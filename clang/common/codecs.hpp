#include <iostream>
#include <string>
#include <common/debug.h>

#ifndef __CODECS__
#define __CODECS__

// All the codec-payload mapping should be specified here...

std::string get_pay_type(std::string codec)
{
  if(codec.compare("x264enc") == 0)
    return "rtph264pay";
  else {
    DEBUG("Invalid codec type");
    return NULL;
  }
}


std::string get_depay_type(std::string codec)
{
  if(codec.compare("avdec_h264") == 0)
    return "rtph264depay";
  else {
    DEBUG("Invalid codec type");
    return NULL;
  }
}


std::string get_gpu_codec(std::string codec)
{
  // TODO
  // return string codec to gpu codec
  return NULL;
}


std::string get_caps_type(std::string codec)
{
  if(codec.compare("avdec_h264") == 0)
    return "H264";
  else {
    DEBUG("Invalid codec type");
    return NULL;
  }
}

#endif

