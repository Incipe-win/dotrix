#include "setup.hpp"
#include "core/config.hpp"
#include "ui/reporter.hpp"
#include "util/process.hpp"
#include "vendor/json.hpp"
#include "../recipes/builtin.hpp"
#include <fuibase/fuibase.hpp>
#include <iostream>
#include <fstream>
#include <set>
#include <iomanip>
#include <thread>
#include <atomic>
#include <cstdlib>

namespace dotrix {

// ============================================================
// Recipe loading — builtin from src/recipes/builtin.hpp, custom from disk
// ============================================================

static std::vector<SetupCommand::Recipe> parse_recipes_json(const fs::path& path) {
    std::vector<SetupCommand::Recipe> out;
    if (!fs::exists(path)) return out;

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return out;

    for (auto& item : j) {
        SetupCommand::Recipe r;
        r.name       = item.value("name", "");
        r.desc       = item.value("desc", "");
        r.check      = item.value("check", r.name);
        r.install    = item.value("install", "");
        r.needs_sudo = item.value("needs_sudo", false);
        if (!r.name.empty()) out.push_back(std::move(r));
    }
    return out;
}

std::vector<SetupCommand::Recipe> SetupCommand::load_recipes() {
    auto path = recipes_path();

    // First run: seed with builtin recipes
    if (!fs::exists(path)) {
        fs::create_directories(path.parent_path());

        // Migrate from old location (repo/.dotrix/recipes.json) if exists
        auto cfg = Config::load();
        auto old_path = cfg.repo / ".dotrix" / "recipes.json";
        if (fs::exists(old_path)) {
            fs::copy_file(old_path, path);
            fs::remove(old_path);
        } else {
            std::ofstream out(path);
            out << BUILTIN_RECIPES << "\n";
        }
    }

    return parse_recipes_json(path);
}

fs::path SetupCommand::recipes_path() {
    auto* h = std::getenv("HOME");
    auto dir = h ? fs::path(h) / ".config/dotrix" : fs::path();
    return dir / "recipes.json";
}

// ============================================================
// List / Install / TUI / Add / Remove
// ============================================================

int SetupCommand::run_list(const std::vector<Recipe>& all) {
    int installed = 0, missing = 0;

    std::cout << "\033[0;34mAvailable tools:\033[0m\n\n";
    for (auto& t : all) {
        auto check_cmd = std::string(t.check);
        bool has = false;
        if (check_cmd.find(' ') != std::string::npos) {
            // Shell expression (e.g. "test -d $HOME/.oh-my-zsh")
            has = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        } else {
            // Simple binary name
            has = !util::capture({"which", check_cmd}).empty();
        }
        if (has) ++installed; else ++missing;

        std::cout << "  " << (has ? "\033[0;32m✓\033[0m" : "\033[0;31m✗\033[0m")
                  << "  " << std::left << std::setw(14) << t.name
                  << t.desc;
        if (t.needs_sudo && !t.install.empty())
            std::cout << "  (needs sudo)";
        std::cout << "\n";
    }

    std::cout << "\n" << installed << " installed, " << missing << " missing";
    if (missing > 0)
        std::cout << "    dotrix setup --pick  to select interactively";
    std::cout << "\n";
    return 0;
}

int SetupCommand::run_install(const std::vector<Recipe>& all,
                               const std::vector<std::string>& filter) {
    std::set<std::string> f(filter.begin(), filter.end());
    auto* home = std::getenv("HOME");
    fs::path local_bin = home ? fs::path(home) / ".local/bin" : fs::path("/tmp");
    fs::create_directories(local_bin);

    // Load config for GITHUB_TOKEN
    auto dotrix_cfg = Config::load();
    if (!dotrix_cfg.github_token.empty() &&
        dotrix_cfg.github_token.find("__DOTRIX_REDACTED__") == std::string::npos) {
        setenv("GITHUB_TOKEN", dotrix_cfg.github_token.c_str(), 1);
    }

    // ----- Pre-scan: count tools to install -----
    int to_install = 0;
    for (auto& t : all) {
        if (!f.empty() && !f.count(t.name)) continue;
        if (t.install.empty() || t.needs_sudo) continue;
        auto check_cmd = std::string(t.check);
        bool already = false;
        if (check_cmd.find(' ') != std::string::npos)
            already = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        else
            already = !util::capture({"which", check_cmd}).empty();
        if (!already) to_install++;
    }

    int installed_now = 0, skipped_sudo = 0, attempted = 0;

    // Animated spinner frames
    static const char* spinners[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};

    for (auto& t : all) {
        if (!f.empty() && !f.count(t.name)) continue;

        // Re-check installation
        auto check_cmd = std::string(t.check);
        bool already = false;
        if (check_cmd.find(' ') != std::string::npos)
            already = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        else
            already = !util::capture({"which", check_cmd}).empty();
        if (already) continue;

        if (t.install.empty()) {
            Reporter::warn("skip: " + t.name + " — system tool, no install script");
            continue;
        }
        if (t.needs_sudo) {
            Reporter::warn("skip: " + t.name + " — needs sudo. Run manually:");
            std::cerr << "  " << t.install << "\n";
            ++skipped_sudo;
            continue;
        }

        // ---- Overall progress bar ----
        int bar_w = 20;
        int filled = to_install > 0 ? (attempted * bar_w / to_install) : 0;
        std::string bar_head;
        for (int i = 0; i < bar_w; i++)
            bar_head += (i < filled) ? "\033[0;32m█" : "\033[0;2m░";
        std::string counter = std::to_string(attempted + 1) + "/" + std::to_string(to_install);

        // Write temp script
        auto script = fs::temp_directory_path() / ("dotrix-setup-" + t.name + ".sh");
        auto log_path = (fs::temp_directory_path() / ("dotrix-log-" + t.name + ".txt")).string();
        {
            std::ofstream out(script);
            out << "#!/usr/bin/env bash\nset -ex\n" << t.install << "\n";
        }
        fs::permissions(script, fs::perms::owner_exec, fs::perm_options::add);

        // ---- Run install with animated per-tool spinner ----
        std::atomic<bool> done{false};
        std::atomic<bool> ok{false};
        std::thread worker([&]() {
            ok = util::run_silent({script.string()}, log_path);
            done = true;
        });

        int fi = 0;
        while (!done) {
            std::cerr << "\r\033[K"
                      << "\033[0;34m[" << bar_head << "\033[0;34m]\033[0m "
                      << counter << "  "
                      << "\033[0;35m" << spinners[fi % 10] << "\033[0m"
                      << " installing " << t.name << " ..." << std::flush;
            fi++;
            usleep(80000);
        }
        worker.join();

        // Clear the spinner line
        std::cerr << "\r\033[K" << std::flush;
        attempted++;

        fs::remove(script);

        if (ok) {
            fs::remove(log_path);
            filled = to_install > 0 ? (attempted * bar_w / to_install) : 0;
            std::string bar_final;
            for (int i = 0; i < bar_w; i++)
                bar_final += (i < filled) ? "\033[0;32m█" : "\033[0;2m░";
            Reporter::ok("\033[0;34m[" + bar_final + "\033[0;34m]\033[0m "
                        + std::to_string(attempted) + "/" + std::to_string(to_install)
                        + "  " + t.name + " installed");
            ++installed_now;
        } else {
            Reporter::error(t.name + " failed:");
            std::ifstream lf(log_path);
            if (lf.is_open()) {
                std::string content((std::istreambuf_iterator<char>(lf)),
                                     std::istreambuf_iterator<char>());
                // Show last 15 lines
                int lines = 0; size_t p = content.size();
                while (p > 0 && lines < 15) {
                    p = content.rfind('\n', p - 1);
                    if (p == std::string::npos) { p = 0; break; }
                    lines++;
                }
                std::cerr << content.substr(p) << "\n";
            }
        }
    }

    if (installed_now) Reporter::ok(std::to_string(installed_now) + " tool(s) installed");
    if (skipped_sudo) Reporter::warn(std::to_string(skipped_sudo) + " tool(s) needs sudo — run commands above manually");

    auto path = std::getenv("PATH");
    if (path && std::string(path).find(local_bin.string()) == std::string::npos)
        Reporter::warn("$HOME/.local/bin is NOT in PATH — add: export PATH=$PATH:$HOME/.local/bin");

    return 0;
}

int SetupCommand::run_tui(const std::vector<Recipe>& all) {
    using namespace fb;

    struct ToolItem {
        std::string name, desc;
        bool checked = false;
        bool installed = false;
        bool needs_sudo = false;
    };

    // ---- Build item list ----
    std::vector<ToolItem> items;
    items.reserve(all.size());
    for (auto& t : all) {
        auto check_cmd = std::string(t.check);
        bool has = false;
        if (check_cmd.find(' ') != std::string::npos)
            has = util::run({"sh", "-c", "\"" + check_cmd + "\" >/dev/null 2>&1"});
        else
            has = !util::capture({"which", check_cmd}).empty();
        items.push_back({t.name, t.desc, false, has, t.needs_sudo && !t.install.empty()});
    }

    if (items.empty()) { Reporter::info("no tools available"); return 0; }

    // ---- Install state ----
    enum Phase { Selecting, Installing, Results };
    Phase phase = Selecting;

    // Selection state
    int cursor = 0, scroll = 0;
    for (int i = 0; i < (int)items.size(); i++)
        if (!items[i].installed) { cursor = i; break; }

    // Install state
    struct InstallTask {
        int recipe_idx;
        std::string name;
        TaskState state = TaskState::Pending;
        std::string log_path;     // temp log file (only kept on failure)
        std::string log_content;  // loaded on demand for viewing
    };
    std::vector<InstallTask> tasks;
    int current_install = 0;
    int install_ok_count = 0;
    int install_fail_count = 0;
    int skipped_sudo = 0;

    // Results phase state
    int  result_cursor = 0;   // which failed task is selected
    bool viewing_log  = false;
    int  log_scroll   = 0;

    // Background worker
    std::thread worker;
    std::atomic<bool> worker_done{false};
    std::atomic<bool> worker_ok{false};
    bool worker_started = false;

    // Config (loaded once)
    auto dotrix_cfg = Config::load();
    if (!dotrix_cfg.github_token.empty() &&
        dotrix_cfg.github_token.find("__DOTRIX_REDACTED__") == std::string::npos) {
        setenv("GITHUB_TOKEN", dotrix_cfg.github_token.c_str(), 1);
    }

    auto* home = std::getenv("HOME");
    fs::path local_bin = home ? fs::path(home) / ".local/bin" : fs::path("/tmp");
    fs::create_directories(local_bin);

    // Helper: start install for current task
    auto start_worker = [&](int task_idx) {
        auto& task = tasks[task_idx];
        task.state = TaskState::Running;
        auto& recipe = all[task.recipe_idx];

        auto script = fs::temp_directory_path() / ("dotrix-setup-" + recipe.name + ".sh");
        task.log_path = (fs::temp_directory_path() / ("dotrix-log-" + recipe.name + ".txt")).string();
        fs::create_directories(script.parent_path());
        {
            std::ofstream out(script);
            out << "#!/usr/bin/env bash\nset -ex\n" << recipe.install << "\n";
        }
        fs::permissions(script, fs::perms::owner_exec, fs::perm_options::add);

        worker_done = false;
        worker_ok = false;
        worker_started = true;
        worker = std::thread([script, log = task.log_path, &wd = worker_done, &wo = worker_ok]() {
            wo = util::run_silent({script.string()}, log);
            wd = true;
        });
    };

    // ---- App (scoped so ~App clears screen BEFORE we print logs) ----
    {
    App app;
    app.on_render([&](Screen& g, Key key, Theme& theme) {
        int w = g.width(), h = g.height();

        // ============================================================
        // PHASE 1: SELECTION
        // ============================================================
        if (phase == Selecting) {
            auto inner = panel(g, "dotrix setup", rect_at(1, 1, w-2, h-4), theme);
            int r = inner.row, c = inner.col, iw = inner.w;
            int visible_h = std::max(5, h - 10);

            if (cursor < scroll) scroll = cursor;
            if (cursor >= scroll + visible_h) scroll = cursor - visible_h + 1;
            scroll = std::max(0, std::min(scroll, (int)items.size() - visible_h));

            switch (key) {
                case Key_up:    case Key_k: cursor--; break;
                case Key_down:  case Key_j: cursor++; break;
                case Key_page_up:          cursor -= visible_h; break;
                case Key_page_down:        cursor += visible_h; break;
                case Key_home:             cursor = 0; break;
                case Key_end:              cursor = (int)items.size() - 1; break;
                case Key_space:   items[cursor].checked = !items[cursor].checked; break;
                case Key_a: for (auto& it : items) it.checked = true; break;
                case Key_n: for (auto& it : items) if (!it.installed) it.checked = false; break;
                case Key_enter: {
                    // Build install task list from checked items
                    for (int i = 0; i < (int)items.size(); i++) {
                        if (!items[i].checked || items[i].installed) continue;
                        if (items[i].needs_sudo) { skipped_sudo++; continue; }
                        tasks.push_back({i, items[i].name, TaskState::Pending, {}, {}});
                    }
                    if (tasks.empty()) { phase = Results; break; }
                    // Skip system tools (no install script)
                    for (auto it = tasks.begin(); it != tasks.end(); ) {
                        if (all[it->recipe_idx].install.empty()) {
                            skipped_sudo++;
                            it = tasks.erase(it);
                        } else ++it;
                    }
                    if (tasks.empty()) { phase = Results; break; }
                    phase = Installing;
                    break;
                }
                case Key_escape: case Key_q: app.quit(); break;
                default: break;
            }
            cursor = std::clamp(cursor, 0, (int)items.size() - 1);

            // Render selection list
            Style row_hl; row_hl.bg = theme.surface;
            Style cursor_mark; cursor_mark.fg = theme.accent; cursor_mark.bold = true;
            Style norm_s;  norm_s.fg = theme.text;
            Style bright_s; bright_s.fg = theme.text; bright_s.bold = true;
            Style dim_s;   dim_s.fg = theme.text_dim; dim_s.dim = true;
            Style green_s; green_s.fg = theme.success; green_s.bold = true;
            Style warn_s;  warn_s.fg = theme.warning;

            for (int i = 0; i < visible_h && scroll + i < (int)items.size(); i++) {
                int idx = scroll + i;
                auto& it = items[idx];
                bool is_cur = (idx == cursor);

                if (is_cur)
                    for (int x = 0; x < iw; x++) g.put(r + i, c + x, U' ', row_hl);

                g.text(r + i, c + 1, is_cur ? "❯" : " ", cursor_mark);

                std::string box;
                Style box_s = is_cur ? green_s : norm_s;
                if (it.installed) { box = it.checked ? "[✓]" : "[ ]"; box_s = dim_s; }
                else              { box = it.checked ? "[x]" : "[ ]"; }
                g.text(r + i, c + 3, box, box_s);

                Style name_s = it.installed ? dim_s : (is_cur ? bright_s : norm_s);
                g.text(r + i, c + 7, it.name, name_s);
                g.text(r + i, c + 7 + (int)it.name.size() + 1, it.desc, dim_s);

                if (it.needs_sudo) g.text(r + i, c + iw - 10, "(sudo)", warn_s);
                if (it.installed)  g.text(r + i, c + iw - 22, "[installed]", dim_s);
            }

            if (scroll > 0) { Style s; s.fg = theme.text_dim; g.text(r, c + iw/2 - 5, "▲ more ▲", s); }
            if (scroll + visible_h < (int)items.size()) {
                Style s; s.fg = theme.text_dim;
                g.text(r + visible_h - 1, c + iw/2 - 5, "▼ more ▼", s);
            }

            key_hints(g, h - 2, 2, w - 4, {
                {"↑↓/jk", "move"}, {"Space", "toggle"},
                {"a", "all"}, {"n", "none"},
                {"Enter", "install"}, {"q", "quit"},
            }, theme);
        }

        // ============================================================
        // PHASE 2: INSTALLING
        // ============================================================
        else if (phase == Installing) {
            // Allow quitting during install
            if (key == Key_escape || key == Key_q) {
                if (worker_started && worker.joinable()) { worker.join(); worker_started = false; }
                app.quit();
                return;
            }

            // Check if worker finished
            if (worker_started && worker_done) {
                worker.join();
                worker_started = false;
                auto& task = tasks[current_install];
                task.state = worker_ok ? TaskState::Done : TaskState::Error;
                if (worker_ok) {
                    install_ok_count++;
                    fs::remove(task.log_path);  // clean log on success
                } else {
                    install_fail_count++;  // keep log on failure
                }
                // Clean up temp script
                auto script = fs::temp_directory_path() / ("dotrix-setup-" + task.name + ".sh");
                fs::remove(script);
                current_install++;
            }

            // Start next install if any
            if (!worker_started && current_install < (int)tasks.size()) {
                start_worker(current_install);
            }

            // All done?
            if (current_install >= (int)tasks.size() && !worker_started) {
                phase = Results;
            }

            // Render progress
            auto inner = panel(g, "Installing tools", rect_at(2, 2, w-4, h-6), theme);
            int r = inner.row, c = inner.col, iw = inner.w;

            // Overall progress bar
            int total = (int)tasks.size();
            int done_count = install_ok_count + install_fail_count;
            float pct = total > 0 ? (float)done_count / (float)total : 0;
            progress_bar(g, pct, r, c, std::min(40, iw - 10), theme);
            r += 2;

            // Tool list with status
            int vis_h = std::min((int)tasks.size(), h - 14);
            for (int i = 0; i < vis_h && i < (int)tasks.size(); i++) {
                auto& task = tasks[i];
                Style icon_s, name_s;
                std::string icon;
                switch (task.state) {
                    case TaskState::Done:    icon = " ✓ "; icon_s.fg = theme.success; name_s.fg = theme.text; break;
                    case TaskState::Running: icon = " ⠋ "; icon_s.fg = theme.accent; icon_s.bold = true; name_s.fg = theme.text; name_s.bold = true; break;
                    case TaskState::Error:   icon = " ✗ "; icon_s.fg = theme.error; name_s.fg = theme.error; break;
                    default:                 icon = " ○ "; icon_s.fg = theme.text_dim; name_s.fg = theme.text_dim; break;
                }
                g.text(r + i, c, icon, icon_s);
                g.text(r + i, c + 4, task.name, name_s);
            }

            // Running tool: show animated spinner
            if (worker_started && current_install < (int)tasks.size()) {
                static const char* sp[] = {"⠋","⠙","⠹","⠸","⠼","⠴","⠦","⠧","⠇","⠏"};
                int fi = (detail::tick_ms() / 80) % 10;
                Style spin_s; spin_s.fg = theme.accent; spin_s.bold = true;
                g.text(r + vis_h + 1, c, sp[fi], spin_s);
                g.text(r + vis_h + 1, c + 3, "Installing " + tasks[current_install].name + " ...",
                       Style{theme.text_dim});
            }

            key_hints(g, h - 2, 2, w - 4, {{"Installing", "please wait..."}}, theme);
        }

        // ============================================================
        // PHASE 3: RESULTS
        // ============================================================
        else if (phase == Results) {
            // --- Log viewing sub-mode ---
            if (viewing_log) {
                // Find the selected failed task
                int fail_idx = 0, task_idx = -1;
                for (int i = 0; i < (int)tasks.size(); i++) {
                    if (tasks[i].state == TaskState::Error) {
                        if (fail_idx == result_cursor) { task_idx = i; break; }
                        fail_idx++;
                    }
                }

                auto inner = panel(g, "Log: " + (task_idx >= 0 ? tasks[task_idx].name : ""),
                                   rect_at(2, 2, w-4, h-4), theme);
                int r = inner.row, c = inner.col, iw = inner.w, ih = inner.h;

                // Load log content lazily
                if (task_idx >= 0 && tasks[task_idx].log_content.empty() && !tasks[task_idx].log_path.empty()) {
                    std::ifstream lf(tasks[task_idx].log_path);
                    if (lf.is_open())
                        tasks[task_idx].log_content.assign(
                            std::istreambuf_iterator<char>(lf),
                            std::istreambuf_iterator<char>());
                }

                // Split log into lines
                std::vector<std::string_view> log_lines;
                if (task_idx >= 0) {
                    std::string_view content = tasks[task_idx].log_content;
                    size_t pos = 0;
                    while (pos < content.size()) {
                        size_t nl = content.find('\n', pos);
                        log_lines.push_back(content.substr(pos, nl - pos));
                        pos = (nl == std::string_view::npos) ? content.size() : nl + 1;
                    }
                }

                // Scroll / exit log view
                int max_scroll = std::max(0, (int)log_lines.size() - ih);
                if (key == Key_up || key == Key_k)    log_scroll--;
                if (key == Key_down || key == Key_j)  log_scroll++;
                if (key == Key_page_up)               log_scroll -= ih;
                if (key == Key_page_down)             log_scroll += ih;
                if (key == Key_escape || key == Key_q || key == Key_enter) {
                    viewing_log = false; log_scroll = 0;
                }
                log_scroll = std::clamp(log_scroll, 0, max_scroll);

                // Render log lines
                Style line_s; line_s.fg = theme.text_dim;
                Style hl_s; hl_s.fg = theme.error; hl_s.bold = true;
                for (int i = 0; i < ih && log_scroll + i < (int)log_lines.size(); i++) {
                    auto& line = log_lines[log_scroll + i];
                    // Highlight error lines
                    bool is_err = (line.find("error") != std::string_view::npos ||
                                   line.find("Error") != std::string_view::npos ||
                                   line.find("fatal") != std::string_view::npos);
                    // Truncate to fit
                    auto display = detail::ellipsize(line, iw - 2);
                    g.text(r + i, c, display, is_err ? hl_s : line_s);
                }

                key_hints(g, h - 2, 2, w - 4, {
                    {"↑↓/jk", "scroll"}, {"PgUp/Dn", "page"},
                    {"Enter/Esc/q", "back to results"},
                }, theme);
                return;
            }

            // --- Results summary ---
            auto inner = panel(g, "Install complete", rect_at(2, 2, w-4, h-6), theme);
            int r = inner.row, c = inner.col;

            Style ok_s; ok_s.fg = theme.success;
            Style err_s; err_s.fg = theme.error;
            Style warn_s; warn_s.fg = theme.warning;
            Style text_s; text_s.fg = theme.text;

            if (install_ok_count > 0 || install_fail_count > 0) {
                g.text(r, c, "✓ " + std::to_string(install_ok_count) + " installed", ok_s);
                r++;
            } else {
                g.text(r, c, "No tools were installed", Style{theme.text_dim});
                r++;
            }

            // List failed tools with cursor
            std::vector<int> failed_indices;
            for (int i = 0; i < (int)tasks.size(); i++)
                if (tasks[i].state == TaskState::Error)
                    failed_indices.push_back(i);

            if (!failed_indices.empty()) {
                g.text(r, c, "✗ " + std::to_string(install_fail_count) + " failed (select to view log):", err_s);
                r++;
                // Nav
                if (key == Key_up || key == Key_k)    result_cursor--;
                if (key == Key_down || key == Key_j)  result_cursor++;
                if (key == Key_enter || key == Key_space) { viewing_log = true; log_scroll = 0; }
                result_cursor = std::clamp(result_cursor, 0, (int)failed_indices.size() - 1);

                for (int i = 0; i < (int)failed_indices.size(); i++) {
                    int ti = failed_indices[i];
                    bool is_cur = (i == result_cursor);
                    Style mark_s; mark_s.fg = theme.accent; mark_s.bold = true;
                    g.text(r + i, c, is_cur ? "❯" : " ", mark_s);
                    g.text(r + i, c + 2, tasks[ti].name,
                           is_cur ? Style{theme.text, {}, true} : err_s);
                }
                r += (int)failed_indices.size();
            }

            if (skipped_sudo > 0) {
                g.text(r, c, "⚠ " + std::to_string(skipped_sudo) + " skipped (system/sudo tools)", warn_s);
                r++;
            }
            r += 2;
            if (!failed_indices.empty())
                g.text(r, c, "Press Enter to view log, any other key to exit", Style{theme.text_dim});
            else
                g.text(r, c, "Press any key to exit", Style{theme.text_dim});

            if (key != Key_none && key != Key_up && key != Key_down &&
                key != Key_j && key != Key_k && key != Key_enter && key != Key_space) {
                app.quit();
            }
        }
    });

    app.run();

    // Join worker if still running (user cancelled during install)
    if (worker_started && worker.joinable()) {
        worker.join();
        worker_started = false;
    }
    } // ~App runs here — screen cleared, terminal restored

    // Print final summary to stderr (details were shown in TUI)
    if (install_ok_count > 0)
        Reporter::ok(std::to_string(install_ok_count) + " tool(s) installed");
    if (install_fail_count > 0)
        Reporter::error(std::to_string(install_fail_count) + " tool(s) failed (view logs in TUI)");
    if (skipped_sudo > 0)
        Reporter::warn(std::to_string(skipped_sudo) + " tool(s) skipped (system/sudo)");

    auto* path_env = std::getenv("PATH");
    if (path_env && std::string(path_env).find(local_bin.string()) == std::string::npos)
        Reporter::warn("$HOME/.local/bin is NOT in PATH — add: export PATH=$PATH:$HOME/.local/bin");

    return (install_fail_count > 0) ? 1 : 0;
}

int SetupCommand::run_add() {
    using namespace fb;

    Recipe recipe;
    int focus = 0;   // 0=name, 1=desc, 2=check, 3=install, 4=sudo, 5=save
    bool cancelled = false;
    bool saved = false;

    App app;
    app.on_render([&](Screen& g, Key key, Theme& theme) {
        int w = g.width(), h = g.height();
        int pw = std::max(50, w - 6);  // use most of the screen width
        auto in = panel(g, "Add tool recipe", rect_at(2, (w - pw)/2, pw, h - 4), theme);
        int r = in.row, c = in.col;

        // Navigation
        if (key == Key_tab)       focus = (focus + 1) % 7;
        if (key == Key_enter && focus == 5) saved = true;
        if (key == Key_enter && focus == 6) cancelled = true;
        if (key == Key_escape || key == Key_q) cancelled = true;

        Style label_s; label_s.fg = theme.text;
        Style hint_s;  hint_s.fg = theme.text_dim; hint_s.dim = true;
        auto key_for  = [&](int n) { return focus == n ? key : Key_none; };
        auto focused  = [&](int n) { return focus == n; };

        // Name
        g.text(r, c, "Name", label_s);
        input(g, "", recipe.name, r, c+6, 25, key_for(0), theme, focused(0));
        g.text(r+1, c+6, "one word, e.g. wezterm", hint_s);
        r += 3;

        // Description
        g.text(r, c, "Description", label_s);
        input(g, "", recipe.desc, r, c+13, 40, key_for(1), theme, focused(1));
        r += 3;

        // Check
        g.text(r, c, "Check", label_s);
        input(g, "", recipe.check, r, c+7, 25, key_for(2), theme, focused(2));
        g.text(r+1, c+7, "leave empty to use name", hint_s);
        r += 3;

        // Install
        g.text(r, c, "Install", label_s);
        input(g, "", recipe.install, r, c+9, 46, key_for(3), theme, focused(3));
        g.text(r+1, c+9, "shell script to install the tool", hint_s);
        r += 3;

        // Needs sudo
        checkbox(g, "Needs sudo", recipe.needs_sudo, r, c, key_for(4), theme, focused(4));
        r += 3;

        // Buttons
        button(g, "Save",   r, c,      key_for(5), theme, focused(5));
        button(g, "Cancel", r, c + 8,  key_for(6), theme, focused(6));

        key_hints(g, h-2, 2, w-4,
                  {{"Tab", "next"}, {"Enter", "confirm"}, {"Esc/q", "quit"}}, theme);

        if (saved || cancelled) app.quit();
    });

    app.run();

    if (cancelled || recipe.name.empty()) { Reporter::info("cancelled"); return 0; }

    if (recipe.check.empty()) recipe.check = recipe.name;

    // Load existing custom recipes
    nlohmann::json j = nlohmann::json::array();
    auto path = recipes_path();
    if (fs::exists(path)) {
        std::ifstream in(path);
        j = nlohmann::json::parse(in, nullptr, false);
        if (!j.is_array()) j = nlohmann::json::array();
    }

    j.push_back({
        {"name", recipe.name},
        {"desc", recipe.desc},
        {"check", recipe.check},
        {"install", recipe.install},
        {"needs_sudo", recipe.needs_sudo}
    });

    fs::create_directories(path.parent_path());
    std::ofstream out(path);
    out << j.dump(2) << "\n";

    Reporter::ok("added: " + recipe.name);
    Reporter::info("recipes: " + path.string());
    return 0;
}

int SetupCommand::run_rm(const std::vector<std::string>& names) {
    auto path = recipes_path();
    if (!fs::exists(path) || names.empty()) {
        Reporter::warn("usage: dotrix setup --rm <name>");
        return 1;
    }

    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (!j.is_array()) return 1;

    nlohmann::json out_json = nlohmann::json::array();
    std::set<std::string> targets(names.begin(), names.end());
    int removed = 0;

    for (auto& item : j) {
        if (targets.count(item.value("name", ""))) {
            Reporter::ok("removed: " + item.value("name", ""));
            ++removed;
        } else {
            out_json.push_back(item);
        }
    }

    if (removed == 0) {
        Reporter::warn("no matching custom recipes");
        return 1;
    }

    std::ofstream ofs(path);
    ofs << out_json.dump(2) << "\n";
    Reporter::ok("removed " + std::to_string(removed) + " custom recipe(s)");
    return 0;
}

// ============================================================
// Execute
// ============================================================
int SetupCommand::execute(const std::vector<std::string>& args) {
    auto all = load_recipes();

    bool run = false;
    std::string mode;
    std::vector<std::string> filter;

    for (auto& a : args) {
        if (a == "--run")  run = true;
        else if (a == "--pick") mode = "pick";
        else if (a == "--add")  mode = "add";
        else if (a == "--rm")   mode = "rm";
        else filter.push_back(a);
    }

    if (mode == "add") return run_add();
    if (mode == "rm")  return run_rm(filter);
    if (mode == "pick") return run_tui(all);
    if (run) return run_install(all, filter);
    return run_list(all);
}

} // namespace dotrix
