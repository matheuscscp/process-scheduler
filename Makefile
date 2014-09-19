all:
	g++ src/main.cpp src/execprocd.cpp src/execproc.cpp src/cancela_proc.cpp src/termina_execprocd.cpp src/Message.cpp src/MessageBox.cpp src/Time.cpp src/ProcessLauncher.cpp -o trabalho-so
	g++ src/infinito.cpp -o infinito
	g++ src/nsegs.cpp -D"NSEGS=31" -o 31segs
