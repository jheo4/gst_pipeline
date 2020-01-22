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

pb_server = pb_gen.process('helloworld.proto', preserve_path_from : meson.current_source_dir())
grpc_server = grpc_gen.process('helloworld.proto', preserve_path_from : meson.current_source_dir())

