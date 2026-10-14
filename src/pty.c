#include "script.h"
#include <string.h>

int	pty_create(t_script *script)
{
	struct winsize	ws;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
	{
		ws.ws_col = 80;
		ws.ws_row = 24;
		ws.ws_xpixel = 0;
		ws.ws_ypixel = 0;
	}
	script->winsize = ws;
	if (openpty(&script->master_fd, &script->slave_fd, NULL, NULL, &ws) < 0)
	{
		utils_perror("openpty");
		return (-1);
	}
	return (0);
}

int	pty_setup_child(t_script *script)
{
	struct termios	term;

	setsid();
	if (ioctl(script->slave_fd, TIOCSCTTY, 0) < 0)
	{
		utils_perror("ioctl TIOCSCTTY");
		return (-1);
	}
	dup2(script->slave_fd, STDIN_FILENO);
	dup2(script->slave_fd, STDOUT_FILENO);
	dup2(script->slave_fd, STDERR_FILENO);
	if (script->slave_fd > STDERR_FILENO)
		close(script->slave_fd);
	tcgetattr(STDIN_FILENO, &term);
	term.c_lflag |= ECHO | ICANON | ISIG | IEXTEN;
	term.c_iflag |= ICRNL | IXON;
	term.c_oflag |= OPOST | ONLCR;
	term.c_cflag |= CS8;
	tcsetattr(STDIN_FILENO, TCSANOW, &term);
	return (0);
}

void	pty_resize(t_script *script)
{
	struct winsize	ws;

	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
		return ;
	script->winsize = ws;
	if (script->master_fd >= 0)
		ioctl(script->master_fd, TIOCSWINSZ, &ws);
}
