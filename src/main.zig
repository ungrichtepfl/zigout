const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL2/SDL.h");
});
const math = std.math;

const WINDOW_WIDTH = 800;
const WINDOW_HEIGHT = 600;
const FPS = 60;
const FRAME_TARGET_TIME_MS = 1000 / FPS;
const PROJ_SPEED: i32 = 300;
const DELTA_TIME_SEC: f32 = 1.0 / @intToFloat(f32, FPS);
const PROJ_WIDTH = 30;
const PROJ_HEIGHT = 30;
const BAR_HEIGHT = 20;
const BAR_WIDTH = 80;
const BAR_START_X = 400;
const BAR_START_Y = 500;
const BAR_ACC_CHANGE: i32 = 1000;
const BAR_SPEED: i32 = PROJ_SPEED + 1; // bigger than PROJ_SPEED to prevent Proj sticking to Bar
const BAR_DRAG: f32 = 0.01;
const BAR_MAX_SPEED: i32 = 700;
const BAR_MAX_ACC: i32 = BAR_ACC_CHANGE * FPS;

pub const Vector2D = struct {
    x: i32,
    y: i32,
};

pub fn createRect(x: i32, y: i32, w: i32, h: i32) sdl.SDL_Rect {
    return sdl.SDL_Rect{
        .x = x,
        .y = y,
        .w = w,
        .h = h,
    };
}

pub fn vecMult(vec: *const Vector2D, scalar: f32) Vector2D {
    return Vector2D{
        .x = @floatToInt(i32, @intToFloat(f32, vec.x) * scalar),
        .y = @floatToInt(i32, @intToFloat(f32, vec.y) * scalar),
    };
}

pub fn multToVec(vec: *const Vector2D, scalar: f32) void {
    vec.x = @floatToInt(i32, @intToFloat(f32, vec.x) * scalar);
    vec.y = @floatToInt(i32, @intToFloat(f32, vec.y) * scalar);
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

pub const Projectile = struct {
    pos: Vector2D,
    vel: Vector2D,
};

pub fn updateProj(proj: *Projectile, bar: *const Bar) void {
    const n_pos = addVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
    const barRect = createBarRect(bar);
    const projRect_x = createRect(n_pos.x, proj.pos.y, PROJ_WIDTH, PROJ_HEIGHT);
    const intersects_bar_x = sdl.SDL_HasIntersection(&barRect, &projRect_x) != 0;

    if (n_pos.x < 0 or n_pos.x + PROJ_WIDTH > WINDOW_WIDTH or intersects_bar_x) {
        proj.vel.x = -proj.vel.x;
    }
    const projRect_y = createRect(proj.pos.x, n_pos.y, PROJ_WIDTH, PROJ_HEIGHT);
    const intersects_bar_y = sdl.SDL_HasIntersection(&barRect, &projRect_y) != 0;
    if (proj.pos.y < 0 or proj.pos.y + PROJ_HEIGHT > WINDOW_HEIGHT or intersects_bar_y) {
        proj.vel.y = -proj.vel.y;
    }
    addToVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
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

pub fn createProjRect(proj: *const Projectile) sdl.SDL_Rect {
    return createRect(proj.pos.x, proj.pos.y, PROJ_WIDTH, PROJ_HEIGHT);
}

pub fn drawProj(proj: *const Projectile, renderer: *sdl.SDL_Renderer) void {
    const rect = createProjRect(proj);
    _ = sdl.SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF);
    _ = sdl.SDL_RenderFillRect(renderer, &rect);
}

pub const Bar = struct {
    pos: Vector2D,
    vel: i32,
};

pub fn createBar() Bar {
    return Bar{
        .pos = Vector2D{
            .x = BAR_START_X,
            .y = BAR_START_Y,
        },
        .vel = 0,
    };
}

pub fn createBarRect(bar: *const Bar) sdl.SDL_Rect {
    return createRect(bar.pos.x, bar.pos.y, BAR_WIDTH, BAR_HEIGHT);
}

pub fn moveBarLeft(bar: *Bar) void {
    moveBar(bar, -1);
}

pub fn moveBarRight(bar: *Bar) void {
    moveBar(bar, 1);
}

fn moveBar(bar: *Bar, direction: i32) void {
    bar.vel = direction * BAR_SPEED;
}

pub fn updateBar(bar: *Bar) void {
    var nx = bar.pos.x + @floatToInt(i32, @intToFloat(f32, bar.vel) * DELTA_TIME_SEC);
    nx = math.clamp(nx, 0, WINDOW_WIDTH - BAR_WIDTH);
    bar.pos.x = nx;
}

pub fn drawBar(proj: *const Bar, renderer: *sdl.SDL_Renderer) void {
    const rect = createBarRect(proj);
    _ = sdl.SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 0xFF);
    _ = sdl.SDL_RenderFillRect(renderer, &rect);
}

pub fn drawBackground(renderer: *sdl.SDL_Renderer) void {
    _ = sdl.SDL_SetRenderDrawColor(renderer, 0x18, 0x18, 0x18, 0xFF);
    _ = sdl.SDL_RenderClear(renderer);
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

    const keyboard_state = sdl.SDL_GetKeyboardState(null);

    var quit = false;
    var pause = false;
    var proj = createProj();
    var bar = createBar();

    drawBackground(renderer);
    drawProj(&proj, renderer);
    drawBar(&bar, renderer);

    while (!quit) {
        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                sdl.SDL_QUIT => quit = true,
                sdl.SDL_KEYDOWN => {
                    switch (event.key.keysym.sym) {
                        'q' => quit = true,
                        'p' => pause = !pause,
                        else => {},
                    }
                },
                else => {},
            }
        }

        const a_pressed = keyboard_state[sdl.SDL_SCANCODE_A] != 0;
        const d_pressed = keyboard_state[sdl.SDL_SCANCODE_D] != 0;
        if (a_pressed and !d_pressed) {
            moveBarLeft(&bar);
        } else if (d_pressed and !a_pressed) {
            moveBarRight(&bar);
        } else {
            bar.vel = 0;
        }

        if (!pause) {
            updateProj(&proj, &bar);
            updateBar(&bar);
        }
        drawBackground(renderer);
        drawProj(&proj, renderer);
        drawBar(&bar, renderer);

        sdl.SDL_RenderPresent(renderer);
        sdl.SDL_Delay(FRAME_TARGET_TIME_MS);
    }
}
