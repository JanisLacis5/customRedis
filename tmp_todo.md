# Structs
```c
enum UNALIVE_REASON {
    KEY_MODIFIED = 1,
    KEY_EXPIRED = 2,
    QUEUE_ERROR = 3
};
```

```c
struct Command {
    int argc;  // no of arguments
    dstr **argv;  // argument array
};
```

```c
struct TB {
    bool alive = true;  // Whether this block is alive or not
    int unalive_reason = -1;  // UNALIVE_REASON
    int cmdlen;  // len(cmdv)
    int cmdcap;  // total capacity of the cmdv array
    Command *cmdv;  // Array of commands that must be executed int his transaction block
};
```

```c
struct Conn {
    ...
    int id;  // 64 bit hash for an id
    bool in_multi = false;  // flag that tells if conn is in multi mode (does not execute commands)
    TB *tb = NULL;  // if there is a transaction block (in_multi == true) it is here
    DList watched_keys;  // DList<WatchNode> for O(n) cleanup after exiting the multi mode
};
```

```c
struct WatchNode {
    Conn *conn;
    WatchNode *prev, *next;
};
```

```c
struct GlobalDB {
    ...
    HMap watched_keys;  // map<string, DList<WatchNode>> - the string is the key
    DList alive_conns;  // list of alive connections, for O(1) lookups, GlobalDB->fd_to_conn array exists 
};
```

# Prerequisites
- keep track of alive conn's
- remove inactive conn's from alive_conns
- make a checker that checks if a command is valid
- make dynamic array functions

# Steps
1) *MULTI*
    - Conn is in multi mode (conn->in_multi == true)
    - New TB struct is added to conn->tb (conn->tb != NULL)
    - Returns "OK" to every valid query
    - Returns the error + unalives the transaction block (conn->tb->alive = false)
2) *WATCH*
    - Adds the conn->id to the global_data->watched_keys[key] list
    - Returns "OK" if not in multi mode, else a message that it cant be done, nothing changes though
3) *EXEC*
    - Checks if the transaction block is alive
    - Loops over commands in conn->tb->commands, for each command:
        - execute command
        - return the answer
    - Exits multi mode (conn->in_multi = false)
    - State is cleaned up on a seperate thread (remove conn from global_data->watched_keys)

# How it works
1) *MULTI*, res = "OK"
2) user enters commands, res = "QUEUED"
3) *EXEC* - the command stack gets executed if none of the watched keys were modified (tb->alive == true). No other client can modify the keyspace while some client is in EXEC mode

# User behaviour
- Valid command entered --> "QUEUED"
- Invalid command entered --> "command does not exist", conn->tb->alive = false;
- User's client disconnects --> queue is dropped

# Edge cases
- *MULTI* in *MULTI* (nested multi's) --> message that it is not possible, nothing else changes, exec will still be valid
- Key expires (hits the TTL)

# Facts
- Commands get executed up until an error and are NOT rolled back

# Functions
## Clean up after exiting multi mode
```cpp
void multi_cleanup() {

}
```

## Function to execute on each key's modification
```cpp
void key_modified(dstr *key) {

}
```
