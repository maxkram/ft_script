#include "script.h"
#include <string.h>

static void	print_usage(void)
{
	utils_write(STDERR_FILENO, "Usage: ft_script [-a] [-c command] [-e] [-q] [-t timing] [-o size] [file]\n");
}

static int	parse_output_limit(char *str)
{
	int	limit;

	limit = 0;
	while (*str >= '0' && *str <= '9')
	{
		limit = limit * 10 + (*str - '0');
		str++;
	}
	return (limit);
}

int	script_parse_args(t_script *script, int argc, char **argv)
{
	int	opt;
	int	i;

	opterr = 0;
	while ((opt = getopt(argc, argv, "ac:eqt:o:")) != -1)
	{
		if (opt == 'a')
			script->append = 1;
		else if (opt == 'c')
			script->command = optarg;
		else if (opt == 'e')
			script->exit_status = 1;
		else if (opt == 'q')
			script->quiet = 1;
		else if (opt == 't')
			script->timing_file = optarg;
		else if (opt == 'o')
			script->output_limit = parse_output_limit(optarg);
		else
		{
			print_usage();
			return (-1);
		}
	}
	i = optind;
	if (i < argc)
		return (i);
	return (0);
}

void	script_init(t_script *script)
{
	int	i;
	char	*ptr;

	ptr = (char *)script;
	i = 0;
	while (i < (int)sizeof(t_script))
	{
		ptr[i] = 0;
		i++;
	}
	script->master_fd = -1;
	script->slave_fd = -1;
	script->log_fd = -1;
	script->timing_fd = -1;
}

int	main(int argc, char **argv)
{
	t_script	script;
	int			file_idx;
	int			ret;

	script_init(&script);
	file_idx = script_parse_args(&script, argc, argv);
	if (file_idx < 0)
		return (1);
	ret = script_run(&script, argc, argv, file_idx);
	script_cleanup(&script);
	if (script.exit_status && WIFEXITED(script.child_pid))
		return (WEXITSTATUS(script.child_pid));
	return (ret);
}
