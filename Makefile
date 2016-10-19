all:test_btree
	g++ -Wall filesystem.cpp -I. -I/usr/local/include/fuse3  -pthread -L/usr/local/lib -lfuse3 -o filesystem -fpermissive -g

clean:
	rm -f filesystem test_btree

test_btree:
	g++ test_btree.cpp -fpermissive -g -o test_btree