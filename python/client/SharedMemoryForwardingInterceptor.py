import ctypes
import inspect
import grpc
import multiprocessing.shared_memory


# True = Python shared memory; False = use sideband DLL
# Using the sideband DLL is probably preferred since we ideally would do all the work we can there
# and not duplicate memory offset details into the python client code
use_Python_shared_memory = False


class SharedMemoryForwardingInterceptor(grpc.UnaryUnaryClientInterceptor):
    def __init__(self, no_intercept_client):
        # See https://github.com/grpc/grpc/issues/21659#issuecomment-577964174 for how to get the response deserializers
        self._deserializers = {}
        stub_method_tuples = list(
            filter(
                lambda member: not member[0].startswith("__"),
                inspect.getmembers(no_intercept_client),
            )
        )
        for stub_method, method_handler in stub_method_tuples:
            self._deserializers[stub_method] = method_handler._response_deserializer

        # Initialize shared memory directly or via the sideband DLL
        if use_Python_shared_memory:
            self._shared_mem = multiprocessing.shared_memory.SharedMemory(
                name="TESTBUFFER_TestBuffer"
            )
            self._shared_mem_buffer = self._shared_mem.buf
            self._buffer_size = len(self._shared_mem_buffer)
        else:
            self._sideband_dll = ctypes.cdll.LoadLibrary(
                r"C:\dev\git\grpc-sideband\build\Release\ni_grpc_sideband.dll"
            )
            # C++ Signature: InitClientSidebandData(const char* sidebandServiceUrl, ::SidebandStrategy strategy, const char* usageId, int bufferSize, int64_t* out_tokenId)
            # C++ invocation: InitClientSidebandData("TestBuffer", SidebandStrategy::SHARED_MEMORY, "TestBuffer", 4096, &sideband_token);
            SidebandStrategy__SHARED_MEMORY = 2
            sideband_usage_id = ctypes.c_char_p(b"TestBuffer")
            sideband_data_length = ctypes.c_int(4096)
            self._sideband_token = ctypes.c_longlong()
            self._sideband_dll.InitClientSidebandData(
                sideband_usage_id,
                SidebandStrategy__SHARED_MEMORY,
                sideband_usage_id,
                sideband_data_length,
                ctypes.byref(self._sideband_token),
            )
            # C++ Signature: SidebandData_BeginDirectWrite(int64_t sidebandToken, uint8_t** buffer)
            self._shared_mem_buffer = ctypes.POINTER(ctypes.c_uint8)()
            self._sideband_dll.SidebandData_BeginDirectWrite(
                self._sideband_token, ctypes.byref(self._shared_mem_buffer)
            )
            self._buffer_size = sideband_data_length.value
        self._local_buffer = bytearray(self._buffer_size)

    def _signal_ready(self):
        self._shared_mem_buffer[0] = 1  # _clientWriteReady

    def _wait_for_read_ready(self):
        while self._shared_mem_buffer[4] != 1:  # _clientReadReady
            pass
        self._shared_mem_buffer[4] = 0  # indicate we've "read" response

    def _fill_local_buffer(self, client_call_details, request):
        method_name = client_call_details.method
        method_name_length = len(method_name)
        method_name_length_start = 8  # first eight bytes are reserved for read/write flags
        method_name_start = 4 + method_name_length_start
        if method_name_length < 256:
            self._local_buffer[method_name_length_start] = method_name_length
        else:
            self._local_buffer[method_name_length_start:method_name_start] = (
                method_name_length.to_bytes(4, byteorder="little")
            )
        # methodName is the next set of bytes
        request_length_start = method_name_start + method_name_length
        self._local_buffer[method_name_start:request_length_start] = method_name.encode("ascii")

        # requestLength is the next 4 bytes, but not 4-byte aligned; request is the final set of bytes
        request_start = 4 + request_length_start
        request_message = request.SerializePartialToString(deterministic=True)

        self._request_message = request_message

        request_message_length = len(request_message)  # request.ByteSize() might also work
        request_end = request_start + request_message_length
        if request_message_length < 256:
            self._local_buffer[request_length_start] = request_message_length
        else:
            self._local_buffer[request_length_start:request_start] = (
                request_message_length.to_bytes(4, byteorder="little")
            )
        self._local_buffer[request_start:request_end] = request_message

    def _send_request(self, client_call_details, request):
        self._fill_local_buffer(client_call_details, request)
        if use_Python_shared_memory:
            self._shared_mem_buffer[: len(self._local_buffer)] = self._local_buffer
        else:
            ctypes.memmove(
                self._shared_mem_buffer,
                (ctypes.c_uint8 * self._buffer_size).from_buffer_copy(self._local_buffer),
                self._buffer_size,
            )
        self._signal_ready()

    def _get_response(self, client_call_details):
        self._wait_for_read_ready()

        data_length_index = 8
        data_index = data_length_index + 4
        data_length_bytes = self._shared_mem_buffer[data_length_index:data_index]
        data_length = int.from_bytes(bytes(data_length_bytes), byteorder="little")
        data = self._shared_mem_buffer[data_index : data_index + data_length]

        method_name = client_call_details.method.split("/")[-1]
        response_deserializer = self._deserializers.get(method_name)
        response = response_deserializer(bytes(data))
        return grpc._interceptor._UnaryOutcome(response, None)

    def intercept_unary_unary(self, continuation, client_call_details, request):
        self._send_request(client_call_details, request)
        return self._get_response(client_call_details)
