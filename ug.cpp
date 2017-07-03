
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

#define BETWEEN(x, a, b) ((x) >= (a) && (x) <= (b))
#define IS_LOWER_ALPHA(x) (BETWEEN((x), 'a', 'z'))
#define IS_UPPER_ALPHA(x) (BETWEEN((x), 'A', 'Z'))
#define IS_DIGIT(x) (BETWEEN((x), '0', '9'))
#define IS_DOMAIN_SYMBOL(x) (IS_LOWER_ALPHA(x) || (x) == '.' || (x) == '-' || IS_DIGIT(x) || IS_UPPER_ALPHA(x))
#define IS_PATH_SYMBOL(x) (IS_LOWER_ALPHA(x) || (x) == '.' || (x) == ',' || (x) == '/' || (x) == '+' || (x) == '_' || IS_DIGIT(x) || IS_UPPER_ALPHA(x))

unsigned long process(istream& input, domain_map_t& domains, path_map_t& paths)
{
    unsigned long counter = 0;
    string::size_type pos;
    string line;
    const char * lineptr, * lineptr_end, * tokenptr;

    while(getline(input, line)) {
        pos = 0;
        lineptr = line.c_str();
        lineptr_end = &lineptr[line.size()];
        for(; (pos = line.find("http", pos)) != string::npos; pos++) {
            if(pos == 0 || lineptr[pos-1] == ' ' || lineptr[pos-1] == '\t') {
                const char * strptr = line[pos + 4] == 's' ? &lineptr[pos+5] : &lineptr[pos+4];
                if(*strptr++ == ':' && *strptr++ == '/' && *strptr++ == '/') {
                    tokenptr = strptr;
                    for(; strptr<lineptr_end && IS_DOMAIN_SYMBOL(*strptr); strptr++) {}
                    if(strptr == tokenptr) continue;
                    counter++;
                    domains[string(tokenptr, strptr - tokenptr)] += 1;
                    if(*strptr == '/') {
                        tokenptr = strptr;
                        for(strptr++; strptr<lineptr_end && IS_PATH_SYMBOL(*strptr); strptr++) {}
                        paths[string(tokenptr, strptr - tokenptr)] += 1;
                    } else {
                        paths["/"] += 1;
                    }
                }
            }
        }
    }

    return counter;
}

string print_top(const frequency_map_t& source, unsigned long topN, const string& caption)
{
    stringstream output;
    vector< pair<frequency_map_t::key_type, frequency_map_t::mapped_type> > slist;
    for_each(source.begin(), source.end(), [&slist](const frequency_map_t::value_type& a) {
        slist.push_back(a);
    });
    topN = !topN || topN > slist.size() ? slist.size() : topN;
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
    unsigned long counter = process(input, domains, paths);
    output << "total urls " << counter << ", domains " << domains.size() << ", paths " << paths.size() << endl << endl;
    output << print_top(domains, topN, "top domains") << endl;
    output << print_top(paths, topN, "top paths");
    return output.str();
}

Arguments parse_args(int argc, char * argv[])
{
    Arguments a;
    a.topN = 0;
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
            if(opt[0] == '-') {
                throw ArgumentParseException("-n option should not be negative");
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
