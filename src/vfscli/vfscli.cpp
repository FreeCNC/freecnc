#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#include "../freecnc/gameengine.h"
#include "../freecnc/scripting/gameconfigscript.h"

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

GameEngine game;

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
    po::variables_map vm;
    vector<string> files;
};

VFSCli::VFSCli()
{
}

void VFSCli::parse(int argc, char** argv)
{
    string outdir;

    po::options_description general("General options");
    general.add_options()
        ("help,h", "show this message")
        ("basedir", po::value<string>(&game.config.basedir)->default_value("."),
            "use this location to find the data files")
        ("mod", po::value<string>(&game.config.mod)->default_value("td"),
            "specify which mod to use (td or ra)")
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

    game.log.open((game.config.basedir + "/vfscli.log").c_str());

    game.log << "Basedir: \"" << game.config.basedir << "\" Mod: \"" << game.config.mod << "\"\n";
    game.log << "Outputting to: " << outdir << "\n";
    game.vfs.add(outdir);

    {
        GameConfigScript gcs;
        gcs.parse(game.config.basedir + "/data/manifest.lua");
    }
}

void VFSCli::extract_files()
{
    vector<string>::const_iterator it, end;
    end = files.end();
    for (it = files.begin(); it != end; ++it) {
        cout << "Extracting " << *it << ": ";
        shared_ptr<VFS::File> f = game.vfs.open(*it);
        if (!f) {
            cout << "not found\n";
            continue;
        }
        shared_ptr<VFS::File> output = game.vfs.open_write(*it);
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

// Make linking against GameEngine work
void GameEngine::shutdown()
{
}
