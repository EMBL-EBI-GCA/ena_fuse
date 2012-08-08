mkdir mount_dir

../src/ena_fuse mount_dir root_dir permissions paths

echo 'ls mount_dir/streeter/ERS000001/streeterERS000001password'
ls mount_dir/streeter/ERS000001/streeterERS000001password

echo 'ls mount_dir/streeter/ERS000002/streeterERS000002password'
ls mount_dir/streeter/ERS000002/streeterERS000002password

fusermount -u mount_dir
