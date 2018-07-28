#include <switch.h>
#define MAX_LINE_LENGTH 300

extern Mutex actionLock;
extern Semaphore actionSem;
extern int sock;

void fatalLater(Result err);
int setupServerSocket();
