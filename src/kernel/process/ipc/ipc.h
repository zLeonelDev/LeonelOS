#ifndef LEONELOS_IPC_H
#define LEONELOS_IPC_H

#include <types.h>

s32 ipc_init();
s32 ipc_send(usize target_pid, const void* message, usize size);
s32 ipc_receive(void* buffer, usize size, usize* sender);
s32 ipc_create_port(const char* name);
s32 ipc_connect(usize port_id);

#endif