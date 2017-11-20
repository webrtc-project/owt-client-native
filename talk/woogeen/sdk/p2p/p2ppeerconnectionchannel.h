/*
 * Intel License
 */

#ifndef WOOGEEN_P2P_P2PPEERCONNECTIONCHANNEL_H_
#define WOOGEEN_P2P_P2PPEERCONNECTIONCHANNEL_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

#include "talk/woogeen/sdk/base/mediaconstraintsimpl.h"
#include "talk/woogeen/sdk/base/peerconnectiondependencyfactory.h"
#include "talk/woogeen/sdk/base/peerconnectionchannel.h"
#include "talk/woogeen/sdk/include/cpp/woogeen/base/stream.h"
#include "talk/woogeen/sdk/include/cpp/woogeen/p2p/p2pexception.h"
#include "talk/woogeen/sdk/include/cpp/woogeen/p2p/p2psignalingsenderinterface.h"
#include "talk/woogeen/sdk/include/cpp/woogeen/p2p/p2psignalingreceiverinterface.h"
#include "webrtc/rtc_base/json.h"
#include "webrtc/rtc_base/messagehandler.h"
#include "webrtc/rtc_base/task_queue.h"

namespace woogeen {
namespace p2p {

using namespace woogeen::base;

// P2PPeerConnectionChannel callback interface.
// Usually, PeerClient should implement these methods and notify application.
class P2PPeerConnectionChannelObserver {
 public:
  // Triggered when received an invitation.
  virtual void OnInvited(const std::string& remote_id) = 0;
  // Triggered when remote user accepted the invitation.
  virtual void OnAccepted(const std::string& remote_id) = 0;
  // Triggered when the WebRTC session is started.
  virtual void OnStarted(const std::string& remote_id) = 0;
  // Triggered when the WebRTC session is ended.
  virtual void OnStopped(const std::string& remote_id) = 0;
  // Triggered when remote user denied the invitation.
  virtual void OnDenied(const std::string& remote_id) = 0;
  // Triggered when remote user send data via data channel.
  // Currently, data is string.
  virtual void OnData(const std::string& remote_id,
                      const std::string& message) = 0;
  // Triggered when a new stream is added.
  virtual void OnStreamAdded(
      std::shared_ptr<RemoteCameraStream> stream) = 0;
  virtual void OnStreamAdded(
      std::shared_ptr<RemoteScreenStream> stream) = 0;
  // Triggered when a remote stream is removed.
  virtual void OnStreamRemoved(
      std::shared_ptr<RemoteCameraStream> stream) = 0;
  virtual void OnStreamRemoved(
      std::shared_ptr<RemoteScreenStream> stream) = 0;
};

// An instance of P2PPeerConnectionChannel manages a session for a specified
// remote client.
class P2PPeerConnectionChannel : public P2PSignalingReceiverInterface,
                                 public PeerConnectionChannel {
 public:
  explicit P2PPeerConnectionChannel(
      PeerConnectionChannelConfiguration configuration,
      const std::string& local_id,
      const std::string& remote_id,
      P2PSignalingSenderInterface* sender,
      std::shared_ptr<rtc::TaskQueue> event_queue);
  // If event_queue is not provided, a new event queue will be used. That means,
  // a new thread will be created for each P2PPeerConnection. Currently, iOS
  // SDK's RTCP2PPeerConnection is a pure Obj-C file, so it does not maintain
  // event queue.
  explicit P2PPeerConnectionChannel(
      PeerConnectionChannelConfiguration configuration,
      const std::string& local_id,
      const std::string& remote_id,
      P2PSignalingSenderInterface* sender);
  virtual ~P2PPeerConnectionChannel();
  // Add a P2PPeerConnectionChannel observer so it will be notified when this
  // object have some events.
  void AddObserver(P2PPeerConnectionChannelObserver* observer);
  // Remove a P2PPeerConnectionChannel observer. If the observer doesn't exist,
  // it will do nothing.
  void RemoveObserver(P2PPeerConnectionChannelObserver* observer);
  // Implementation of P2PSignalingReceiverInterface. Handle signaling message
  // received from remote side.
  void OnIncomingSignalingMessage(const std::string& message) override;
  // Invite a remote client to start a WebRTC session.
  void Invite(std::function<void()> on_success,
              std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Accept a remote client's invitation.
  void Accept(std::function<void()> on_success,
              std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Deny a remote client's invitation.
  void Deny(std::function<void()> on_success,
            std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Publish a local stream to remote user.
  void Publish(std::shared_ptr<LocalStream> stream,
               std::function<void()> on_success,
               std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Unpublish a local stream to remote user.
  void Unpublish(std::shared_ptr<LocalStream> stream,
                 std::function<void()> on_success,
                 std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Send message to remote user.
  void Send(const std::string& message,
            std::function<void()> on_success,
            std::function<void(std::unique_ptr<P2PException>)> on_failure);
  // Stop current WebRTC session.
  void Stop(std::function<void()> on_success,
            std::function<void(std::unique_ptr<P2PException>)> on_failure);

  // Get statistics data for the specific connection.
  void GetConnectionStats(
      std::function<void(std::shared_ptr<ConnectionStats>)> on_success,
      std::function<void(std::unique_ptr<P2PException>)> on_failure);

 protected:
  void CreateOffer();
  void CreateAnswer();

  // Received messages from remote client.
  void OnMessageInvitation(Json::Value& ua);
  void OnMessageAcceptance(Json::Value& ua);
  void OnMessageStop();
  void OnMessageDeny();
  void OnMessageSignal(Json::Value& signal);
  void OnMessageNegotiationNeeded();
  void OnMessageStreamType(Json::Value& type_info);

  // PeerConnectionObserver
  virtual void OnSignalingChange(
      PeerConnectionInterface::SignalingState new_state);
  virtual void OnAddStream(rtc::scoped_refptr<MediaStreamInterface> stream);
  virtual void OnRemoveStream(rtc::scoped_refptr<MediaStreamInterface> stream);
  virtual void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel);
  virtual void OnRenegotiationNeeded();
  virtual void OnIceConnectionChange(
      PeerConnectionInterface::IceConnectionState new_state);
  virtual void OnIceGatheringChange(
      PeerConnectionInterface::IceGatheringState new_state);
  virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

  // DataChannelObserver
  virtual void OnDataChannelStateChange();
  virtual void OnDataChannelMessage(const webrtc::DataBuffer& buffer);

  // CreateSessionDescriptionObserver
  virtual void OnCreateSessionDescriptionSuccess(
      webrtc::SessionDescriptionInterface* desc);
  virtual void OnCreateSessionDescriptionFailure(const std::string& error);

  // SetSessionDescriptionObserver
  virtual void OnSetLocalSessionDescriptionSuccess();
  virtual void OnSetLocalSessionDescriptionFailure(const std::string& error);
  virtual void OnSetRemoteSessionDescriptionSuccess();
  virtual void OnSetRemoteSessionDescriptionFailure(const std::string& error);

  enum SessionState : int;
  enum NegotiationState : int;

 private:
  void ChangeSessionState(SessionState state);
  void SendSignalingMessage(
      const Json::Value& data,
      std::function<void()> success,
      std::function<void(std::unique_ptr<P2PException>)> failure);
  // Publish and/or unpublish all streams in pending stream list.
  void DrainPendingStreams();
  void CheckWaitedList();  // Check pending streams and negotiation requests.
  void SendAcceptance(
      std::function<void()> on_success,
      std::function<void(std::unique_ptr<P2PException>)> on_failure);
  void SendStop(std::function<void()> on_success,
                std::function<void(std::unique_ptr<P2PException>)> on_failure);
  void SendDeny(std::function<void()> on_success,
                std::function<void(std::unique_ptr<P2PException>)> on_failure);
  void ClosePeerConnection();  // Stop session and clean up.
  // Returns true if |pointer| is not nullptr. Otherwise, return false and
  // execute |on_failure|.
  bool CheckNullPointer(
      uintptr_t pointer,
      std::function<void(std::unique_ptr<P2PException>)> on_failure);
  webrtc::DataBuffer CreateDataBuffer(const std::string& data);
  void CreateDataChannel(const std::string& label);
  // Send all messages in pending message list.
  void DrainPendingMessages();
  void TriggerOnStopped();
  // Cleans all variables associated with last peerconnection.
  void CleanLastPeerConnection();
  // Returns user agent info as JSON object.
  Json::Value UaInfo();
  // Set remote capability flags based on UA.
  void HandleRemoteCapability(Json::Value& ua);

  P2PSignalingSenderInterface* signaling_sender_;
  std::string local_id_;
  std::string remote_id_;
  bool is_caller_;
  SessionState session_state_;
  // Indicates if negotiation needed event is triggered or received negotiation
  // request from remote side, but haven't send out offer.
  bool negotiation_needed_;
  // Key is remote media stream's label, value is type ("audio", "video",
  // "screen").
  std::unordered_map<std::string, std::string> remote_stream_type_;
  // Key is remote media stream's label, value is RemoteStream instance.
  std::unordered_map<std::string, std::shared_ptr<RemoteStream>> remote_streams_;
  std::vector<std::shared_ptr<LocalStream>>
      pending_publish_streams_;  // Streams need to be published.
  std::vector<std::shared_ptr<LocalStream>>
      pending_unpublish_streams_;  // Streams need to be unpublished.
  // A set of labels for streams published or will be publish to remote side.
  // |Publish| adds its argument to this vector, |Unpublish| removes it.
  std::unordered_set<std::string> published_streams_;
  std::mutex pending_publish_streams_mutex_;
  std::mutex pending_unpublish_streams_mutex_;
  std::mutex published_streams_mutex_;
  std::vector<P2PPeerConnectionChannelObserver*> observers_;
  // Store remote SDP if it cannot be set currently.
  SetSessionDescriptionMessage* set_remote_sdp_task_;
  std::chrono::time_point<std::chrono::system_clock>
      last_disconnect_;  // Last time |peer_connection_| changes its state to
                         // "disconnect"
  int reconnect_timeout_;  // Unit: second
  std::vector<std::shared_ptr<std::string>> pending_messages_;  // Messages need
                                                                // to be sent
                                                                // once data
                                                                // channel is
                                                                // ready.
  std::mutex pending_messages_mutex_;
  // Indicates whether remote client supports WebRTC Plan B
  // (https://tools.ietf.org/html/draft-uberti-rtcweb-plan-00).
  // If plan B is not supported, at most one audio/video track is supported.
  bool remote_side_supports_plan_b_;
  bool remote_side_supports_remove_stream_;
  bool is_creating_offer_;  // It will be true during creating and setting offer.
  std::mutex is_creating_offer_mutex_;
  // Queue for callbacks and events.
  std::shared_ptr<rtc::TaskQueue> event_queue_;
};
}
}

#endif  // WOOGEEN_P2P_P2PPEERCONNECTIONCHANNEL_H_
