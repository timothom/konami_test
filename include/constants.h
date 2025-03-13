/* Some copyright */

#include <time.h>

#define DEFAULT_SERVER_IP           "127.0.0.1"
#define DEFAULT_SERVER_PORT         5000

#define BUFFER_SIZE                 4096
#define WORKER_THREAD_COUNT         5
#define TASK_DELAY_TIME             0
#define WORK_QUEUE_DEPTH            64
#define MESSAGE_TARGET_FIELD        "Command"
#define MESSAGE_ACK_CODE            "ACK" 

#define USE_LOCKING                 1  //Turn this off to see all kinds of threading nightmares and crashes.  
#define BENCHMARK                   0  //Turn on to supress output and run as fast as possible.  Not compliant with requirements 

//TODO optimize client_message for cache line size, it's a little too big
typedef struct {  
    int     client_socket;
    time_t  receive_timestamp;
    char    message[BUFFER_SIZE];
} client_message;