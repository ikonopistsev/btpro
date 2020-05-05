#pragma once

#include "btpro/dns.hpp"
#include "btpro/tcp/bevfn.hpp"

namespace btpro {
namespace tcp {

template<class T>
class bevsockfn;

//template<class T>
//class bevsock
//{
//protected:
//    bevsockfn<T>& storage_;
//    T& self_;
//    bev bev_;
//};


//template<class T, int BEV_OPT_DEF = BEV_OPT_CLOSE_ON_FREE>
//class bevsockfn
//{
//public:
//    typedef void (T::*on_connect_t)();

//private:
//    T& self_;
//    on_connect_t on_connect_ = nullptr;

//public:
//    void connect(const std::string& hostname, int port, on_connect_t fn)
//    {   }

//    void shutdown(buffer_handle_t)
//    {   }
//};

} // namespace tcp
} // namespace btpro
