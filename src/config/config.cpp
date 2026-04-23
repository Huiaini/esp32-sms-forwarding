#include "config.h"
#include <Preferences.h>
#include "logger.h"
#include <time.h>

Config config;
RebootSchedule rebootSchedule;

static Preferences preferences;

void loadConfig() {
  preferences.begin("sms_config", false);
  config.adminPhone = preferences.getString("adminPhone", "");
  config.webUser    = preferences.getString("webUser", DEFAULT_WEB_USER);
  config.webPass    = preferences.getString("webPass", DEFAULT_WEB_PASS);

  config.pushCount = (int)preferences.getUChar("pushCount", 5);
  if (config.pushCount < 1) config.pushCount = 1;
  if (config.pushCount > MAX_PUSH_CHANNELS) config.pushCount = MAX_PUSH_CHANNELS;

  for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
    char prefix[8], key[16];
    snprintf(prefix, sizeof(prefix), "push%d", i);
    snprintf(key, sizeof(key), "%sen", prefix);
    config.pushChannels[i].enabled    = preferences.getBool(key, false);
    snprintf(key, sizeof(key), "%stype", prefix);
    config.pushChannels[i].type       = (PushType)preferences.getUChar(key, PUSH_TYPE_POST_JSON);
    snprintf(key, sizeof(key), "%surl", prefix);
    config.pushChannels[i].url        = preferences.getString(key, "");
    snprintf(key, sizeof(key), "%sname", prefix);
    { char defName[12]; snprintf(defName, sizeof(defName), "通道%d", i + 1);
      config.pushChannels[i].name = preferences.isKey(key) ? preferences.getString(key, defName) : defName; }
    snprintf(key, sizeof(key), "%sk1", prefix);
    config.pushChannels[i].key1       = preferences.getString(key, "");
    snprintf(key, sizeof(key), "%sk2", prefix);
    config.pushChannels[i].key2       = preferences.getString(key, "");
    snprintf(key, sizeof(key), "%sbody", prefix);
    config.pushChannels[i].customBody = preferences.getString(key, "");
    snprintf(key, sizeof(key), "%sretry", prefix);
    config.pushChannels[i].retryOnFail = preferences.getBool(key, false);
  }

  // 兼容旧配置：迁移旧 httpUrl 到第一个通道
  String oldHttpUrl = preferences.getString("httpUrl", "");
  if (oldHttpUrl.length() > 0 && !config.pushChannels[0].enabled) {
    config.pushChannels[0].enabled = true;
    config.pushChannels[0].url     = oldHttpUrl;
    config.pushChannels[0].type    = preferences.getUChar("barkMode", 0) != 0 ? PUSH_TYPE_BARK : PUSH_TYPE_POST_JSON;
    config.pushChannels[0].name    = "迁移通道";
    LOG("Config", "已迁移旧HTTP配置到推送通道1");
  }

  config.simNotifyEnabled = preferences.getBool("simNotify", false);
  config.dataTraffic       = preferences.getBool("dataTraffic", false);

  // Multi-WiFi: 读取 wifiCount，如不存在则迁移旧单 WiFi 键（FR-017）
  if (preferences.isKey("wifiCount")) {
    config.wifiCount = (int)preferences.getUChar("wifiCount", 0);
    if (config.wifiCount < 0) config.wifiCount = 0;
    if (config.wifiCount > MAX_WIFI_ENTRIES) config.wifiCount = MAX_WIFI_ENTRIES;
    for (int i = 0; i < config.wifiCount; i++) {
      char ks[16], kp[16];
      snprintf(ks, sizeof(ks), "wifi%dssid", i);
      snprintf(kp, sizeof(kp), "wifi%dpass", i);
      config.wifiList[i].ssid     = preferences.getString(ks, "");
      config.wifiList[i].password = preferences.getString(kp, "");
    }
  } else {
    // 迁移旧单 WiFi 键
    String legacySsid = preferences.getString("wifiSsid", "");
    String legacyPass = preferences.getString("wifiPass",  "");
    if (legacySsid.length() > 0) {
      config.wifiList[0].ssid     = legacySsid;
      config.wifiList[0].password = legacyPass;
      config.wifiCount = 1;
      LOG("Config", "已迁移旧单WiFi配置到wifiList[0]");
    } else {
      config.wifiCount = 0;
    }
  }

  config.pushStrategy = (PushStrategy)preferences.getUChar("pushStrategy", 0);

  config.blacklistCount = preferences.getInt("blCount", 0);
  if (config.blacklistCount > MAX_BLACKLIST_ENTRIES) config.blacklistCount = MAX_BLACKLIST_ENTRIES;
  for (int i = 0; i < config.blacklistCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "bl%d", i);
    config.blacklist[i] = preferences.getString(key, "");
  }

  preferences.end();
  LOG("Config", "配置已加载");
}

void saveConfig() {
  preferences.begin("sms_config", false);
  preferences.putString("adminPhone", config.adminPhone);
  preferences.putString("webUser",    config.webUser);
  preferences.putString("webPass",    config.webPass);

  preferences.putUChar("pushCount", (uint8_t)config.pushCount);
  for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
    char key[16];
    snprintf(key, sizeof(key), "push%den", i);    preferences.putBool(key,   config.pushChannels[i].enabled);
    snprintf(key, sizeof(key), "push%dtype", i);  preferences.putUChar(key,  (uint8_t)config.pushChannels[i].type);
    snprintf(key, sizeof(key), "push%durl", i);   preferences.putString(key, config.pushChannels[i].url);
    snprintf(key, sizeof(key), "push%dname", i);  preferences.putString(key, config.pushChannels[i].name);
    snprintf(key, sizeof(key), "push%dk1", i);    preferences.putString(key, config.pushChannels[i].key1);
    snprintf(key, sizeof(key), "push%dk2", i);    preferences.putString(key, config.pushChannels[i].key2);
    snprintf(key, sizeof(key), "push%dbody", i);  preferences.putString(key, config.pushChannels[i].customBody);
    snprintf(key, sizeof(key), "push%dretry", i); preferences.putBool(key,   config.pushChannels[i].retryOnFail);
  }

  preferences.putBool("simNotify",    config.simNotifyEnabled);
  preferences.putBool("dataTraffic",  config.dataTraffic);

  preferences.putUChar("wifiCount", (uint8_t)config.wifiCount);
  for (int i = 0; i < config.wifiCount; i++) {
    char ks[16], kp[16];
    snprintf(ks, sizeof(ks), "wifi%dssid", i);
    snprintf(kp, sizeof(kp), "wifi%dpass", i);
    preferences.putString(ks, config.wifiList[i].ssid);
    preferences.putString(kp, config.wifiList[i].password);
  }

  preferences.putUChar("pushStrategy", (uint8_t)config.pushStrategy);

  preferences.putInt("blCount", config.blacklistCount);
  for (int i = 0; i < config.blacklistCount; i++) {
    char key[8];
    snprintf(key, sizeof(key), "bl%d", i);
    preferences.putString(key, config.blacklist[i]);
  }

  preferences.end();
  LOG("Config", "配置已保存");
}

void loadRebootSchedule(RebootSchedule& sched) {
  preferences.begin("reboot_cfg", false);
  sched.enabled   = preferences.getBool("rb_enabled", false);
  sched.mode      = preferences.getUChar("rb_mode", 0);
  sched.hour      = preferences.getUChar("rb_hour", 3);
  sched.minute    = preferences.getUChar("rb_minute", 0);
  sched.intervalH = preferences.getUShort("rb_interval", 24);
  preferences.end();
}

void saveRebootSchedule(const RebootSchedule& sched) {
  preferences.begin("reboot_cfg", false);
  preferences.putBool("rb_enabled",    sched.enabled);
  preferences.putUChar("rb_mode",      sched.mode);
  preferences.putUChar("rb_hour",      sched.hour);
  preferences.putUChar("rb_minute",    sched.minute);
  preferences.putUShort("rb_interval", sched.intervalH);
  preferences.end();
}

void rebootTick() {
  if (!rebootSchedule.enabled) return;

  if (rebootSchedule.mode == 1) {
    // 按间隔模式：millis() / 3600000 >= intervalH
    if ((millis() / 3600000UL) >= rebootSchedule.intervalH) {
      LOG("Config", "定时重启触发（间隔模式）");
      ESP.restart();
    }
  } else {
    // 每日定时模式：依赖 NTP
    time_t now = time(nullptr);
    if (now < 100000) {
      // NTP 未同步，跳过
      static unsigned long lastWarn = 0;
      if (millis() - lastWarn >= 60000) {
        lastWarn = millis();
        LOG("Config", "定时重启：NTP未同步，跳过检查");
      }
      return;
    }

    // 从 NVS 加载上次重启的 epoch（仅首次调用时读取，避免重启后 static 变量丢失）
    static uint32_t lastRebootEpoch = 0;
    static bool lastRebootLoaded = false;
    if (!lastRebootLoaded) {
      Preferences p;
      p.begin("reboot_cfg", true);
      lastRebootEpoch = p.getUInt("rb_last_epoch", 0);
      p.end();
      lastRebootLoaded = true;
    }

    struct tm t;
    localtime_r(&now, &t);

    // 计算今天目标时刻的 epoch
    struct tm trigger = t;
    trigger.tm_hour = rebootSchedule.hour;
    trigger.tm_min  = rebootSchedule.minute;
    trigger.tm_sec  = 0;
    time_t triggerEpoch = mktime(&trigger);

    // 当前时间在目标分钟窗口内，且本次触发窗口尚未重启过
    if (now >= triggerEpoch && now < triggerEpoch + 60 && lastRebootEpoch < (uint32_t)triggerEpoch) {
      // 重启前先持久化，防止重启后重复触发
      Preferences p;
      p.begin("reboot_cfg", false);
      p.putUInt("rb_last_epoch", (uint32_t)now);
      p.end();
      LOG("Config", "定时重启触发（每日模式）");
      ESP.restart();
    }
  }
}

void resetConfig() {
  Preferences p;
  p.begin("sms_config", false);
  p.clear();
  p.end();
  p.begin("reboot_cfg", false);
  p.clear();
  p.end();

  config = Config{};
  config.webUser   = DEFAULT_WEB_USER;
  config.webPass   = DEFAULT_WEB_PASS;
  config.pushCount = 5;
  for (int i = 0; i < MAX_PUSH_CHANNELS; i++) {
    config.pushChannels[i].name = "通道" + String(i + 1);
    config.pushChannels[i].type = PUSH_TYPE_POST_JSON;
  }
  config.wifiCount = 1;
  config.wifiList[0] = WifiEntry{"", ""};

  rebootSchedule = RebootSchedule{};
  rebootSchedule.hour      = 3;
  rebootSchedule.intervalH = 24;

  LOG("Config", "配置已重置为出厂默认值");
}

bool isPushChannelValid(const PushChannel& ch) {
  if (!ch.enabled) return false;
  switch (ch.type) {
    case PUSH_TYPE_POST_JSON:
    case PUSH_TYPE_BARK:
    case PUSH_TYPE_GET:
    case PUSH_TYPE_DINGTALK:
    case PUSH_TYPE_FEISHU:
    case PUSH_TYPE_CUSTOM:
      return ch.url.length() > 0;
    case PUSH_TYPE_PUSHPLUS:
    case PUSH_TYPE_SERVERCHAN:
      return ch.key1.length() > 0;
    case PUSH_TYPE_GOTIFY:
      return ch.url.length() > 0 && ch.key1.length() > 0;
    case PUSH_TYPE_TELEGRAM:
      return ch.key1.length() > 0 && ch.key2.length() > 0;
    case PUSH_TYPE_WECHAT_WORK:
      return ch.url.length() > 0;
    case PUSH_TYPE_SMS:
      return ch.url.length() > 0;
    default:
      return false;
  }
}

bool isConfigValid() {
  for (int i = 0; i < config.pushCount; i++) {
    if (isPushChannelValid(config.pushChannels[i])) {
      return true;
    }
  }
  return false;
}
