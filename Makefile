all:test_btree
	g++ -Wall filesystem.cpp -I. -I/usr/local/include/fuse3  -pthread -L/usr/local/lib -lfuse3 -o filesystem -fpermissive -g

clean:
	rm -f filesystem test_btree btree_test_file memory_file

test_btree:
	g++ test_btree.cpp -fpermissive -g -o test_btree

test_mount:
	sudo LD_LIBRARY_PATH=/usr/local/lib ./filesystem /mnt/filesystem

test_umount:
	fusermount -u /mnt/filesystem