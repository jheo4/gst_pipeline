project('video-streamer', 'c',
        meson_version : '>=0.45',
        version : '1.0.0',
        license : 'MIT')

add_global_arguments(language : 'c')

gst_dep = dependency('gstreamer-1.0', method : 'pkg-config')
gst_aud_dep = dependency('gstreamer-video-1.0')
gst_vid_dep = dependency('gstreamer-audio-1.0')

inc = include_directories('inc')

#executable('server', 'server.c', dependencies : [gst_dep, gst_aud_dep, gst_vid_dep], include_directories : inc)
executable('client', 'client.c', dependencies : [gst_dep, gst_aud_dep, gst_vid_dep], include_directories : inc)
executable('creator', 'creator.c', dependencies : [gst_dep, gst_aud_dep, gst_vid_dep], include_directories : inc)
