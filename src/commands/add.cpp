#include "add.hpp"
#include "ui/reporter.hpp"
#include "util/fs.hpp"
#include "guard/secrets.hpp"
#include <fuibase/fuibase.hpp>
#include <sstream>
#include <algorithm>

namespace dotrix {

// ---- TUI file browser ----
static std::vector<std::string> run_add_tui() {
    using namespace fb;
    auto* home = std::getenv("HOME");
    fs::path cwd = home ? fs::path(home) : fs::current_path();

    std::vector<fs::path> entries;
    std::vector<bool> selected;
    int cursor = 0, scroll = 0;
    bool done = false, cancelled = false;

    auto refresh_dir = [&]() {
        entries.clear();
        entries.push_back(cwd / "..");  // parent dir entry
        std::error_code ec;
        for (auto& e : fs::directory_iterator(cwd, ec))
            entries.push_back(e.path());
        std::sort(entries.begin() + 1, entries.end(), [](auto& a, auto& b) {
            bool ad = fs::is_directory(a), bd = fs::is_directory(b);
            if (ad != bd) return ad;
            return a.filename().string() < b.filename().string();
        });
        selected.assign(entries.size(), false);
        cursor = 0; scroll = 0;
    };
    refresh_dir();

    App app;
    app.on_render([&](Screen& g, Key key, Theme& theme) {
        int w = g.width(), h = g.height();
        auto in = panel(g, "Add files — " + cwd.string(),
                        rect_at(1, 1, w-2, h-4), theme);
        int r = in.row, c = in.col, iw = in.w, ih = in.h;

        // Scroll
        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + ih) scroll = cursor - ih + 1;
        scroll = std::max(0, std::min(scroll, (int)entries.size() - ih));

        // Keys
        switch (key) {
            case Key_up: case Key_k:    cursor--; break;
            case Key_down: case Key_j:  cursor++; break;
            case Key_page_up:           cursor -= ih; break;
            case Key_page_down:         cursor += ih; break;
            case Key_home:              cursor = 0; break;
            case Key_end:               cursor = (int)entries.size() - 1; break;
            case Key_enter:
                if (fs::is_directory(entries[cursor])) {
                    cwd = fs::canonical(entries[cursor]);
                    refresh_dir();
                } else {
                    // Toggle selection on file
                    selected[cursor] = !selected[cursor];
                    cursor++;
                }
                break;
            case Key_backspace:
                cwd = cwd.parent_path();
                refresh_dir();
                break;
            case Key_space:
                if (!fs::is_directory(entries[cursor]))
                    selected[cursor] = !selected[cursor];
                cursor++;
                break;
            case Key_tab:
                done = true;
                break;
            case Key_escape: case Key_q:
                cancelled = true; break;
            default: break;
        }
        cursor = std::clamp(cursor, 0, (int)entries.size() - 1);

        // Count selected
        int sel_count = 0;
        for (auto s : selected) if (s) sel_count++;

        // Render
        Style dir_s;  dir_s.fg = theme.primary; dir_s.bold = true;
        Style file_s; file_s.fg = theme.text_dim;
        Style sel_s;  sel_s.fg = theme.success; sel_s.bold = true;
        Style mark_s; mark_s.fg = theme.accent; mark_s.bold = true;
        Style row_hl; row_hl.bg = theme.surface;
        Style count_s; count_s.fg = theme.accent;

        for (int i = 0; i < ih && scroll + i < (int)entries.size(); i++) {
            int idx = scroll + i;
            auto& p = entries[idx];
            bool is_dir = fs::is_directory(p);
            bool is_cur = (idx == cursor);
            bool is_sel = selected[idx];
            std::string name = p.filename().string();
            if (idx == 0) name = "../";

            if (is_cur)
                for (int x = 0; x < iw; x++) g.put(r + i, c + x, U' ', row_hl);

            g.text(r + i, c + 1, is_cur ? "❯" : " ", mark_s);

            std::string marker = is_dir ? "📁 " : (is_sel ? "[✓] " : "[ ] ");
            g.text(r + i, c + 3, marker, is_sel ? sel_s : (is_dir ? dir_s : file_s));

            auto& style = is_dir ? dir_s : (is_sel ? sel_s : (is_cur ? file_s : file_s));
            g.text(r + i, c + 7, detail::ellipsize(name, iw - 10),
                   is_cur ? Style{theme.text, {}, true} : style);
        }

        // Footer
        g.text(h - 3, c, "Selected: " + std::to_string(sel_count), count_s);
        key_hints(g, h - 2, 2, w - 4, {
            {"↑↓/jk", "move"}, {"Enter", "open/toggle"},
            {"Space", "select"}, {"Tab", "confirm"},
            {"⌫", "up"}, {"q", "quit"},
        }, theme);

        if (done || cancelled) app.quit();
    });

    app.run();

    if (cancelled) return {};
    std::vector<std::string> result;
    for (int i = 1; i < (int)entries.size(); i++)  // skip ".."
        if (selected[i]) result.push_back(entries[i].string());
    return result;
}

int AddCommand::execute(const std::vector<std::string>& args) {
    // No args → launch TUI file browser
    if (args.empty()) {
        auto files = run_add_tui();
        if (files.empty()) { Reporter::info("nothing selected"); return 0; }
        return execute(files);
    }

    git_.ensure_initialized();

    Manifest added;
    int redacted_count = 0;

    for (auto& arg : args) {
        if (arg == "--force") continue;

        fs::path src = fs::absolute(fs::path(arg));

        if (!fs::exists(src)) {
            Reporter::warn("not found: " + arg);
            continue;
        }

        std::string rel;
        try {
            rel = store_.to_relative(src);
        } catch (...) {
            Reporter::warn("outside $HOME, skipping: " + arg);
            continue;
        }

        if (manifest_.contains(rel)) {
            Reporter::ok("already managed: ~/" + rel);
            continue;
        }

        // ---- Secret scan ----
        auto findings = SecretsGuard::scan_dir(src);
        if (!findings.empty()) {
            SecretsGuard::report(findings, src);

            auto dst = store_.repo_path(rel);
            int n = 0;
            if (fs::is_directory(src)) {
                fs::create_directories(dst);
                for (auto& e : fs::recursive_directory_iterator(src)) {
                    if (!e.is_directory()) {
                        auto rp = fs::relative(e.path(), src);
                        n += SecretsGuard::redact_file(e.path(), dst / rp);
                    }
                }
            } else {
                fs::create_directories(dst.parent_path());
                n = SecretsGuard::redact_file(src, dst);
            }
            redacted_count += n;
            Reporter::ok("redacted " + std::to_string(n) + " secret(s), added: ~/" + rel);
        } else {
            store_.store(rel);
            Reporter::ok("added: ~/" + rel);
        }

        added.push_back({rel});
    }

    if (added.empty()) {
        Reporter::info("nothing new to add");
        return 0;
    }

    if (redacted_count > 0)
        Reporter::warn(std::to_string(redacted_count) + " value(s) redacted — live files untouched");

    manifest_.append(added);

    std::ostringstream msg;
    msg << "dotrix: add";
    for (auto& e : added) msg << " ~/" << e.relative_path;
    git_.commit(msg.str());
    Reporter::ok("committed");

    return 0;
}

} // namespace dotrix
