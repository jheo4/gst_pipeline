project('video-streamer', 'c',
        meson_version : '>=0.45',
        version : '1.0.0',
        license : 'MIT')

add_global_arguments(language : 'c')

# glib_dep = dependency('glib-2.0')
# gobj_dep = dependency('gobject-2.0')
# gtk_dep = dependency('gtk-3.0')
gst_dep = dependency('gstreamer-1.0', method : 'pkg-config')
gst_aud_dep = dependency('gstreamer-video-1.0')
gst_vid_dep = dependency('gstreamer-audio-1.0')

inc = include_directories('inc')

# executable('server', 'server.c', dependencies : [glib_dep, gobj_dep, gtk_dep, gst_dep, gst_aud_dep, gst_vid_dep])
executable('server', 'server.c', dependencies : [gst_dep, gst_aud_dep, gst_vid_dep], include_directories : inc)
executable('client', 'client.c', dependencies : [gst_dep, gst_aud_dep, gst_vid_dep], include_directories : inc)
