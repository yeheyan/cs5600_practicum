## Client Operations
The server uses a dedicated directory rfs_storage as root path for better security.

### WRITE
Write operation writes a local file from a client to the remote server.
```ruby
./rfs WRITE localfile_path remotefile_path
```
Scenerio Analysis:

A. Local file path and remote file path are provided
```ruby
./rfs WRITE user/jdoe/proj/example.txt project/
```
1. Program identifies WRITE OP;
2. Program verifies local file path user/jdoe/proj/example.txt;
3. Program verifies rfs_storage/project. if the path exists, copy example.txt content to rfs_storage/project/example.txt.

Remote path is a file:
```ruby
./rfs WRITE user/jdoe/proj/example.txt project/file.txt
```
- It will copy example.txt to file.txt if there is a file.txt in the server's diretcory rfs_storage/project
- The old file will be versioned and a new file.txt will be created.
- If no file.txt in the rfs_storage/project, then the program will create a file called file.txt in the directory. The new file will recieve all content in example.txt.


B. Remote path not provided
```ruby
./rfs WRITE user/jdoe/proj/example.txt
```
- Saves to: rfs_storage/example.txt (filename only)
- Not to: rfs_storage/user/jdoe/proj/example.txt (fullpath)
- It doesn't make sense to assume the absolute path on the client's side exists on the server side, too. We want to prevents creating irrelevant directory structures on the server.

### GET
Get operation get a file from the remote server to a client
```ruby
./rfs GET remote_folder/remote_file.html local_folder/local_file.html
```

Scenerio Analysis:

A. remote path and local path are provided completely

```ruby
./rfs GET project/file.txt  /Users/yyan/Desktop/get.txt
```
1. Program identifies GET OP;
2. Program verifies remote file path rfs_storage/project/file.txt
3. Program verifies /Users/yyan/Desktop/get.txt. If the path exists, copy file.txt content to /Users/yyan/Desktop/get.txt.

B. local path not provided
```ruby
./rfs GET project/file.txt
```
Remote file rfs_storage/project/file.txt will be saved in the current directory(wherever the client runs the program).


### GETVERSION
Get version operation can get a specific history version of a file. Otherwise similar to GET OP. Client can check which version number with LS OP (see below).

```ruby
./rfs GETVERSION remote_file_path  local_file_path(or omit) # version
```

For example, getting the first versioned file.txt in the rfs_storage/project, clients can enter

```ruby
./rfs GETVERSION project/file.txt  1
```

1. Program identifies GETVERSION OP;
2. Program verifies remote file path rfs_storage/project/file.txt;
3. Program uses client current path to recieve file as no local path has been provided;
4. Program fetches the newest history version (not the current one, but the historical one!) of file.txt  to current path if that version exists.

### LS
Ls operation display all versions of a file in the remote server.

```ruby
./rfs LS remote_path_file
```
The program will display the version number, the full name of the versioned file, file's size and the time it was written.

### RM
Rm operation removes all versions of a file in the remote server.

```ruby
./rfs RM remote_path_file
```

### STOP
STOP operation sends a STOP signal to the remote server to stop the server. Server will wait for all clients activities to finsih then shut down.

```ruby
./rfs STOP
```

## Concurrency and Threading
Overview
Our server uses a multi-threaded architecture with fine-grained locking to handle concurrent client requests safely and efficiently.
### Threading Model
Each client connection spawns a new thread using POSIX threads (pthreads):

The main server loop accepts incoming connections
Each accepted connection is handled by a dedicated thread
Threads are detached and clean up automatically after completion
Multiple clients can be served simultaneously without blocking each other

### Synchronization Strategy
The server implements a multi-level locking strategy to protect shared resources:
1. File-Level Locks (flock)
Individual files are protected using flock() to coordinate concurrent access:

WRITE operations: Acquire exclusive lock (LOCK_EX)

Blocks all other readers and writers
Ensures file integrity during write operations


GET operations: Acquire shared lock (LOCK_SH)

Allows multiple concurrent readers
Blocks writers until all readers complete


Example scenario:
````
Client A: Writing to file.txt (LOCK_EX acquired)
Client B: Attempts to read file.txt → BLOCKED until Client A finishes
Client C: Attempts to write file.txt → BLOCKED until Client A finishes

Client A: Completes write, releases lock
Client B: Acquires LOCK_SH, reads file
Client C: Acquires LOCK_SH, reads file  (concurrent with Client B)
````
2. Version Management Locks (pthread_mutex)
File versioning operations use a hash-based mutex array:

Each file is mapped to a mutex using a hash function
WRITE operations lock the mutex before creating backups and writing
This ensures version creation is atomic and prevents timestamp collisions

Example scenario:
````
Client A: WRITE file.txt (acquires version mutex)
  → Checks if file exists
  → Backs up to file.txt.v1764092560000123
  → Writes new file.txt
  → Releases version mutex

Client B: WRITE file.txt (waits for version mutex)
  → Acquires mutex after Client A releases
  → Backs up to file.txt.v1764092560000456
  → Writes new file.txt
  → Releases mutex

Result: Two distinct versions created sequentially
````
3. Directory Creation Lock (pthread_mutex)

A global mutex protects directory creation operations:

Prevents race conditions when multiple threads create the same directory
Ensures mkdir() operations don't conflict

4. Server State Lock (pthread_mutex)

The server_running flag is protected by a mutex:

Threads can safely check if the server is shutting down
The STOP command safely signals shutdown across all threads

#### Graceful Shutdown
The server uses select() with a timeout to check the server_running flag periodically:

When a STOP command is received, the flag is set to 0
The main loop detects this within 1 second
Active operations complete before the server exits
Listening socket is closed properly

#### Concurrency Benefits

Performance: Multiple clients can operate on different files simultaneously without blocking
Safety: Same-file operations are properly serialized to prevent corruption
Efficiency: Readers don't block each other (shared locks)
Scalability: Hash-based mutex array minimizes lock contention

#### Error Handling
File write operations include comprehensive error checking:

Storage full errors are detected via fwrite() failures
Incomplete writes are detected and partial files are removed
All error paths properly release locks to prevent deadlocks

# Network Connection Testing

Server IP and port can be configured in `config.h`.

## Testing Strategies

**Local Testing (Primary Method)**
We test using localhost connections where both server and client run on the same machine or local network. This avoids firewall complications that would prevent direct connections between machines on different networks.

- **Single Machine**: Run the server in one terminal, then launch multiple client programs in separate terminals. All connect to `localhost` (127.0.0.1).
- **Multi-threaded Testing**: We simulate concurrent requests by executing identical operations from multiple clients simultaneously, allowing us to verify the server's thread-safety and concurrency controls.

# NEU Discovery: Real Network Communication Testing

We use NEU Discovery compute nodes to test real network communication between physically separate machines, avoiding local firewall complications. Discovery nodes can communicate directly within the cluster's internal network.

## Prerequisites

Upload your project to Discovery:
```ruby
scp -r ~/cs5600_practicum username@login.discovery.neu.edu:~/
```
Or use git pull from repository.
```ruby
git clone https://github.com/yeheyan/cs5600_practicum.git
```
SSH to Discovery:
```ruby
ssh username@login.discovery.neu.edu
```
## Setup: Create Separate Server and Client Directories

Discovery uses a shared filesystem (NFS) across all compute nodes, meaning the same files are visible everywhere. To maintain separate configurations for server and client, create two copies:
```
cp -r cs5600_practicum cs5600_server
```
```
cp -r cs5600_practicum cs5600_client
```
## Step-by-Step Testing Guide

### Terminal 1: Start the Server

1. Request a compute node:
```ruby

srun --partition=short --nodes=1 --ntasks=1 --time=01:00:00 --pty /bin/bash
```
2. Note your server's hostname and IP:
```ruby
hostname -I
```
Example output: 10.99.248.243 172.16.207.25

3. Configure and start the server:

cd ~/cs5600_server

Edit config.h - server should listen on all interfaces
```
vi config.h
```
Set:
````
#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 8080
````
Compile and run
```ruby
make clean && make
./server
```

The server should display: Server listening on 0.0.0.0:8080

### Terminal 2: Run the Client

1. Request a different compute node (excluding the server node):

Replace 'c3018' with your actual server hostname
```ruby
srun --partition=short --nodes=1 --exclude=c3018 --pty /bin/bash
# or exclude any hostname the server node has
```
2. Verify you're on a different node:
```
hostname
```

3. Configure and run the client:
```ruby
cd ~/cs5600_client
```
Edit config.h - use the server's IP from step 1
```
vi config.h
```
Set:
 ````
 #define SERVER_IP "10.99.248.243"  // Use your server's IP
#define SERVER_PORT 8080
````
Compile and test connection
make clean && make

Test operations
```
./rfs STOP
```
```
./rfs WRITE local_file.txt remote_file.txt
```
```
./rfs GET remote_file.txt downloaded.txt
```
## Verification

To confirm real network communication is working:

Test connectivity from client node:

### Troubleshoot Network Connection:

Ping the server node
```
ping c3018
```
Test if the port is open
```
nc -zv hostname 8080
```

Both should succeed if the server is running and network communication is working.

## Why This Setup Works

- Real Network Communication: Client and server run on different physical machines, communicating via TCP over Discovery's internal network
- No Firewall Issues: Discovery nodes can communicate freely within the cluster
- True Distributed Testing: Demonstrates actual network latency, packet transmission, and multi-threaded server handling of remote connections