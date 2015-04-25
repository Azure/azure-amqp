#include "session.h"
#include "connection.h"

extern SESSION_HANDLE session_create(CONNECTION_HANDLE connection);
extern void session_destroy(SESSION_HANDLE handle);
extern void session_dowork(SESSION_HANDLE handle);
