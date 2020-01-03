#include <gst/gst.h>

#ifndef __SESSION__
#define __SESSION__

// session: audio/video bin with id and # of references
typedef struct _SessionData_t
{
  int num_ref;
  guint id;
  GstElement *bin, *rtp_bin;
  GstCaps* caps;
} SessionData_t;


// increase # of the given session & return it
static SessionData_t* session_ref(SessionData_t* session)
{
  g_atomic_int_inc(&session->num_ref);
  return session;
}


// if there is no ref, free the session
static void session_unref(SessionData_t* session)
{
  if(g_atomic_int_dec_and_test(&session->num_ref))
    g_free((gpointer)session);
}


// create a new empty session
static SessionData_t* make_session(guint session_id)
{
  SessionData_t *new_session = g_new0(SessionData_t, 1);
  new_session->id = session_id;
  return session_ref(new_session);
}
#endif
