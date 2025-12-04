#!/bin/bash
# test_rfs_split.sh

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Starting Server in Background ===${NC}"
make && ./server > server.log 2>&1 &
SERVER_PID=$!
sleep 2

echo -e "${GREEN}=== Running Client Tests ===${NC}"

# good tests
# Test 1
echo -e "${BLUE}Test 1: WRITE operation${NC}"
echo "test content" > test.txt
./rfs WRITE test.txt remote.txt
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ WRITE passed${NC}"; else echo -e "${RED}✗ WRITE failed${NC}";
fi

# Test 2
echo -e "${BLUE}Test 2: WRITE without remote path${NC}"
echo "another test content" > test2.txt
./rfs WRITE test2.txt
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ WRITE passed${NC}"; else echo -e "${RED}✗ WRITE failed${NC}";
fi

# Test 3
echo -e "${BLUE}Test 3: GET operation${NC}"
./rfs GET remote.txt downloaded.txt
diff test.txt downloaded.txt > /dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ GET passed${NC}"; else echo -e "${RED}✗ GET failed${NC}";
fi

# Test 3b
echo -e "${BLUE}Test 3b: GET without local path${NC}"
./rfs GET remote.txt
diff test.txt remote.txt > /dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ GET without local path passed${NC}"; else echo -e "${RED}✗ GET without local path failed${NC}";
fi

# Test 4
echo -e "${BLUE}Test 4: Versioning${NC}"
echo "version 1" > versioned.txt
./rfs WRITE versioned.txt remote_versioned.txt
sleep 0.5

echo "version 2" > versioned.txt
./rfs WRITE versioned.txt remote_versioned.txt
sleep 0.5

echo "version 3" > versioned.txt
./rfs WRITE versioned.txt remote_versioned.txt
sleep 0.5

echo -e "${BLUE}Test 4a: LS operation${NC}"
./rfs LS remote_versioned.txt
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ LS passed${NC}"; else echo -e "${RED}✗ LS failed${NC}";
fi

# Test 4b: GET specific version
echo -e "${BLUE}Test 4b: GET specific version${NC}"
./rfs GETVERSION remote_versioned.txt 2
diff remote_versioned.txt.v2 <(echo "version 2") > /dev/null 2>&1
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ GET specific version passed${NC}"; else echo -e "${RED}✗ GET specific version failed${NC}";
fi

# Test 5
echo -e "${BLUE}Test 5: REMOVE operation${NC}"
./rfs RM remote_versioned.txt
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ REMOVE passed${NC}"; else echo -e "${RED}✗ REMOVE failed${NC}";
fi

# Test 6: Concurrent WRITE operations
echo -e "${BLUE}Test 6: Concurrent WRITE to different files${NC}"

# Store background PIDs
PIDS=()

for i in {1..5}
do
  echo "concurrent content $i" > concurrent_$i.txt
  ./rfs WRITE concurrent_$i.txt remote_concurrent_$i.txt &
  PIDS+=($!)  # Store each background PID
done

echo "Launched ${#PIDS[@]} concurrent clients, waiting for completion..."

# Wait for ALL background jobs
for pid in "${PIDS[@]}"; do
  wait $pid
done

echo -e "${GREEN}✓ All concurrent operations completed${NC}"

# Test 7
echo -e "${BLUE}Test 7: STOP operation${NC}"
./rfs STOP
wait $SERVER_PID
if [ $? -eq 0 ]; then echo -e "${GREEN}✓ STOP passed${NC}"; else echo -e "${RED}✗ STOP failed${NC}";
fi

echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo "Check server.log for detailed server output"
echo ""
echo -e "${BLUE}=== Last 20 lines of server.log ===${NC}"
tail -20 server.log

echo -e "${BLUE}=== Tests Completed ===${NC}"
kill $SERVER_PID 2>/dev/null
rm -f test.txt test2.txt downloaded.txt versioned.txt server.log concurrent_*.txt remote.txt remote_versioned.txt remote_versioned.txt.v2 remote_concurrent_*.txt
make clean
exit 0

