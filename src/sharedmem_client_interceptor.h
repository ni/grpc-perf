class SharedMemoryForwardingInterceptorFactory : public grpc::experimental::ClientInterceptorFactoryInterface 
{
public:
    grpc::experimental::Interceptor* CreateClientInterceptor(grpc::experimental::ClientRpcInfo* info) override;
};
