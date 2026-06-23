#include <unistd.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <cstring>
#include <iostream>
#include <lvgl.h>

#include "dash_app.h"
#include "receiver_app.h"
#include "remote_app.h"

// MAC Addresses
static uint8_t receiver_mac[] = {0xEC, 0x64, 0xC9, 0xCC, 0xD8, 0x54};
static uint8_t dash_mac[] = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};
static uint8_t remote_mac[] = {0xD0, 0xCF, 0x13, 0x32, 0x42, 0x3C};

extern "C" void remote_onDataRecv(const uint8_t *mac,
                                  const uint8_t *incomingData, int len);
namespace EspnowReceiver {
extern "C" void onDataRecv(uint8_t *mac, uint8_t *incomingData, uint8_t len);
}
extern "C" void dash_onDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                                int data_len);

// Global simulation variables accessible by other screens
bool sim_comm_loss = false;
bool sim_settings_active = false;
uint8_t sim_remote_btn_state = 0;

extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data,
                             size_t len) {
  if (sim_comm_loss) {
    // ponytail: drop all packets if comm loss is enabled to simulate
    // disconnected status
    return;
  }
  if (memcmp(peer_addr, receiver_mac, 6) == 0) {
    // ponytail: deduce sender MAC dynamically based on packet length
    const uint8_t *sender_mac = remote_mac;
    if (len == 4) { // sizeof(EscConfigPacket)
      sender_mac = dash_mac;
    } else if (len == 8) { // sizeof(ControlPacket)
      sender_mac = remote_mac;
    }
    EspnowReceiver::onDataRecv(const_cast<uint8_t *>(sender_mac),
                               const_cast<uint8_t *>(data), len);
  } else if (memcmp(peer_addr, remote_mac, 6) == 0) {
    remote_onDataRecv(dash_mac, data, len);
  } else if (memcmp(peer_addr, dash_mac, 6) == 0) {
    dash_onDataRecv(receiver_mac, data, len);
  }
}

extern "C" uint32_t millis() { return SDL_GetTicks(); }
extern "C" void delay(uint32_t ms) {
  (void)ms;
} // ponytail: no-op — SDL_Delay inside update() blocks the event loop and
  // causes "not responding"

struct SimWindow {
  SDL_Texture *texture = nullptr;
  lv_disp_drv_t disp_drv;
  lv_disp_draw_buf_t draw_buf;
  lv_color_t *buf = nullptr;
  lv_disp_t *disp = nullptr;
  lv_indev_drv_t indev_drv;
  lv_indev_t *indev = nullptr;

  int offset_x = 0;
  int offset_y = 0;
  int width = 0;
  int height = 0;

  int mouse_x = 0;
  int mouse_y = 0;
  bool mouse_pressed = false;
};

SDL_Window *main_window = nullptr;
SDL_Renderer *main_renderer = nullptr;

SimWindow dash_win;
SimWindow remote_win;
SimWindow receiver_win;

static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                          lv_color_t *color_p) {
  SimWindow *sim = (SimWindow *)disp_drv->user_data;
  SDL_Rect r;
  r.x = area->x1;
  r.y = area->y1;
  r.w = area->x2 - area->x1 + 1;
  r.h = area->y2 - area->y1 + 1;
  SDL_UpdateTexture(sim->texture, &r, color_p, r.w * sizeof(lv_color_t));
  lv_disp_flush_ready(disp_drv);
}

static void my_mouse_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  SimWindow *sim = (SimWindow *)indev_drv->user_data;
  data->point.x = sim->mouse_x;
  data->point.y = sim->mouse_y;
  data->state = sim->mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static void init_virtual_display(SimWindow *sim, int ox, int oy, int w, int h) {
  sim->offset_x = ox;
  sim->offset_y = oy;
  sim->width = w;
  sim->height = h;
  sim->texture = SDL_CreateTexture(main_renderer, SDL_PIXELFORMAT_RGB565,
                                   SDL_TEXTUREACCESS_STREAMING, w, h);

  sim->buf = (lv_color_t *)malloc(w * h / 10 * sizeof(lv_color_t));
  lv_disp_draw_buf_init(&sim->draw_buf, sim->buf, NULL, w * h / 10);

  lv_disp_drv_init(&sim->disp_drv);
  sim->disp_drv.hor_res = w;
  sim->disp_drv.ver_res = h;
  sim->disp_drv.flush_cb = my_disp_flush;
  sim->disp_drv.draw_buf = &sim->draw_buf;
  sim->disp_drv.user_data = sim;
  sim->disp = lv_disp_drv_register(&sim->disp_drv);

  lv_indev_drv_init(&sim->indev_drv);
  sim->indev_drv.type = LV_INDEV_TYPE_POINTER;
  sim->indev_drv.read_cb = my_mouse_read;
  sim->indev_drv.user_data = sim;
  sim->indev_drv.disp = sim->disp;
  sim->indev = lv_indev_drv_register(&sim->indev_drv);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  SDL_Init(SDL_INIT_VIDEO);
  lv_init();

  std::cout << "Starting Redesigned Simulator (Single-Window, 1050x600)..."
            << std::endl;

  // Single window of size 1050x600
  main_window = SDL_CreateWindow("ACCELER8 Simulator", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED, 1050, 600, 0);
  main_renderer = SDL_CreateRenderer(main_window, -1, SDL_RENDERER_SOFTWARE);

  // Initialize virtual displays inside the main window
  init_virtual_display(&remote_win, 20, 140, 170,
                       320); // Remote (170x320) at x=20, y=140
  init_virtual_display(&dash_win, 210, 60, 480,
                       480); // Dash (480x480) at x=210, y=60
  init_virtual_display(&receiver_win, 700, 10, 320,
                       580); // Controls (320x580) at x=700, y=10

  // Initialize applications
  lv_disp_set_default(dash_win.disp);
  DashApp::init();

  lv_disp_set_default(remote_win.disp);
  RemoteApp::init();

  lv_disp_set_default(receiver_win.disp);
  ReceiverApp::init();

  while (true) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT)
        exit(0);

      // Arrow keys → D-pad simulation (bits match
      // btn_up/down/left/right/confirm handlers)
      if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        bool pressed = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.sym) {
        case SDLK_UP:
          pressed ? (sim_remote_btn_state |= (1 << 0))
                  : (sim_remote_btn_state &= ~(1 << 0));
          break;
        case SDLK_DOWN:
          pressed ? (sim_remote_btn_state |= (1 << 1))
                  : (sim_remote_btn_state &= ~(1 << 1));
          break;
        case SDLK_LEFT:
          pressed ? (sim_remote_btn_state |= (1 << 2))
                  : (sim_remote_btn_state &= ~(1 << 2));
          break;
        case SDLK_RIGHT:
          pressed ? (sim_remote_btn_state |= (1 << 3))
                  : (sim_remote_btn_state &= ~(1 << 3));
          break;
        case SDLK_RETURN:
          pressed ? (sim_remote_btn_state |= (1 << 4))
                  : (sim_remote_btn_state &= ~(1 << 4));
          break;
        default:
          break;
        }
      }

      if (e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN ||
          e.type == SDL_MOUSEBUTTONUP) {
        int mx = 0, my = 0;
        if (e.type == SDL_MOUSEMOTION) {
          mx = e.motion.x;
          my = e.motion.y;
        } else {
          mx = e.button.x;
          my = e.button.y;
        }

        // Check which display it falls into and translate coordinates
        SimWindow *targets[] = {&remote_win, &dash_win, &receiver_win};
        for (SimWindow *t : targets) {
          if (mx >= t->offset_x && mx < t->offset_x + t->width &&
              my >= t->offset_y && my < t->offset_y + t->height) {

            t->mouse_x = mx - t->offset_x;
            t->mouse_y = my - t->offset_y;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
              t->mouse_pressed = true;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
              t->mouse_pressed = false;
            }
          } else {
            // Release mouse state if released outside
            if (e.type == SDL_MOUSEBUTTONUP) {
              t->mouse_pressed = false;
            }
          }
        }
      }
    }

    // Update applications
    lv_disp_set_default(dash_win.disp);
    DashApp::update();

    lv_disp_set_default(remote_win.disp);
    RemoteApp::update();

    lv_disp_set_default(receiver_win.disp);
    ReceiverApp::update();

    lv_timer_handler();

    // Render everything onto the single main window
    SDL_SetRenderDrawColor(main_renderer, 30, 30, 30, 255); // Gray background
    SDL_RenderClear(main_renderer);

    SDL_Rect remote_rect = {remote_win.offset_x, remote_win.offset_y,
                            remote_win.width, remote_win.height};
    SDL_RenderCopy(main_renderer, remote_win.texture, NULL, &remote_rect);

    SDL_Rect dash_rect = {dash_win.offset_x, dash_win.offset_y, dash_win.width,
                          dash_win.height};
    SDL_RenderCopy(main_renderer, dash_win.texture, NULL, &dash_rect);

    SDL_Rect receiver_rect = {receiver_win.offset_x, receiver_win.offset_y,
                              receiver_win.width, receiver_win.height};
    SDL_RenderCopy(main_renderer, receiver_win.texture, NULL, &receiver_rect);

    // Draw borders around the virtual displays
    SDL_SetRenderDrawColor(main_renderer, 80, 80, 80, 255);
    SDL_RenderDrawRect(main_renderer, &remote_rect);
    SDL_RenderDrawRect(main_renderer, &dash_rect);
    SDL_RenderDrawRect(main_renderer, &receiver_rect);

    SDL_RenderPresent(main_renderer);

    lv_tick_inc(16);
    SDL_Delay(16); // ponytail: ~60fps cap, keeps event loop responsive
  }
  return 0;
}
