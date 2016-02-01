#############################################################  
# Makefile for executable file.  
# 编译静态链接库，每个cpp编写为一个可执行程序 
#############################################################  
#set your own environment option  

cc = g++
flags = -W -g -Wno-deprecated
INC=-I./common/include\
	-I/usr/local/protobuf2.4.1/include\
    -I/usr/local/sofa-pbrpc/include \
    
LIBS=-L./common/lib -lcommon\
	 -L/usr/local/sofa-pbrpc/lib -lsofa-pbrpc \
     -L/usr/local/protobuf2.4.1/lib -lprotobuf -lprotoc -lprotobuf-lite\
	 -ldl -lpthread -lz
	 
TARGET=$(patsubst %.cpp,%,$(wildcard *.cpp))
	 
.PHONY:all clean  
all:$(TARGET) 
	
%:%.cpp
	@echo "正在编译:" $<"---->"$@  
	$(cc) -o $@ $(flags) $< $(LIBS) $(INC)

clean:
	rm -f obj/*.o $(TARGET)
