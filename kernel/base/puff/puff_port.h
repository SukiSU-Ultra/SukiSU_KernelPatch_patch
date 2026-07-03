#ifndef _PUFF_PORT_H_
#define _PUFF_PORT_H_

typedef unsigned long jmp_buf[16];

int setjmp(jmp_buf env) __attribute__((returns_twice));
void longjmp(jmp_buf env, int val) __attribute__((noreturn));

#endif
