# common_protocol
简介：最简单的tcpserver，使用单进程阻塞方式，主要是为了解决tcp的粘包、分包的问题，定义了一个通用的包结构，包头+包体，包体是pb协议
本代码编译和安装时都使用了pb库，运行时需要pb的so库，所以需要首先指明路径：
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/protobuf2.4.1/lib
然后在运行才可以

