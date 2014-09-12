all:
	g++ src/main.cpp src/execprocd.cpp src/execproc.cpp src/cancela_proc.cpp src/termina_execprocd.cpp src/Message.cpp src/MessageBox.cpp src/Time.cpp -o trabalho-so -lSDL2
	g++ src/infinito.cpp -o infinito
	g++ src/31segs.cpp -o 31segs
