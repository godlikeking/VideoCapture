#include "Common.h"
#include <queue>
#include <mutex>
#include <condition_variable>

namespace video {

FrameQueue::FrameQueue(size_t max_size) : max_size_(max_size) {}
FrameQueue::~FrameQueue() { wake_all(); }

void FrameQueue::push(Frame frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_producer_.wait(lock, [this] { return queue_.size() < max_size_ || shutdown_; });
    if (shutdown_) return;
    queue_.push(std::move(frame));
    cv_consumer_.notify_one();
}

bool FrameQueue::try_push(Frame frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= max_size_ || shutdown_) return false;
    queue_.push(std::move(frame));
    cv_consumer_.notify_one();
    return true;
}

void FrameQueue::pop(Frame& frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_consumer_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
    if (queue_.empty()) return;
    frame = std::move(queue_.front());
    queue_.pop();
    cv_producer_.notify_one();
}

bool FrameQueue::try_pop(Frame& frame) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    frame = std::move(queue_.front());
    queue_.pop();
    cv_producer_.notify_one();
    return true;
}

size_t FrameQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void FrameQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<Frame> empty;
    queue_.swap(empty);
}

void FrameQueue::wake_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    cv_producer_.notify_all();
    cv_consumer_.notify_all();
}

PacketQueue::PacketQueue(size_t max_size) : max_size_(max_size) {}
PacketQueue::~PacketQueue() { wake_all(); }

void PacketQueue::push(Packet packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_producer_.wait(lock, [this] { return queue_.size() < max_size_ || shutdown_; });
    if (shutdown_) return;
    queue_.push(std::move(packet));
    cv_consumer_.notify_one();
}

bool PacketQueue::try_push(Packet packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.size() >= max_size_ || shutdown_) return false;
    queue_.push(std::move(packet));
    cv_consumer_.notify_one();
    return true;
}

void PacketQueue::pop(Packet& packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_consumer_.wait(lock, [this] { return !queue_.empty() || shutdown_; });
    if (queue_.empty()) return;
    packet = std::move(queue_.front());
    queue_.pop();
    cv_producer_.notify_one();
}

bool PacketQueue::try_pop(Packet& packet) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) return false;
    packet = std::move(queue_.front());
    queue_.pop();
    cv_producer_.notify_one();
    return true;
}

size_t PacketQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void PacketQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::queue<Packet> empty;
    queue_.swap(empty);
}

void PacketQueue::wake_all() {
    std::lock_guard<std::mutex> lock(mutex_);
    shutdown_ = true;
    cv_producer_.notify_all();
    cv_consumer_.notify_all();
}

} // namespace video
