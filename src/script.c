#include "script.h"
#include <string.h>
#include <time.h>

extern char	**environ;

volatile sig_atomic_t	g_child_exited = 0;
volatile sig_atomic_t	g_window_resized = 0;

static int	open_log_file(t_script *script, char *filename)
{
	int	flags;

	flags = O_WRONLY | O_CREAT;
	if (script->append)
		flags |= O_APPEND;
	else
		flags |= O_TRUNC;
	script->log_fd = open(filename, flags, 0644);
	if (script->log_fd < 0)
	{
		utils_perror(filename);
		return (-1);
	}
	return (0);
}

static int	open_timing_file(t_script *script)
{
	if (!script->timing_file)
		return (0);
	script->timing_fd = open(script->timing_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (script->timing_fd < 0)
	{
		utils_perror(script->timing_file);
		return (-1);
	}
	return (0);
}

static void	write_start_message(t_script *script, char *filename)
{
	time_t	now;
	char	buf[BUFFER_SIZE];
	char	*time_str;
	int		i;

	if (!script->quiet)
	{
		utils_write(STDOUT_FILENO, "Script started, file is ");
		utils_write(STDOUT_FILENO, filename);
		utils_write(STDOUT_FILENO, "\n");
	}
	now = time(NULL);
	time_str = utils_ctime(&now);
	i = 0;
	while (time_str[i] && i < BUFFER_SIZE - 1)
	{
		buf[i] = time_str[i];
		i++;
	}
	buf[i] = '\0';
	utils_write(script->log_fd, "Script started on ");
	utils_write(script->log_fd, buf);
}

static void	write_done_message(t_script *script)
{
	time_t	now;
	char	buf[BUFFER_SIZE];
	char	*time_str;
	int		i;

	now = time(NULL);
	time_str = utils_ctime(&now);
	i = 0;
	while (time_str[i] && i < BUFFER_SIZE - 1)
	{
		buf[i] = time_str[i];
		i++;
	}
	buf[i] = '\0';
	utils_write(script->log_fd, "Script done on ");
	utils_write(script->log_fd, buf);
	if (!script->quiet)
	{
		utils_write(STDOUT_FILENO, "\nScript done, file is ");
		if (script->timing_file)
		{
			utils_write(STDOUT_FILENO, script->timing_file);
			utils_write(STDOUT_FILENO, "\n");
		}
		else
			utils_write(STDOUT_FILENO, "typescript\n");
	}
}

static char	*get_shell(void)
{
	int	i;
	int	j;

	i = 0;
	while (environ[i])
	{
		j = 0;
		while (environ[i][j] && environ[i][j] != '=')
			j++;
		if (j == 5 && environ[i][0] == 'S' && environ[i][1] == 'H' &&
			environ[i][2] == 'E' && environ[i][3] == 'L' && environ[i][4] == 'L')
			return (&environ[i][j + 1]);
		i++;
	}
	return ("/bin/sh");
}

static void	child_process(t_script *script, char *command)
{
	char	*shell;
	char	*args[4];

	pty_setup_child(script);
	close(script->master_fd);
	if (command)
	{
		args[0] = "sh";
		args[1] = "-c";
		args[2] = command;
		args[3] = NULL;
		execvp("sh", args);
	}
	else
	{
		shell = get_shell();
		args[0] = shell;
		args[1] = NULL;
		execvp(shell, args);
	}
	utils_perror("execvp");
	_exit(1);
}

static void	parent_process(t_script *script)
{
	fd_set	readfds;
	char	buffer[BUFFER_SIZE];
	int		maxfd;
	int		nread;
	struct timeval	last_time;
	struct timeval	current_time;
	double			delta;

	if (script->timing_file)
		gettimeofday(&last_time, NULL);
	maxfd = script->master_fd;
	if (STDIN_FILENO > maxfd)
		maxfd = STDIN_FILENO;
	while (!g_child_exited || script->master_fd >= 0)
	{
		if (g_window_resized)
		{
			pty_resize(script);
			g_window_resized = 0;
		}
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds);
		if (script->master_fd >= 0)
			FD_SET(script->master_fd, &readfds);
		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0)
		{
			if (errno == EINTR)
				continue;
			break;
		}
		if (FD_ISSET(STDIN_FILENO, &readfds))
		{
			nread = read(STDIN_FILENO, buffer, BUFFER_SIZE);
			if (nread > 0)
			{
				write(script->master_fd, buffer, nread);
				if (script->timing_file)
				{
					gettimeofday(&current_time, NULL);
					delta = (current_time.tv_sec - last_time.tv_sec) +
							(current_time.tv_usec - last_time.tv_usec) / 1000000.0;
					utils_write_double(script->timing_fd, delta);
					utils_write(script->timing_fd, " ");
					utils_write_number(script->timing_fd, nread);
					utils_write(script->timing_fd, "\n");
					last_time = current_time;
				}
			}
		}
		if (script->master_fd >= 0 && FD_ISSET(script->master_fd, &readfds))
		{
			nread = read(script->master_fd, buffer, BUFFER_SIZE);
			if (nread > 0)
			{
				write(STDOUT_FILENO, buffer, nread);
				if (script->output_limit > 0)
				{
					if (script->bytes_written + nread > script->output_limit)
					{
						nread = script->output_limit - script->bytes_written;
						if (nread > 0)
							write(script->log_fd, buffer, nread);
						script->bytes_written = script->output_limit;
						kill(script->child_pid, SIGTERM);
						break;
					}
					script->bytes_written += nread;
				}
				write(script->log_fd, buffer, nread);
			}
			else if (nread <= 0)
				break;
		}
	}
	write_done_message(script);
}

int	script_run(t_script *script, int argc, char **argv, int file_idx)
{
	char	*filename;
	int		status;

	filename = (file_idx > 0 && file_idx < argc) ? argv[file_idx] : DEFAULT_FILE;
	if (open_log_file(script, filename) < 0)
		return (1);
	if (open_timing_file(script) < 0)
		return (1);
	if (pty_create(script) < 0)
		return (1);
	signals_setup(script);
	tcgetattr(STDIN_FILENO, &script->orig_termios);
	script->child_pid = fork();
	if (script->child_pid < 0)
	{
		utils_perror("fork");
		return (1);
	}
	if (script->child_pid == 0)
		child_process(script, script->command);
	else
	{
		close(script->slave_fd);
		script->slave_fd = -1;
		write_start_message(script, filename);
		parent_process(script);
		waitpid(script->child_pid, &status, 0);
		script->child_pid = status;
		tcsetattr(STDIN_FILENO, TCSANOW, &script->orig_termios);
	}
	return (0);
}

void	script_cleanup(t_script *script)
{
	if (script->master_fd >= 0)
		close(script->master_fd);
	if (script->slave_fd >= 0)
		close(script->slave_fd);
	if (script->log_fd >= 0)
		close(script->log_fd);
	if (script->timing_fd >= 0)
		close(script->timing_fd);
}
