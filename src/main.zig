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
    x: f32,
    y: f32,
};

pub fn vecMult(vec: *const Vector2D, scalar: f32) Vector2D {
    return Vector2D{
        .x = vec.x * scalar,
        .y = vec.y * scalar,
    };
}

pub fn multToVec(vec: *const Vector2D, scalar: f32) void {
    vec.x = vec.x * scalar;
    vec.y = vec.y * scalar;
}

pub fn addToVec(a: *Vector2D, b: *const Vector2D) void {
    a.x += b.x;
    a.y += b.y;
}

pub fn addVec(a: *const Vector2D, b: *const Vector2D) Vector2D {
    return Vector2D{
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

const Projectile = struct {
    pos: Vector2D,
    vel: Vector2D,
};

pub fn updateProj(proj: *Projectile) void {
    addToVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
    if (proj.pos.x < 0 or proj.pos.x + PROJ_WIDTH > WINDOW_WIDTH) {
        proj.vel.x = -proj.vel.x;
    }
    if (proj.pos.y < 0 or proj.pos.y + PROJ_HEIGHT > WINDOW_HEIGHT) {
        proj.vel.y = -proj.vel.y;
    }
}

pub fn createProj() Projectile {
    return Projectile{
        .pos = Vector2D{
            .x = 0,
            .y = 0,
        },
        .vel = Vector2D{
            .x = PROJ_SPEED,
            .y = PROJ_SPEED,
        },
    };
}

pub fn drawBackground(renderer: *sdl.SDL_Renderer) void {
    _ = sdl.SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    _ = sdl.SDL_RenderClear(renderer);
}

pub fn drawProj(proj: *const Projectile, renderer: *sdl.SDL_Renderer) void {
    const rect = sdl.SDL_Rect{
        .x = @floatToInt(i32, proj.pos.x),
        .y = @floatToInt(i32, proj.pos.y),
        .w = PROJ_WIDTH,
        .h = PROJ_HEIGHT,
    };
    _ = sdl.SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    _ = sdl.SDL_RenderFillRect(renderer, &rect);
}

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
    var proj = createProj();

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

        drawBackground(renderer);

        drawProj(&proj, renderer);

        updateProj(&proj);

        sdl.SDL_RenderPresent(renderer);
        sdl.SDL_Delay(FRAME_TARGET_TIME_MS);
    }
}
