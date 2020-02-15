build_args = ['-fpermissive']


clientpipelinelib = shared_library('clientpipeline',
                                   'pipeline.cc',
                                   dependencies : [gst_dep, gst_aud_dep, gst_vid_dep],
                                   include_directories : inc,
                                   cpp_args : build_args,
                                   install : true)

#grpcclientlib = shared_library('grpc_client',
#                               'grpc_client.cc', server_pb, server_grpc,
#                               dependencies : [protobuf_dep, grpcpp_dep, grpc_reflection_dep],
#                               include_directories : inc,
#                               cpp_args : build_args,
#                               install : true)

