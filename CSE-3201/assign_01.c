/****************************************************************
 *
 *  Custom Shell - OS Lab Assignment [SH_01]
 *
 *  Implements:
 *    - Basic shell loop (input, parse, execute)
 *    - Commands: pwd, ls, cd, mkdir, touch, rm, cp, mv, cat, echo
 *    - Built-ins: cd, echo, exit
 *    - Optional: command arguments, error handling, pipes,
 *                I/O redirection, background execution
 *
 ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>

#define MAX_INPUT   1024
#define MAX_ARGS    64
#define MAX_PATH    512

/* ──────────────────────────────────────────────
   Built-in: pwd
   Uses getcwd() system call
────────────────────────────────────────────── */
void cmd_pwd() {
    char cwd[MAX_PATH];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s\n", cwd);
    else
        perror("pwd");
}

/* ──────────────────────────────────────────────
   Built-in: cd
   Uses chdir() system call
────────────────────────────────────────────── */
void cmd_cd(char *path) {
    if (path == NULL || strcmp(path, "~") == 0) {
        path = getenv("HOME");
        if (path == NULL) { fprintf(stderr, "cd: HOME not set\n"); return; }
    }
    if (chdir(path) != 0)
        perror("cd");
}

/* ──────────────────────────────────────────────
   Built-in: echo
   Prints all arguments separated by spaces
────────────────────────────────────────────── */
void cmd_echo(char **args) {
    int i = 1;
    while (args[i] != NULL) {
        printf("%s", args[i]);
        if (args[i+1] != NULL) printf(" ");
        i++;
    }
    printf("\n");
}

/* ──────────────────────────────────────────────
   Implemented: ls
   Uses opendir/readdir — no execve
────────────────────────────────────────────── */
void cmd_ls(char *path) {
    if (path == NULL) path = ".";
    DIR *dir = opendir(path);
    if (dir == NULL) { perror("ls"); return; }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* skip hidden files (starting with .) */
        if (entry->d_name[0] != '.')
            printf("%s\n", entry->d_name);
    }
    closedir(dir);
}

/* ──────────────────────────────────────────────
   Implemented: mkdir
   Uses mkdir() system call
────────────────────────────────────────────── */
void cmd_mkdir(char *name) {
    if (name == NULL) { fprintf(stderr, "mkdir: missing operand\n"); return; }
    if (mkdir(name) != 0) perror("mkdir");
}

/* ──────────────────────────────────────────────
   Implemented: touch
   Creates file if it does not exist
────────────────────────────────────────────── */
void cmd_touch(char *name) {
    if (name == NULL) { fprintf(stderr, "touch: missing operand\n"); return; }
    int fd = open(name, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) perror("touch");
    else close(fd);
}

/* ──────────────────────────────────────────────
   Implemented: rm
   Uses unlink() system call
────────────────────────────────────────────── */
void cmd_rm(char *name) {
    if (name == NULL) { fprintf(stderr, "rm: missing operand\n"); return; }
    if (unlink(name) != 0) perror("rm");
}

/* ──────────────────────────────────────────────
   Implemented: cat
   Reads and prints file contents
────────────────────────────────────────────── */
void cmd_cat(char *name) {
    if (name == NULL) { fprintf(stderr, "cat: missing operand\n"); return; }
    int fd = open(name, O_RDONLY);
    if (fd < 0) { perror("cat"); return; }
    char buf[512];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0)
        write(STDOUT_FILENO, buf, n);
    close(fd);
}

/* ──────────────────────────────────────────────
   Implemented: cp
   Copies src to dst
────────────────────────────────────────────── */
void cmd_cp(char *src, char *dst) {
    if (!src || !dst) { fprintf(stderr, "cp: missing operand\n"); return; }
    int in  = open(src, O_RDONLY);
    if (in < 0) { perror("cp"); return; }
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { perror("cp"); close(in); return; }
    char buf[512]; ssize_t n;
    while ((n = read(in, buf, sizeof(buf))) > 0)
        write(out, buf, n);
    close(in); close(out);
}

/* ──────────────────────────────────────────────
   Implemented: mv
   Uses rename() system call
────────────────────────────────────────────── */
void cmd_mv(char *src, char *dst) {
    if (!src || !dst) { fprintf(stderr, "mv: missing operand\n"); return; }
    if (rename(src, dst) != 0) perror("mv");
}

/* ──────────────────────────────────────────────
   BONUS: pipe handler
   Splits "cmd1 | cmd2" and connects them
────────────────────────────────────────────── */
void run_pipe(char *left_cmd, char *right_cmd) {
    int pipefd[2];
    if (pipe(pipefd) < 0) { perror("pipe"); return; }

    pid_t pid1 = fork();
    if (pid1 == 0) {
        /* left side writes to pipe */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        char *args[MAX_ARGS]; int i = 0;
        char *tok = strtok(left_cmd, " \t");
        while (tok) { args[i++] = tok; tok = strtok(NULL, " \t"); }
        args[i] = NULL;
        execvp(args[0], args);
        perror(args[0]); exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        /* right side reads from pipe */
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]); close(pipefd[1]);
        char *args[MAX_ARGS]; int i = 0;
        char *tok = strtok(right_cmd, " \t");
        while (tok) { args[i++] = tok; tok = strtok(NULL, " \t"); }
        args[i] = NULL;
        execvp(args[0], args);
        perror(args[0]); exit(1);
    }

    close(pipefd[0]); close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

/* ──────────────────────────────────────────────
   BONUS: I/O redirection
   Handles > (output) and < (input) in args
────────────────────────────────────────────── */
void handle_redirection(char **args, int *argc) {
    for (int i = 0; i < *argc; i++) {
        if (strcmp(args[i], ">") == 0 && args[i+1]) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) { perror("redirection"); return; }
            dup2(fd, STDOUT_FILENO); close(fd);
            args[i] = NULL; *argc = i;
        } else if (strcmp(args[i], "<") == 0 && args[i+1]) {
            int fd = open(args[i+1], O_RDONLY);
            if (fd < 0) { perror("redirection"); return; }
            dup2(fd, STDIN_FILENO); close(fd);
            args[i] = NULL; *argc = i;
        }
    }
}

/* ──────────────────────────────────────────────
   Parse input into args array
   Returns number of args
────────────────────────────────────────────── */
int parse_input(char *input, char **args) {
    int count = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && count < MAX_ARGS - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[count] = NULL;
    return count;
}

/* ──────────────────────────────────────────────
   Execute external command using fork + execvp
   Supports background execution with &
────────────────────────────────────────────── */
void execute(char **args, int argc, int background) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return;
    }
    if (pid == 0) {
        /* child process */
        handle_redirection(args, &argc);
        execvp(args[0], args);
        /* if execvp returns, command was not found */
        fprintf(stderr, "myshell: %s: command not found\n", args[0]);
        exit(1);
    } else {
        /* parent process */
        if (!background)
            waitpid(pid, NULL, 0);
        else
            printf("[background] pid %d\n", pid);
    }
}

/* ──────────────────────────────────────────────
   MAIN SHELL LOOP
────────────────────────────────────────────── */
int main() {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("Welcome to MyShell! Type 'exit' to quit.\n\n");

    while (1) {
        /* print prompt with current directory */
        char cwd[MAX_PATH];
        if (getcwd(cwd, sizeof(cwd)))
            printf("myshell:%s$ ", cwd);
        else
            printf("myshell$ ");
        fflush(stdout);

        /* read input */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n"); break;
        }

        /* skip empty lines */
        if (input[0] == '\n') continue;

        /* remove trailing newline */
        input[strcspn(input, "\n")] = 0;

        /* BONUS: check for pipe */
        char *pipe_pos = strchr(input, '|');
        if (pipe_pos) {
            *pipe_pos = '\0';
            run_pipe(input, pipe_pos + 1);
            continue;
        }

        /* BONUS: check for background execution */
        int background = 0;
        int len = strlen(input);
        if (len > 0 && input[len-1] == '&') {
            background = 1;
            input[len-1] = '\0';
        }

        /* parse into args */
        int argc = parse_input(input, args);
        if (argc == 0) continue;

        /* ── built-in commands ── */
        if (strcmp(args[0], "exit") == 0) {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(args[0], "cd") == 0) {
            cmd_cd(args[1]);
        }
        else if (strcmp(args[0], "echo") == 0) {
            cmd_echo(args);
        }
        /* ── manually implemented commands ── */
        else if (strcmp(args[0], "pwd") == 0) {
            cmd_pwd();
        }
        else if (strcmp(args[0], "ls") == 0) {
            cmd_ls(args[1]);
        }
        else if (strcmp(args[0], "mkdir") == 0) {
            cmd_mkdir(args[1]);
        }
        else if (strcmp(args[0], "touch") == 0) {
            cmd_touch(args[1]);
        }
        else if (strcmp(args[0], "rm") == 0) {
            cmd_rm(args[1]);
        }
        else if (strcmp(args[0], "cat") == 0) {
            cmd_cat(args[1]);
        }
        else if (strcmp(args[0], "cp") == 0) {
            cmd_cp(args[1], args[2]);
        }
        else if (strcmp(args[0], "mv") == 0) {
            cmd_mv(args[1], args[2]);
        }
        /* ── unknown: try as external command ── */
        else {
            execute(args, argc, background);
        }
    }

    return 0;
}