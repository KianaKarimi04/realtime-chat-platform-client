#include "protocol.h"
#include <string.h>

/* Fake server-side storage (for demo only) */
static char stored_username[50] = "";
static char stored_password[50] = "";
static int account_exists = 0;

ServerResponse protocol_create_account(const char *username, const char *password) {
    if (strlen(username) == 0 || strlen(password) == 0)
        return SERVER_NACK;

    /* Simulate server + server manager storing account */
    strcpy(stored_username, username);
    strcpy(stored_password, password);
    account_exists = 1;

    return SERVER_ACK;
}

ServerResponse protocol_login(const char *username, const char *password) {
    if (!account_exists)
        return SERVER_NACK;

    if (strcmp(username, stored_username) == 0 &&
        strcmp(password, stored_password) == 0)
        return SERVER_ACK;

    return SERVER_NACK;
}

void protocol_logout(void) {
    /* Nothing required for demo */
}
