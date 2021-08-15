#include <pty.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utmp.h>

int main(void) {
	int fd_master, fd_slave;
	struct winsize parent_winsize;
	struct termios orig_termios, raw_termios;
	tcgetattr(STDOUT_FILENO, &orig_termios);
	cfmakeraw(&raw_termios);
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &raw_termios);
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &parent_winsize);
	struct winsize child_winsize = {
		.ws_row = parent_winsize.ws_row - 1,
		.ws_col = parent_winsize.ws_col
		/* don't need the other two */
	};
	int result = openpty(
		&fd_master,
		&fd_slave,
		NULL,       /* filename of the slave */
		NULL,       /* struct termios */
		&child_winsize
	);
	if (result == -1) {
		perror(NULL);
		return -1;
	}
	pid_t child_pid = fork();
	if (child_pid == 0) { /* this is the child */
		login_tty(fd_slave);
		//execlp("nvim", (char *) NULL);
		printf("debug: first message\n");
		sleep(3);
		printf("debug: second message\n");
		sleep(3);
		printf("debug: third message\n");
		return 0;
	} else { /* this is the parent */
		size_t buffer_len = 256;
		unsigned char buffer[buffer_len];
		size_t iteration = 0;
		fd_set set_read, set_write;
		while (1) { /* loop until child exits */
			//int child_status;
			//waitpid(child_pid, &child_status, WNOHANG);
			//if (WIFEXITED(child_status)) { /* if child has exited */
			//	fprintf(stderr, "child exited at check %ld\r\n", iteration);
			//	break;
			//}
			//iteration++;
			
			/* initialize file descriptor sets for select(2) */
			FD_ZERO(&set_read);
			FD_SET(STDIN_FILENO, &set_read);
			FD_SET(fd_master, &set_read);
			int nfds = (STDIN_FILENO > fd_master) ? STDIN_FILENO : fd_master;
			nfds++;

			select(nfds, &set_read, NULL, NULL, NULL);

			/* parent stdin -> child stdin */
			if FD_ISSET(STDIN_FILENO, &set_read) {
				ssize_t bytes_written, bytes_read = read(STDIN_FILENO, buffer, buffer_len);
				fprintf(stderr, "%d bytes read from stdin\r\n", bytes_read);
				if (bytes_read < 0) {
					perror("reading from parent stdin");
					return -1; /* TODO: clean up child */
				} else if (bytes_read > 0) {
					if (buffer[0] == 'q')
						break; /* TODO: clean up child */
					bytes_written = write(fd_master, buffer, bytes_read);
					if (bytes_written < 0) {
						perror("writing to child stdin");
						return -1; /* TODO: clean up child */
					}
				}
			}

			/* child stdout -> parent stdout */
			if FD_ISSET(fd_master, &set_read) {
				ssize_t bytes_written, bytes_read = read(fd_master, buffer, buffer_len);
				fprintf(stderr, "%d bytes read from fd_master\r\n", bytes_read);
				if (bytes_read < 0) {
					perror("reading from child stdout");
					return -1; /* TODO: clean up child */
				} else if (bytes_read > 0) {
					bytes_written = write(STDOUT_FILENO, buffer, bytes_read);
					if (bytes_written < 0) {
						perror("writing to parent stdout");
						return -1; /* TODO: clean up child */
					}
				}
			}
		}
	}

	tcsetattr(STDOUT_FILENO, TCSADRAIN, &orig_termios);
	fprintf(stderr, "successful return\n");
	close(fd_slave); /* TODO: no idea if this is how this should be done */
	close(fd_master);
	return 0;
}
