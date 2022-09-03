#ifndef NOTIFY_H
#define NOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*notify_callback_t)(void);

void notify_init(notify_callback_t cb);
void notify_out(void);

#ifdef __cplusplus
}
#endif

#endif