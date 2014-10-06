FLAGS=`pkg-config --cflags --libs gstreamer-1.0` \
	-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads/ \
	-I/opt/vc/include/interface/vmcs_host/linux/ -I/opt/vc/userland

all:
	gcc -g -c RaspiCapture.c $(FLAGS)
	gcc -g -c RaspiCamControl.c $(FLAGS)
	gcc -g -c RaspiPreview.c $(FLAGS)
	gcc -g -c gstrpicam-enum-types.c $(FLAGS)
	gcc -g -c gstrpicamsrc.c $(FLAGS)
	ld -g -r *.o -o rpicamsrc.o
	ar -rcs libgstrpicamsrc.a rpicamsrc.o
 
clean:
	rm -rf *.o libgstrpicamsrc.a


