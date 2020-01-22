# Projecte Settings
project('MPAR', 'cpp',
        meson_version : '>=0.45',
        version : '1.0.0',
        license : 'MIT')
add_global_arguments(language : 'cpp')
cppc = meson.get_compiler('cpp')

# Server & Client only

# Protobuf & gRPC Settings
#protoc = find_program('protoc', required : true)
#protobuf_dep = dependency('protobuf', method : 'pkg-config', required : true)

grpc_cpp = find_program('grpc_cpp_plugin', required : true)
grpcpp_dep = dependency('grpc++', method : 'pkg-config', required : true)
grpc_dep = dependency('grpc', method : 'pkg-config', required : true)
grpc_reflection_dep = cppc.find_library('grpc++_reflection', required: false)

#if not protoc.found() or not protobuf_dep.found() or not grpcpp_dep.found() or not grpc_dep.found() or not grpc_cpp.found()
#  error('MESON_SKIP_TEST: protobuf, grpc dependencies not found')
#endif

## Protobuf Generator
#pb_gen = generator(protoc, \
#                   output : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
#                   arguments : ['--proto_path=@CURRENT_SOURCE_DIR@',
#                                '--cpp_out=@BUILD_DIR@', '@INPUT@'])
#grpc_gen = generator(protoc, \
#                     output : ['@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
#                     arguments : ['--proto_path=@CURRENT_SOURCE_DIR@',
#                                  '--plugin=protoc-gen-grpc=' + grpc_cpp.path(),
#                                  '--grpc_out=@BUILD_DIR@', '@INPUT@'])



# GStreamer Settings
gst_dep = dependency('gstreamer-1.0', method : 'pkg-config', required : true)
gst_aud_dep = dependency('gstreamer-video-1.0', method : 'pkg-config', required : true)
gst_vid_dep = dependency('gstreamer-audio-1.0', method : 'pkg-config', required : true)
if not gst_dep.found() or not gst_aud_dep.found() or not gst_vid_dep.found()
  error('MESON_SKIP_TEST: gstreamer dependencies not found')
endif


# Include Directory
inc = include_directories('include')

subdir('src')
#subdir('proto')

# Build Options
build_args = ['-fpermissive']

# Target Executables
executable('client', 'client.cpp',
           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
           include_directories : inc,
           link_with : viewerlib,
           cpp_args : build_args, )

executable('server', 'server.cpp',
           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
           include_directories : inc,
           link_with : streamerlib,
           cpp_args : build_args, )

# GRPC Example
#pb_server = pb_gen.process('helloworld.proto', preserve_path_from : meson.current_source_dir())
#grpc_server = grpc_gen.process('helloworld.proto', preserve_path_from : meson.current_source_dir())
#executable('greeter_server', 'greeter_server.cc', pb_server, grpc_server,
#           dependencies : [protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep])
