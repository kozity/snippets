#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#define TP_MAX_X 1212
#define TP_MAX_Y 792

void main() {
	// Open the touchpad device for reading.
	char touchpad_path[] = "/dev/input/event7";
	int touchpad_desc = open(touchpad_path, O_RDONLY);
	if (touchpad_desc == -1) {
		perror("error opening touchpad");
		fprintf(stderr, "\t@ %s\n", touchpad_path);
		return;
	}

	// Enter raw mode.
	struct termios term_info;
	tcgetattr(STDIN_FILENO, &term_info);
	term_info.c_lflag &= ~(ECHO | ICANON);
	term_info.c_cc[VMIN] = 0;
	term_info.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_info);
	struct input_event ev;
	int read_status = 0;
	struct Point {
		int x;
		int y;
	} point = { .x = 0, .y = 0 };
	struct winsize w;
	ioctl(STDIN_FILENO, TIOCGWINSZ, &w);
	int cols = w.ws_col;
	int rows = w.ws_row;
	while ((read_status = read(touchpad_desc, &ev, sizeof(struct input_event))) != -1) {
		int code = ev.code;
		int value = ev.value;
		if (ev.type == EV_ABS) {
			if (code == ABS_X)
				point.x = value;
			else if (code == ABS_Y)
				point.y = value;
			int thou_x = (point.x * 1000) / TP_MAX_X;
			int thou_y = (point.y * 1000) / TP_MAX_Y;
			int col = (thou_x * cols) / 1000;
			int row = (thou_y * rows) / 1000;
			printf("g\x1b[%d;%dH", row, col);
			fflush(stdout);
		}
	}
	term_info.c_lflag |= ECHO | ICANON;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &term_info);
	close(touchpad_desc);
}
