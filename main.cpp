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
constexpr int MAX_HEADERS = 50;
constexpr int MAX_BODY = 50;
constexpr int MAX_REQUEST = 4'000;

constexpr char request[] = "POST /submit-form HTTP/1.1\n"
    "Host: www.example.com\n"
    "Content-Type: application/x-www-form-urlencoded\n"
    "Content-Length: 29\n"
    "Connection: keep-alive\n"
    "Accept: text/html, application/xhtml+xml, application/xml;q=0.9, */*;q=0.8\n"
    "Accept-Language: en-US, en;q=0.5\n"
    "\n"
    "name=John+Doe&email=john.doe%40example.com\n"
    "foo bar baz";

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
    char headers[MAX_HEADERS][MAX_LENGTH] = {0};
    for (int i = 0; i < headers_size; i++) {
        strcpy(headers[i], lines[1 + i]);
    }

    // size = end_exclusive - start_inclusive;
    int body_size = snd_blank_ix - (blank_ix + 1);
    char body[MAX_BODY][MAX_LENGTH] = {0};
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
    while (i < MAX_REQUEST && request[i] != '\0') {
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

void copying_heap_v2(const string &request) {
    cout << request << endl;
    cout << endl;

    istringstream stream(request);
    string line;
    vector<string> lines;

    string start_line;
    vector<string> headers;
    vector<string> body;

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

    cout << "start line: " << start_line << endl;
    for (const auto& h : headers) {
        cout << "header: " << h << endl;
    }
    for (const auto& b : body) {
        cout << "body: " << b << endl;
    }
}

void print_indices_heap(vector<int> indices, const string &request) {
    for (int i = 0; i < indices.size() - 1; i++) {
        int length = indices[i + 1] - indices[i] - 1;
        print_raw(request.substr(indices[i] + 1, length));
        cout << endl;
    }
}

void indices_heap_v2(const string &request) {
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

    int blank_ix = 0;
    for (int i = 0; i < indices.size() - 1; i++) {
        if (indices[i + 1] == indices[i] + 1) {
            blank_ix = i;
        }
    }

    vector<int> start_line;
    vector<int> headers;
    vector<int> body;
    for (int i = 0; i < indices.size(); i++) {
        if (i == 0) {
            start_line.push_back(indices[i]);
            start_line.push_back(indices[i + 1]);
        } else if (i <= blank_ix) {
            headers.push_back(indices[i]);
        } else if (i > blank_ix) {
            body.push_back(indices[i]);
        }
    }

    // cout << "start_line" << endl;
    // print_indices_heap(start_line, request);
    // cout << "headers" << endl;
    // print_indices_heap(headers, request);
    // cout << "body" << endl;
    // print_indices_heap(body, request);
}

void print_indices_stack(int indices[], int indices_size, const char *request) {
    for (int i = 0; i < indices_size - 1; i++) {
        int length = indices[i + 1] - indices[i] - 1;
        print_raw(request, indices[i] + 1, length);
        cout << endl;
    }
}

void indices_stack_v2(const char *request) {
    // cout << request << endl;
    // cout << endl;

    int indices[MAX_LINES] = {0};
    int indices_size = 0;
    indices[indices_size++] = -1;
    int i = 0;
    while (i < MAX_REQUEST && request[i] != '\0') {
        if (request[i] == '\n') {
            indices[indices_size++] = i;
        }
        i++;
    }
    indices[indices_size++] = i;

    int blank_ix = 0;
    for (int i = 0; i < indices_size - 1; i++) {
        if (indices[i + 1] == indices[i] + 1) {
            blank_ix = i;
        }
    }

    int start_line[MAX_LINES] = {0};
    int headers[MAX_LINES] = {0};
    int body[MAX_LINES] = {0};

    int start_line_size = 0;
    int headers_size = 0;
    int body_size = 0;

    for (int i = 0; i < indices_size; i++) {
        if (i == 0) {
            start_line[start_line_size++] = indices[i];
            start_line[start_line_size++] = indices[i + 1];
        } else if (i <= blank_ix) {
            headers[headers_size++] = indices[i];
        } else if (i > blank_ix) {
            body[body_size++] = indices[i];
        }
    }

    // cout << "start_line" << endl;
    // print_indices_stack(start_line, start_line_size, request);
    // cout << "headers" << endl;
    // print_indices_stack(headers, headers_size, request);
    // cout << "body" << endl;
    // print_indices_stack(body, body_size, request);
}

void copying_heap_v3(const string &request) {
    cout << request << endl;
    cout << endl;

    istringstream stream(request);
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

    string start_line;
    vector<string> headers;
    vector<string> body;
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

    string method;
    string target;
    string http_version;
    i = 0;
    for (const auto& w : words) {
        if (i == 0) {
            method = w;
        } else if (i == 1) {
            target = w;
        } else if (i == 2) {
            http_version = w;
        }
        i++;
    }

    vector<string> header_keys;
    vector<string> header_vals;
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

    cout << "method: " << method << endl;
    cout << "target: " << target << endl;
    cout << "http_version: " << http_version << endl;
    for (int i = 0; i < header_keys.size(); i++) {
        cout << "header key: " << header_keys[i] << endl;
        cout << "header val: " << header_vals[i] << endl;
    }
    for (const auto& b : body) {
        cout << "body: " << b << endl;
    }
}

void print_indices_heap_v3(vector<int> indices, const string &request) {
    for (int i = 0; i < indices.size(); i += 2) {
        int length = indices[i + 1] - indices[i];
        print_raw(request.substr(indices[i], length));
        cout << endl;
    }
}

void indices_heap_v3(const string &request) {
    // cout << request << endl;
    // cout << endl;

    vector<int> indices;
    for (size_t i = 0; i < request.size(); i++) {
        if (request[i] == '\n') {
            if (indices.size() == 0) {
                indices.push_back(0);
            } else {
                indices.push_back(indices.back() + 1);
            }
            indices.push_back(i);
        }
    }
    indices.push_back(indices.back() + 1);
    indices.push_back(request.size());

    int blank_ix = 0;
    for (int i = 0; i < indices.size(); i += 2) {
        if (indices[i] == indices[i + 1]) {
            blank_ix = i;
        }
    }

    vector<int> start_line;
    vector<int> headers;
    vector<int> body;
    for (int i = 0; i < indices.size(); i += 2) {
        if (i == 0) {
            start_line.push_back(indices[i]);
            start_line.push_back(indices[i + 1]);
        } else if (i < blank_ix) {
            headers.push_back(indices[i]);
            headers.push_back(indices[i + 1]);
        } else if (i > blank_ix) {
            body.push_back(indices[i]);
            body.push_back(indices[i + 1]);
        }
    }

    vector<int> start_line_spaces;

    for (int i = start_line[0]; i < start_line[1]; i++) {
        char c = request[i];
        if (c == ' ') {
            start_line_spaces.push_back(i);
        }
    }

    vector<int> method = {start_line[0], start_line_spaces[0]};
    vector<int> target = {start_line_spaces[0] + 1, start_line_spaces[1]};
    vector<int> http_version = {start_line_spaces[1] + 1, start_line[1]};

    vector<int> header_keys;
    vector<int> header_vals;

    for (int i = 0; i < headers.size(); i += 2) {
        for (int j = headers[i]; j < headers[i + 1]; j++) {
            if (request[j] == ':' && request[j + 1] == ' ') {
                header_keys.push_back(headers[i]);
                header_keys.push_back(j);

                header_vals.push_back(j + 2);
                header_vals.push_back(headers[i + 1]);
                break;
            }
        }
    }

    // cout << "method" << endl;
    // print_indices_heap_v3(method, request);
    // cout << "target" << endl;
    // print_indices_heap_v3(target, request);
    // cout << "http_version" << endl;
    // print_indices_heap_v3(http_version, request);
    // cout << "header keys" << endl;
    // print_indices_heap_v3(header_keys, request);
    // cout << "header vals" << endl;
    // print_indices_heap_v3(header_vals, request);
    // cout << "body" << endl;
    // print_indices_heap_v3(body, request);
}

void print_indices_stack_v3(int indices[], int indices_size, const char *request) {
    for (int i = 0; i < indices_size; i += 2) {
        int length = indices[i + 1] - indices[i];
        print_raw(request, indices[i], length);
        cout << endl;
    }
}

void indices_stack_v3(const char *request) {
    // cout << request << endl;
    // cout << endl;

    int indices[MAX_LINES * 2] = {0};
    int indices_size = 0;
    int i = 0;
    while (i < MAX_REQUEST && request[i] != '\0') {
        if (request[i] == '\n') {
            if (indices_size == 0) {
                indices[indices_size++] = 0;
            } else {
                indices[indices_size++] = indices[indices_size - 1] + 1;
            }
            indices[indices_size++] = i;
        }
        i++;
    }
    indices[indices_size++] = indices[indices_size - 1] + 1;
    indices[indices_size++] = i;

    int blank_ix = 0;
    for (int i = 0; i < indices_size; i += 2) {
        if (indices[i] == indices[i + 1]) {
            blank_ix = i;
        }
    }

    int start_line[2];
    int headers[MAX_HEADERS * 2];
    int body[MAX_BODY * 2];

    int start_line_size = 0;
    int headers_size = 0;
    int body_size = 0;

    for (int i = 0; i < indices_size; i += 2) {
        if (i == 0) {
            start_line[start_line_size++] = indices[i];
            start_line[start_line_size++] = indices[i + 1];
        } else if (i < blank_ix) {
            headers[headers_size++] = indices[i];
            headers[headers_size++] = indices[i + 1];
        } else if (i > blank_ix) {
            body[body_size++] = indices[i];
            body[body_size++] = indices[i + 1];
        }
    }

    int start_line_spaces[2];
    int start_line_spaces_size = 0;

    for (int i = start_line[0]; i < start_line[1]; i++) {
        char c = request[i];
        if (c == ' ') {
            start_line_spaces[start_line_spaces_size++] = i;
        }
    }

    int method[2] = {start_line[0], start_line_spaces[0]};
    int target[2] = {start_line_spaces[0] + 1, start_line_spaces[1]};
    int http_version[2] = {start_line_spaces[1] + 1, start_line[1]};

    int header_keys[MAX_HEADERS * 2];
    int header_vals[MAX_HEADERS * 2];

    int header_keys_size = 0;
    int header_vals_size = 0;

    for (int i = 0; i < headers_size; i += 2) {
        for (int j = headers[i]; j < headers[i + 1]; j++) {
            if (request[j] == ':' && request[j + 1] == ' ') {
                header_keys[header_keys_size++] = headers[i];
                header_keys[header_keys_size++] = j;

                header_vals[header_vals_size++] = j + 2;
                header_vals[header_vals_size++] = headers[i + 1];
                break;
            }
        }
    }

    // cout << "method" << endl;
    // print_indices_stack_v3(method, 2, request);
    // cout << "target" << endl;
    // print_indices_stack_v3(target, 2, request);
    // cout << "http_version" << endl;
    // print_indices_stack_v3(http_version, 2, request);
    // cout << "header keys" << endl;
    // print_indices_stack_v3(header_keys, header_keys_size, request);
    // cout << "header vals" << endl;
    // print_indices_stack_v3(header_vals, header_vals_size, request);
    // cout << "body" << endl;
    // print_indices_stack_v3(body, body_size, request);
}

void bench() {
    int num_iters = 10'000;
    string str_request = string(request);
    for (int i = 0; i < num_iters; i++) {
        #pragma clang optimize off
        // indices_heap(str_request);
        // indices_heap_v3(str_request);
        indices_stack_v3(request);
        #pragma clang optimize on
        __asm__ __volatile__("" ::: "memory");
    }
}

int main(int argc, const char * argv[]) {
    bench();
    // string str_request = string(request);
    // indices_stack(request);
    // indices_stack_v2(request);
    // indices_stack_v3(request);
    return 0;
}
