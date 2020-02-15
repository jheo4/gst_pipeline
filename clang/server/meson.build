build_args = ['-fpermissive']


serverpipelinelib = shared_library('serverpipeline',
                                   'pipeline.cc',
                                   dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
                                   cpp_args : build_args,
                                   include_directories : inc,
                                   install : true)

#grpcserverlib = shared_library('grpc_server',
#                               'grpc_server.cc', server_pb, server_grpc,
#                               dependencies : [protobuf_dep, grpcpp_dep, grpc_reflection_dep],
#                               cpp_args : build_args,
#                               include_directories : inc,
#                               install : true)

