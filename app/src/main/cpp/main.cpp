#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <cerrno>
#include <fstream>
#include <iterator>
#include <sys/types.h>

#include "quickmem.h"
#include "linenoise.h"

// Global storage for parsed arguments
struct AppConfig {
    pid_t pid = 0;
    enum Mode { MODE_UNSET, MODE_STDIN, MODE_FILE, MODE_EVAL, MODE_REPL } mode = MODE_UNSET;
    std::string script_path;      // For file mode
    std::string inline_script;    // For -e mode
};

AppConfig g_config;

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <pid> [-e \"<js>\"] [script.js]\n"
              << "\n"
              << "Arguments:\n"
              << "  <pid>           Process ID to attach to (required)\n"
              << "  -e \"<js>\"       Execute inline JavaScript expression\n"
              << "  --repl          Start interactive REPL\n"
              << "  script.js       Path to JavaScript file to execute\n"
              << "  (none)          Read JavaScript from stdin until EOF\n"
              << "\n"
              << "Examples:\n"
              << "  " << program_name << " 1234 -e \"console.log('hello')\"\n"
              << "  " << program_name << " 1234 myscript.js\n"
              << "  " << program_name << " 1234 < myscript.js\n";
}

bool parse_pid(const char* pid_str, pid_t& out_pid) {
    if (!pid_str || *pid_str == '\0') {
        return false;
    }
    
    // Validate all characters are digits
    for (const char* p = pid_str; *p; ++p) {
        if (*p < '0' || *p > '9') {
            return false;
        }
    }
    
    // Convert using stringstream to avoid overflow issues
    std::stringstream ss(pid_str);
    long long val;
    ss >> val;
    
    if (ss.fail() || val <= 0) {
        return false;
    }
    
    out_pid = static_cast<pid_t>(val);
    return true;
}

bool parse_arguments(int argc, char* argv[], AppConfig& config) {
    if (argc < 2) {
        std::cerr << "Error: PID argument is required\n\n";
        return false;
    }
    
    // Parse PID (argv[1])
    if (!parse_pid(argv[1], config.pid)) {
        std::cerr << "Error: Invalid PID '" << argv[1] << "'\n"
                  << "PID must be a positive integer\n\n";
        return false;
    }
    
    // Determine mode based on remaining arguments
    if (argc == 2) {
        // Only PID provided - read from stdin
        config.mode = AppConfig::MODE_STDIN;
    } else if (argc == 4 && strcmp(argv[2], "-e") == 0) {
        // -e flag with inline script
        config.mode = AppConfig::MODE_EVAL;
        config.inline_script = argv[3];
    } else if (argc == 3 && strcmp(argv[2], "--repl") == 0) {
        config.mode = AppConfig::MODE_REPL;
    } else if (argc == 3) {
        // File path provided
        config.mode = AppConfig::MODE_FILE;
        config.script_path = argv[2];
    } else {
        std::cerr << "Error: Invalid argument combination\n\n";
        return false;
    }
    
    return true;
}

/**
 * Read entire file into a string.
 * Returns empty string on error.
 */
std::string read_file(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << path << "\n";
        return "";
    }
    
    // Read entire file into string
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    if (file.bad()) {
        std::cerr << "Error: Failed to read file: " << path << "\n";
        return "";
    }
    
    return content;
}

/**
 * Read all available stdin into a string.
 */
std::string read_stdin() {
    std::string content((std::istreambuf_iterator<char>(std::cin)),
                        std::istreambuf_iterator<char>());
    return content;
}

/**
 * Evaluate JavaScript code and handle result/exception.
 * 
 * @param ctx QuickJS context
 * @param code JavaScript source code
 * @param filename Source name for error messages
 * @return Exit code: 0 on success, 1 on exception
 */
int eval_js(JSContext* ctx, const std::string& code, const char* filename) {
    JSValue result = JS_Eval(ctx, code.c_str(), code.length(), filename, JS_EVAL_TYPE_GLOBAL);
    
    if (JS_IsException(result)) {
        // Get exception value
        JSValue exception = JS_GetException(ctx);
        
        // Convert to string and print to stderr
        JSValue err_str = JS_ToString(ctx, exception);
        const char* err_cstr = JS_ToCString(ctx, err_str);
        
        if (err_cstr) {
            std::cerr << err_cstr << "\n";
            JS_FreeCString(ctx, err_cstr);
        }
        
        JS_FreeValue(ctx, err_str);
        JS_FreeValue(ctx, exception);
        JS_FreeValue(ctx, result);
        
        return 1;
    }
    
    // Print result only if not undefined
    if (!JS_IsUndefined(result)) {
        JSValue result_str = JS_ToString(ctx, result);
        const char* result_cstr = JS_ToCString(ctx, result_str);
        
        if (result_cstr) {
            std::cout << result_cstr << "\n";
            JS_FreeCString(ctx, result_cstr);
        }
        
        JS_FreeValue(ctx, result_str);
    }
    
    JS_FreeValue(ctx, result);
    return 0;
}

void run_repl(JSContext* ctx) {
    char* line;
    while ((line = linenoise("quickmem> ")) != nullptr) {
        if (line[0] == '\0') {
            linenoiseFree(line);
            continue;
        }
        if (strcmp(line, ".exit") == 0) {
            linenoiseFree(line);
            break;
        }
        
        linenoiseHistoryAdd(line);
        
        JSValue result = JS_Eval(ctx, line, strlen(line), "<repl>", JS_EVAL_TYPE_GLOBAL);
        
        if (JS_IsException(result)) {
            JSValue exception = JS_GetException(ctx);
            JSValue err_str = JS_ToString(ctx, exception);
            const char* err_cstr = JS_ToCString(ctx, err_str);
            if (err_cstr) { std::cerr << err_cstr << "\n"; JS_FreeCString(ctx, err_cstr); }
            JS_FreeValue(ctx, err_str);
            JS_FreeValue(ctx, exception);
        } else if (!JS_IsUndefined(result)) {
            JSValue result_str = JS_ToString(ctx, result);
            const char* result_cstr = JS_ToCString(ctx, result_str);
            if (result_cstr) { std::cout << result_cstr << "\n"; JS_FreeCString(ctx, result_cstr); }
            JS_FreeValue(ctx, result_str);
        }
        JS_FreeValue(ctx, result);
        linenoiseFree(line);
    }
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    if (!parse_arguments(argc, argv, g_config)) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Initialize QuickJS runtime
    if (quickjs_init(g_config.pid) != 0) {
        std::cerr << "Error: Failed to initialize QuickJS runtime\n";
        return 1;
    }
    
    JSContext* ctx = quickjs_ctx();
    int exit_code = 0;
    
    switch (g_config.mode) {
        case AppConfig::MODE_STDIN: {
            std::string code = read_stdin();
            if (code.empty() && std::cin.bad()) {
                std::cerr << "Error: Failed to read from stdin\n";
                exit_code = 1;
            } else {
                exit_code = eval_js(ctx, code, "<stdin>");
            }
            break;
        }
        case AppConfig::MODE_FILE: {
            std::string code = read_file(g_config.script_path.c_str());
            if (code.empty()) {
                exit_code = 1;
            } else {
                exit_code = eval_js(ctx, code, g_config.script_path.c_str());
            }
            break;
        }
        case AppConfig::MODE_EVAL:
            exit_code = eval_js(ctx, g_config.inline_script, "<cmdline>");
            break;
        case AppConfig::MODE_REPL:
            run_repl(ctx);
            break;
        default:
            std::cerr << "Error: Unhandled mode\n";
            exit_code = 1;
            break;
    }
    
    // Clean up QuickJS runtime
    quickjs_cleanup();
    
    return exit_code;
}
