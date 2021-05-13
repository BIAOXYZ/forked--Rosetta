// ==============================================================================
// Copyright 2020 The LatticeX Foundation
// This file is part of the Rosetta library.
//
// The Rosetta library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The Rosetta library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with the Rosetta library. If not, see <http://www.gnu.org/licenses/>.
// ==============================================================================

#pragma once
#include "cc/modules/io/include/internal/comm.h"
#include "cc/modules/common/include/utils/msg_id.h"
#include "cc/modules/io/include/internal/cycle_buffer.h"
#include "cc/modules/io/include/internal/socket.h"
#include "cc/modules/io/include/internal/ssl_socket.h"

#include <atomic>
#include <map>
#include <iostream>
#include <mutex>
using namespace std;

namespace rosetta {
namespace io {

struct Connection {
 public:
  Connection(int _fd, int _events, bool _is_server);
  virtual ~Connection();

 public:
  virtual bool handshake() {return true;}
  virtual void close();
  bool is_server() const { return is_server_; }

 public:
  ssize_t send(const char* data, size_t len, int64_t timeout = -1L);
  ssize_t recv(char* data, size_t len, int64_t timeout = -1L);
  ssize_t recv(const msg_id_t& msg_id, char* data, size_t len, int64_t timeout = -1L);

  // Read & Write
 public:
  ssize_t peek(int sockfd, void* buf, size_t len);
  ssize_t readn(int connfd, char* vptr, size_t n);
  ssize_t writen(int connfd, const char* vptr, size_t n);

  virtual ssize_t readImpl(int fd, char* data, size_t len) {
    ssize_t ret = ::read(fd, data, len);
    return ret;
  }
  virtual ssize_t writeImpl(int fd, const char* data, size_t len) {
    ssize_t ret = ::write(fd, data, len);
    return ret;
  }

 protected:
  std::mutex mtx_send_;
  std::atomic<int> atomic_send_{0};

 public:
  enum State {
    Invalid = 1,
    Handshaking,
    Handshaked,
    Connecting,
    Connected,
    Closing,
    Closed,
    Failed,
  };
  State state_ = State::Invalid;

  int verbose_ = 0;
  int fd_ = -1;
  int events_ = 0;
  bool is_server_ = false;
  string client_ip_ = "";
  int client_port_ = 0;

  //! buffer manage
  //! for all messages
  shared_ptr<cycle_buffer> buffer_ = nullptr;
  //! for one message which id is msg_id_t
  map<msg_id_t, shared_ptr<cycle_buffer>> mapbuffer_;
  std::mutex mapbuffer_mtx_;

#if USE_LIBEVENT_AS_BACKEND
  bool has_set_client_id = false;
  std::mutex set_client_id_mtx_;
  void* thread_ptr_ = nullptr; // do not delete this pointer in this class
  void* obj_ptr_ = nullptr; // do not delete this pointer in this class
  struct bufferevent* bev_ = nullptr; // buffevent, do not delete this pointer in this class
#endif

  SSL_CTX* ctx_ = nullptr; // do not delete this pointer in this class
};

class SSLConnection : public Connection {
  using Connection::Connection;
  SSL* ssl_ = nullptr;
  std::mutex ssl_rw_mtx_;

 public:
  ~SSLConnection();
#if USE_LIBEVENT_AS_BACKEND
  virtual void close() {}
  virtual bool handshake() {return true;}
#else
  virtual void close();
  virtual bool handshake();
  virtual ssize_t readImpl(int fd, char* data, size_t len);
  virtual ssize_t writeImpl(int fd, const char* data, size_t len);
#endif
};

} // namespace io
} // namespace rosetta
