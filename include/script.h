#ifndef SCRIPT_H
# define SCRIPT_H

# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/wait.h>
# include <sys/select.h>
# include <sys/ioctl.h>
# include <sys/time.h>
# include <fcntl.h>
# include <termios.h>
# include <signal.h>
# include <errno.h>
# include <pty.h>

# define BUFFER_SIZE	4096
# define DEFAULT_FILE	"typescript"

typedef struct s_script
{
	int		master_fd;
	int		slave_fd;
	int		log_fd;
	int		timing_fd;
	int		output_limit;
	int		bytes_written;
	int		quiet;
	int		append;
	int		flush;
	int		exit_status;
	char	*command;
	char	*timing_file;
	pid_t	child_pid;
	struct termios	orig_termios;
	struct winsize	winsize;
}	t_script;

void	script_init(t_script *script);
int		script_parse_args(t_script *script, int argc, char **argv);
int		script_run(t_script *script, int argc, char **argv, int file_idx);
void	script_cleanup(t_script *script);

int		pty_create(t_script *script);
int		pty_setup_child(t_script *script);
void	pty_resize(t_script *script);

void	signals_setup(t_script *script);
void	signals_handle_sigchld(int sig);
void	signals_handle_sigwinch(int sig);

void	utils_write(int fd, const char *str);
void	utils_write_number(int fd, int num);
void	utils_write_double(int fd, double num);
char	*utils_ctime(const time_t *timep);
void	utils_perror(const char *msg);

extern volatile sig_atomic_t	g_child_exited;
extern volatile sig_atomic_t	g_window_resized;

#endif
