#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void initialize(struct State *);
void exec_step(struct State *);
void report(struct State *);

#ifdef __cplusplus
}
#endif
