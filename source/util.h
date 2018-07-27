#include <switch.h>

extern Mutex actionLock;
extern int sock;

void fatalLater(Result err);
int setupServerSocket();
