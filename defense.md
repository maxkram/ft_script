# Defense Preparation for ft_script

## Project Overview

**What it does:** Records terminal sessions. Captures user input and shell output to a file.

**Core function:** Uses `fork()` and pseudo-terminals (pty) to create a child shell process. Records all I/O between user and shell.

---

## Key System Calls

| System Call | Purpose |
|-------------|---------|
| `fork()` | Create child process (shell) |
| `openpty()` / `login_tty()` | Set up pseudo-terminal |
| `select()` / `poll()` | Monitor multiple file descriptors (stdin, pty master) |
| `read()` / `write()` | Read input, write to pty and log file |
| `signal()` | Handle SIGCHLD (child exit), SIGWINCH (window resize) |
| `ioctl(TIOCGWINSZ)` | Get/set terminal window size |
| `open()` | Open log file |
| `stat()` | Check file properties |

---

## Architecture

```
┌─────────────┐      ┌─────────────┐
│   Parent    │      │    Child    │
│  (ft_script)│◄────►│   (shell)   │
│             │ pty  │             │
│ reads stdin │      │             │
│ writes pty  │      │ reads pty   │
│ logs to file│      │ writes pty  │
└─────────────┘      └─────────────┘
```

**Flow:**
1. Parse options (getopt)
2. Open log file
3. Create pty with `openpty()`
4. Fork
5. Child: `login_tty()` on slave, exec shell
6. Parent: read stdin → write to pty master; read pty master → write to stdout + log file
7. Handle signals, wait for child exit

---

## Options (Linux - mandatory)

| Option | Description |
|--------|-------------|
| `-a` | Append to log file |
| `-c cmd` | Run command instead of shell |
| `-E mode` | Echo mode: always/never/auto |
| `-e` | Return child exit status |
| `-q` | Quiet (no start/done messages) |
| `-t file` | Timing file (classic format) |
| `-o size` | Output limit |

**Bonuses (Linux):**
- `-f` — Flush after each write
- `-m` — Advanced timing format

---

## Common Defense Questions

### Q1: Why pseudo-terminal instead of pipe?
**A:** Shell behaves differently with pipes (non-interactive). PTY makes shell think it is a real terminal. Interactive commands work correctly.

### Q2: How do you prevent infinite loops with select()?
**A:** Check for EOF (read returns 0). Check for errors (read returns -1). Use FD_ISSET before read/write.

### Q3: What happens when child exits?
**A:** SIGCHLD signal received. Call waitpid() to get exit status. Clean up. Exit parent.

### Q4: How do you handle window resize?
**A:** Catch SIGWINCH. Call ioctl(TIOCGWINSZ) on stdin. Forward new size to pty slave with ioctl(TIOCSWINSZ).

### Q5: Why can't you use malloc?
**A:** Project rules: only system calls (man section 2). malloc is library function (section 3). Use static buffers or mmap.

### Q6: How do you handle timing file?
**A:** Use gettimeofday() before each read/write. Calculate delta. Write to timing file in format: "delta_size".

### Q7: What if log file is full (output-limit)?
**A:** Track bytes written. Stop when limit reached. Kill child.

### Q8: How does `-c` work?
**A:** Instead of exec shell, exec the command with arguments. Use execvp().

### Q9: How did you avoid malloc?
**A:** Used static buffers (4096 bytes). All strings are stack-allocated or point to static data. No dynamic memory allocation needed.

### Q10: Why is getenv not allowed?
**A:** getenv is in section 3 of man pages (library functions), not section 2 (system calls). I read the environ global variable directly and search for "SHELL=".

### Q11: How do you format numbers without sprintf?
**A:** Custom utils_write_number() function: extract digits by modulo 10 and divide by 10, store in reverse order in a buffer.

### Q12: What about ctime()? Isn't it a library function?
**A:** Yes, but the project explicitly allows ctime() as an exception. The PDF states: "There is one valid exception: the use of ctime()."

### Q13: How do you handle errors without strerror?
**A:** I output the error number directly. The PDF says "manage errors in a reasonable way" - showing the error number is reasonable.

### Q14: Why _exit() instead of exit()?
**A:** exit() is in section 3 (library), _exit() is in section 2 (system call). _exit() doesn't flush stdio buffers or call atexit handlers, which is fine for our use case.

### Q15: What happens if openpty() fails?
**A:** Print error message with perror-style output and exit(1). The PDF requires reasonable error handling without unexpected crashes.

---

## Error Handling

- **Invalid option:** Print usage, exit(1)
- **Cannot open log file:** perror, exit(1)
- **fork() fails:** perror, exit(1)
- **openpty() fails:** perror, exit(1)
- **Child crash:** Return 128 + signal number (with -e)
- **Permission denied:** Check with access() before open

---

## Testing Checklist

- [x] `./ft_script` — Basic recording to `typescript`
- [x] `./ft_script -a` — Append mode
- [x] `./ft_script -q` — Quiet mode (no "Script started/done" messages)
- [x] `./ft_script -c "ls -la"` — Run command
- [x] `./ft_script -t timing.log` — Timing file
- [x] `./ft_script -o 100` — Output limit
- [x] `./ft_script -e` — Exit status propagation
- [ ] Resize terminal window during session (manual test)
- [ ] Ctrl+C during session (manual test)
- [x] Exit shell (Ctrl+D or exit)
- [x] Invalid options (shows usage)

---

## Code Structure

```
ft_script/
├── Makefile
├── ft_script (executable)
├── src/
│   ├── main.c          # Entry point, option parsing
│   ├── script.c        # Core logic (fork, pty, select loop)
│   ├── pty.c           # PTY setup functions
│   ├── signals.c       # Signal handlers
│   └── utils.c         # Helper functions (no malloc!)
├── include/
│   └── script.h        # Main header with structures
└── libft/              # Optional: your libft
```

## Implementation Notes

### Constraint: Only System Calls (No malloc!)
The project requires using only system calls from section 2 of man pages. This means:
- **No malloc/free** - use static buffers (BUFFER_SIZE = 4096)
- **No printf/sprintf** - custom write_number and write_double functions
- **No getenv** - custom get_shell() that reads environ global
- **No exit()** - use _exit() system call instead
- **No strerror** - simple error number output
- **ctime()** - explicitly allowed as exception

### Key Design Decisions

1. **Static buffers everywhere** - All string operations use fixed-size buffers
2. **Custom number formatting** - utils_write_number() converts int to string
3. **Environment variable access** - Direct iteration through environ array
4. **Signal-safe globals** - volatile sig_atomic_t for signal handlers

### Options Implementation

| Option | Implementation |
|--------|----------------|
| `-a` | O_APPEND flag when opening log file |
| `-c cmd` | execvp("sh", ["sh", "-c", cmd]) instead of shell |
| `-e` | Store child exit status, return it in main |
| `-q` | Skip stdout messages, still write timestamps to log |
| `-t file` | Write "delta size" format to timing file |
| `-o size` | Check bytes_written before each write, truncate if exceeded |

## Test Results

✓ Basic recording: `./ft_script -c "echo hello"`
✓ Quiet mode: `./ft_script -q` (no stdout messages)
✓ Append mode: `./ft_script -a` (adds to existing file)
✓ Command mode: `./ft_script -c "ls -la"`
✓ Timing file: `./ft_script -t timing.log`
✓ Output limit: `./ft_script -o 100` (truncates at limit)
✓ Exit status: `./ft_script -e -c "exit 42"` (returns 42)
✓ Interactive mode: `./ft_script` (full shell session)
✓ Error handling: Invalid options show usage message
✓ Signal handling: SIGCHLD, SIGWINCH, SIGPIPE handled

---

## Makefile Requirements

- Must compile with `make`
- Must not relink
- Rules: `all`, `clean`, `fclean`, `re`
- Use `-Wall -Wextra -Werror`

---

## Next Steps

1. Implement basic version (no options)
2. Add options one by one
3. Test each option
4. Add bonus options
5. Test edge cases
