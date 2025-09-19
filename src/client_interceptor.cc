#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <atomic>
#include <sched.h>
#include <fcntl.h>
#include <map>
//#include "absl/log/check.h"
#include <grpcpp/support/client_interceptor.h>
#include <sharedmem_client_interceptor.h>
#include <sideband_data.h>
#include <perftest.pb.h>
#include <google/protobuf/io/tokenizer.h>
#include <boost/interprocess/sync/named_semaphore.hpp>

//static HANDLE StartCallEvent = INVALID_HANDLE_VALUE;
//static HANDLE CallCompleteEvent = INVALID_HANDLE_VALUE;
int64_t sideband_token = 0;
uint8_t* sideband_memory = nullptr;
//SharedAtomic* _clientReadReady = nullptr;
//SharedAtomic* _clientWriteReady = nullptr;
//volatile int32_t* _clientReadReady = nullptr;
//volatile int32_t* _clientWriteReady = nullptr;

//boost::interprocess::named_semaphore StartCallEvent(boost::interprocess::open_or_create, "StartCallEvent", 0);
//boost::interprocess::named_semaphore CallCompleteEvent(boost::interprocess::open_or_create, "CallCompleteEvent", 0);

class SharedMemoryForwardingInterceptor : public grpc::experimental::Interceptor 
{    
public:
    struct SharedAtomic {
        std::atomic<int> flag;
    };

    typedef struct {
        std::atomic<int> count;
    } spin_semaphore_t;

    spin_semaphore_t *create_or_open_semaphore(const char *name, int initial_count, int create) {
        int flags = O_RDWR;
        if (create) flags |= O_CREAT;

        int fd = shm_open(name, flags, 0666);
        if (fd == -1) {
            perror("shm_open");
            exit(1);
        }

        if (create) {
            if (ftruncate(fd, sizeof(spin_semaphore_t)) == -1) {
                perror("ftruncate");
                exit(1);
            }
        }

        spin_semaphore_t* sem = (spin_semaphore_t*)mmap(NULL, sizeof(spin_semaphore_t),
                                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (sem == MAP_FAILED) {
            perror("mmap");
            exit(1);
        }
        if (create) {
            atomic_init(&sem->count, initial_count);
        }
        close(fd); // fd no longer needed after mmap
        return sem;
    }

    void spin_wait(spin_semaphore_t *sem) {
        int val;
        do {
            while (std::atomic_load_explicit(&sem->count, std::memory_order_relaxed) <= 0)
                sched_yield();
            val = std::atomic_fetch_sub_explicit(&sem->count, 1, std::memory_order_acquire);
        } while (val <= 0);
    }

    void spin_post(spin_semaphore_t *sem) {
        std::atomic_fetch_add_explicit(&sem->count, 1, std::memory_order_release);
    }

    static spin_semaphore_t* _startCallEvent;
    static spin_semaphore_t* _callCompleteEvent;

    SharedMemoryForwardingInterceptor(grpc::experimental::ClientRpcInfo* info) 
    {
        info_ = info;
        if (sideband_token == 0)
        {
            _startCallEvent = create_or_open_semaphore("/StartCallEvent", 0, false);
            _callCompleteEvent = create_or_open_semaphore("/CallCompleteEvent", 0, false);
            //StartCallEvent = CreateEvent(nullptr, false, false, "StartCallEvent");
            //CallCompleteEvent = CreateEvent(nullptr, false, false, "CallCompleteEvent");
            InitClientSidebandData("TestBuffer", SidebandStrategy::SHARED_MEMORY, "TestBuffer", 4096, &sideband_token);
            SidebandData_BeginDirectWrite(sideband_token, &sideband_memory);
            unsigned int* locks = (unsigned int*)sideband_memory;
            //_clientWriteReady = new (locks) SharedAtomic();
            //_clientWriteReady = reinterpret_cast<volatile int32_t*>(locks);
            //_clientReadReady = new (locks + sizeof(SharedAtomic)) SharedAtomic();
            //_clientReadReady = reinterpret_cast<volatile int32_t*>(locks) + 1;
            //sideband_memory += sizeof(int32_t) * 2;
        }
    }

    void SignalReady()
    {
        //std::cout << "Signaling call starting" << std::endl;
        //SetEvent(StartCallEvent);
        //InterlockedExchange(_clientWriteReady, 1);
        //_clientWriteReady->flag.store(1, std::memory_order_release);
        //*_clientWriteReady = 1;
        //StartCallEvent.post();
        spin_post(_startCallEvent);
        //std::cout << "Done signaling call starting" << std::endl;
    }

    void WaitForReadReady()
    {
        //std::cout << "Waiting for call result" << std::endl;
        //WaitForSingleObject(CallCompleteEvent, INFINITE);
        //while (InterlockedCompareExchange(_clientReadReady, 0, 1) == 0);
        // int expected = 1;
        // do { expected = 1; _clientReadReady->flag.compare_exchange_strong(expected, 0); } while (expected == 0);
        // while (*_clientReadReady == 0) 
        // {
        //     // Spin until the read is ready
        //     //std::this_thread::yield();
        // }
        // *_clientReadReady = 0;
        //std::cout << "Call result received" << std::endl;
        //CallCompleteEvent.wait();
        spin_wait(_callCompleteEvent);
        //std::cout << "Done waiting for call result" << std::endl;
    }

    void Intercept(::grpc::experimental::InterceptorBatchMethods* methods) override 
    {
        bool hijack = false;
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_INITIAL_METADATA)) {
            // Hijack all calls
            hijack = true;
            // Create a stream on which this interceptor can make requests
            // stub_ = keyvaluestore::KeyValueStore::NewStub(methods->GetInterceptedChannel());
            // stream_ = stub_->GetValues(&context_);
            //std::cout << "Pre Send Initial Metadata" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_MESSAGE)) {
            // We know that clients perform a Read and a Write in a loop, so we don't
            // need to maintain a list of the responses.
            // std::string requested_key;

            auto packedRequest = sideband_memory;
            auto method = info_->method();
            auto methodLen = strlen(method);
            *(int32_t*)packedRequest = methodLen;
            packedRequest += 4;
            memcpy(packedRequest, method, methodLen);
            packedRequest += methodLen;
            grpc::Slice requestMessage;
            methods->GetSerializedSendMessage()->DumpToSingleSlice(&requestMessage);
            *(int32_t*)packedRequest = requestMessage.size();
            packedRequest += 4;
            memcpy(packedRequest, requestMessage.begin(), requestMessage.size());
            //SetEvent(StartCallEvent);
            SignalReady();
            auto details = methods->GetSendInitialMetadata();
            //std::cout << "Pre Send Message" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_SEND_CLOSE)) 
        {
            // stream_->WritesDone();
            //std::cout << "Pre Send Close" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_MESSAGE)) 
        {
            //keyvaluestore::Response* resp = static_cast<keyvaluestore::Response*>(methods->GetRecvMessage());
            //resp->set_value(response_);
            google::protobuf::MessageLite* result = static_cast<google::protobuf::MessageLite*>(methods->GetRecvMessage());
            //WaitForSingleObject(CallCompleteEvent, INFINITE);
            WaitForReadReady();
            //niPerfTest::InitResult r;
            //r.set_status(42);
            //void* data = new int8_t[4096];
            //r.SerializeToArray(data, 4096);
            result->ParseFromArray(sideband_memory, 4096);
            //std::cout << "Pre Receive Message" << std::endl;
        }
        if (methods->QueryInterceptionHookPoint(grpc::experimental::InterceptionHookPoints::PRE_RECV_STATUS)) 
        {
            auto* status = methods->GetRecvStatus();
            *status = grpc::Status::OK;
            //std::cout << "Pre Receive Status" << std::endl;
        }
        // One of Hijack or Proceed always needs to be called to make progress.
        if (hijack) {
            // Hijack is called only once when PRE_SEND_INITIAL_METADATA is present in
            // the hook points
            methods->Hijack();
        } 
        else 
        {
            // Proceed is an indicator that the interceptor is done intercepting the
            // batch.
            methods->Proceed();
        }
  }

 private:
     grpc::experimental::ClientRpcInfo* info_;
    //grpc::ClientContext context_;
    //std::unique_ptr<keyvaluestore::KeyValueStore::Stub> stub_;
    //std::unique_ptr<grpc::ClientReaderWriter<keyvaluestore::Request, keyvaluestore::Response>> stream_;
    //std::map<std::string, std::string> cached_map_;
    //std::string response_;
};

SharedMemoryForwardingInterceptor::spin_semaphore_t* SharedMemoryForwardingInterceptor::_startCallEvent;
SharedMemoryForwardingInterceptor::spin_semaphore_t* SharedMemoryForwardingInterceptor::_callCompleteEvent;

grpc::experimental::Interceptor* SharedMemoryForwardingInterceptorFactory::CreateClientInterceptor(grpc::experimental::ClientRpcInfo* info)
{
    return new SharedMemoryForwardingInterceptor(info);
}
