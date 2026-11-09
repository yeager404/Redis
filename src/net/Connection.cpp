#include "redis/net/Connection.h"

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

namespace redis::net {
  namespace  {
    constexpr std::size_t READ_BUFFER_SIZE = 16 * 1024;
    constexpr std::size_t MAX_INPUT_BUFFER = 64 * 1024 * 1024;
    constexpr std::size_t MAX_OUTPUT_BUFFER = 64 * 1024 * 1024;
  } // namespace 
  
  Connection::Connection(ConnectionId id, std::unique_ptr<ISocket> socket, std::unique_ptr<protocol::ProtocolCodec> codec)
    : id_(id),
      socket_(std::move(socket)),
      codec_(std::move(codec)){}

  ConnectionId Connection::id() const noexcept {
    return id_;
  }

  bool Connection::onReadable(application:CommandDispatcher& dispatcher){
    std::array<std::byte, READ_BUFFER_SIZE> bytes{};

    // Read repeatedly until the non-blocking socket would block.
    while(true){
      IoResult result = socket_->read(bytes);
      if(result.status == IoStatus::Progress) {
        input_.append(std::string_view(reinterpret_cast<const char*>(bytes.data()), result.bytesTransferred));

        if(input_.size() > MAX_INPUT_BUFFER){
          return false;
        }
        continue;
      }

      if(result.status == IoStatus::WouldBlock){
        break;
      }

      if(result.status == IoStatus::PeerClosed){
        peerClosed_ = true;
        break;
      }

      return false;
    }

    // Decode all complete requests currently buffered.
    while(!input_.empty()){
      protocol::DecodeResult decoded = codec_->decodeOne(input_.view());

      if(decoded.status == protocol::DecodeStatus::Incomplete){
        break;
      }

      if(decoded.status == protocol::DecodeStatus::Error){
        // a malformed request is a protocol error.
        // queue an error if possiblem, then close after writing.
        const std::string errorWire = codec_->encode(application::Response::error("ERR protocol error: " + decoded.error));

        output_.append(errorWire);
        peerClosed_ = true;
        break;
      }

      if(decoded.consume == 0) {
        // Protect against a broken codec that never advances.
        return false;
      }

      input_.consume(decoded.consumed);

      application::Response response = dispatcher.execute(decoded.request);

      const std::string wire = codec_->encode(response);
      output_.append(wire);

      if(output_.size() > MAX_OUTPUT_BUFFER){
        return false;
      }
    }

    if(peerClosed_ && output_.empty()){
      return false;
    }

    return true;
  }

  bool Connection::onWritable(){
    while(!output_.empty()){
      const std::string_view pending = output_.view();

      const auto* bytes = reinterpret_cast<const std::byte*>(pending.data());

      IoResult result = socket->write(std::span<const std::byte>(bytes, pending.size()));
      if(result.status == IoStatus::Progress){
        output_.consume(result.bytesTransferred);
        continue;
      }

      if(result.status == IoStatus::WouldBlock){
        return true;
      }

      return false;
    }

    if(peerClosed_){
      return false;
    }

    return true;
  }

  bool Connection::wantsWrite() const noexcept {
    return !output_.empty();
  }
} // namespace redis::net
