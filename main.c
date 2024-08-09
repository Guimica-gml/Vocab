#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include <raylib.h>

#define SOMUI_IMPLEMENTATION
#include "./somui.h"

#define VOCAB_ATTEMPTS_COUNT 6
#define VOCAB_WORD_LENGTH 5

typedef struct {
    const char word[VOCAB_WORD_LENGTH];
    char grid[VOCAB_ATTEMPTS_COUNT][VOCAB_WORD_LENGTH];
    size_t current_attempt;
    size_t cursor;
} Vocab;

void DrawTextCenter(
    Font font,
    const char *text,
    Vector2 position,
    float rotation,
    float font_size,
    float spacing,
    Color tint)
{
    Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
    Vector2 origin = { text_size.x / 2, text_size.y / 2 };
    DrawTextPro(font, text, position, origin, rotation, font_size, spacing, tint);
}

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Vocab");

    Vocab vocab = {0};
    UI_Stack ui = {0};

    Font font = LoadFont("./resources/ComicMono.ttf");
    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        if (vocab.current_attempt < VOCAB_ATTEMPTS_COUNT) {
            // Erase letter
            if (vocab.cursor > 0 && IsKeyPressed(KEY_BACKSPACE)) {
                vocab.grid[vocab.current_attempt][vocab.cursor - 1] = '\0';
                vocab.cursor -= 1;
            }

            // Attempt word
            if (vocab.cursor >= VOCAB_WORD_LENGTH && IsKeyPressed(KEY_ENTER)) {
                vocab.current_attempt += 1;
                vocab.cursor = 0;
            }

            // Type letter
            for (int ch = GetCharPressed(); ch != 0; ch = GetCharPressed()) {
                if (vocab.cursor < VOCAB_WORD_LENGTH && isalpha(ch)) {
                    vocab.grid[vocab.current_attempt][vocab.cursor] = (char) ch;
                    vocab.cursor += 1;
                }
            }
        }

        int32_t square_size = 100;
        int32_t vocab_width = square_size * VOCAB_WORD_LENGTH;
        int32_t vocab_height = square_size * VOCAB_ATTEMPTS_COUNT;

        int32_t window_width = GetScreenWidth();
        int32_t window_height = GetScreenHeight();

        UI_Rect rect = {
            (window_width - vocab_width) / 2,
            (window_height - vocab_height) / 2,
            vocab_width,
            vocab_height,
        };

        int32_t gap = 10;
        float font_size = 48.0f;

        ui_layout_begin(&ui, rect, UI_VERT, ui_marginv(gap), gap, VOCAB_ATTEMPTS_COUNT);
        for (size_t i = 0; i < VOCAB_ATTEMPTS_COUNT; ++i) {
            UI_Rect row = ui_layout_rect(&ui);
            ui_layout_begin(&ui, row, UI_HORI, ui_marginv(0), gap, VOCAB_WORD_LENGTH);
            for (size_t j = 0; j < VOCAB_WORD_LENGTH; ++j) {
                UI_Rect square = ui_layout_rect(&ui);
                DrawRectangleLines(square.x, square.y, square.w, square.h, WHITE);

                Vector2 text_pos = { square.x + square.w / 2, square.y + square.h / 2 };
                DrawTextCenter(font, TextFormat("%c", toupper(vocab.grid[i][j])), text_pos, 0.0f, font_size, 6, WHITE);
            }
            ui_layout_end(&ui);
        }
        ui_layout_end(&ui);

        EndDrawing();
    }

    ui_stack_free(&ui);
    UnloadFont(font);
    CloseWindow();
    return 0;
}
