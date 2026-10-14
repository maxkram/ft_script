#include "script.h"
#include <time.h>

void	utils_write(int fd, const char *str)
{
	int	i;

	if (!str)
		return ;
	i = 0;
	while (str[i])
		i++;
	write(fd, str, i);
}

void	utils_write_number(int fd, int num)
{
	char	buf[32];
	int		i;
	int		neg;

	i = 31;
	buf[i] = '\0';
	neg = 0;
	if (num < 0)
	{
		neg = 1;
		num = -num;
	}
	if (num == 0)
	{
		buf[--i] = '0';
	}
	else
	{
		while (num > 0)
		{
			buf[--i] = (num % 10) + '0';
			num /= 10;
		}
	}
	if (neg)
		buf[--i] = '-';
	utils_write(fd, &buf[i]);
}

void	utils_write_double(int fd, double num)
{
	int		int_part;
	int		frac_part;

	int_part = (int)num;
	frac_part = (int)((num - int_part) * 1000000);
	if (frac_part < 0)
		frac_part = -frac_part;
	utils_write_number(fd, int_part);
	utils_write(fd, ".");
	if (frac_part < 10)
		utils_write(fd, "00000");
	else if (frac_part < 100)
		utils_write(fd, "0000");
	else if (frac_part < 1000)
		utils_write(fd, "000");
	else if (frac_part < 10000)
		utils_write(fd, "00");
	else if (frac_part < 100000)
		utils_write(fd, "0");
	utils_write_number(fd, frac_part);
}

char	*utils_ctime(const time_t *timep)
{
	return (ctime(timep));
}

void	utils_perror(const char *msg)
{
	int		err_num;

	err_num = errno;
	if (msg)
	{
		utils_write(STDERR_FILENO, msg);
		utils_write(STDERR_FILENO, ": ");
	}
	utils_write(STDERR_FILENO, "Error no. ");
	utils_write_number(STDERR_FILENO, err_num);
	utils_write(STDERR_FILENO, "\n");
}
