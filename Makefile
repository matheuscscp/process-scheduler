all:
	g++ src/main.cpp src/execprocd.cpp src/execproc.cpp src/cancela_proc.cpp src/termina_execprocd.cpp src/Message.cpp src/MessageBox.cpp src/Time.cpp src/ProcessLauncher.cpp -o trabalho-so
	g++ src/test_program.cpp -o test_program
