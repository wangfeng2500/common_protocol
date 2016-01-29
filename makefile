all:
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/protobuf2.4.1/lib
	make -f makefile_server
	make -f makefile_client

clean:
	make -f makefile_server clean
	make -f makefile_client clean