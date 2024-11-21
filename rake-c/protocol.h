// Maximum size of the variable header (in bytes)
#define MAX_HEADER_SIZE         16

// Maximum size of the CMDTYPE header field
#define MAX_CMDTYPE_SIZE        2

// Maximum size of the length header field
#define MAX_LENGTH_SIZE         MAX_HEADER_SIZE - MAX_CMDTYPE_SIZE

// Describes the type of packet sent/received
typedef enum {
    
    CMD_QUOTE_REQUEST = 0,
    CMD_QUOTE_REPLY,
    
    CMD_SEND_FILE,
    
    CMD_EXECUTE,
    CMD_RETURN_STATUS,
    CMD_RETURN_STDOUT,
    CMD_RETURN_STDERR,
    CMD_RETURN_FILE,
    
    CMD_END,
} CMDTYPE; 
