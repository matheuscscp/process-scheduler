all:
	g++ src/main.cpp src/execprocd.cpp src/execproc.cpp src/cancela_proc.cpp src/termina_execprocd.cpp src/Message.cpp src/MessageBox.cpp src/Time.cpp -o trabalho-so -lSDL2
	g++ src/infinito.cpp -o infinito
	g++ src/nsegs.cpp -D"NSEGS=32" -o 31segs
