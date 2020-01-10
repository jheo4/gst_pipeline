#include <gst/gst.h>

#ifndef __STREAM_SESSION__
#define __STREAM_SESSION__

// Stream: audio/video bin with id and # of references
typedef struct _StreamSession_t
{
  int num_ref;
  guint id;
  GstElement *bin, *rtp_bin;
  GstCaps* caps;
} StreamSession_t;


// increase # of the given session & return it
static StreamSession_t* stream_session_ref(StreamSession_t* stream_session)
{
  g_atomic_int_inc(&stream_session->num_ref);
  return stream_session;
}


// if there is no ref, free the session
static void stream_session_unref(StreamSession_t* stream_session)
{
  if(g_atomic_int_dec_and_test(&stream_session->num_ref))
    g_free((gpointer)stream_session);
}


// create a new empty session
static StreamSession_t* create_stream_session(guint session_id)
{
  StreamSession_t *new_session = g_new0(StreamSession_t, 1);
  new_session->id = session_id;
  return stream_session_ref(new_session);
}
#endif
