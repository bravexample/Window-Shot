all:
	gcc -O3 -Wall source/main.c -lgdi32 -o window_shot.exe
	.\window_shot.exe