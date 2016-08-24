//
// detail/impl/resolver_service_base.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef NET_TS_DETAIL_IMPL_RESOLVER_SERVICE_BASE_IPP
#define NET_TS_DETAIL_IMPL_RESOLVER_SERVICE_BASE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <experimental/__net_ts/detail/config.hpp>
#include <experimental/__net_ts/detail/resolver_service_base.hpp>

#include <experimental/__net_ts/detail/push_options.hpp>

namespace std {
namespace experimental {
namespace net {
inline namespace v1 {
namespace detail {

class resolver_service_base::work_io_context_runner
{
public:
  work_io_context_runner(std::experimental::net::io_context& io_context)
    : io_context_(io_context) {}
  void operator()() { io_context_.run(); }
private:
  std::experimental::net::io_context& io_context_;
};

resolver_service_base::resolver_service_base(
    std::experimental::net::io_context& io_context)
  : io_context_impl_(std::experimental::net::use_service<io_context_impl>(io_context)),
    work_io_context_(new std::experimental::net::io_context),
    work_io_context_impl_(std::experimental::net::use_service<
        io_context_impl>(*work_io_context_)),
    work_(std::experimental::net::make_work_guard(*work_io_context_)),
    work_thread_(0)
{
}

resolver_service_base::~resolver_service_base()
{
  base_shutdown();
}

void resolver_service_base::base_shutdown()
{
  work_.reset();
  if (work_io_context_.get())
  {
    work_io_context_->stop();
    if (work_thread_.get())
    {
      work_thread_->join();
      work_thread_.reset();
    }
    work_io_context_.reset();
  }
}

void resolver_service_base::base_notify_fork(
    std::experimental::net::io_context::fork_event fork_ev)
{
  if (work_thread_.get())
  {
    if (fork_ev == std::experimental::net::io_context::fork_prepare)
    {
      work_io_context_->stop();
      work_thread_->join();
    }
    else
    {
      work_io_context_->restart();
      work_thread_.reset(new std::experimental::net::detail::thread(
            work_io_context_runner(*work_io_context_)));
    }
  }
}

void resolver_service_base::construct(
    resolver_service_base::implementation_type& impl)
{
  impl.reset(static_cast<void*>(0), socket_ops::noop_deleter());
}

void resolver_service_base::destroy(
    resolver_service_base::implementation_type& impl)
{
  NET_TS_HANDLER_OPERATION((io_context_impl_.context(),
        "resolver", &impl, 0, "cancel"));

  impl.reset();
}

void resolver_service_base::move_construct(implementation_type& impl,
    implementation_type& other_impl)
{
  impl = NET_TS_MOVE_CAST(implementation_type)(other_impl);
}

void resolver_service_base::move_assign(implementation_type& impl,
    resolver_service_base&, implementation_type& other_impl)
{
  destroy(impl);
  impl = NET_TS_MOVE_CAST(implementation_type)(other_impl);
}

void resolver_service_base::cancel(
    resolver_service_base::implementation_type& impl)
{
  NET_TS_HANDLER_OPERATION((io_context_impl_.context(),
        "resolver", &impl, 0, "cancel"));

  impl.reset(static_cast<void*>(0), socket_ops::noop_deleter());
}

void resolver_service_base::start_resolve_op(operation* op)
{
  start_work_thread();
  io_context_impl_.work_started();
  work_io_context_impl_.post_immediate_completion(op, false);
}

void resolver_service_base::start_work_thread()
{
  std::experimental::net::detail::mutex::scoped_lock lock(mutex_);
  if (!work_thread_.get())
  {
    work_thread_.reset(new std::experimental::net::detail::thread(
          work_io_context_runner(*work_io_context_)));
  }
}

} // namespace detail
} // inline namespace v1
} // namespace net
} // namespace experimental
} // namespace std

#include <experimental/__net_ts/detail/pop_options.hpp>

#endif // NET_TS_DETAIL_IMPL_RESOLVER_SERVICE_BASE_IPP