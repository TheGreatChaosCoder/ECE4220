#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

main() {
int pid=fork();
if (pid == 0) pid=fork();
pid=fork();
if (pid == 0) printf("hello world\n"); }
