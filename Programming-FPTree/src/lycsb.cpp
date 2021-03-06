#include <leveldb/db.h>
#include <string>

#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../workloads/";

const string load = workload + "1w-rw-50-50-load.txt"; // done
const string run  = workload + "1w-rw-50-50-run.txt"; // done

const string filePath = "";

const int READ_WRITE_NUM = 10000; // done: how many operations

int main()
{        
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;
    // done: open and initial the levelDB
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
    assert(status.ok());

    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000]; // the key and value are same
//    bool* ifInsert = new bool[2200000]; // the operation is insertion or not
    int* op = new int[2200000]; // the operation op 0:insert 1:delete 2:update 3:read
	FILE *ycsb_load, *ycsb_run; // the files that store the ycsb operations
	char *buf = NULL;
	size_t len = 0;
    struct timespec start, finish; // use to caculate the time
    double single_time; // single operation time

    printf("Load phase begins \n");
    // done: read the ycsb_load and store
    ycsb_load = (FILE*)malloc(sizeof(FILE));
    if ((ycsb_load = fopen(load.c_str(), "r")) == NULL) {
        printf("cannot open file!\n");
        return -1;
    }

    buf = new char[32];
    while (fgets(buf,32,ycsb_load) != NULL)  {
        len = strlen(buf);
        buf[len] = '\0';
       	op[t] = 0;
        key[t] = atoll(buf + 7);
        t++;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);

    // done: load the workload in LevelDB
    int i;
    for (i = 0; i < READ_WRITE_NUM; i++) {
        if (op[i] == 0) {
            status = db->Put(write_options, to_string(key[i]), to_string(key[i]));
            assert(status.ok());
            inserted++;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);

    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

	int operation_num = 0;
    inserted = 0;		

    // done:read the ycsb_run and store
    if ((ycsb_run = fopen(run.c_str(), "r")) == NULL) {
        printf("cannot open file!\n");
        return -1;
    }
    t = 0;
    while (fgets(buf,32,ycsb_run) != NULL)  {
        len = strlen(buf);
        buf[len] = '\0';
        switch(buf[0]){
			case 'I': //insert
				op[t] = 0;
				key[t] = atoll(buf + 7);
				break;
			case 'D': //delete
				op[t] = 1;
				key[t] = atoll(buf + 7);
				break;
			case 'U': //update
				op[t] = 2;
				key[t] = atoll(buf + 7);
				break;
			case 'R': //read
				op[t] = 3;
				key[t] = atoll(buf + 5);
				break;
		}
        operation_num++;
        t++; 
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // done: operate the levelDB
    for (i = 0; i < operation_num; i++) {
    	//增其实就是改 因为key == value 
        if (op[i] == 0 && op[i] == 2) {
            status = db->Put(write_options, to_string(key[i]), to_string(key[i]));
            assert(status.ok());
            inserted++;
        }
        else if(op[i] == 1){
		    // 删除key
		    status = db->Delete(leveldb::WriteOptions(), to_string(key[i]));
		    assert(status.ok());
		}
        else if(op[i] == 3){
            string val;
            status = db->Get(leveldb::ReadOptions(), to_string(key[i]), &val);
            assert(status.ok());
            queried++;
        }
		
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", inserted, operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    return 0;
    fclose(ycsb_load);
    fclose(ycsb_run);
}