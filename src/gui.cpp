#include "gui.hpp"

#include "gfx.h"
#include "loaders/interface.hpp"
#include <cstddef>
#include <forward_list>
#include <map>
#include <memory>
#include <sys/time.h>
#include <sys/unistd.h>
#include <utility>

gTicks gfxSystemTicks() {
  timeval t;
  gettimeofday(&t, nullptr);
  return t.tv_usec;
}

gTicks gfxMillisecondsToTicks(gDelay ms) { return ms * 1000; }

std::unique_ptr<Executable>
do_gui(std::forward_list<std::unique_ptr<Executable>> &executable_list) {
  gfxInit();
  auto font = gdispOpenFont("*");
  gwinSetDefaultFont(font);
  gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);
  gdispClear(GFX_WHITE);

  std::map<int, std::unique_ptr<Executable> *> list_id_to_entry;

  GHandle button_run;
  GHandle button_exit;
  GHandle list_names;
  GHandle label_path = NULL;
  GHandle label_version = NULL;
  GHandle label_author = NULL;
  GHandle label_description = NULL;

  std::unique_ptr<char[]> path;
  std::unique_ptr<char[]> version;
  std::unique_ptr<char[]> author;
  std::unique_ptr<char[]> description;

  GWidgetInit init;
  gwinWidgetClearInit(&init);

  auto screen_width = gdispGetWidth();
  auto screen_height = gdispGetHeight();

  constexpr gCoord border = 3;
  constexpr gCoord widget_height = 25;

  init.g.width = screen_width - border * 2;
  init.g.x = border;
  init.g.y = border;
  init.g.show = gTrue;
  init.g.height = widget_height;

  init.text = "Yet Another Launcher";
  init.g.width -= widget_height + border;
  gwinLabelCreate(nullptr, &init);
  init.g.x += init.g.width + border;
  init.g.width = init.g.height;
  init.text = "X";
  button_exit = gwinButtonCreate(nullptr, &init);
  init.g.x = border;
  init.g.width = screen_width - border * 2;
  init.g.y += init.g.height + border;

  init.g.height = widget_height * 10;
  list_names = gwinListCreate(nullptr, &init, gFalse);
  gwinListSetScroll(list_names, scroll_t::scrollAlways);
  init.g.y += init.g.height + border;
  init.g.height = widget_height;

  for (auto &exe : executable_list) {
    auto id = gwinListAddItem(list_names, exe->getName().get(), gTrue);
    list_id_to_entry[id] = &exe; // only gets reordered on remove
  }

  init.text = "Run";
  button_run = gwinButtonCreate(nullptr, &init);
  init.g.y += init.g.height + border;

  auto save_init = init;
  init.g.width = 75;
  save_init.g.x += init.g.width + border;
  save_init.g.width -= save_init.g.x - init.g.x;
  init.text = "Path:";
  gwinLabelCreate(nullptr, &init);
  init.g.y += init.g.height + border;
  init.text = "Version:";
  gwinLabelCreate(nullptr, &init);
  init.g.y += init.g.height + border;
  init.text = "Author:";
  gwinLabelCreate(nullptr, &init);
  save_init.g.y = init.g.y;
  init.g.y += init.g.height + border;

  init.g.width = screen_width - border * 2;
  init.text = "Description:";
  gwinLabelCreate(nullptr, &init);
  init.g.y += init.g.height + border;
  init.g.height = screen_height - (init.g.y - border);
  // init now at description; to be used later

  GListener listener;
  geventListenerInit(&listener);
  gwinAttachListener(&listener);

  while (true) {
    auto event = geventEventWait(&listener, gDelayForever);
    switch (event->type) {
    case GEVENT_GWIN_BUTTON: {
      auto button_event = reinterpret_cast<GEventGWinButton *>(event);
      if (button_event->gwin == button_exit) {
        gfxDeinit();
        return nullptr;
      }
      if (button_event->gwin == button_run) {
        auto selected = gwinListGetSelected(list_names);
        gfxDeinit();
        if (selected == -1)
          return nullptr;
        return std::move(*list_id_to_entry[selected]);
      }
    } break;
    case GEVENT_GWIN_LIST: {
      auto list_event = reinterpret_cast<GEventGWinList *>(event);
      if (list_event->gwin != list_names)
        break;
      auto &exe = *list_id_to_entry[list_event->item];
      if (label_path)
        gwinDestroy(label_path);
      if (label_version)
        gwinDestroy(label_version);
      if (label_author)
        gwinDestroy(label_author);
      if (label_description)
        gwinDestroy(label_description);
      save_init.g.y -= (save_init.g.height + border) * 2;
      path = exe->getPath();
      save_init.text = path.get();
      gwinLabelCreate(nullptr, &save_init);
      save_init.g.y += save_init.g.height + border;
      version = exe->getVersion();
      if (version) {
        save_init.text = version.get();
        gwinLabelCreate(nullptr, &save_init);
      }
      save_init.g.y += save_init.g.height + border;
      author = exe->getAuthor();
      if (author) {
        save_init.text = author.get();
        gwinLabelCreate(nullptr, &save_init);
      }
      // we left init for this
      description = exe->getDescription();
      if (description) {
        init.text = description.get();
        gwinLabelCreate(nullptr, &init);
      }
    } break;
    }
  }
}