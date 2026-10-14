NAME	=	ft_script

SRCS	=	src/main.c \
			src/script.c \
			src/pty.c \
			src/signals.c \
			src/utils.c

OBJS	=	$(SRCS:.c=.o)

CC		=	gcc
CFLAGS	=	-Wall -Wextra -Werror -Iinclude
LDFLAGS	=	-lutil

all:		$(NAME)

$(NAME):	$(OBJS)
			$(CC) $(OBJS) -o $(NAME) $(LDFLAGS)

%.o:		%.c
			$(CC) $(CFLAGS) -c $< -o $@

clean:
			rm -f $(OBJS)

fclean:		clean
			rm -f $(NAME)

re:			fclean all

.PHONY:		all clean fclean re
