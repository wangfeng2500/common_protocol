#############################################################  
# Makefile for static library.  
# 编译静态链接库  
#############################################################  
#set your own environment option  

cc = g++
AR = ar
flags = -W -g -Wno-deprecated -lpthread 
INC=-I/usr/local/protobuf2.4.1/include\
    -I/usr/local/sofa-pbrpc/include \
    -I./include
	
TARGET=libcommon.a
PROTO_SRC=netserver.proto

OBJS=$(patsubst src/%.cpp,obj/%.o,$(wildcard src/*.cpp))
OBJS_PB=$(patsubst %.proto,obj/%.pb.o,$(PROTO_SRC))

obj/%.o:src/%.cpp
	$(cc) -c $< $(flags) $(INC) -o $@
obj/%.pb.o:src/%.pb.cc
	$(cc) -c $< $(flags) $(INC) -o $@
%.pb.cc:%.proto
	/usr/local/protobuf2.4.1/bin/protoc -I=./ --cpp_out=./ $<
	mv *.h include/
	mv *.cc src/
	
$(TARGET):$(OBJS_PB) $(OBJS) 
	 ${AR} -r lib/$@ $(OBJS_PB) $(OBJS) 

clean:
	rm -f obj/*.o
	rm -f lib/$(TARGET)
