#include <unistd.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <lvgl.h>
#include <cstring>
#include <iostream>

#include "dash_app.h"
#include "remote_app.h"
#include "receiver_app.h"

// MAC Addresses
static uint8_t receiver_mac[] = {0xEC, 0x64, 0xC9, 0xCC, 0xD8, 0x54};
static uint8_t dash_mac[]     = {0x3C, 0x0F, 0x02, 0xC2, 0xD4, 0xCC};
static uint8_t remote_mac[]   = {0xD0, 0xCF, 0x13, 0x32, 0x42, 0x3C};

extern "C" void remote_onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);
extern "C" void receiver_onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len);
extern "C" void dash_onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);

extern "C" void esp_now_send(const uint8_t *peer_addr, const uint8_t *data, size_t len) {
    if (memcmp(peer_addr, receiver_mac, 6) == 0) {
        receiver_onDataRecv(const_cast<uint8_t*>(remote_mac), const_cast<uint8_t*>(data), len);
    } else if (memcmp(peer_addr, remote_mac, 6) == 0) {
        remote_onDataRecv(dash_mac, data, len);
    } else if (memcmp(peer_addr, dash_mac, 6) == 0) {
        dash_onDataRecv(receiver_mac, data, len);
    }
}

extern "C" uint32_t millis() { return SDL_GetTicks(); }
extern "C" void delay(uint32_t ms) { SDL_Delay(ms); }

struct SimWindow {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    lv_disp_drv_t disp_drv;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t* buf;
    lv_disp_t* disp;
    lv_indev_drv_t indev_drv;
    lv_indev_t* indev;
    int mouse_x = 0;
    int mouse_y = 0;
    bool mouse_pressed = false;
};

SimWindow dash_win;
SimWindow remote_win;
SimWindow receiver_win;

static void my_disp_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    SimWindow* sim = (SimWindow*)disp_drv->user_data;
    SDL_Rect r;
    r.x = area->x1;
    r.y = area->y1;
    r.w = area->x2 - area->x1 + 1;
    r.h = area->y2 - area->y1 + 1;
    SDL_UpdateTexture(sim->texture, &r, color_p, r.w * sizeof(lv_color_t));
    lv_disp_flush_ready(disp_drv);
}

static void my_mouse_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    SimWindow* sim = (SimWindow*)indev_drv->user_data;
    data->point.x = sim->mouse_x;
    data->point.y = sim->mouse_y;
    data->state = sim->mouse_pressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

static void init_sim_window(SimWindow* sim, const char* title, int w, int h) {
    sim->window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, 0);
    sim->renderer = SDL_CreateRenderer(sim->window, -1, SDL_RENDERER_SOFTWARE);
    sim->texture = SDL_CreateTexture(sim->renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, w, h);
    
    sim->buf = (lv_color_t*)malloc(w * h / 10 * sizeof(lv_color_t));
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

    std::cout << "Starting Native Full Stack Simulation (Multi-Window)..." << std::endl;

    init_sim_window(&dash_win, "Dashboard", 480, 480);
    init_sim_window(&remote_win, "Remote", 170, 320);
    init_sim_window(&receiver_win, "Receiver", 400, 300);

    lv_disp_set_default(dash_win.disp);
    DashApp::init();

    lv_disp_set_default(remote_win.disp);
    RemoteApp::init();

    lv_disp_set_default(receiver_win.disp);
    ReceiverApp::init();

    while(true) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type == SDL_QUIT) exit(0);
            
            SimWindow* target = nullptr;
            if(e.type == SDL_MOUSEMOTION || e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP) {
                uint32_t win_id = e.window.windowID;
                if (win_id == SDL_GetWindowID(dash_win.window)) target = &dash_win;
                else if (win_id == SDL_GetWindowID(remote_win.window)) target = &remote_win;
                else if (win_id == SDL_GetWindowID(receiver_win.window)) target = &receiver_win;
                
                if (target) {
                    if (e.type == SDL_MOUSEMOTION) {
                        target->mouse_x = e.motion.x;
                        target->mouse_y = e.motion.y;
                    } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                        target->mouse_pressed = true;
                    } else if (e.type == SDL_MOUSEBUTTONUP) {
                        target->mouse_pressed = false;
                    }
                }
            }
        }

        lv_disp_set_default(dash_win.disp);
        DashApp::update();

        lv_disp_set_default(remote_win.disp);
        RemoteApp::update();

        lv_disp_set_default(receiver_win.disp);
        ReceiverApp::update();
        
        lv_timer_handler();

        SDL_RenderClear(dash_win.renderer);
        SDL_RenderCopy(dash_win.renderer, dash_win.texture, NULL, NULL);
        SDL_RenderPresent(dash_win.renderer);

        SDL_RenderClear(remote_win.renderer);
        SDL_RenderCopy(remote_win.renderer, remote_win.texture, NULL, NULL);
        SDL_RenderPresent(remote_win.renderer);

        SDL_RenderClear(receiver_win.renderer);
        SDL_RenderCopy(receiver_win.renderer, receiver_win.texture, NULL, NULL);
        SDL_RenderPresent(receiver_win.renderer);

        lv_tick_inc(5);
        SDL_Delay(5);
    }
    return 0;
}
