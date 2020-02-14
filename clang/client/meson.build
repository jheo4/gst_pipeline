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

grpcserverlib = shared_library('grpc_server',
                               'grpc_server.cpp', server_pb, server_grpc,
                               dependencies : [protobuf_dep, grpcpp_dep, grpc_reflection_dep],
                               include_directories : inc,
                               cpp_args : build_args,
                               install : true)

grpcclientlib = shared_library('grpc_client',
                               'grpc_client.cpp', server_pb, server_grpc,
                               dependencies : [protobuf_dep, grpcpp_dep, grpc_reflection_dep],
                               include_directories : inc,
                               cpp_args : build_args,
                               install : true)

