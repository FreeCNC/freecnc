#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include "../freecnc/vfs/vfs.h"

namespace po = boost::program_options;

using std::runtime_error;
using std::cerr;
using std::cin;
using std::cout;
using std::flush;
using std::string;
using std::vector;
using boost::shared_ptr;
using boost::to_upper;

struct UsageMessage : public runtime_error
{
    UsageMessage(const string& msg) : runtime_error(msg) {}
};

class VFSCli
{
public:
    VFSCli();
    void parse(int argc, char** argv);
    // TODO
    void input_loop();
private:
    void extract_files();
    VFS::VFS vfs;
    po::variables_map vm;
    vector<string> files;
};

VFSCli::VFSCli()
{
}

void VFSCli::parse(int argc, char** argv)
{
    string outdir, basedir;

    po::options_description general("General options");
    general.add_options()
        ("help,h", "show this message")
        ("basedir", po::value<string>(&basedir)->default_value("."),
            "use this location to find the data files")
        ("output-dir,O", po::value<string>(&outdir)->default_value("."),
            "use this location to store the extracted files");

    po::options_description implicit("Implicit options");
    implicit.add_options()
        ("files", po::value<vector<string> >(&files));

    po::positional_options_description pd;
    pd.add("files", -1);

    po::options_description cmdline_options;
    cmdline_options.add(general).add(implicit);

    po::store(po::command_line_parser(argc, argv).
        options(cmdline_options).positional(pd).run(), vm);

    po::notify(vm);

    if (vm.count("help") || files.empty()) {
        std::ostringstream s;
        s << general;
        throw UsageMessage(s.str());
    }

    vfs.add(outdir);

    // This needs to do whatever the main program does for selecting directories
    // to load
    vfs.add(basedir + "/data");
    vfs.add(basedir + "/data/td");
}

void VFSCli::extract_files()
{
    vector<string>::const_iterator it, end;
    end = files.end();
    for (it = files.begin(); it != end; ++it) {
        cout << "Extracting " << *it << ": ";
        shared_ptr<VFS::File> f = vfs.open(*it);
        if (!f) {
            cout << "not found\n";
            continue;
        }
        shared_ptr<VFS::File> output = vfs.open_write(*it);
        if (!output) {
            cout << "unable to open output file\n";
            continue;
        }

        vector<unsigned char> buf(f->size());
        f->read(buf, f->size());
        if (output->write(buf) == f->size()) {
            cout << "done\n";
        } else {
            cout << "truncated, aborting\n";
            throw runtime_error("Failed to write " + *it);
        }
    }
}

void VFSCli::input_loop()
{
    extract_files();
/*
    cout << "> " << flush;

    string input;
    while (getline(cin, input)) {
        to_upper(input);
        if (input == "QUIT") {
            return;
        }
        cout << "> " << flush;
    }
*/
}

int main(int argc, char** argv)
{
    VFSCli cli;
    try {
        cli.parse(argc, argv);
        cli.input_loop();
    } catch (UsageMessage& e) {
        cout << e.what() << "\n";
        return 1;
    } catch (std::exception& e) {
        cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
