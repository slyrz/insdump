/**
 * insdump - instruction dump.
 */
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/user.h>
#include <sys/wait.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <bfd.h>
#include <dis-asm.h>

/**
 * An x86 instruction doesn't exceed 15 bytes. Since reading a memory address
 * with ptrace() returns a long value, we use long to store the instruction at
 * the instruction pointer. So if the size of long is 8 bytes, we use long[2]
 * to store the current instruction and if the size is only 4 bytes, we have to
 * use long[4].
 */
#if (BFD_ARCH_SIZE == 64)
#define WORDS_PER_INSTR 2
#else
#define WORDS_PER_INSTR 4
#endif

/**
 * The parent process is the process which traces, disassembles and prints the
 * commands instruction. The child process is executing the command.
 */
enum id_role { ID_PARENT, ID_CHILD };

struct id_args {
  int argc;
  char **argv;
};

struct id_buffer {
  size_t cap;
  size_t len;
  char data[128];
};

struct id_instr {
  long ip;
  long data[WORDS_PER_INSTR];
};

struct id_pids {
  pid_t child;
  pid_t parent;
};

/**
 * The fprintf function callback which must be passed to libopcodes. Instead
 * of printing the output directly to stdout, we write it to a buffer so that
 * we can get the print_insn_i386 result (the size of the instruction)
 * and print the raw instruction bytes first, just like the objdump does.
 */
static int id_buffer_printf (void *, const char *, ...);

static void id_print (struct id_instr *, disassemble_info *);
static void id_init (int, char **);
static void id_fork () ;
static void id_main_child (void);
static void id_main_parent (void);

/**
 * Global variables.
 */
static enum id_role role;
static struct id_args args;
static struct id_buffer buffer;
static struct id_pids pids;

static int
id_buffer_printf (void *unused, const char *fmt, ...)
{
  va_list args;
  int ret;

  if (buffer.len >= buffer.cap)
    return -1;

  va_start (args, fmt);
  ret = vsnprintf (buffer.data + buffer.len, buffer.cap - buffer.len, fmt, args);
  va_end (args);

  if (ret >= 0) {
    /* If the buffer is too small (impossible), end it with "...". */
    if (buffer.len + ret > buffer.cap)
      strcpy (buffer.data + (buffer.cap - 4), "...");
    buffer.len += ret;
  }
  else {
    buffer.len = 0;
    buffer.data[0] = '\0';
  }
  return ret;
}

/**
 * Disassembles and pretty prints the instruction stored in ins. The result
 * looks like
 *
 *    7f09959621f0: 41 89 f8      mov    %edi,%r8d
 *    +-----------  +-------      +---------------
 *    |             |             |
 *    |             |             Disassembled Instruction
 *    |             Raw Bytes
 *    Instruction Pointer
 *
 */
static void
id_print (struct id_instr *ins, disassemble_info * dis)
{
  char *bytes = (char *) ins->data;
  int i;
  int s;

  buffer.len = 0;
  buffer.data[0] = '\0';

  /**
   * This writes the instruction to a buffer and returns the number of bytes
   * consumed. We need this number to print the instruction's hex bytes first,
   * and then we print the string of the disassembled instruction.
   */
  s = print_insn_i386 (0, dis);
  if (s <= 0)
    return;

  printf (" %lx:\t", ins->ip);
  for (i = 0; i < s; i++)
    printf ("%02x ", bytes[i] & 0xff);

  /**
   * The spaces in the format string and the &...[s * 3] slicing
   * make sure we keep the minimum width of the instruction byte column
   * at 7 * 3 characters.
   *
   * 3 is the character width of a single instruction byte (see previous
   * printf) and 7 is an arbitrary good fit between the average instruction
   * length and the produced whitespace in the output.
   *
   * The strange array indexing instead of a simple addition is used to silence
   * clang's warning.
   */
  if (s > 7)
    s = 7;
  printf (&"                     \t%s\n"[s * 3], buffer.data);
}

static void
id_init (int argc, char **argv)
{
  if (argc < 2)
    errx (EXIT_FAILURE, "Usage: %s COMMAND [ARGS...]", argv[0]);

  /**
   * Skip the path to our own executable in argv[0], so we can pass this
   * directly to exec().
   */
  args.argc = argc - 1;
  args.argv = argv + 1;

  /* Role and child's pid set by the id_fork() function. */
  pids.parent = getpid ();
  pids.child = 0;

  buffer.cap = sizeof (buffer.data);
  buffer.len = 0;
}

static void
id_fork ()
{
  pid_t pid;

  if (pid = fork (), pid < 0)
    err (EXIT_FAILURE, "fork");

  if (pid > 0) {
    role = ID_PARENT;
    pids.child = pid;
  }
  else {
    role = ID_CHILD;
    pids.child = getpid ();
  }
}

static void
id_main_child (void)
{
  /**
   * Allow parent process to trace us, then stop and wait until parent is
   * ready and wakes us up.
   */
  ptrace (PTRACE_TRACEME, 0, 0, 0);
  raise (SIGSTOP);

  /**
   * execvp() expects the list of arguments to be terminated by a null pointer.
   * The C standard says "argv[argc] shall be a null pointer", so it's safe to
   * pass argv directly to execvp().
   */
  execvp (args.argv[0], args.argv);

  /* The exec() functions return only if an error has occurred. */
  err (EXIT_FAILURE, "execvp");
}

static void
id_main_parent (void)
{
  const pid_t pid = pids.child;
  disassemble_info dis;

  struct id_instr ins;
  struct user_regs_struct regs;

  int status = 0;

  /* Get in sync with child process. Wait for it to stop. */
  while (waitpid (pid, &status, WSTOPPED) < 0) {
    if (errno != EINTR)
      err (EXIT_FAILURE, "waitpid");
  }

  memset (&dis, 0, sizeof (disassemble_info));

  init_disassemble_info (&dis, NULL, id_buffer_printf);
  dis.buffer = (bfd_byte *) ins.data;
  dis.buffer_length = sizeof (ins.data);
  dis.endian = BFD_ENDIAN_LITTLE;

#ifdef __x86_64__
  dis.mach = bfd_mach_x86_64;
#else
  dis.mach = bfd_mach_i386_i386;
#endif

  /* Wake the child up and start tracing every single instruction. */
  kill (pid, SIGCONT);

  for (;;) {
    ptrace (PTRACE_SINGLESTEP, pid, 0, 0);

    while (wait (&status) < 0) {
      if (errno == EINTR)
        continue;
      err (EXIT_FAILURE, "waitpid");
    }

    if (WIFEXITED (status))
      break;

    /* Get register content to read current value of instruction pointer. */
    ptrace (PTRACE_GETREGS, pid, 0, &regs);

#ifdef __x86_64__
    ins.ip = regs.rip;
#else
    ins.ip = regs.eip;
#endif

    ins.data[0] = ptrace (PTRACE_PEEKDATA, pid, ins.ip, 0);
    ins.data[1] = ptrace (PTRACE_PEEKDATA, pid, ins.ip + 1 * sizeof (long), 0);
#if (WORDS_PER_INSTR == 4)
    ins.data[2] = ptrace (PTRACE_PEEKDATA, pid, ins.ip + 2 * sizeof (long), 0);
    ins.data[3] = ptrace (PTRACE_PEEKDATA, pid, ins.ip + 3 * sizeof (long), 0);
#endif

    id_print (&ins, &dis);
  }
}

int
main (int argc, char **argv)
{
  id_init (argc, argv);
  id_fork ();

  switch (role) {
    case ID_PARENT:
      id_main_parent ();
      break;
    case ID_CHILD:
      id_main_child ();
      break;
  }
  return EXIT_SUCCESS;
}
