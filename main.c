#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <raylib.h>

#define SOMUI_IMPLEMENTATION
#include "./somui.h"

// Because I want the da_* functions
#include "./comp.h"

#define VOCAB_ATTEMPTS_COUNT 6
#define VOCAB_WORD_LENGTH 5

// Put the words in a separate file
#include "words.c"

#define HASH_SET_CSTR_CAP 512

typedef struct {
    Da_Cstr buckets[HASH_SET_CSTR_CAP];
} Hash_Set_Cstr;

size_t hash_cstr(Cstr cstr) {
    size_t hash = 0;
    while (*cstr++ != '\0') {
        hash = hash * 13 + (size_t)*cstr;
    }
    return hash;
}

void hash_set_cstr_insert(Hash_Set_Cstr *set, Cstr cstr) {
    size_t index = hash_cstr(cstr) % HASH_SET_CSTR_CAP;
    Da_Cstr *bucket = &set->buckets[index];
    da_append(bucket, cstr);
}

bool hash_set_cstr_contains(Hash_Set_Cstr *set, Cstr cstr) {
    size_t index = hash_cstr(cstr) % HASH_SET_CSTR_CAP;
    Da_Cstr *bucket = &set->buckets[index];
    for (size_t i = 0; i < bucket->count; ++i) {
        if (strcmp(cstr, bucket->items[i]) == 0) {
            return true;
        }
    }
    return false;
}

static Hash_Set_Cstr words_set = {0};

typedef enum {
    VOCAB_BLACK = 0,
    VOCAB_GRAY,
    VOCAB_YELLOW,
    VOCAB_GREEN,
} Vocab_Color;

typedef struct {
    const char *word;
    char grid[VOCAB_ATTEMPTS_COUNT][VOCAB_WORD_LENGTH];
    Vocab_Color color_grid[VOCAB_ATTEMPTS_COUNT][VOCAB_WORD_LENGTH];
    size_t current_attempt;
    size_t cursor;
} Vocab;

int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 720, "Vocab");

    // Preparing the words set
    for (size_t i = 0; i < words_count; ++i) {
        hash_set_cstr_insert(&words_set, words[i]);
    }

    Vocab vocab = {0};
    UI_Stack ui = {0};

    // Choose random world
    {
        srand(time(NULL));
        size_t random_index = rand() % words_count;
        vocab.word = words[random_index];
    }

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
                char word[VOCAB_WORD_LENGTH + 1] = {0};
                strncpy(word, vocab.grid[vocab.current_attempt], VOCAB_WORD_LENGTH);

                bool contains = hash_set_cstr_contains(&words_set, word);
                if (contains) {
                    vocab.current_attempt += 1;
                    vocab.cursor = 0;
                }
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

                Color background_color;
                switch (vocab.color_grid[i][j]) {
                case VOCAB_BLACK: background_color = BLACK; break;
                case VOCAB_GRAY: background_color = GRAY; break;
                case VOCAB_YELLOW: background_color = YELLOW; break;
                case VOCAB_GREEN: background_color = GREEN; break;
                }

                DrawRectangle(square.x, square.y, square.w, square.h, background_color);
                DrawRectangleLines(square.x, square.y, square.w, square.h, WHITE);

                int spacing = 6;
                const char *text = TextFormat("%c", toupper(vocab.grid[i][j]));
                Vector2 text_pos = { square.x + square.w / 2, square.y + square.h / 2 };
                Vector2 text_size = MeasureTextEx(font, text, font_size, spacing);
                Vector2 origin = { text_size.x / 2, text_size.y / 2 };
                DrawTextPro(font, text, text_pos, origin, 0.0f, font_size, spacing, WHITE);
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
