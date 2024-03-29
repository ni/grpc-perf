# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import perftest_pb2 as perftest__pb2


class niPerfTestServiceStub(object):
    """---------------------------------------------------------------------
    The niPerfTest service definition.
    ---------------------------------------------------------------------
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.StreamLatencyTest = channel.stream_stream(
                '/niPerfTest.niPerfTestService/StreamLatencyTest',
                request_serializer=perftest__pb2.StreamLatencyClient.SerializeToString,
                response_deserializer=perftest__pb2.StreamLatencyServer.FromString,
                )
        self.StreamLatencyTestClient = channel.stream_unary(
                '/niPerfTest.niPerfTestService/StreamLatencyTestClient',
                request_serializer=perftest__pb2.StreamLatencyClient.SerializeToString,
                response_deserializer=perftest__pb2.StreamLatencyServer.FromString,
                )
        self.StreamLatencyTestServer = channel.unary_stream(
                '/niPerfTest.niPerfTestService/StreamLatencyTestServer',
                request_serializer=perftest__pb2.StreamLatencyClient.SerializeToString,
                response_deserializer=perftest__pb2.StreamLatencyServer.FromString,
                )
        self.TestWrite = channel.unary_unary(
                '/niPerfTest.niPerfTestService/TestWrite',
                request_serializer=perftest__pb2.TestWriteParameters.SerializeToString,
                response_deserializer=perftest__pb2.TestWriteResult.FromString,
                )
        self.TestWriteContinuously = channel.stream_stream(
                '/niPerfTest.niPerfTestService/TestWriteContinuously',
                request_serializer=perftest__pb2.TestWriteParameters.SerializeToString,
                response_deserializer=perftest__pb2.TestWriteResult.FromString,
                )
        self.BeginTestSidebandStream = channel.unary_unary(
                '/niPerfTest.niPerfTestService/BeginTestSidebandStream',
                request_serializer=perftest__pb2.BeginTestSidebandStreamRequest.SerializeToString,
                response_deserializer=perftest__pb2.BeginTestSidebandStreamResponse.FromString,
                )
        self.TestSidebandStream = channel.stream_stream(
                '/niPerfTest.niPerfTestService/TestSidebandStream',
                request_serializer=perftest__pb2.TestSidebandStreamRequest.SerializeToString,
                response_deserializer=perftest__pb2.TestSidebandStreamResponse.FromString,
                )
        self.Init = channel.unary_unary(
                '/niPerfTest.niPerfTestService/Init',
                request_serializer=perftest__pb2.InitParameters.SerializeToString,
                response_deserializer=perftest__pb2.InitResult.FromString,
                )
        self.ConfigureVertical = channel.unary_unary(
                '/niPerfTest.niPerfTestService/ConfigureVertical',
                request_serializer=perftest__pb2.ConfigureVerticalRequest.SerializeToString,
                response_deserializer=perftest__pb2.ConfigureVerticalResponse.FromString,
                )
        self.ConfigureHorizontalTiming = channel.unary_unary(
                '/niPerfTest.niPerfTestService/ConfigureHorizontalTiming',
                request_serializer=perftest__pb2.ConfigureHorizontalTimingRequest.SerializeToString,
                response_deserializer=perftest__pb2.ConfigureHorizontalTimingResponse.FromString,
                )
        self.InitiateAcquisition = channel.unary_unary(
                '/niPerfTest.niPerfTestService/InitiateAcquisition',
                request_serializer=perftest__pb2.InitiateAcquisitionRequest.SerializeToString,
                response_deserializer=perftest__pb2.InitiateAcquisitionResponse.FromString,
                )
        self.Read = channel.unary_unary(
                '/niPerfTest.niPerfTestService/Read',
                request_serializer=perftest__pb2.ReadParameters.SerializeToString,
                response_deserializer=perftest__pb2.ReadResult.FromString,
                )
        self.ReadComplex = channel.unary_unary(
                '/niPerfTest.niPerfTestService/ReadComplex',
                request_serializer=perftest__pb2.ReadParameters.SerializeToString,
                response_deserializer=perftest__pb2.ReadComplexResult.FromString,
                )
        self.ReadContinuously = channel.unary_stream(
                '/niPerfTest.niPerfTestService/ReadContinuously',
                request_serializer=perftest__pb2.ReadContinuouslyParameters.SerializeToString,
                response_deserializer=perftest__pb2.ReadContinuouslyResult.FromString,
                )


class niPerfTestServiceServicer(object):
    """---------------------------------------------------------------------
    The niPerfTest service definition.
    ---------------------------------------------------------------------
    """

    def StreamLatencyTest(self, request_iterator, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def StreamLatencyTestClient(self, request_iterator, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def StreamLatencyTestServer(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def TestWrite(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def TestWriteContinuously(self, request_iterator, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def BeginTestSidebandStream(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def TestSidebandStream(self, request_iterator, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def Init(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ConfigureVertical(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ConfigureHorizontalTiming(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def InitiateAcquisition(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def Read(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ReadComplex(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def ReadContinuously(self, request, context):
        """Missing associated documentation comment in .proto file."""
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_niPerfTestServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'StreamLatencyTest': grpc.stream_stream_rpc_method_handler(
                    servicer.StreamLatencyTest,
                    request_deserializer=perftest__pb2.StreamLatencyClient.FromString,
                    response_serializer=perftest__pb2.StreamLatencyServer.SerializeToString,
            ),
            'StreamLatencyTestClient': grpc.stream_unary_rpc_method_handler(
                    servicer.StreamLatencyTestClient,
                    request_deserializer=perftest__pb2.StreamLatencyClient.FromString,
                    response_serializer=perftest__pb2.StreamLatencyServer.SerializeToString,
            ),
            'StreamLatencyTestServer': grpc.unary_stream_rpc_method_handler(
                    servicer.StreamLatencyTestServer,
                    request_deserializer=perftest__pb2.StreamLatencyClient.FromString,
                    response_serializer=perftest__pb2.StreamLatencyServer.SerializeToString,
            ),
            'TestWrite': grpc.unary_unary_rpc_method_handler(
                    servicer.TestWrite,
                    request_deserializer=perftest__pb2.TestWriteParameters.FromString,
                    response_serializer=perftest__pb2.TestWriteResult.SerializeToString,
            ),
            'TestWriteContinuously': grpc.stream_stream_rpc_method_handler(
                    servicer.TestWriteContinuously,
                    request_deserializer=perftest__pb2.TestWriteParameters.FromString,
                    response_serializer=perftest__pb2.TestWriteResult.SerializeToString,
            ),
            'BeginTestSidebandStream': grpc.unary_unary_rpc_method_handler(
                    servicer.BeginTestSidebandStream,
                    request_deserializer=perftest__pb2.BeginTestSidebandStreamRequest.FromString,
                    response_serializer=perftest__pb2.BeginTestSidebandStreamResponse.SerializeToString,
            ),
            'TestSidebandStream': grpc.stream_stream_rpc_method_handler(
                    servicer.TestSidebandStream,
                    request_deserializer=perftest__pb2.TestSidebandStreamRequest.FromString,
                    response_serializer=perftest__pb2.TestSidebandStreamResponse.SerializeToString,
            ),
            'Init': grpc.unary_unary_rpc_method_handler(
                    servicer.Init,
                    request_deserializer=perftest__pb2.InitParameters.FromString,
                    response_serializer=perftest__pb2.InitResult.SerializeToString,
            ),
            'ConfigureVertical': grpc.unary_unary_rpc_method_handler(
                    servicer.ConfigureVertical,
                    request_deserializer=perftest__pb2.ConfigureVerticalRequest.FromString,
                    response_serializer=perftest__pb2.ConfigureVerticalResponse.SerializeToString,
            ),
            'ConfigureHorizontalTiming': grpc.unary_unary_rpc_method_handler(
                    servicer.ConfigureHorizontalTiming,
                    request_deserializer=perftest__pb2.ConfigureHorizontalTimingRequest.FromString,
                    response_serializer=perftest__pb2.ConfigureHorizontalTimingResponse.SerializeToString,
            ),
            'InitiateAcquisition': grpc.unary_unary_rpc_method_handler(
                    servicer.InitiateAcquisition,
                    request_deserializer=perftest__pb2.InitiateAcquisitionRequest.FromString,
                    response_serializer=perftest__pb2.InitiateAcquisitionResponse.SerializeToString,
            ),
            'Read': grpc.unary_unary_rpc_method_handler(
                    servicer.Read,
                    request_deserializer=perftest__pb2.ReadParameters.FromString,
                    response_serializer=perftest__pb2.ReadResult.SerializeToString,
            ),
            'ReadComplex': grpc.unary_unary_rpc_method_handler(
                    servicer.ReadComplex,
                    request_deserializer=perftest__pb2.ReadParameters.FromString,
                    response_serializer=perftest__pb2.ReadComplexResult.SerializeToString,
            ),
            'ReadContinuously': grpc.unary_stream_rpc_method_handler(
                    servicer.ReadContinuously,
                    request_deserializer=perftest__pb2.ReadContinuouslyParameters.FromString,
                    response_serializer=perftest__pb2.ReadContinuouslyResult.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'niPerfTest.niPerfTestService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class niPerfTestService(object):
    """---------------------------------------------------------------------
    The niPerfTest service definition.
    ---------------------------------------------------------------------
    """

    @staticmethod
    def StreamLatencyTest(request_iterator,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.stream_stream(request_iterator, target, '/niPerfTest.niPerfTestService/StreamLatencyTest',
            perftest__pb2.StreamLatencyClient.SerializeToString,
            perftest__pb2.StreamLatencyServer.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def StreamLatencyTestClient(request_iterator,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.stream_unary(request_iterator, target, '/niPerfTest.niPerfTestService/StreamLatencyTestClient',
            perftest__pb2.StreamLatencyClient.SerializeToString,
            perftest__pb2.StreamLatencyServer.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def StreamLatencyTestServer(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/niPerfTest.niPerfTestService/StreamLatencyTestServer',
            perftest__pb2.StreamLatencyClient.SerializeToString,
            perftest__pb2.StreamLatencyServer.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def TestWrite(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/TestWrite',
            perftest__pb2.TestWriteParameters.SerializeToString,
            perftest__pb2.TestWriteResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def TestWriteContinuously(request_iterator,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.stream_stream(request_iterator, target, '/niPerfTest.niPerfTestService/TestWriteContinuously',
            perftest__pb2.TestWriteParameters.SerializeToString,
            perftest__pb2.TestWriteResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def BeginTestSidebandStream(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/BeginTestSidebandStream',
            perftest__pb2.BeginTestSidebandStreamRequest.SerializeToString,
            perftest__pb2.BeginTestSidebandStreamResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def TestSidebandStream(request_iterator,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.stream_stream(request_iterator, target, '/niPerfTest.niPerfTestService/TestSidebandStream',
            perftest__pb2.TestSidebandStreamRequest.SerializeToString,
            perftest__pb2.TestSidebandStreamResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def Init(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/Init',
            perftest__pb2.InitParameters.SerializeToString,
            perftest__pb2.InitResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ConfigureVertical(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/ConfigureVertical',
            perftest__pb2.ConfigureVerticalRequest.SerializeToString,
            perftest__pb2.ConfigureVerticalResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ConfigureHorizontalTiming(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/ConfigureHorizontalTiming',
            perftest__pb2.ConfigureHorizontalTimingRequest.SerializeToString,
            perftest__pb2.ConfigureHorizontalTimingResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def InitiateAcquisition(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/InitiateAcquisition',
            perftest__pb2.InitiateAcquisitionRequest.SerializeToString,
            perftest__pb2.InitiateAcquisitionResponse.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def Read(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/Read',
            perftest__pb2.ReadParameters.SerializeToString,
            perftest__pb2.ReadResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ReadComplex(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/niPerfTest.niPerfTestService/ReadComplex',
            perftest__pb2.ReadParameters.SerializeToString,
            perftest__pb2.ReadComplexResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def ReadContinuously(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            insecure=False,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_stream(request, target, '/niPerfTest.niPerfTestService/ReadContinuously',
            perftest__pb2.ReadContinuouslyParameters.SerializeToString,
            perftest__pb2.ReadContinuouslyResult.FromString,
            options, channel_credentials,
            insecure, call_credentials, compression, wait_for_ready, timeout, metadata)
