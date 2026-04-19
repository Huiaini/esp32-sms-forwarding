#include "push_retry.h"
#include "push.h"
#include "logger.h"
#include <queue>

// 内部重试条目：包装 PushRetryTask + 下次重试时间戳
struct RetryEntry {
  PushRetryTask     task;
  unsigned long     nextRetryMs;
};

static std::queue<RetryEntry> s_retryQueue;

void pushRetryInit() {
  while (!s_retryQueue.empty()) {
    s_retryQueue.pop();
  }
}

void pushRetryEnqueue(int channelIndex, const String& sender, const String& message,
                      const String& timestamp, MsgType msgType) {
  if (s_retryQueue.size() >= PUSH_RETRY_QUEUE_MAX) {
    LOG("Retry", "重试队列已满（%d 条），丢弃最旧条目", PUSH_RETRY_QUEUE_MAX);
    s_retryQueue.pop();
  }
  RetryEntry entry;
  entry.task.channelIndex = channelIndex;
  entry.task.sender       = sender;
  entry.task.message      = message;
  entry.task.timestamp    = timestamp;
  entry.task.msgType      = msgType;
  entry.nextRetryMs       = millis() + PUSH_RETRY_INTERVAL_MS;
  s_retryQueue.push(entry);
}

void pushRetryTick() {
  if (s_retryQueue.empty()) return;
  RetryEntry& front = s_retryQueue.front();
  if (millis() < front.nextRetryMs) return;

  const PushRetryTask& t = front.task;
  bool ok = sendPushChannel(t.channelIndex, t.sender, t.message, t.timestamp, t.msgType);
  if (ok) {
    LOG("Retry", "重试成功，通道索引 %d，出队", t.channelIndex);
    s_retryQueue.pop();
  } else {
    LOG("Retry", "重试失败，通道索引 %d，下次重试 %lu ms 后", t.channelIndex, PUSH_RETRY_INTERVAL_MS);
    front.nextRetryMs = millis() + PUSH_RETRY_INTERVAL_MS;
  }
}
