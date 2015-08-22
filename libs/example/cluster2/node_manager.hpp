﻿///
/// Copyright (c) 2009-2015 Nous Xiong (348944179 at qq dot com)
///
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
/// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// See https://github.com/nousxiong/gce for latest version.
///

#ifndef CLUSTER2_NODE_MANAGER_HPP
#define CLUSTER2_NODE_MANAGER_HPP

#include "node_info.hpp"
#include <gce/actor/all.hpp>
#include <gce/log/all.hpp>

/// 返回是否登录成功+需要连接的其它node的信息
static bool login(gce::stackful_actor& self, std::set<gce::ctxid_t>& connected_list)
{
  using namespace gce;

  context& ctx = self.get_context();
  ctxid_t ctxid = ctx.get_ctxid();
  log::logger_t lg = ctx.get_logger();

  svcid_t master_mgr = make_svcid("master", "master_mgr");
  errcode_t ec;

  while (true)
  {
    GCE_INFO(lg) << "node<" << ctxid << "> login master...";
    self.link(master_mgr);
    self->send(master_mgr, "login");

    bool ret = false;
    message msg;
    self->match("login_ret").guard(master_mgr, ec).raw(msg).recv(ret);
    if (ec)
    {
      GCE_INFO(lg) << "node<" << ctxid << "> login master error: " << ec;
    }
    else
    {
      if (ret)
      {
        GCE_INFO(lg) << "node<" << ctxid << "> login master success";
        cluster2::node_info_list ni_list;
        msg >> ni_list;
        /// 连接指定的其它node，来构建全联通
        BOOST_FOREACH(cluster2::node_info const& ni, ni_list.list_)
        {
          std::pair<std::set<ctxid_t>::iterator, bool> pr = connected_list.insert(ni.ctxid_);
          if (pr.second)
          {
            /**
             *   node进程启动之后，保证只调用一次gce::connect，否则每调用一次就会多一个连接，
             * 虽然并无较大影响，但如果调用次数过多，连接会逐渐增多，最终可能会成为系统负担
             */
            connect(self, ni.ctxid_, ni.ep_);
          }
        }
        return true;
      }
      else
      {
        /// 这个node在master端找不到，说明出了配置错误，很有可能是人为错误，不再继续，直接退出，让相关人员处理
        std::string errmsg;
        msg >> errmsg;
        GCE_INFO(lg) << "node<" << ctxid << "> login master failed: " << errmsg;
        return false;
      }
    }
  }
}

static void node_manager(gce::stackful_actor self)
{
  using namespace gce;

  context& ctx = self.get_context();
  ctxid_t ctxid = ctx.get_ctxid();
  log::logger_t lg = ctx.get_logger();
  register_service(self, "node_mgr");

  /// 保存已经建立过连接的node
  std::set<ctxid_t> connected_list;

  bool need_login = true;
  svcid_t master_mgr = make_svcid("master", "master_mgr");
  svcid_t node_mgr = make_svcid(ctxid, "node_mgr");
  while (true)
  {
    try
    {
      /// 只有在需要登录的时候才进行
      if (need_login && !login(self, connected_list))
      {
        break;
      }
      need_login = true;

      message msg;
      errcode_t ec;
      match_t type;
      self->match("quit", type).guard(master_mgr, ec).raw(msg).recv();
      if (!ec)
      {
        if (type == atom("quit"))
        {
          self->send(master_mgr, "quit_ret");
          break;
        }
        else
        {
          std::string errmsg = "not available command: ";
          errmsg += atom(type);
          self->send(master_mgr, "error", errmsg);

          /// 此种情况不需要重新登录了
          need_login = false;
        }
      }
    }
    catch (std::exception& ex)
    {
      GCE_ERROR(lg) << "node_manager<" << ctxid << "> except: " << ex.what();
    }
  }

  deregister_service(self, "node_mgr");
  GCE_INFO(lg) << "node<" << ctxid << "> quit";
}

#endif /// CLUSTER2_NODE_MANAGER_HPP
