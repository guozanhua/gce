﻿///
/// Copyright (c) 2009-2014 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef GCE_ACTOR_COROUTINE_STACKLESS_ACTOR_HPP
#define GCE_ACTOR_COROUTINE_STACKLESS_ACTOR_HPP

#include <gce/actor/config.hpp>
#include <gce/actor/basic_actor.hpp>
#include <gce/actor/pattern.hpp>
#include <gce/actor/detail/mailbox_fwd.hpp>
#include <gce/actor/detail/scoped_bool.hpp>
#include <boost/bind.hpp>

namespace gce
{
namespace detail
{
class cache_pool;
class mailbox;
}
class thread_mapped_actor;
class context;

template <class> class actor;

class coroutine_stackless_actor
  : public basic_actor
{
  typedef basic_actor base_type;

public:
  inline void send(aid_t recver, message const& m)
  {
    base_type::pri_send(recver, m);
  }

  inline void send(svcid_t recver, message const& m)
  {
    base_type::pri_send_svc(recver, m);
  }

  inline void relay(aid_t des, message& m)
  {
    base_type::pri_relay(des, m);
  }

  inline void relay(svcid_t des, message& m)
  {
    base_type::pri_relay_svc(des, m);
  }

  inline resp_t request(aid_t recver, message const& m)
  {
    resp_t res(new_request(), get_aid(), recver);
    base_type::pri_request(res, recver, m);
    return res;
  }

  inline resp_t request(svcid_t recver, message const& m)
  {
    resp_t res(new_request(), get_aid(), recver);
    base_type::pri_request_svc(res, recver, m);
    return res;
  }

  inline void reply(aid_t recver, message const& m)
  {
    base_type::pri_reply(recver, m);
  }

  inline void link(aid_t target)
  {
    base_type::pri_link(target);
  }

  inline void monitor(aid_t target)
  {
    base_type::pri_monitor(target);
  }

  typedef actor<stackless>& self_ref_t;
  typedef boost::function<void (self_ref_t)> func_t;
  typedef boost::function<void (self_ref_t, aid_t, message)> recv_handler_t;
  typedef boost::function<void (self_ref_t)> wait_handler_t;

public:
  coroutine_stackless_actor(aid_t aid, detail::cache_pool*);
  ~coroutine_stackless_actor();

public:
  void recv(aid_t& sender, message& msg, pattern const& patt = pattern());
  aid_t recv(message& msg, match_list_t const& match_list = match_list_t());
  void recv(
    resp_t res, aid_t& sender, message& msg, 
    duration_t tmo = seconds_t(GCE_DEFAULT_REQUEST_TIMEOUT_SEC)
    );
  aid_t recv(resp_t res, message& msg);

  void wait(duration_t);

public:
  /// internal use
  static detail::actor_type type() { return detail::actor_stackless; }
  void recv(recv_handler_t const&, pattern const& patt = pattern());
  void recv(
    recv_handler_t const&, resp_t res, 
    duration_t tmo = seconds_t(GCE_DEFAULT_REQUEST_TIMEOUT_SEC)
    );

  void wait(wait_handler_t const&, duration_t);
  void quit(exit_code_t exc = exit_normal, std::string const& errmsg = std::string());

public:
  void init(func_t const& f);
  void start();
  void on_recv(detail::pack&, detail::send_hint);

  inline sid_t spawn(
    detail::spawn_type type, std::string const& func, 
    match_t ctxid, std::size_t stack_size
    )
  {
    sid_t sid = new_request();
    base_type::pri_spawn(sid, type, func, ctxid, stack_size);
    return sid;
  }

  inline detail::coro_t& coro() { return coro_; }

  void spawn_handler(self_ref_t, aid_t, aid_t&);
  void run();

private:
  void stop(aid_t self_aid, exit_code_t, std::string const&);
  void start_recv_timer(duration_t, recv_handler_t const&);
  void start_res_timer(duration_t, recv_handler_t const&);
  void start_wait_timer(duration_t, wait_handler_t const&);
  void handle_recv_timeout(errcode_t const&, std::size_t, recv_handler_t const&);
  void handle_res_timeout(errcode_t const&, std::size_t, recv_handler_t const&);
  void handle_wait_timeout(errcode_t const&, std::size_t, wait_handler_t const&);
  void handle_recv(detail::pack&);

  aid_t end_recv(detail::recv_t&, message&);
  aid_t end_recv(resp_t&);

  void recv_handler(self_ref_t, aid_t, message, aid_t&, message&);
  void wait_handler(self_ref_t);

private:
  /// Ensure start from a new cache line.
  byte_t pad0_[GCE_CACHE_LINE_SIZE];

  GCE_CACHE_ALIGNED_VAR(func_t, f_)
  GCE_CACHE_ALIGNED_VAR(detail::coro_t, coro_)

  /// thread local vals
  recv_handler_t recv_h_;
  recv_handler_t res_h_;
  wait_handler_t wait_h_;
  resp_t recving_res_;
  pattern curr_pattern_;
  timer_t tmr_;
  std::size_t tmr_sid_;
};

typedef coroutine_stackless_actor::recv_handler_t recv_handler_t;
typedef coroutine_stackless_actor::wait_handler_t wait_handler_t;
}

#endif /// GCE_ACTOR_COROUTINE_STACKLESS_ACTOR_HPP
