# Maximum size of the variable header (in bytes)
MAX_HEADER_SIZE = 16

# Describes the type of packet sent/received
from enum import Enum
class CMDTYPE(Enum):
    
    QUOTE_REQUEST = 0
    QUOTE_REPLY = 1
    
    SEND_FILE = 2
    
    EXECUTE = 3
    RETURN_STATUS = 4
    RETURN_STDOUT = 5
    RETURN_STDERR = 6
    RETURN_FILE = 7
    
    END = 8