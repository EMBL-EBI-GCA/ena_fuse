ena_fuse : ena_fuse.o ena_data.o ena_permission.o ena_dirstruct.o
	gcc -g `pkg-config fuse --libs` -o ena_fuse ena_fuse.o ena_data.o ena_permission.o ena_dirstruct.o

ena_fuse.o : ena_fuse.c ena_data.h
	gcc -g -Wall `pkg-config fuse --cflags` -c ena_fuse.c

ena_data.o : ena_data.c ena_data.h
	gcc -g -Wall -c ena_data.c

ena_permission.o : ena_permission.c ena_permission.h
	gcc -g -Wall -c ena_permission.c

ena_dirstruct.o : ena_dirstruct.c ena_dirstruct.h
	gcc -g -Wall -c ena_dirstruct.c

clean:
	rm -f ena_fuse *.o
