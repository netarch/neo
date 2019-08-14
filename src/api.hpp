#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void initialize(struct State *);
void execute(struct State *);
void report(struct State *);

#ifdef __cplusplus
}
#endif
