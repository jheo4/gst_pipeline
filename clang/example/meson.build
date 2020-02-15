#executable('client', 'ex_client.cpp', server_pb, server_grpc,
#           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
#           include_directories : inc,
#           link_with : [viewerlib, grpcclientlib],
#           cpp_args : build_args, )

#executable('server', 'ex_server.cpp', server_pb, server_grpc,
#           cpp_args : build_args,
#           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
#           include_directories : inc,
#           link_with : [piplinelib, grpcserverlib],)

executable('server', 'server.cc',
           cpp_args : build_args,
           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
           include_directories : inc,
           link_with : [serverpipelinelib],)

executable('client', 'client.cc',
           dependencies : [gst_dep, gst_aud_dep, gst_vid_dep, protobuf_dep, grpcpp_dep, grpc_dep, grpc_reflection_dep],
           include_directories : inc,
           link_with : [clientpipelinelib],
           cpp_args : build_args, )

