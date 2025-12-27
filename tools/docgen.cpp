/**
 * @file docgen.cpp
 * @brief Nog documentation generator.
 *
 * Extracts @nog_* tagged documentation from C++ source files
 * and generates markdown reference documentation.
 *
 * Usage: docgen <source_dir> <output_dir>
 */

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/**
 * Represents a documented parameter or field.
 */
struct ParamDoc {
    std::string name;
    std::string type;
    std::string description;
};

/**
 * Represents a documented struct.
 */
struct StructDoc {
    std::string name;
    std::string module;
    std::string description;
    std::vector<ParamDoc> fields;
    std::string example;
    std::vector<std::string> notes;
};

/**
 * Represents a documented function.
 */
struct FunctionDoc {
    std::string name;
    std::string module;
    std::string description;
    std::vector<ParamDoc> params;
    std::string returns;
    std::string returns_desc;
    std::string example;
    std::vector<std::string> notes;
    bool is_async = false;
};

/**
 * Represents a documented method.
 */
struct MethodDoc {
    std::string name;
    std::string type;  // The type this method belongs to (e.g., "str")
    std::string description;
    std::vector<ParamDoc> params;
    std::string returns;
    std::string returns_desc;
    std::string example;
    std::vector<std::string> notes;
    bool is_async = false;
};

/**
 * Represents a documented syntax element.
 */
struct SyntaxDoc {
    std::string name;
    std::string category;
    std::string description;
    std::string syntax;
    std::string example;
    std::vector<std::string> notes;
    int order = 0;  // For ordering within category
};

/**
 * Holds all extracted documentation.
 */
struct Documentation {
    std::vector<StructDoc> structs;
    std::vector<FunctionDoc> functions;
    std::vector<MethodDoc> methods;
    std::vector<SyntaxDoc> syntax;
};

/**
 * Extracts doc comment blocks from file content.
 */
std::vector<std::string> extract_doc_blocks(const std::string& content) {
    std::vector<std::string> blocks;
    std::regex block_regex(R"(/\*\*[\s\S]*?\*/)");

    auto begin = std::sregex_iterator(content.begin(), content.end(), block_regex);
    auto end = std::sregex_iterator();

    for (auto it = begin; it != end; ++it) {
        std::string block = it->str();

        if (block.find("@nog_") != std::string::npos) {
            blocks.push_back(block);
        }
    }

    return blocks;
}

/**
 * Cleans a doc block line by removing comment markers.
 */
std::string clean_line(const std::string& line) {
    std::string result = line;

    // Remove leading whitespace
    size_t start = result.find_first_not_of(" \t");

    if (start == std::string::npos) {
        return "";
    }

    result = result.substr(start);

    // Remove comment markers
    if (result.starts_with("/**")) {
        result = result.substr(3);
    } else if (result.starts_with("*/")) {
        return "";
    } else if (result.starts_with("*")) {
        result = result.substr(1);
    }

    // Remove leading space after *
    if (!result.empty() && result[0] == ' ') {
        result = result.substr(1);
    }

    return result;
}

/**
 * Parses a field/param line: "name type - description"
 */
ParamDoc parse_param(const std::string& line) {
    ParamDoc param;
    std::istringstream iss(line);
    iss >> param.name >> param.type;

    // Skip " - " and get description
    std::string rest;
    std::getline(iss, rest);

    size_t dash = rest.find(" - ");

    if (dash != std::string::npos) {
        param.description = rest.substr(dash + 3);
    } else {
        // No dash, rest is description
        size_t start = rest.find_first_not_of(" \t");

        if (start != std::string::npos) {
            param.description = rest.substr(start);
        }
    }

    return param;
}

/**
 * Parses a doc block into structured data.
 */
void parse_doc_block(const std::string& block, Documentation& docs) {
    std::istringstream iss(block);
    std::string line;

    std::string doc_type;  // struct, fn, method, syntax
    std::string name;
    std::string module;
    std::string type;
    std::string category;
    std::string description;
    std::string syntax_pattern;
    std::string returns;
    std::string returns_desc;
    std::vector<ParamDoc> params;
    std::vector<ParamDoc> fields;
    std::string example;
    std::vector<std::string> notes;
    bool is_async = false;
    int order = 0;

    bool in_example = false;
    bool in_description = false;

    while (std::getline(iss, line)) {
        std::string cleaned = clean_line(line);

        if (cleaned.empty()) {
            if (in_example) {
                example += "\n";
            }

            continue;
        }

        if (cleaned.starts_with("@nog_struct ")) {
            doc_type = "struct";
            name = cleaned.substr(12);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@nog_fn ")) {
            doc_type = "fn";
            name = cleaned.substr(8);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@nog_method ")) {
            doc_type = "method";
            name = cleaned.substr(12);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@bishop_syntax ")) {
            doc_type = "syntax";
            name = cleaned.substr(12);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@module ")) {
            module = cleaned.substr(8);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@type ")) {
            type = cleaned.substr(6);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@category ")) {
            category = cleaned.substr(10);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@description ")) {
            description = cleaned.substr(13);
            in_example = false;
            in_description = true;
        } else if (cleaned.starts_with("@syntax ")) {
            syntax_pattern = cleaned.substr(8);
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@field ")) {
            fields.push_back(parse_param(cleaned.substr(7)));
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@param ")) {
            params.push_back(parse_param(cleaned.substr(7)));
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@returns ")) {
            std::string rest = cleaned.substr(9);
            size_t dash = rest.find(" - ");

            if (dash != std::string::npos) {
                returns = rest.substr(0, dash);
                returns_desc = rest.substr(dash + 3);
            } else {
                returns = rest;
            }

            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@example")) {
            in_example = true;
            in_description = false;

            // Check if example starts on same line
            if (cleaned.length() > 8) {
                example = cleaned.substr(9);
            }
        } else if (cleaned.starts_with("@note ")) {
            notes.push_back(cleaned.substr(6));
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@async")) {
            is_async = true;
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@order ")) {
            order = std::stoi(cleaned.substr(7));
            in_example = false;
            in_description = false;
        } else if (cleaned.starts_with("@")) {
            // Unknown tag, ignore
            in_example = false;
            in_description = false;
        } else if (in_example) {
            if (!example.empty()) {
                example += "\n";
            }

            example += cleaned;
        } else if (in_description) {
            description += " " + cleaned;
        }
    }

    // Trim example
    while (!example.empty() && example.back() == '\n') {
        example.pop_back();
    }

    // Create appropriate doc object
    if (doc_type == "struct") {
        StructDoc doc;
        doc.name = name;
        doc.module = module;
        doc.description = description;
        doc.fields = fields;
        doc.example = example;
        doc.notes = notes;
        docs.structs.push_back(doc);
    } else if (doc_type == "fn") {
        FunctionDoc doc;
        doc.name = name;
        doc.module = module;
        doc.description = description;
        doc.params = params;
        doc.returns = returns;
        doc.returns_desc = returns_desc;
        doc.example = example;
        doc.notes = notes;
        doc.is_async = is_async;
        docs.functions.push_back(doc);
    } else if (doc_type == "method") {
        MethodDoc doc;
        doc.name = name;
        doc.type = type;
        doc.description = description;
        doc.params = params;
        doc.returns = returns;
        doc.returns_desc = returns_desc;
        doc.example = example;
        doc.notes = notes;
        doc.is_async = is_async;
        docs.methods.push_back(doc);
    } else if (doc_type == "syntax") {
        SyntaxDoc doc;
        doc.name = name;
        doc.category = category;
        doc.description = description;
        doc.syntax = syntax_pattern;
        doc.example = example;
        doc.notes = notes;
        doc.order = order;
        docs.syntax.push_back(doc);
    }
}

/**
 * Reads a file and returns its content.
 */
std::string read_file(const fs::path& path) {
    std::ifstream file(path);

    if (!file) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * Scans a directory for C++ files and extracts documentation.
 */
Documentation scan_directory(const fs::path& dir) {
    Documentation docs;

    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        auto ext = entry.path().extension();

        if (ext != ".cpp" && ext != ".hpp" && ext != ".h") {
            continue;
        }

        std::string content = read_file(entry.path());
        auto blocks = extract_doc_blocks(content);

        for (const auto& block : blocks) {
            parse_doc_block(block, docs);
        }
    }

    return docs;
}

/**
 * Generates markdown for syntax documentation.
 */
std::string generate_syntax_markdown(const Documentation& docs) {
    std::stringstream ss;
    ss << "# Bishop Language Reference\n\n";

    // Group by category
    std::map<std::string, std::vector<const SyntaxDoc*>> by_category;

    for (const auto& s : docs.syntax) {
        by_category[s.category].push_back(&s);
    }

    // Sort within categories by order
    for (auto& [cat, items] : by_category) {
        std::sort(items.begin(), items.end(),
                  [](const SyntaxDoc* a, const SyntaxDoc* b) { return a->order < b->order; });
    }

    // Define category order
    std::vector<std::string> category_order = {"Types",    "Variables",    "Functions", "Structs",
                                               "Methods",  "Control Flow", "Operators", "Async",
                                               "Channels", "Imports",      "Visibility"};

    for (const auto& category : category_order) {
        auto it = by_category.find(category);

        if (it == by_category.end()) {
            continue;
        }

        ss << "## " << category << "\n\n";

        for (const auto* s : it->second) {
            ss << "### " << s->name << "\n\n";

            if (!s->description.empty()) {
                ss << s->description << "\n\n";
            }

            if (!s->syntax.empty()) {
                ss << "**Syntax:**\n```\n" << s->syntax << "\n```\n\n";
            }

            if (!s->example.empty()) {
                ss << "**Example:**\n```nog\n" << s->example << "\n```\n\n";
            }

            for (const auto& note : s->notes) {
                ss << "> " << note << "\n\n";
            }
        }
    }

    return ss.str();
}

/**
 * Generates markdown for a module's documentation.
 */
std::string generate_module_markdown(const std::string& module, const Documentation& docs) {
    std::stringstream ss;

    if (module == "builtins") {
        ss << "# Built-in Functions\n\n";
    } else {
        ss << "# " << module << " Module\n\n";
        ss << "```nog\nimport " << module << ";\n```\n\n";
    }

    // Structs
    bool has_structs = false;

    for (const auto& s : docs.structs) {
        if (s.module == module) {
            if (!has_structs) {
                ss << "## Structs\n\n";
                has_structs = true;
            }

            ss << "### " << s.name << "\n\n";

            if (!s.description.empty()) {
                ss << s.description << "\n\n";
            }

            if (!s.fields.empty()) {
                ss << "**Fields:**\n\n";
                ss << "| Field | Type | Description |\n";
                ss << "|-------|------|-------------|\n";

                for (const auto& f : s.fields) {
                    ss << "| `" << f.name << "` | `" << f.type << "` | " << f.description << " |\n";
                }

                ss << "\n";
            }

            if (!s.example.empty()) {
                ss << "**Example:**\n```nog\n" << s.example << "\n```\n\n";
            }
        }
    }

    // Functions
    bool has_functions = false;

    for (const auto& f : docs.functions) {
        if (f.module == module) {
            if (!has_functions) {
                ss << "## Functions\n\n";
                has_functions = true;
            }

            ss << "### " << f.name << "\n\n";

            if (!f.description.empty()) {
                ss << f.description << "\n\n";
            }

            // Signature
            ss << "```nog\n";

            if (f.is_async) {
                ss << "async ";
            }

            ss << "fn " << f.name << "(";

            for (size_t i = 0; i < f.params.size(); i++) {
                if (i > 0) {
                    ss << ", ";
                }

                ss << f.params[i].type << " " << f.params[i].name;
            }

            ss << ")";

            if (!f.returns.empty()) {
                ss << " -> " << f.returns;
            }

            ss << "\n```\n\n";

            if (!f.params.empty()) {
                ss << "**Parameters:**\n\n";

                for (const auto& p : f.params) {
                    ss << "- `" << p.name << "` (`" << p.type << "`): " << p.description << "\n";
                }

                ss << "\n";
            }

            if (!f.returns.empty()) {
                ss << "**Returns:** `" << f.returns << "`";

                if (!f.returns_desc.empty()) {
                    ss << " - " << f.returns_desc;
                }

                ss << "\n\n";
            }

            if (!f.example.empty()) {
                ss << "**Example:**\n```nog\n" << f.example << "\n```\n\n";
            }
        }
    }

    return ss.str();
}

/**
 * Generates markdown for type methods.
 */
std::string generate_methods_markdown(const std::string& type_name, const Documentation& docs) {
    std::stringstream ss;
    ss << "# " << type_name << " Methods\n\n";

    for (const auto& m : docs.methods) {
        if (m.type == type_name) {
            ss << "## " << m.name << "\n\n";

            if (!m.description.empty()) {
                ss << m.description << "\n\n";
            }

            // Signature
            ss << "```nog\n";
            ss << "s." << m.name << "(";

            for (size_t i = 0; i < m.params.size(); i++) {
                if (i > 0) {
                    ss << ", ";
                }

                ss << m.params[i].type << " " << m.params[i].name;
            }

            ss << ")";

            if (!m.returns.empty()) {
                ss << " -> " << m.returns;
            }

            ss << "\n```\n\n";

            if (!m.params.empty()) {
                ss << "**Parameters:**\n\n";

                for (const auto& p : m.params) {
                    ss << "- `" << p.name << "` (`" << p.type << "`): " << p.description << "\n";
                }

                ss << "\n";
            }

            if (!m.returns.empty()) {
                ss << "**Returns:** `" << m.returns << "`";

                if (!m.returns_desc.empty()) {
                    ss << " - " << m.returns_desc;
                }

                ss << "\n\n";
            }

            if (!m.example.empty()) {
                ss << "**Example:**\n```nog\n" << m.example << "\n```\n\n";
            }
        }
    }

    return ss.str();
}

/**
 * Writes content to a file, creating directories as needed.
 */
void write_file(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream file(path);
    file << content;
}

/**
 * Converts a name to a lowercase filename with dots replaced by underscores.
 */
std::string to_filename(const std::string& name) {
    std::string result = name;
    std::replace(result.begin(), result.end(), '.', '_');
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

/**
 * Generates all documentation files.
 */
void generate_docs(const Documentation& docs, const fs::path& output_dir) {
    // Generate syntax.md
    if (!docs.syntax.empty()) {
        write_file(output_dir / "syntax.md", generate_syntax_markdown(docs));
        std::cout << "Generated: syntax.md\n";
    }

    // Collect modules
    std::set<std::string> modules;

    for (const auto& s : docs.structs) {
        if (!s.module.empty()) {
            modules.insert(s.module);
        }
    }

    for (const auto& f : docs.functions) {
        if (!f.module.empty()) {
            modules.insert(f.module);
        }
    }

    // Check for builtins
    bool has_builtins = false;

    for (const auto& f : docs.functions) {
        if (f.module.empty() || f.module == "builtins") {
            has_builtins = true;
            break;
        }
    }

    // Generate module docs
    for (const auto& module : modules) {
        std::string md = generate_module_markdown(module, docs);
        std::string filename = to_filename(module) + ".md";
        write_file(output_dir / "stdlib" / filename, md);
        std::cout << "Generated: stdlib/" << filename << "\n";
    }

    // Generate builtins
    if (has_builtins) {
        Documentation builtins_docs;

        for (const auto& f : docs.functions) {
            if (f.module.empty() || f.module == "builtins") {
                FunctionDoc bf = f;
                bf.module = "builtins";
                builtins_docs.functions.push_back(bf);
            }
        }

        std::string md = generate_module_markdown("builtins", builtins_docs);
        write_file(output_dir / "stdlib" / "builtins.md", md);
        std::cout << "Generated: stdlib/builtins.md\n";
    }

    // Collect method types
    std::set<std::string> method_types;

    for (const auto& m : docs.methods) {
        method_types.insert(m.type);
    }

    // Generate method docs
    for (const auto& type : method_types) {
        std::string md = generate_methods_markdown(type, docs);
        std::string filename = to_filename(type) + ".md";
        write_file(output_dir / "stdlib" / filename, md);
        std::cout << "Generated: stdlib/" << filename << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: docgen <source_dir> <output_dir>\n";
        return 1;
    }

    fs::path source_dir = argv[1];
    fs::path output_dir = argv[2];

    if (!fs::exists(source_dir)) {
        std::cerr << "Error: Source directory does not exist: " << source_dir << "\n";
        return 1;
    }

    std::cout << "Scanning " << source_dir << " for documentation...\n";
    Documentation docs = scan_directory(source_dir);

    std::cout << "Found: " << docs.structs.size() << " structs, " << docs.functions.size()
              << " functions, " << docs.methods.size() << " methods, " << docs.syntax.size()
              << " syntax elements\n";

    generate_docs(docs, output_dir);
    std::cout << "Documentation generated in " << output_dir << "\n";

    return 0;
}
