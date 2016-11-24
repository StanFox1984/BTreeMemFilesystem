all:test_btree
	g++ -Wall filesystem.cpp -I. -I/usr/local/include/fuse3  -pthread -L/usr/local/lib -lfuse3 -o filesystem -fpermissive -g

clean:test_umount
	rm -f filesystem test_btree btree_test_file memory_file /btree_file /btree_file_backup /memory_file /memory_file_backup ./btree_log;sudo killall -9 filesystem

install:
	mkdir /mnt/btree_mem_filesystem

test_btree:
	g++ test_btree.cpp -fpermissive -g -o test_btree

test_mount:
	sudo LD_LIBRARY_PATH=/usr/local/lib ./filesystem /mnt/btree_mem_filesystem -d 2>&1 | tee ./btree_log &

test_umount:
	sudo fusermount -u /mnt/btree_mem_filesystem

test_gdb:
	sudo LD_LIBRARY_PATH=/usr/local/lib gdb ./filesystem

test_files:
	sudo cp -rf test_dir1 /mnt/btree_mem_filesystem