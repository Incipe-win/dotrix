#include "select.hpp"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <csignal>
#include <cstdio>
#include <algorithm>

namespace dotrix::tui {

// ---- Terminal raw mode ----
static struct termios orig_termios;

static void raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    auto raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  // no echo, no line buffering
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void restore() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

// ---- ANSI helpers ----
static void clear()      { std::cout << "\033[2J\033[H"; }
static void cursor(int r, int c) { std::cout << "\033[" << r << ";" << c << "H"; }
static void bold()       { std::cout << "\033[1m"; }
static void dim()        { std::cout << "\033[2m"; }
static void green()      { std::cout << "\033[0;32m"; }
static void cyan()       { std::cout << "\033[0;36m"; }
static void yellow()     { std::cout << "\033[1;33m"; }
static void reset()      { std::cout << "\033[0m"; }
static void hide_cursor(){ std::cout << "\033[?25l"; }
static void show_cursor(){ std::cout << "\033[?25h"; }
static void line_clear() { std::cout << "\033[2K"; }

// ---- Ctrl+C handler (defined after restore/show_cursor) ----
static volatile sig_atomic_t g_interrupted = 0;

static void sigint_handler(int) {
    restore();
    show_cursor();
    _exit(130);
}

static int term_width() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col > 0 ? w.ws_col : 80;
}

// ---- Render ----
static void draw(const std::vector<Item>& items, int cursor_idx,
                  const std::string& title) {
    clear();
    int w = term_width();

    // Title bar
    bold(); cyan();
    std::cout << "  " << title << "\n";
    reset();

    // Divider
    dim(); std::cout << "  " << std::string(std::min(w - 4, 60), '-') << "\n"; reset();
    std::cout << "\n";

    // Items
    int visible_start = std::max(0, cursor_idx - 16); // scroll when needed
    int visible_end = std::min((int)items.size(), visible_start + 20);

    if (visible_start > 0) {
        dim(); std::cout << "  ... " << visible_start << " more above ...\n"; reset();
    }

    for (int i = visible_start; i < visible_end; ++i) {
        auto& it = items[i];
        cursor(4 + i - visible_start, 3);
        line_clear();

        bool is_cursor = (i == cursor_idx);

        if (is_cursor) std::cout << "\033[7m";  // inverted

        // Checkbox
        if (it.installed) {
            green(); std::cout << "[" << (it.checked ? "✓" : " ") << "]"; reset();
            if (is_cursor) std::cout << "\033[7m";
            dim(); std::cout << " " << it.name; reset();
        } else {
            std::cout << "[" << (it.checked ? "x" : " ") << "]";
            if (is_cursor) std::cout << "\033[7m";
            std::cout << " " << it.name;
        }

        // Description
        if (is_cursor) reset();
        std::cout << "  ";
        dim(); std::cout << it.desc; reset();

        if (it.needs_sudo) { dim(); std::cout << "  (sudo)"; reset(); }
        if (it.installed)  { dim(); std::cout << "  [installed]"; reset(); }

        if (is_cursor) reset();
    }

    if (visible_end < (int)items.size()) {
        cursor(4 + visible_end - visible_start + 1, 3);
        dim(); std::cout << "  ... " << (items.size() - visible_end) << " more below ..."; reset();
    }

    // Footer
    int footer_row = 4 + (visible_end - visible_start) + 2;
    cursor(footer_row, 3);
    dim(); std::cout << "  " << std::string(std::min(w - 4, 60), '-'); reset();
    cursor(footer_row + 1, 3);
    std::cout << "  ↑↓/jk: move   Space: toggle   Enter: install   q: quit   a/n: all/none";

    std::cout.flush();
}

// ---- Main loop ----
std::vector<int> select(std::vector<Item>& items, const std::string& title) {
    if (items.empty()) return {};

    int cursor_idx = 0;

    // Catch Ctrl+C → restore terminal before exit
    auto old_sigint = signal(SIGINT, sigint_handler);

    raw_mode();
    hide_cursor();

    // Move cursor to first non-installed item
    for (int i = 0; i < (int)items.size(); ++i) {
        if (!items[i].installed) { cursor_idx = i; break; }
    }

    while (true) {
        draw(items, cursor_idx, title);

        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) {
            if (g_interrupted) { signal(SIGINT, old_sigint); return {}; }
            continue;
        }

        if (c == 'q' || c == 'Q' || c == '\033') {
            // ESC — check for arrow key sequences
            if (c == '\033') {
                char seq[2];
                if (read(STDIN_FILENO, seq, 2) == 2) {
                    if (seq[0] == '[') {
                        if (seq[1] == 'A') cursor_idx = std::max(0, cursor_idx - 1);       // ↑
                        else if (seq[1] == 'B') cursor_idx = std::min((int)items.size()-1, cursor_idx+1); // ↓
                    }
                } else {
                    break; // bare ESC = quit
                }
            } else {
                break; // q = quit
            }
        }
        else if (c == 'j') cursor_idx = std::min((int)items.size() - 1, cursor_idx + 1);
        else if (c == 'k') cursor_idx = std::max(0, cursor_idx - 1);
        else if (c == ' ') {
            items[cursor_idx].checked = !items[cursor_idx].checked;
        }
        else if (c == 'a') {
            for (auto& it : items) it.checked = true;
        }
        else if (c == 'n') {
            for (auto& it : items) if (!it.installed) it.checked = false;
        }
        else if (c == '\n' || c == '\r') {
            break; // Enter = confirm
        }
    }

    restore();
    show_cursor();
    signal(SIGINT, old_sigint);  // restore default handler
    std::cout << "\n";

    // Return indices of checked items
    std::vector<int> out;
    for (int i = 0; i < (int)items.size(); ++i)
        if (items[i].checked && !items[i].installed)
            out.push_back(i);
    return out;
}

} // namespace dotrix::tui
