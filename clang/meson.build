# Projecte Settings
project('MPAR', 'cpp',
        meson_version : '>=0.45',
        version : '1.0.0',
        license : 'MIT')
add_global_arguments(language : 'cpp')
cppc = meson.get_compiler('cpp')

# Server & Client only

# Protobuf & gRPC Settings
protoc = find_program('protoc', required : true)
protobuf_dep = dependency('protobuf', method : 'pkg-config', required : true)

grpc_cpp = find_program('grpc_cpp_plugin', required : true)
grpcpp_dep = dependency('grpc++', method : 'pkg-config', required : true)
grpc_dep = dependency('grpc', method : 'pkg-config', required : true)
grpc_reflection_dep = cppc.find_library('grpc++_reflection', required: false)

if not protoc.found() or not protobuf_dep.found() or not grpcpp_dep.found() or not grpc_dep.found() or not grpc_cpp.found()
  error('MESON_SKIP_TEST: protobuf, grpc dependencies not found')
endif
subdir('proto')

# GStreamer Settings
gst_dep = dependency('gstreamer-1.0', method : 'pkg-config', required : true)
gst_aud_dep = dependency('gstreamer-video-1.0', method : 'pkg-config', required : true)
gst_vid_dep = dependency('gstreamer-audio-1.0', method : 'pkg-config', required : true)
if not gst_dep.found() or not gst_aud_dep.found() or not gst_vid_dep.found()
  error('MESON_SKIP_TEST: gstreamer dependencies not found')
endif


# Include Directory
inc = include_directories('.')
subdir('common')
subdir('server')
subdir('client')
subdir('example')

# Build Options
build_args = ['-fpermissive', '-pthread']

# Target Executables
#executable('client', 'ex_client.cpp', server_pb, server_grpc,
#           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
#           include_directories : inc,
#           link_with : [viewerlib, grpcclientlib],
#           cpp_args : build_args, )

#executable('server', 'ex_server.cpp', server_pb, server_grpc,
#           cpp_args : build_args,
#           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
#           include_directories : inc,
#           link_with : [streamerlib, grpcserverlib],)

# GRPC Example
#executable('greeter_server', 'greeter_server.cc', pb_server, grpc_server,
#           dependencies : [protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep])
