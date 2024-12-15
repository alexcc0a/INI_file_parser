#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <stdexcept>
#include <regex>
#include <cctype>
#include <vector>

class ini_parser {
public:
    explicit ini_parser(const std::string& filename) {
        parse_file(filename);
    }

    template<typename T>
    T get_value(const std::string& key) const {
        auto pos = key.find('.');
        if (pos == std::string::npos) {
            throw std::invalid_argument("Key must be in 'section.variable' format.");
        }

        std::string section = key.substr(0, pos);
        std::string variable = key.substr(pos + 1);

        auto section_it = data.find(section);
        if (section_it == data.end()) {
            throw std::invalid_argument("Section '" + section + "' not found.");
        }

        const auto& variables = section_it->second;
        auto var_it = variables.find(variable);
        if (var_it == variables.end()) {
            std::vector<std::string> suggestions;
            for (const auto& var : variables) {
                suggestions.push_back(var.first);
            }

            std::string message = "Variable '" + variable + "' not found in section '" + section + "'. Did you mean one of: ";
            message += join(suggestions, ", ");
            throw std::invalid_argument(message);
        }

        return convert_value<T>(var_it->second);
    }

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data;

    void parse_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::string line;
        std::string current_section;
        size_t line_number = 0;
        std::regex section_regex(R"(\s*\[(\w+)\]\s*)");
        std::regex variable_regex(R"(\s*(\w+)\s*=\s*(.*?)(?:\s*;.*)?\s*$)");
        std::regex comment_regex(R"(\s*;.*)");

        while (std::getline(file, line)) {
            ++line_number;
            std::smatch match;

            if (std::regex_match(line, match, comment_regex) || line.empty()) {
                continue;
            }

            if (std::regex_match(line, match, section_regex)) {
                current_section = match[1];
                data[current_section];
            } else if (std::regex_match(line, match, variable_regex)) {
                if (current_section.empty()) {
                    throw std::runtime_error("Variable outside of a section at line " + std::to_string(line_number));
                }
                std::string variable = match[1];
                std::string value = match[2];
                data[current_section][variable] = value;
            } else {
                throw std::runtime_error("Syntax error at line " + std::to_string(line_number));
            }
        }

        if (file.bad()) {
            throw std::runtime_error("Error reading file: " + filename);
        }
    }

    template<typename T>
    T convert_value(const std::string& value) const {
        std::istringstream iss(value);
        T result;
        if (!(iss >> result)) {
            throw std::invalid_argument("Failed to convert value: " + value);
        }
        return result;
    }

    template<>
    std::string convert_value<std::string>(const std::string& value) const {
        return value;
    }

    static std::string join(const std::vector<std::string>& elements, const std::string& delimiter) {
        std::ostringstream oss;
        for (size_t i = 0; i < elements.size(); ++i) {
            oss << elements[i];
            if (i < elements.size() - 1) {
                oss << delimiter;
            }
        }
        return oss.str();
    }
};

int main() {
    try {
        ini_parser parser("config.ini");

        int int_value = parser.get_value<int>("Section1.var1");
        std::string string_value = parser.get_value<std::string>("Section2.var2");

        std::cout << "Section1.var1: " << int_value << std::endl;
        std::cout << "Section2.var2: " << string_value << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
