
#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

class ApplicationError: public exception
{
public:
    ApplicationError(const string& msg): msg(msg) {}
    virtual const char* what() const throw() { return msg.c_str(); }
    virtual ~ApplicationError() throw() {};
private:
    string msg;
};

class ArgumentParseException: public ApplicationError
{
public:
    ArgumentParseException(const string& msg): ApplicationError(msg) {}
};

typedef struct _Arguments
{
    string input;
    string output;
    unsigned long topN;
} Arguments;

typedef unordered_map<string, unsigned long> frequency_map_t;
typedef frequency_map_t domain_map_t;
typedef frequency_map_t path_map_t;

inline bool between(register int x, register int a, register int b) {
    return x >= a && x <= b;
}

inline bool is_path_symbol(register char x) {
    // a-z A-Z 0-9 . , / + _
    return between(x, 'a', 'z') || between(x, 'A', 'Z') || between(x, '0', '9') || x == '.' || x == ',' || x == '/' || x == '+' || x == '_';
}

inline bool is_domain_symbol(register char x) {
    // a-z A-Z 0-9 . -
    return between(x, 'a', 'z') || between(x, 'A', 'Z') || between(x, '0', '9') || x == '.' || x == '-';
}

class UrlParser
{
public:
    UrlParser(istream& input): input(input), sz(0) {};
    string next() {
        do {
            for(; (sz = line.find("http", sz)) != string::npos; sz++) {
                if(sz == 0 || line[sz-1] == ' ' || line[sz-1] == '\t') {
                    string::size_type strptr = line[sz + 4] == 's' ? sz+5 : sz+4;
                    if(line.find("://", strptr) == strptr) {
                        string suffix = "";
                        for(strptr+=3; strptr<line.size() && is_domain_symbol(line[strptr]); strptr++) {}
                        if(line[strptr] == '/') {
                            for(strptr+=1; strptr<line.size() && is_path_symbol(line[strptr]); strptr++) {}
                        } else {
                            suffix = "/";
                        }
                        string result = line.substr(sz, strptr - sz) + suffix;
                        sz = strptr;
                        return result;
                    }
                }
            }
            line = ""; sz = 0;
        } while(getline(input, line));
        return string();
    }
private:
    istream& input;
    string line;
    string::size_type sz;
};

string print_top(const frequency_map_t& source, unsigned long topN, const string& caption)
{
    stringstream output;
    vector< pair<frequency_map_t::key_type, frequency_map_t::mapped_type> > slist;
    for_each(source.begin(), source.end(), [&slist](const frequency_map_t::value_type& a) {
        slist.push_back(a);
    });
    topN = slist.size() > topN ? topN : slist.size();
    partial_sort(slist.begin(), slist.begin() + topN, slist.end(), [](const frequency_map_t::value_type& a, const frequency_map_t::value_type& b) {
        return b.second < a.second || (b.second == a.second && a.first < b.first);
    });
    output << caption << endl;
    for_each(slist.begin(), slist.begin() + topN, [&output](const frequency_map_t::value_type& a) {
        output << a.second << " " << a.first << endl;
    });
    return output.str();
}

string get_statistics(istream& input, unsigned long topN)
{
    stringstream output;
    domain_map_t domains;
    path_map_t paths;
    UrlParser parser(input);
    unsigned long counter = 0;
    for(string url=parser.next(); url.size(); url=parser.next(), counter ++) {
        string::size_type domain = url.find("://") + 3, path = url.find("/", domain+1);
        domains[url.substr(domain, path - domain)] += 1;
        paths[url.substr(path)] += 1;
    }
    output << "total urls " << counter << ", domains " << domains.size() << ", paths " << paths.size() << endl << endl;
    output << print_top(domains, topN, "top domains") << endl;
    output << print_top(paths, topN, "top paths");
    return output.str();
}

Arguments parse_args(int argc, char * argv[])
{
    Arguments a;
    a.topN = 10;
    for(int i=0; i<argc; i++) {
        string arg(argv[i]);
        if(arg.find("-n") == 0) {
            string opt = arg.substr(2);
            if(!opt.size()) {
                if(i+1 == argc) {
                    throw ArgumentParseException("Argument missed for option -n");
                }
                opt = string(argv[++i]);
            }
            string::size_type sz;
            a.topN = stoul(opt, &sz);
            if(sz != opt.size()) {
                throw ArgumentParseException("Unable to parse int from string \"" + opt + "\"");
            }
            if(opt[0] == '-' || a.topN < 1) {
                throw ArgumentParseException("-n option should be greater than 0");
            }
        } else if(!a.input.size()) {
            a.input = arg;
        } else if(!a.output.size()) {
            a.output = arg;
        } else {
            throw ArgumentParseException("Unknown argument " + arg);
        }
    }
    return a;
}

void print_usage(char * prog)
{
    cerr << "Usage: " << prog << " [-n N] [input-file [output-file]]" << endl;
}

int main(int argc, char * argv[])
{
    try {
        Arguments args;
        try {
            args = parse_args(argc-1, &argv[1]);
        } catch(ArgumentParseException& e) {
            cerr << "Argument parse error: " << e.what() << endl;
            print_usage(argv[0]);
            return 1;
        }
        istream * input = args.input.size() ? new ifstream(args.input) : &cin;
        if(input->fail()) throw ApplicationError("Unable to open file \"" + args.input + "\" for read");
        string report = get_statistics(*input, args.topN);
        ostream * output = args.output.size() ? new ofstream(args.output) : &cout;
        if(output->fail()) throw ApplicationError("Unable to open file \"" + args.output + "\" for write");
        (*output) << report;
    } catch(ApplicationError& e) {
        cerr << e.what() << endl;
        return 1;
    } catch(exception& e) {
        cerr << "Unhandled exception: " << e.what() << endl;
        return 1;
    }
    return 0;
}
