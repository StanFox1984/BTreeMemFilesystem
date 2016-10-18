all:
	g++ -Wall filesystem.cpp -I. -I/usr/local/include/fuse3  -pthread -L/usr/local/lib -lfuse3 -o filesystem -fpermissive -g

clean:
	rm -f filesystem