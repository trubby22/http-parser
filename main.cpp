#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <boost/algorithm/string.hpp>
#include <regex>
#include <typeinfo>
#include <cxxabi.h>
#include <memory>
#include <cstring>
#include <algorithm>
#include <cstring>

using namespace std;

constexpr int MAX_STRINGS = 15;
constexpr int MAX_LINES = 15;
constexpr int MAX_LENGTH = 300;
constexpr int MAX_HEADERS_SIZE = 50;
constexpr int MAX_BODY_SIZE = 50;
constexpr int MAX_REQUEST_SIZE = 4'000;

constexpr char request[] = "POST /submit-form HTTP/1.1\n"
    "Host: www.example.com\n"
    "Content-Type: application/x-www-form-urlencoded\n"
    "Content-Length: 29\n"
    "Connection: keep-alive\n"
    "Accept: text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8\n"
    "Accept-Language: en-US, en;q=0.5\n"
    "\n"
    "name=John+Doe&email=john.doe%40example.com";

#define REQ                                                                                                                        \
    "GET /wp-content/uploads/2010/03/hello-kitty-darth-vader-pink.jpg HTTP/1.1\r\n"                                                \
    "Host: www.kittyhell.com\r\n"                                                                                                  \
    "User-Agent: Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.6; ja-JP-mac; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3 "             \
    "Pathtraq/0.9\r\n"                                                                                                             \
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"                                                  \
    "Accept-Language: ja,en-us;q=0.7,en;q=0.3\r\n"                                                                                 \
    "Accept-Encoding: gzip,deflate\r\n"                                                                                            \
    "Accept-Charset: Shift_JIS,utf-8;q=0.7,*;q=0.7\r\n"                                                                            \
    "Keep-Alive: 115\r\n"                                                                                                          \
    "Connection: keep-alive\r\n"                                                                                                   \
    "Cookie: wp_ozh_wsa_visits=2; wp_ozh_wsa_visit_lasttime=xxxxxxxxxx; "                                                          \
    "__utma=xxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.xxxxxxxxxx.x; "                                                             \
    "__utmz=xxxxxxxxx.xxxxxxxxxx.x.x.utmccn=(referral)|utmcsr=reader.livedoor.com|utmcct=/reader/|utmcmd=referral\r\n"             \
    "\r\n"

void print_raw(const string& str) {
    for (char c : str) {
        switch (c) {
            case '\n': cout << "\\n"; break;
            case '\t': cout << "\\t"; break;
            case '\\': cout << "\\\\"; break;
            case '\"': cout << "\\\""; break;
            case '\r': cout << "\\r"; break;
            default: cout << c; break;
        }
    }
}

void print_raw(const char *str, int start_ix, int length) {
    for (int i = start_ix; i < start_ix + length; i++) {
        char c = str[i];
        switch (c) {
            case '\n': cout << "\\n"; break;
            case '\t': cout << "\\t"; break;
            case '\\': cout << "\\\\"; break;
            case '\"': cout << "\\\""; break;
            case '\r': cout << "\\r"; break;
            default: cout << c; break;
        }
    }
}

class HttpParser {
public:
    HttpParser(string text) : text(text) {
        istringstream stream(text);
        string line;
        vector<string> lines;

        int blank_ix = 0;
        int i = 0;

        while (getline(stream, line)) {
            lines.push_back(line);
            if (line == "") {
                blank_ix = i;
            }
            i++;
        }

        if (blank_ix == 0) {
            blank_ix = i;
        }

        i = 0;
        for (const auto& l : lines) {
            if (i == 0) {
                start_line = l;
            } else if (i < blank_ix) {
                headers.push_back(l);
            } else if (i > blank_ix) {
                body.push_back(l);
            }
            i++;
        }

        istringstream start_line_stream(start_line);
        string word;
        vector<string> words;

        while (start_line_stream >> word) { // The stream operator (>>) extracts words separated by whitespace
            words.push_back(word);
        }

        i = 0;

        for (const auto& w : words) {
            if (i == 0) {
                method = w;
            } else if (i == 1) {
                url = w;
            } else if (i == 2) {
                http_version = w;
            }
            i++;
        }

        i = 0;
        for (const auto& h : headers) {
            string delimiter = ": ";
            regex regex(delimiter);
            sregex_token_iterator iter(h.begin(), h.end(), regex, -1);
            sregex_token_iterator end;
            vector<string> parts(iter, end);

            int j = 0;
            for (const auto& p : parts) {
                if (j == 0) {
                    header_keys.push_back(p);
                } else if (j == 1) {
                    header_vals.push_back(p);
                } else {
                    // cout << "Something went wrong!" << endl;
                    throw runtime_error("Something went wrong!");
                }
                j++;
            }
            i++;
        }

        for (i = 0; i < header_vals.size(); i++) {
            auto h_val = header_vals[i];
            string delimiter = ", ";
            regex regex(delimiter);
            sregex_token_iterator iter(h_val.begin(), h_val.end(), regex, -1);
            sregex_token_iterator end;
            vector<string> parts(iter, end);

            vector<string> vec;
            header_value_entries.push_back(vec);

            for (int j = 0; j < parts.size(); j++) {
                auto p = parts[j];
                header_value_entries[i].push_back(p);
            }
        }

        for (i = 0; i < header_value_entries.size(); i++) {
            auto entries = header_value_entries[i];
            vector<double> vec;
            header_value_qualities.push_back(vec);
            for (int j = 0; j < entries.size(); j++) {
                auto e = entries[j];

                string delimiter = ";q=";
                regex regex(delimiter);
                sregex_token_iterator iter(e.begin(), e.end(), regex, -1);
                sregex_token_iterator end;
                vector<string> parts(iter, end);

                header_value_entries[i][j] = parts[0];
                if (parts.size() == 2) {
                    header_value_qualities[i].push_back(stod(parts[1]));
                } else {
                    header_value_qualities[i].push_back(1.0);
                }
            }
        }
    }

    void describe() {
        cout << "method" << endl;
        cout << method << endl;
        cout << "url" << endl;
        cout << url << endl;
        cout << "http_version" << endl;
        cout << http_version << endl;
        for (int i = 0; i < header_keys.size(); i++) {
            cout << "header key" << endl;
            cout << header_keys[i] << endl;
            for (int j = 0; j < header_value_entries[i].size(); j++) {
                auto hv_entry = header_value_entries[i][j];
                auto hv_quality = header_value_qualities[i][j];
                cout << "header value entry" << endl;
                cout << hv_entry << endl;
                cout << "header value quality" << endl;
                cout << hv_quality << endl;
            }
        }
        cout << "body" << endl;
        for (const auto& b : body) {
            cout << b << endl;
        }
    }
private:
    string text;
    string start_line;
    vector<string> headers;
    vector<string> body;
    string method;
    string url;
    string http_version;
    vector<string> header_keys;
    vector<string> header_vals;
    vector<vector<string>> header_value_entries;
    vector<vector<double>> header_value_qualities;
};

struct Location {
    int start_ix;
    int end_ix;
};

template <typename T>
string type_name() {
    int status = 0;
    unique_ptr<char[], void(*)(void*)> res{
        abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status),
        free
    };
    return (status == 0) ? res.get() : typeid(T).name();
}

void classic_main() {
    auto parser = HttpParser(request);
    parser.describe();
}

void parse_http_request(string text) {
    string start_line;
    vector<string> headers;
    vector<string> body;
    string method;
    string url;
    string http_version;
    vector<string> header_keys;
    vector<string> header_vals;
    vector<vector<string>> header_value_entries;
    vector<vector<double>> header_value_qualities;

    istringstream stream(text);
    string line;
    vector<string> lines;

    int blank_ix = 0;
    int i = 0;

    while (getline(stream, line)) {
        lines.push_back(line);
        if (line == "") {
            blank_ix = i;
        }
        i++;
    }

    if (blank_ix == 0) {
        blank_ix = i;
    }

    i = 0;
    for (const auto& l : lines) {
        if (i == 0) {
            start_line = l;
        } else if (i < blank_ix) {
            headers.push_back(l);
        } else if (i > blank_ix) {
            body.push_back(l);
        }
        i++;
    }

    istringstream start_line_stream(start_line);
    string word;
    vector<string> words;

    while (start_line_stream >> word) { // The stream operator (>>) extracts words separated by whitespace
        words.push_back(word);
    }

    i = 0;

    for (const auto& w : words) {
        if (i == 0) {
            method = w;
        } else if (i == 1) {
            url = w;
        } else if (i == 2) {
            http_version = w;
        }
        i++;
    }

    i = 0;
    for (const auto& h : headers) {
        string delimiter = ": ";
        regex regex(delimiter);
        sregex_token_iterator iter(h.begin(), h.end(), regex, -1);
        sregex_token_iterator end;
        vector<string> parts(iter, end);

        int j = 0;
        for (const auto& p : parts) {
            if (j == 0) {
                header_keys.push_back(p);
            } else if (j == 1) {
                header_vals.push_back(p);
            } else {
                // cout << "Something went wrong!" << endl;
                throw runtime_error("Something went wrong!");
            }
            j++;
        }
        i++;
    }

    for (i = 0; i < header_vals.size(); i++) {
        auto h_val = header_vals[i];
        string delimiter = ", ";
        regex regex(delimiter);
        sregex_token_iterator iter(h_val.begin(), h_val.end(), regex, -1);
        sregex_token_iterator end;
        vector<string> parts(iter, end);

        vector<string> vec;
        header_value_entries.push_back(vec);

        for (int j = 0; j < parts.size(); j++) {
            auto p = parts[j];
            header_value_entries[i].push_back(p);
        }
    }

    for (i = 0; i < header_value_entries.size(); i++) {
        auto entries = header_value_entries[i];
        vector<double> vec;
        header_value_qualities.push_back(vec);
        for (int j = 0; j < entries.size(); j++) {
            auto e = entries[j];

            string delimiter = ";q=";
            regex regex(delimiter);
            sregex_token_iterator iter(e.begin(), e.end(), regex, -1);
            sregex_token_iterator end;
            vector<string> parts(iter, end);

            header_value_entries[i][j] = parts[0];
            if (parts.size() == 2) {
                header_value_qualities[i].push_back(stod(parts[1]));
            } else {
                header_value_qualities[i].push_back(1.0);
            }
        }
    }
}

void parse_http_request_simple(string text) {
    string start_line;
    vector<string> headers;
    vector<string> body;

    istringstream stream(text);
    string line;
    vector<string> lines;

    int blank_ix = 0;
    int i = 0;

    while (getline(stream, line)) {
        lines.push_back(line);
        if (line == "") {
            blank_ix = i;
        }
        i++;
    }

    if (blank_ix == 0) {
        blank_ix = i;
    }

    i = 0;
    for (const auto& l : lines) {
        if (i == 0) {
            start_line = l;
        } else if (i < blank_ix) {
            headers.push_back(l);
        } else if (i > blank_ix) {
            body.push_back(l);
        }
        i++;
    }
}

void parse_http_request_very_simple(string text) {
    istringstream stream(text);
    string line;
    vector<string> lines;

    int blank_ix = 0;
    int i = 0;

    while (getline(stream, line)) {
        lines.push_back(line);
    }
}

int split_lines(const char* text, int newlines[MAX_LINES]) {
    int newline_count = 0;
    int i = 0;
    const char* t = text;

    while (*(t + i) != '\0' && newline_count < MAX_LINES) {
        if (*(t + i) == '\n') {
            newlines[newline_count] = i;
            newline_count++;
        }
        i++;
    }

    return newline_count;
}

void parse_http_request_stack_copy(const char *text) {
    char lines[MAX_STRINGS][MAX_LENGTH] = {0};
    // split_lines(text, lines);

    int blank_ix = MAX_LENGTH;
    for (int i = 0; i < MAX_LINES; ++i) {
        if (lines[i][0] == '\0') {
            blank_ix = min(blank_ix, i);
        }
    }

    cout << "blank_ix " << blank_ix << endl;

    int snd_blank_ix = MAX_LENGTH;
    for (int i = 0; i < MAX_LINES; ++i) {
        if (lines[i][0] == '\0' && i > blank_ix) {
            snd_blank_ix = min(snd_blank_ix, i);
        }
    }

    cout << "snd_blank_ix " << snd_blank_ix << endl;

    // Print the result to verify
    for (int i = 0; i < MAX_LINES; ++i) {
        cout << "Line " << i + 1 << ": " << lines[i] << endl;
    }

    char start_line[MAX_LENGTH];

    strcpy(start_line, lines[0]);

    int headers_size = blank_ix - 1;
    char headers[MAX_HEADERS_SIZE][MAX_LENGTH] = {0};
    for (int i = 0; i < headers_size; i++) {
        strcpy(headers[i], lines[1 + i]);
    }

    // size = end_exclusive - start_inclusive;
    int body_size = snd_blank_ix - (blank_ix + 1);
    char body[MAX_BODY_SIZE][MAX_LENGTH] = {0};
    for (int i = 0; i < body_size; i++) {
        strcpy(body[i], lines[(blank_ix + 1) + i]);
    }

    cout << "start line: " << start_line << endl;

    for (int i = 0; i < headers_size; i++) {
        cout << "header" << endl;
        cout << headers[i] << endl;
    }

    for (int i = 0; i < body_size; i++) {
        cout << "body" << endl;
        cout << body[i] << endl;
    }
}

void parse_http_request_stack_indices(const char *text) {
    // char lines[MAX_STRINGS][MAX_LENGTH] = {0};
    int newlines[MAX_LINES] = {0};
    int newline_count = split_lines(text, newlines);



    // int blank_ix = MAX_LENGTH;
    // for (int i = 0; i < MAX_LINES; ++i) {
    //     if (lines[i][0] == '\0') {
    //         blank_ix = min(blank_ix, i);
    //     }
    // }

    // int snd_blank_ix = MAX_LENGTH;
    // for (int i = 0; i < MAX_LINES; ++i) {
    //     if (lines[i][0] == '\0' && i > blank_ix) {
    //         snd_blank_ix = min(snd_blank_ix, i);
    //     }
    // }

    // cout << "blank_ix " << blank_ix << endl;
    // cout << "snd_blank_ix " << snd_blank_ix << endl;

    // for (int i = 0; i < MAX_LINES; ++i) {
    //     cout << "Line " << i << " : " << lines[i] << endl;
    // }

    // Location start_line_loc;
    // Location headers_loc;
    // Location body_loc;

    // start_line_loc.start_ix = 0;
    // start_line_loc.end_ix = ???;
    // headers_loc.start_ix = blank_ix
}

string get_substring(const char *text, int start_ix, int end_ix) {
    string result;
    
    if (start_ix >= 0 && end_ix >= start_ix) {
        for (int i = start_ix; i <= end_ix && text[i] != '\0'; ++i) {
            result += text[i];
        }
    }
    
    return result;
}

void copying_heap(string request) {
    istringstream stream(request);
    string line;
    vector<string> lines;

    int blank_ix = 0;
    int i = 0;

    while (getline(stream, line)) {
        lines.push_back(line);
    }
}

void indices_heap(const string &request) {
    // cout << request << endl;
    // cout << endl;

    vector<int> indices;
    indices.push_back(-1);
    for (size_t i = 0; i < request.size(); ++i) {
        if (request[i] == '\n') {
            indices.push_back(i);
        }
    }
    indices.push_back(request.size());

    // for (int i = 0; i < indices.size() - 1; i++) {
    //     int length = indices[i + 1] - indices[i] - 1;
    //     print_raw(request.substr(indices[i] + 1, length));
    //     cout << endl;
    // }
}

void indices_stack(const char *request) {
    // cout << request << endl;
    // cout << endl;

    int indices[MAX_LINES] = {0};
    int j = 0;
    indices[j++] = -1;
    int i = 0;
    while (i < MAX_REQUEST_SIZE && request[i] != '\0') {
        if (request[i] == '\n') {
            indices[j++] = i;
        }
        i++;
    }
    indices[j++] = i;

    // for (i = 0; i < j - 1; i++) {
    //     int length = indices[i + 1] - indices[i] - 1;
    //     print_raw(request, indices[i] + 1, length);
    //     cout << endl;
    // }
}

void bench() {
    int num_iters = 10'000;
    string str_request = string(request);
    for (int i = 0; i < num_iters; i++) {
        #pragma clang optimize off
        // parse_http_request_very_simple(REQ);
        // parse_http_request_stack(REQ);
        // indices_heap(str_request);
        indices_stack(request);
        #pragma clang optimize on
        __asm__ __volatile__("" ::: "memory");
    }
}

int main(int argc, const char * argv[]) {
    bench();
    // parse_http_request_stack_indices(request);
    // indices_stack(request);
    return 0;
}
