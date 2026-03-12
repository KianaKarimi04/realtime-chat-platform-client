//
// chat.h — Channel list + chat room UI
//

#ifndef CHAT_H
#define CHAT_H

#include "auth.h"

/*
 * Show the channel list, let user pick one, enter chat.
 * Called after successful login.
 */
void show_channel_list(ClientState *state);

#endif