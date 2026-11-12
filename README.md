# CSCI 4061: Introduction to Operating Systems

My coursework for CSCI 4061 in Spring 2023 at the University of Minnesota. The course covers Unix systems programming: file I/O, processes, signals, pipes, sockets, and threads. Everything here is written in C and is intended to be run on Linux.


## Repository layout

| Path | Contents |
|---|---|
| [`proj01-code/`](proj01-code/) | P1: `minitar`, a subset of the POSIX `tar` utility |
| [`proj2-code/`](proj2-code/) | P2: `swish`, a shell with job control and I/O redirection |
| [`proj3-code/`](proj3-code/) | P3: process based word counting and shell pipelines |
| [`proj4-code/`](proj4-code/) | P4: a multithreaded HTTP file server |
| [`proj1-code.zip`](proj1-code.zip), [`proj2-code.zip`](proj2-code.zip), [`proj3-code.zip`](proj3-code.zip), [`proj4-code.zip`](proj4-code.zip) | Original project submissions |
| `lab*-code/`, `lab*-solution/` | Weekly labs; see [Labs](#labs) |


## Projects

### P1: minitar

[`proj01-code/`](proj01-code/) contains `minitar`, an implementation of a small portion of `tar`. It can create, append to, list, update, and extract archives. Archive entries use 512-byte POSIX ustar headers populated from `stat()`. File data follows each header and is padded to a 512-byte boundary.

Appending removes the old end-of-archive blocks, writes the new entries, and restores the footer. Updating verifies that each named file is already present before appending a newer copy. When extracting, the last copy of a file wins. Data is copied with `fread()` and `fwrite()`, so the program handles both binary and text file types.

- **Files I wrote:** archive operations in `minitar.c`, including creation, appending, listing, extraction, and supporting helpers, argument parsing in `minitar_main.c`
- **Topics:** file I/O, binary formats, `stat()`, and file seeking
- **Run:** `make CFLAGS="-Wall -g"`, then `./minitar -c -f archive.tar file1.txt file2.txt`. Use `-a` to append, `-t` to list, `-u` to update, or `-x` to extract. Plain `make` treats warnings as errors, which stops the build on newer GCC versions.

### P2: swish

[`proj2-code/`](proj2-code/) is `swish`, a small Unix shell. It includes `pwd`, `cd`, and `exit` as built-in commands. Other commands run in child processes created with `fork()` and executed with `execvp()`. The child handles `<` and `>` redirection by opening the requested file and replacing standard input or output with `dup2()`.

`swish` also supports job control. Each command gets its own process group. For foreground jobs, the shell transfers terminal ownership with `tcsetpgrp()` and waits with `WUNTRACED`. Jobs stopped with Ctrl-Z are retained in the job list. Commands ending in `&` run in the background. The `fg`, `bg`, `jobs`, `wait-for`, and `wait-all` commands manage background and stopped jobs. The shell ignores `SIGTTIN` and `SIGTTOU` while managing the terminal.

- **Files I wrote:** `swish_funcs.c` for tokenization, process execution, redirection, and job control and  `swish.c` for built-in commands and process launching
- **Topics:** `fork()`, `execvp()`, `waitpid()`, process groups, terminal control, signals, file descriptors, and `dup2()`
- **Run:** `make swish`, then `./swish`. Plain `make` also tries to build a test helper whose source is not included.

### P3: Pipes

[`proj3-code/`](proj3-code/) has two parts.

**Parallel word lengths ([`part1/`](proj3-code/part1/)).** `par_word_lengths` counts words of lengths 1 through 25 across one or more text files. The program forks one child per file. Each child counts its file and writes an array of counts to a shared pipe. The parent reads one result per child and combines them into the final totals.

**Shell pipelines ([`part2/`](proj3-code/part2/)).** This part adds pipelines to `swish`. For each `|`, the shell creates a pipe. It forks one child for each command and uses `dup2()` to connect standard input and output to the appropriate pipe ends. The parent closes its copies of the pipe descriptors and waits for the pipeline to finish.

- **Files I wrote:** `part1/par_word_lengths.c` and `part2/swish_funcs.c`
- **Topics:** pipes, process communication, descriptor cleanup, and waiting for multiple children
- **Run:** Build each part from inside its own folder, since running `make` from `proj3-code/` does not build either part. Use `make` in `part1/` and `make CFLAGS="-Wall -g"` in `part2/`; plain `make` in `part2/` treats warnings as errors, which stops the build on newer GCC versions. Then run `./par_word_lengths file1.txt file2.txt` from `part1/`. From `part2/`, start `./swish` and enter a pipeline such as `cat file.txt | grep the | wc -l` at its `@>` prompt. Part 2 links against a precompiled `swish_funcs_provided.o`, so it builds only on x86-64 Linux.

### P4: HTTP server

[`proj4-code/`](proj4-code/) implements a server that serves files from a local directory.

**Single-threaded server ([`part1/`](proj4-code/part1/)).** The server handles one client connection at a time. It creates a listening TCP socket with `getaddrinfo()`, `socket()`, `bind()`, and `listen()`. It parses HTTP/1.0 `GET` requests and returns the requested file with `Content-Type` and `Content-Length` headers. Missing files receive a `404 Not Found` response. Ctrl-C triggers a clean shutdown.

**Thread pool server ([`part2/`](proj4-code/part2/)).** This part makes the server concurrent with a producer-consumer design. The main thread accepts client connections and puts their sockets in a bounded circular queue with room for five entries. Five worker threads remove connections from that queue and serve them. A mutex lock and two condition variables protect the queue. Worker threads block signals so that the main thread alone receives `SIGINT`. During shutdown, waiting threads are awakened, the worker threads are joined, and the queue is freed. The log in `part2/test_results/` includes a passing concurrent-clients test.

- **Files I wrote:** request parsing and response code in `http.c`, plus `part1/http_server.c`, `part2/http_server.c`, and `part2/connection_queue.c`
- **Topics:** TCP sockets, HTTP, POSIX threads, mutexes, condition variables, bounded buffers, and thread signal masks
- **Run:** `./http_server server_files 8000`, then `curl http://localhost:8000/index.html` from another terminal. The served directory is relative to the current working directory. Course test files are not included, so create `server_files/` with your own content.


## Labs

| Folder | Topic |
|---|---|
| [`lab01-code/`](lab01-code/) | Linked lists of strings |
| [`lab02-code/`](lab02-code/) | File copying with a buffer and seeking within binary files |
| [`lab03-code/`](lab03-code/) | Processes with `fork()`, `wait()`, and `exec` |
| [`lab05-code/`](lab05-code/) | Shell process management and tokenizing text with `strtok()` |
| [`lab06-solution/`](lab06-solution/) | Signals: a `wc`-style program that reports its counts after Ctrl-C using `sigaction()` |
| [`lab10-code/`](lab10-code/) | UDP client that looks up a course name from a department and course number |
| [`lab11-code/`](lab11-code/), [`lab11-solution/`](lab11-solution/) | TCP HTTP client |
| [`lab12-code/`](lab12-code/) | Thread-safe queue using a mutex and condition variables |
| [`lab13-code/`](lab13-code/) | Thread barrier built with semaphores |

The Lab 10 and Lab 11 clients use a hard-coded course-server IP address, which may no longer be available.


## Building and running

The code requires Linux, `gcc`, and `make`. The `testius` test runner requires Python 3.6 or later. The labs can be built and tested the same way:

```bash
cd lab12-code
make             # build
make test        # run all tests
make test-code   # run code tests only
make test-quiz   # check QUESTIONS.txt answers only
```

Build P1 with `make CFLAGS="-Wall -g"`, P2 with `make swish`, and P4 with `make`, each from its own directory. P3 has to be built inside `part1/` with `make` and inside `part2/` with `make CFLAGS="-Wall -g"`. Setting `CFLAGS` this way drops the Makefiles' `-Werror` flag, which stops P1 and P3 part 2 from building on newer GCC versions. The project directories were extracted from submission archives and do not include the original tests or test data, so `make test` is not expected to work for the projects.


## Acknowledgments

- Course staff provided the assignments, starter code, and tests.
- John Kolb wrote the GPLv3 `testius` test runner, based on Chris Kauffman's `testy`.
