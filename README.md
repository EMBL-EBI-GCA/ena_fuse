ena_fuse
========

Dependencies:

Fuse http://sourceforge.net/projects/fuse/files/fuse-2.X/

________________________________________________________________________________

The fuse server is started by running the ena_fuse executable with four arguments

    argv[0] : The mount directory. This should be an empty directory.  When the fuse server is running then files and directories will become visible within this directory

    argv[1] : The root directory.  This is the file system containing files and directories which are to be made visible by fuse.

    argv[2] : A 'permissions' file.  This file contains three columns delimited by space, tab or comma.
                Column 1 : The name of the user who wants access for a study
                Column 2 : The study identifier (accession id)
                Column 3 : A password. This can be different for each user/study combination.  It could be a sha512 hash of user + study + a shorter password.
            The user in column 1 gets read-only access to files associated with the study in column 3

    argv[3] : A 'paths' file.  This file contains two columns delimited by space, tab or comma.
                Column 1 : The study identifier; corresponds to a study identifier in the permissions file.
                Column 2 : The path of the file within the root directory (i.e. within argv[1])
            Each file listed in column 2 will be made available to the users who have access to the study in column 1

________________________________________________________________________________

An example scenario:

    A user called 'streeter' is given access to study ERS000001 with password 'streeterERS000001password'
    There is a file whose path relative to the root directory (argv[0] above) is subdir/file3.txt
    This file is associated with study ERS000001.

    The fuse server is initialised.

    The user streeter looks inside the mount directory:
        ls /path/to/mount_dir
    and he sees ...... nothing!

    But, then he looks deeper into the mount directory:
        ls /path/to/mount_dir/streeter/ERS000002/streeterERS000002password
    and he sees directory subdir.  So he copies its contents to his own directory
        cp /path/to/mount_dir/streeter/ERS000002/streeterERS000002password/subdir/file3.txt   /homes/streeter/file3.txt

________________________________________________________________________________

Other options:

    The fuse server can be refreshed by sending it a 'SIGUSR1' signal:
        kill -s USR1 $procid 

    This prompts the fuse server to read the permissions file and paths file to discover any new files and permissions to be added.
