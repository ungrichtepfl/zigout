const std = @import("std");
const sdl = @cImport({
    @cInclude("SDL2/SDL.h");
    @cInclude("SDL2/SDL_ttf.h");
});
const math = std.math;
var rand = std.rand.DefaultPrng.init(42);

// --- GAME CONFIG --- //

const SAVE_HIGHSCORE = true;
const HIGHSCORE_FILE_NAME = "highscore.txt";

const SCALING = 1;
const DEFAULT_WINDOW_WIDTH = 1200;
const DEFAULT_WINDOW_HEIGHT = 900;
const WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH * SCALING;
const WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT * SCALING;
const BACKGROUND_COLOR = Color{ .r = 0x18, .g = 0x18, .b = 0x18 };
const TEXT_COLOR = Color{ .r = 0xDC, .g = 0xDC, .b = 0xDC };

const FPS = 60;
const FRAME_TARGET_TIME_MS = 1000 / FPS;
const DELTA_TIME_SEC = 1.0 / @as(f32, @floatFromInt(FPS));

const PROJ_SPEED: i32 = 350;
const PROJ_WIDTH = 30;
const PROJ_HEIGHT = 30;
const PROJ_COLOR = Color{ .r = 230, .g = 230, .b = 230 };

const BAR_HEIGHT = 20;
const BAR_WIDTH = 80;
const BAR_START_X = @divTrunc(WINDOW_WIDTH, 2) - @divTrunc(BAR_WIDTH, 2);
const BAR_START_Y = 7 * @divTrunc(WINDOW_HEIGHT, 8);
const BAR_SPEED: i32 = PROJ_SPEED - 1; // smaller than PROJ_SPEED to prevent Proj sticking to Bar
const BAR_COLOR = Color{ .r = 255, .g = 46, .b = 46 };

const TARGET_X_SPACING = 10;
const TARGET_Y_SPACING = 10;
const TARGET_Y_NUMBER = 10 * SCALING;
const TARGET_X_NUMBER = 10 * SCALING;
const TARGET_WIDTH = BAR_WIDTH;
const TARGET_HEIGHT = BAR_HEIGHT;
const TARGET_SPACE_HEIGHT = TARGET_Y_SPACING * (TARGET_Y_NUMBER - 1) + TARGET_HEIGHT * TARGET_Y_NUMBER;
const TARGET_SPACE_WIDTH = TARGET_X_SPACING * (TARGET_X_NUMBER - 1) + TARGET_WIDTH * TARGET_X_NUMBER;
const TARGET_NUMBER = TARGET_Y_NUMBER * TARGET_X_NUMBER;
const TARGET_Y_PADDING = @divTrunc(WINDOW_HEIGHT, 10);
const TARGET_X_PADDING = @divTrunc(WINDOW_WIDTH - TARGET_SPACE_WIDTH, 2);
const TARGET_SCORE = 100;

const PARTICLE_NUMBER = 1000;
const PARTICLE_TO_EMIT = 30;
const PARTICLE_TO_EMIT_VARIABILITY = @divTrunc(PARTICLE_TO_EMIT, 4) * 2;
const PARTICLE_SIZE = 10;
const PARTICLE_SIZE_VARIABLILIY = PARTICLE_SIZE - 1;
const PARTICLE_SPEED = 5;
const PARTICLE_SPEED_VARIABILITY = PARTICLE_SPEED - 1;
const PARTICLE_LIFETIME_SEC = 2;
const PARTICLE_LIFETIME_SEC_VARIABILITY = 1.5;

// ------------------ //

pub const Color = struct {
    r: u8,
    g: u8,
    b: u8,
    a: u8 = 0xFF,
};

pub const Vector2D = struct {
    x: i32,
    y: i32,
};

pub fn createSdlRect(x: i32, y: i32, w: i32, h: i32) sdl.SDL_Rect {
    return .{
        .x = x,
        .y = y,
        .w = w,
        .h = h,
    };
}

pub fn vecMult(vec: *const Vector2D, scalar: f32) Vector2D {
    const x: f32 = @floatFromInt(vec.x);
    const y: f32 = @floatFromInt(vec.y);
    return .{
        .x = @intFromFloat(x * scalar),
        .y = @intFromFloat(y * scalar),
    };
}

pub fn multToVec(vec: *Vector2D, scalar: f32) void {
    const x: f32 = @floatFromInt(vec.x);
    const y: f32 = @floatFromInt(vec.y);

    vec.x = @intFromFloat(x * scalar);
    vec.y = @intFromFloat(y * scalar);
}

pub fn addToVec(a: *Vector2D, b: *const Vector2D) void {
    a.x += b.x;
    a.y += b.y;
}

pub fn addVec(a: *const Vector2D, b: *const Vector2D) Vector2D {
    return .{
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

pub const Particle = struct {
    pos: Vector2D = .{ .x = 0, .y = 0 },
    color: Color = .{ .r = 255, .g = 46, .b = 46 },
    angle: f32 = 0, // between [0,2*pi)
    size: i32 = PARTICLE_SIZE,
    speed: i32 = PARTICLE_SPEED,
    time_alive_sec: f32 = -1.0, // < 0 indicates not active
    max_time_alive_sec: f32 = PARTICLE_LIFETIME_SEC,
};

pub fn initialParticles() [PARTICLE_NUMBER]Particle {
    return [_]Particle{Particle{}} ** PARTICLE_NUMBER;
}

pub fn createParticleRect(particle: *const Particle) sdl.SDL_Rect {
    return createSdlRect(particle.pos.x, particle.pos.y, particle.size, particle.size);
}

pub const Projectile = struct {
    pos: Vector2D,
    vel: Vector2D,
};

inline fn abs(comptime T: type, a: T) T {
    return if (a >= 0) a else -a;
}

pub fn emitParticles(particles: *[PARTICLE_NUMBER]Particle, target: *const Target) void {
    var emitted: usize = 0;
    const rnd: i32 = @intFromFloat((rand.random().float(f32) - 0.5) * PARTICLE_TO_EMIT_VARIABILITY);
    const to_emit = PARTICLE_TO_EMIT + rnd;
    for (particles) |*particle| {
        if (particle.time_alive_sec < 0) {
            particle.time_alive_sec = 0;
            particle.color = target.color;
            particle.max_time_alive_sec += (rand.random().float(f32) - 0.5) * PARTICLE_LIFETIME_SEC_VARIABILITY;
            particle.speed += @intFromFloat((rand.random().float(f32) - 0.5) * PARTICLE_SPEED_VARIABILITY);
            particle.size += @intFromFloat((rand.random().float(f32) - 0.5) * PARTICLE_SIZE_VARIABLILIY);
            particle.pos.x = target.pos.x + @divTrunc(TARGET_WIDTH, 2) - @divTrunc(particle.size, 2);
            particle.pos.y = target.pos.y + @divTrunc(TARGET_HEIGHT, 2) - @divTrunc(particle.size, 2);
            particle.angle = rand.random().float(f32) * math.tau;
            emitted += 1;
            if (emitted >= to_emit) {
                break;
            }
        }
    }
}

pub fn updateProj(proj: *Projectile, targets: *[TARGET_NUMBER]Target, particles: *[PARTICLE_NUMBER]Particle, bar: *const Bar, score: *u64) void {
    const n_pos = addVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
    const barRect = createBarRect(bar);
    const projRect_x = createSdlRect(n_pos.x, proj.pos.y, PROJ_WIDTH, PROJ_HEIGHT);
    const projRect_y = createSdlRect(proj.pos.x, n_pos.y, PROJ_WIDTH, PROJ_HEIGHT);

    var intersects_target_x = false;
    var intersects_target_y = false;
    for (targets) |*target| {
        if (target.is_alive) {
            const targetRect = createTargetRect(target);
            intersects_target_x = sdl.SDL_HasIntersection(&targetRect, &projRect_x) != 0;
            intersects_target_y = sdl.SDL_HasIntersection(&targetRect, &projRect_y) != 0;
            if (intersects_target_x or intersects_target_y) {
                target.is_alive = false;
                score.* += TARGET_SCORE;
                emitParticles(particles, target);
                break;
            }
        }
    }

    const intersects_bar_x = sdl.SDL_HasIntersection(&barRect, &projRect_x) != 0;
    if (n_pos.x < 0 or n_pos.x + PROJ_WIDTH > WINDOW_WIDTH or intersects_bar_x or intersects_target_x) {
        proj.vel.x = -proj.vel.x;
    }
    const intersects_bar_y = sdl.SDL_HasIntersection(&barRect, &projRect_y) != 0;
    if (n_pos.y < 0 or n_pos.y + PROJ_HEIGHT > WINDOW_HEIGHT or intersects_bar_y or intersects_target_y) {
        proj.vel.y = -proj.vel.y;
    }
    if (intersects_bar_y) {
        if (abs(i32, bar.vel) > 0) {
            proj.vel.x = math.sign(bar.vel) * abs(i32, proj.vel.x);
        }
    }

    addToVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
}

pub fn hasLost(proj: *const Projectile) bool {
    const n_pos = addVec(&proj.pos, &vecMult(&proj.vel, DELTA_TIME_SEC));
    return n_pos.y + PROJ_WIDTH > WINDOW_HEIGHT;
}

pub fn hasWon(targets: *[TARGET_NUMBER]Target) bool {
    for (targets) |*target| {
        if (target.is_alive) {
            return false;
        }
    }
    return true;
}

pub fn initialProj() Projectile {
    return Projectile{
        .pos = .{
            .x = BAR_START_X + @divTrunc(BAR_WIDTH, 2) - @divTrunc(PROJ_WIDTH, 2),
            .y = BAR_START_Y - PROJ_HEIGHT,
        },
        .vel = .{
            .x = PROJ_SPEED,
            .y = PROJ_SPEED,
        },
    };
}

pub fn createProjRect(proj: *const Projectile) sdl.SDL_Rect {
    return createSdlRect(proj.pos.x, proj.pos.y, PROJ_WIDTH, PROJ_HEIGHT);
}

pub fn drawProj(proj: *const Projectile, renderer: *sdl.SDL_Renderer) void {
    const rect = createProjRect(proj);
    _ = sdl.SDL_SetRenderDrawColor(renderer, PROJ_COLOR.r, PROJ_COLOR.g, PROJ_COLOR.b, PROJ_COLOR.a);
    _ = sdl.SDL_RenderFillRect(renderer, &rect);
}

pub const Bar = struct {
    pos: Vector2D,
    vel: i32,
};

pub fn initialBar() Bar {
    return .{
        .pos = .{
            .x = BAR_START_X,
            .y = BAR_START_Y,
        },
        .vel = 0,
    };
}

pub fn createBarRect(bar: *const Bar) sdl.SDL_Rect {
    return createSdlRect(bar.pos.x, bar.pos.y, BAR_WIDTH, BAR_HEIGHT);
}

pub fn setBarSpeedLeft(bar: *Bar) void {
    setBarSpeedDir(bar, -1);
}

pub fn setBarSpeedRight(bar: *Bar) void {
    setBarSpeedDir(bar, 1);
}

fn setBarSpeedDir(bar: *Bar, direction: i32) void {
    bar.vel = direction * BAR_SPEED;
}

pub fn updateBar(bar: *Bar) void {
    const vel: f32 = @floatFromInt(bar.vel);
    var nx = bar.pos.x + @as(i32, @intFromFloat(vel * DELTA_TIME_SEC));
    nx = math.clamp(nx, 0, WINDOW_WIDTH - BAR_WIDTH);
    bar.pos.x = nx;
}

pub fn updateParticles(particles: *[PARTICLE_NUMBER]Particle) void {
    for (particles) |*particle| {
        if (particle.time_alive_sec >= 0) {
            particle.time_alive_sec += DELTA_TIME_SEC;
            if (particle.time_alive_sec >= particle.max_time_alive_sec) {
                particle.* = Particle{};
                continue;
            }
            particle.pos.x += @intFromFloat(@as(f32, @floatFromInt(particle.speed)) * math.cos(particle.angle));
            particle.pos.y += @intFromFloat(@as(f32, @floatFromInt(particle.speed)) * math.sin(particle.angle));
            var color = particle.color;
            const alpha = 255.0 * (1 - particle.time_alive_sec / particle.max_time_alive_sec);
            color.a = @intFromFloat(alpha);
            particle.color = color;
        }
    }
}

pub fn drawBar(proj: *const Bar, renderer: *sdl.SDL_Renderer) void {
    const rect = createBarRect(proj);
    _ = sdl.SDL_SetRenderDrawColor(renderer, BAR_COLOR.r, BAR_COLOR.g, BAR_COLOR.b, BAR_COLOR.a);
    _ = sdl.SDL_RenderFillRect(renderer, &rect);
}

pub const Target = struct {
    pos: Vector2D,
    is_alive: bool,
    color: Color,
};

const LinearColor = struct {
    r: f32,
    g: f32,
    b: f32,
    a: f32,
};

fn color_u8_to_f32(x: u8) f32 {
    return @as(f32, @floatFromInt(x)) / 255.0;
}

fn color_f32_to_u8(x: f32) u8 {
    return @intFromFloat(x * 255.0);
}

fn to_linear(x: u8) f32 {
    const f = color_u8_to_f32(x);
    const f2 = if (f <= 0.04045) f / 12.92 else math.pow(f32, (f + 0.055) / 1.055, 2.4);
    return f2;
}

fn srgb_to_linear(color: *const Color) LinearColor {
    return LinearColor{
        .r = to_linear(color.r),
        .g = to_linear(color.g),
        .b = to_linear(color.b),
        .a = color_u8_to_f32(color.a),
    };
}

fn to_srgb(x: f32) u8 {
    const f = if (x <= 0.0031308) x * 12.92 else 1.055 * math.pow(f32, x, 1.0 / 2.4) - 0.055;
    return color_f32_to_u8(f);
}

fn linear_to_srgb(color: *const LinearColor) Color {
    return .{
        .r = to_srgb(color.r),
        .g = to_srgb(color.g),
        .b = to_srgb(color.b),
        .a = color_f32_to_u8(color.a),
    };
}

fn lerp_color_gamma_corrected(color1: *const Color, color2: *const Color, t: f32) Color {
    const c1 = srgb_to_linear(color1);
    const c2 = srgb_to_linear(color2);
    const c = lerp_color(&c1, &c2, t);
    return linear_to_srgb(&c);
}

fn lerp_color(color1: *const LinearColor, color2: *const LinearColor, t: f32) LinearColor {
    const vec1 = [_]f32{ color1.r, color1.g, color1.b, color1.a };
    const vec2 = [_]f32{ color2.r, color2.g, color2.b, color2.a };
    var res = [_]f32{ 0, 0, 0, 0 };
    var i: usize = 0;
    for (&vec1) |*v1| {
        res[i] = v1.* + (vec2[i] - v1.*) * t;
        i += 1;
    }
    return .{
        .r = res[0],
        .g = res[1],
        .b = res[2],
        .a = res[3],
    };
}

pub fn initialTargets() [TARGET_NUMBER]Target {
    const dx = @divTrunc(TARGET_SPACE_WIDTH, TARGET_X_NUMBER);
    const dy = @divTrunc(TARGET_SPACE_HEIGHT, TARGET_Y_NUMBER);
    // Shift the targets to the right so that they are centered:
    const align_dx = @divTrunc(dx - TARGET_WIDTH, TARGET_X_NUMBER - 1);
    const align_dy = @divTrunc(dy - TARGET_HEIGHT, TARGET_Y_NUMBER - 1);

    var targets: [TARGET_NUMBER]Target = undefined;
    const red = Color{
        .r = 255,
        .g = 46,
        .b = 46,
        .a = 255,
    };
    const green = Color{
        .r = 46,
        .g = 255,
        .b = 46,
        .a = 255,
    };
    const blue = Color{
        .r = 46,
        .g = 46,
        .b = 255,
        .a = 255,
    };
    const level = 0.5;

    var idx: i32 = 0;
    for (&targets) |*target| {
        const idx_x = @mod(idx, TARGET_X_NUMBER);
        const idx_y = @divTrunc(idx, TARGET_X_NUMBER);
        const pos_x = TARGET_X_PADDING + (dx + align_dx) * idx_x;
        const pos_y = TARGET_Y_PADDING + (dy + align_dy) * idx_y;

        const t: f32 = @as(f32, @floatFromInt(idx_y)) / @as(f32, @floatFromInt(TARGET_Y_NUMBER));
        const target_color = if (t < level) lerp_color_gamma_corrected(&red, &green, t / level) else lerp_color_gamma_corrected(&green, &blue, (t - level) / (1 - level));
        target.* = Target{
            .pos = .{
                .x = pos_x,
                .y = pos_y,
            },
            .is_alive = true,
            .color = target_color,
        };
        idx += 1;
    }
    return targets;
}

pub fn createTargetRect(target: *const Target) sdl.SDL_Rect {
    return createSdlRect(target.pos.x, target.pos.y, TARGET_WIDTH, TARGET_HEIGHT);
}

pub fn drawTargets(targets: *const [TARGET_NUMBER]Target, renderer: *sdl.SDL_Renderer) void {
    for (targets) |*target| {
        if (target.is_alive) {
            const rect = createTargetRect(target);
            _ = sdl.SDL_SetRenderDrawColor(renderer, target.color.r, target.color.g, target.color.b, target.color.a);
            _ = sdl.SDL_RenderFillRect(renderer, &rect);
        }
    }
}

pub fn drawBackground(renderer: *sdl.SDL_Renderer) void {
    _ = sdl.SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    _ = sdl.SDL_RenderClear(renderer);
}

pub fn renderText(renderer: *sdl.SDL_Renderer, text: [*]const u8, color: *const Color, pos: *const Vector2D, font: *sdl.TTF_Font) void {
    const sdl_color = colorToSdlColor(color);
    const surface: *sdl.SDL_Surface = sdl.TTF_RenderText_Solid(font, text, sdl_color) orelse {
        std.debug.print("TTF_RenderText_Solid: {s}\n", .{sdl.TTF_GetError()});
        return;
    };
    defer sdl.SDL_FreeSurface(surface);
    renderSurface(renderer, surface, pos);
}

pub fn renderXYCenteredText(renderer: *sdl.SDL_Renderer, text: [*]const u8, color: *const Color, font: *sdl.TTF_Font) void {
    const sdl_color = colorToSdlColor(color);
    const surface: *sdl.SDL_Surface = sdl.TTF_RenderText_Solid(font, text, sdl_color) orelse {
        std.debug.print("TTF_RenderText_Solid: {s}\n", .{sdl.TTF_GetError()});
        return;
    };
    defer sdl.SDL_FreeSurface(surface);
    const pos = Vector2D{
        .x = @divTrunc(WINDOW_WIDTH - surface.w, 2),
        .y = @divTrunc(WINDOW_HEIGHT - surface.h, 2),
    };
    renderSurface(renderer, surface, &pos);
}

pub fn renderYCenteredText(renderer: *sdl.SDL_Renderer, text: [*]const u8, color: *const Color, font: *sdl.TTF_Font, x_pos: i32) void {
    const sdl_color = colorToSdlColor(color);
    const surface: *sdl.SDL_Surface = sdl.TTF_RenderText_Solid(font, text, sdl_color) orelse {
        std.debug.print("TTF_RenderText_Solid: {s}\n", .{sdl.TTF_GetError()});
        return;
    };
    defer sdl.SDL_FreeSurface(surface);
    const pos = Vector2D{
        .x = x_pos,
        .y = @divTrunc(WINDOW_HEIGHT - surface.h, 2),
    };
    renderSurface(renderer, surface, &pos);
}

pub fn renderXCenteredText(renderer: *sdl.SDL_Renderer, text: [*]const u8, color: *const Color, font: *sdl.TTF_Font, y_pos: i32) void {
    const sdl_color = colorToSdlColor(color);
    const surface: *sdl.SDL_Surface = sdl.TTF_RenderText_Solid(font, text, sdl_color) orelse {
        std.debug.print("TTF_RenderText_Solid: {s}\n", .{sdl.TTF_GetError()});
        return;
    };
    defer sdl.SDL_FreeSurface(surface);
    const pos = Vector2D{
        .x = @divTrunc(WINDOW_WIDTH - surface.w, 2),
        .y = y_pos,
    };
    renderSurface(renderer, surface, &pos);
}

fn colorToSdlColor(color: *const Color) sdl.SDL_Color {
    return .{
        .r = color.r,
        .g = color.g,
        .b = color.b,
        .a = color.a,
    };
}

pub fn renderSurface(renderer: *sdl.SDL_Renderer, surface: *sdl.SDL_Surface, pos: *const Vector2D) void {
    const texture = sdl.SDL_CreateTextureFromSurface(renderer, surface) orelse {
        std.debug.print("SDL_CreateTextureFromSurface: {s}\n", .{sdl.SDL_GetError()});
        return;
    };
    defer sdl.SDL_DestroyTexture(texture);

    const rect = createSdlRect(pos.x, pos.y, surface.w, surface.h);
    _ = sdl.SDL_RenderCopy(renderer, texture, null, &rect);
}

pub fn drawParticles(particles: *const [PARTICLE_NUMBER]Particle, renderer: *sdl.SDL_Renderer) void {
    for (particles) |*particle| {
        if (particle.time_alive_sec >= 0) {
            const rect = createParticleRect(particle);
            _ = sdl.SDL_SetRenderDrawColor(renderer, particle.color.r, particle.color.g, particle.color.b, particle.color.a);
            _ = sdl.SDL_RenderFillRect(renderer, &rect);
        }
    }
}

const TEXT_BUF_SIZE = 100;

pub fn writeScore(score: u64, highscore: u64, renderer: *sdl.SDL_Renderer, score_font: *sdl.TTF_Font) void {
    var text_buf: [TEXT_BUF_SIZE]u8 = [1]u8{0} ** TEXT_BUF_SIZE;
    const scoreText = std.fmt.bufPrint(&text_buf, "Score: {d}", .{score}) catch unreachable;
    renderText(renderer, scoreText.ptr, &TEXT_COLOR, &.{ .x = 10, .y = 10 }, score_font);
    text_buf = std.mem.zeroes(@TypeOf(text_buf));
    const highscoreText = std.fmt.bufPrint(&text_buf, "Best: {d}", .{highscore}) catch unreachable;
    renderText(renderer, highscoreText.ptr, &TEXT_COLOR, &.{ .x = 10, .y = 30 }, score_font);
}

pub fn readHighscore() !u64 {
    const file = try std.fs.cwd().openFile(
        HIGHSCORE_FILE_NAME,
        .{},
    );
    defer file.close();
    var text_buf: [TEXT_BUF_SIZE]u8 = [1]u8{0} ** TEXT_BUF_SIZE;
    const size = try file.readAll(&text_buf);
    return try std.fmt.parseInt(u64, text_buf[0..size], 10);
}

pub fn saveHighscore(highscore: u64) void {
    const file = std.fs.cwd().createFile(
        HIGHSCORE_FILE_NAME,
        .{},
    ) catch return;
    defer file.close();
    var text_buf: [TEXT_BUF_SIZE]u8 = [1]u8{0} ** TEXT_BUF_SIZE;
    const highscoreText = std.fmt.bufPrint(&text_buf, "{d}", .{highscore}) catch unreachable;
    _ = file.writeAll(highscoreText) catch return;
}

pub fn runGame() !void {
    if (sdl.SDL_Init(sdl.SDL_INIT_VIDEO) != 0) {
        sdl.SDL_Log("Unable to initialize SDL: %s", sdl.SDL_GetError());
        return error.SDLInitializationFailed;
    }
    defer sdl.SDL_Quit();

    if (sdl.TTF_Init() != 0) {
        sdl.SDL_Log("Unable to initialize SDL_ttf: %s", sdl.TTF_GetError());
        return error.SDLInitializationFailed;
    }
    defer sdl.TTF_Quit();

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

    // enable transparent mode
    if (sdl.SDL_SetRenderDrawBlendMode(renderer, sdl.SDL_BLENDMODE_BLEND) != 0) {
        sdl.SDL_Log("Unable to set blend/transparent mode: %s", sdl.SDL_GetError());
        return error.SDLBlendModeFailed;
    }

    const keyboard_state = sdl.SDL_GetKeyboardState(null);

    const game_font = sdl.TTF_OpenFont("Lato-Regular.ttf", 28) orelse {
        sdl.SDL_Log("Unable to load font: %s", sdl.TTF_GetError());
        return error.SDLFontLoadingFailed;
    };

    const score_font = sdl.TTF_OpenFont("Lato-Regular.ttf", 20) orelse {
        sdl.SDL_Log("Unable to load font: %s", sdl.TTF_GetError());
        return error.SDLFontLoadingFailed;
    };

    // ---- State of the game ---- //
    var quit = false;
    var pause = false;
    var started = false;
    var reset = false;
    var won = false;
    var lost = false;
    var score: u64 = 0;
    var highscore: u64 = 0;
    var bar = initialBar();
    var proj = initialProj();
    var targets = initialTargets();
    var particles = initialParticles();
    // --------------------------- //

    if (SAVE_HIGHSCORE) {
        highscore = readHighscore() catch 0;
    }

    drawBackground(renderer);
    drawProj(&proj, renderer);
    drawBar(&bar, renderer);
    drawTargets(&targets, renderer);

    while (!quit) {
        var event: sdl.SDL_Event = undefined;
        while (sdl.SDL_PollEvent(&event) != 0) {
            switch (event.type) {
                sdl.SDL_QUIT => quit = true,
                sdl.SDL_KEYDOWN => {
                    switch (event.key.keysym.sym) {
                        'q' => quit = true,
                        ' ' => pause = !pause,
                        'r' => reset = true,
                        else => {},
                    }
                },
                else => {},
            }
        }

        if (reset) {
            bar = initialBar();
            proj = initialProj();
            targets = initialTargets();
            particles = initialParticles();
            started = false;
            reset = false;
            pause = false;
            won = false;
            lost = false;
            score = 0;
        }

        const a_pressed = keyboard_state[sdl.SDL_SCANCODE_A] != 0;
        const d_pressed = keyboard_state[sdl.SDL_SCANCODE_D] != 0;

        if (!started and (a_pressed or d_pressed)) {
            started = true;
            proj.vel.x = if (a_pressed) -PROJ_SPEED else PROJ_SPEED;
        }

        if (!pause and started) {
            if (!won and !lost) {
                if (a_pressed and !d_pressed) {
                    setBarSpeedLeft(&bar);
                } else if (d_pressed and !a_pressed) {
                    setBarSpeedRight(&bar);
                } else {
                    bar.vel = 0;
                }
                updateBar(&bar);
                updateParticles(&particles);

                lost = hasLost(&proj); // must be before proj has been updated
                updateProj(&proj, &targets, &particles, &bar, &score);

                won = hasWon(&targets);
            } else {
                if (score > highscore) {
                    highscore = score;
                }
            }
        }

        drawBackground(renderer);
        drawProj(&proj, renderer);
        drawBar(&bar, renderer);
        drawTargets(&targets, renderer);
        drawParticles(&particles, renderer);
        writeScore(score, highscore, renderer, score_font);

        if (!started) {
            renderXYCenteredText(renderer, "Press A or D to move the bar and start the game. While playing press SPACE to pause.", &TEXT_COLOR, game_font);
            renderXCenteredText(renderer, "Press Q anytime to quit.", &TEXT_COLOR, game_font, @divTrunc(WINDOW_HEIGHT, 2) + 20 * SCALING);
        } else if (pause) {
            renderXYCenteredText(renderer, "Press SPACE to unpause or Q to quit.", &TEXT_COLOR, game_font);
        } else if (won) {
            renderXYCenteredText(renderer, "You won! Press R to restart or Q to quit.", &TEXT_COLOR, game_font);
        } else if (lost) {
            renderXYCenteredText(renderer, "You lost! Press R to restart or Q to quit.", &TEXT_COLOR, game_font);
        }

        sdl.SDL_RenderPresent(renderer);
        sdl.SDL_Delay(FRAME_TARGET_TIME_MS);
    }

    if (SAVE_HIGHSCORE) {
        saveHighscore(highscore);
    }
}

pub fn main() !void {
    try runGame();
}
