#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct State;

void initialize(struct State *);
void exec_step(struct State *);
void report(struct State *);
void verify_exit(int);

#ifdef __cplusplus
}
#endif
