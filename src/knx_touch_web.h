#pragma once

#include <ESPBase.h>

#include "knx_touch_config.h"

class KnxTouchWeb {
 public:
  using ChangeHandler = void (*)(void* context);

  bool registerRoutes(ESPBase& espBase, KnxTouchConfigStore& store,
                      ChangeHandler handler, void* context = nullptr);

 private:
  static void handlePage(WebRequest& request, WebResponse& response, void* context);
  static void handleSave(WebRequest& request, WebResponse& response, void* context);
  void render(WebResponse& response, const char* message, bool error);

  KnxTouchConfigStore* store_ = nullptr;
  ChangeHandler changeHandler_ = nullptr;
  void* changeContext_ = nullptr;
};
