build_args = ['-fpermissive']


viewerlib = shared_library('viewer',
                           'viewer.cpp',
                           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
                           include_directories : inc,
                           cpp_args : build_args,
                           install : true)


streamerlib = shared_library('streamer',
                             'streamer.cpp',
                             dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
                             include_directories : inc,
                             cpp_args : build_args,
                             install : true)

