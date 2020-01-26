# Protobuf Generator
pb_gen = generator(protoc, \
                   output : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
                   arguments : ['--proto_path=@CURRENT_SOURCE_DIR@',
                                '--cpp_out=@BUILD_DIR@', '@INPUT@'])
grpc_gen = generator(protoc, \
                     output : ['@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
                     arguments : ['--proto_path=@CURRENT_SOURCE_DIR@',
                                  '--plugin=protoc-gen-grpc=' + grpc_cpp.path(),
                                  '--grpc_out=@BUILD_DIR@', '@INPUT@'])

server_pb = pb_gen.process('binder.proto', preserve_path_from : meson.current_source_dir())
server_grpc = grpc_gen.process('binder.proto', preserve_path_from : meson.current_source_dir())

#client_pb = pb_gen.process('client.proto', preserve_path_from : meson.current_source_dir())
#client_grpc = grpc_gen.process('client.proto', preserve_path_from : meson.current_source_dir())

