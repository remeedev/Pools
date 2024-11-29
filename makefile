pools:
	winegcc -o pools main.c -mwindows -lmsimg32 -lgdi32
	./pools.exe
