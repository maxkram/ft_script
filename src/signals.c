#define _POSIX_C_SOURCE 200809L
#include <signal.h>

#include "script.h"

void	signals_handle_sigchld(int sig)
{
	(void)sig;
	g_child_exited = 1;
}

void	signals_handle_sigwinch(int sig)
{
	(void)sig;
	g_window_resized = 1;
}

void	signals_setup(t_script *script)
{
	struct sigaction	sa;

	(void)script;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = signals_handle_sigchld;
	sigaction(SIGCHLD, &sa, NULL);
	sa.sa_handler = signals_handle_sigwinch;
	sigaction(SIGWINCH, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
}
