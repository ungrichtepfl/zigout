const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL2/SDL.h");
});

const WINDOW_WIDTH = 800;
const WINDOW_HEIGHT = 600;
const FPS = 60;
const FRAME_TARGET_TIME_MS = 1000 / FPS;
const PROJ_SPEED: f32 = 300;
const DELTA_TIME_SEC: f32 = 1.0 / @intToFloat(f32, FPS);
const PROJ_WIDTH = 30;
const PROJ_HEIGHT = 30;

const Vector2D = struct {
    proj_x: f32,
    proj_y: f32,
};

pub fn main() !void {
    if (sdl.SDL_Init(sdl.SDL_INIT_VIDEO) != 0) {
        sdl.SDL_Log("Unable to initialize SDL: %s", sdl.SDL_GetError());
        return error.SDLInitializationFailed;
    }
    defer sdl.SDL_Quit();

    const window = sdl.SDL_CreateWindow("Zigout", 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0) orelse {
        sdl.SDL_Log("Unable to create window: %s", sdl.SDL_GetError());
        return error.SDLWindowCreationFailed;
    };

    defer sdl.SDL_DestroyWindow(window);

    const renderer = sdl.SDL_CreateRenderer(window, -1, sdl.SDL_RENDERER_ACCELERATED) orelse {
        sdl.SDL_Log("Unable to create renderer: %s", sdl.SDL_GetError());
        return error.SDLRendererCreationFailed;
    };

    defer sdl.SDL_DestroyRenderer(renderer);

    var quit = false;
    var proj_x: f32 = 0;
    var proj_y: f32 = 0;
    var proj_dx: f32 = 1;
    var proj_dy: f32 = 1;

    while (!quit) {
        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                sdl.SDL_QUIT => quit = true,
                sdl.SDL_KEYDOWN => {
                    switch (event.key.keysym.sym) {
                        'q' => quit = true,
                        else => {},
                    }
                },
                else => {},
            }
        }
        _ = sdl.SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
        _ = sdl.SDL_RenderClear(renderer);
        sdl.SDL_RenderPresent(renderer);

        const proj = sdl.SDL_Rect{
            .x = @floatToInt(i32, proj_x),
            .y = @floatToInt(i32, proj_y),
            .w = PROJ_WIDTH,
            .h = PROJ_HEIGHT,
        };

        _ = sdl.SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
        _ = sdl.SDL_RenderFillRect(renderer, &proj);
        sdl.SDL_RenderPresent(renderer);

        var proj_nx = proj_x + proj_dx * PROJ_SPEED * DELTA_TIME_SEC;
        if (proj_nx < 0 or proj_nx + PROJ_WIDTH > WINDOW_WIDTH) {
            proj_dx *= -1;
            proj_nx = proj_x + proj_dx * PROJ_SPEED * DELTA_TIME_SEC;
        }
        var proj_ny = proj_y + proj_dy * PROJ_SPEED * DELTA_TIME_SEC;
        if (proj_ny < 0 or proj_ny + PROJ_HEIGHT > WINDOW_HEIGHT) {
            proj_dy *= -1;
            proj_ny = proj_y + proj_dy * PROJ_SPEED * DELTA_TIME_SEC;
        }

        proj_x = proj_nx;
        proj_y = proj_ny;

        sdl.SDL_Delay(FRAME_TARGET_TIME_MS);
    }
}
