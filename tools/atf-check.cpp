//
// Automated Testing Framework (atf)
//
// Copyright (c) 2008 The NetBSD Foundation, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND
// CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
// IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
// IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <utility>

#include "atf-c++/application.hpp"
#include "atf-c++/exceptions.hpp"
#include "atf-c++/check.hpp"
#include "atf-c++/fs.hpp"
#include "atf-c++/io.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/text.hpp"

class atf_check : public atf::application::app {
    enum output_check_t {
        OC_IGNORE,
        OC_INLINE,
        OC_FILE,
        OC_EMPTY,
        OC_SAVE
    };

    enum status_check_t {
        SC_EQUAL,
        SC_NOT_EQUAL,
        SC_IGNORE
    };

    output_check_t m_stdout_check;
    std::string m_stdout_arg;

    output_check_t m_stderr_check;
    std::string m_stderr_arg;

    status_check_t m_status_check;
    int m_status_arg;

    static const char* m_description;

    bool file_empty(const atf::fs::path &) const;
    void diff_file_file(const atf::fs::path &, const atf::fs::path &) const;
    void diff_file_str(const atf::fs::path &, const std::string &) const;
    void print_file(const atf::fs::path &) const;

    bool run_status_check(const atf::check::check_result &) const;
    bool run_output_check(const atf::check::check_result &, const std::string &) const;

    std::string specific_args(void) const;
    options_set specific_options(void) const;
    void process_option(int, const char *);
    void process_option_s(const std::string &);
    void process_option_o(const std::string &);
    void process_option_e(const std::string &);


public:
    atf_check(void);
    int main(void);
};


const char* atf_check::m_description =
    "atf-check executes given command and analyzes its results.";

atf_check::atf_check(void) :
    app(m_description, "atf-check(1)", "atf(7)"),
    m_stdout_check(OC_EMPTY),
    m_stderr_check(OC_EMPTY),
    m_status_check(SC_EQUAL),
    m_status_arg(0)
{
}

bool
atf_check::file_empty(const atf::fs::path &p)
    const
{
    atf::fs::file_info f(p);

    return (f.get_size() == 0);
}


void
atf_check::diff_file_file(const atf::fs::path &p1, const atf::fs::path &p2)
    const
{
    std::string cmd("diff " + p1.str() + " " + p2.str() + " >&2");
    ::system(cmd.c_str());
}

void
atf_check::diff_file_str(const atf::fs::path &path, const std::string &str)
    const
{
    char buf[] = "inline.XXXXXX";

    int fd = ::mkstemp(buf);
    if (fd == -1)
        throw atf::system_error("atf_check::diff_file_str", 
                "mktemp(3) failed", errno);

    if (::write(fd, str.c_str(), str.length()) == -1)
        throw atf::system_error("atf_check::diff_file_str", 
                "write(2) failed", errno);

    if (::close(fd) == -1)
        throw atf::system_error("atf_check::diff_file_str", 
                "close(2) failed", errno);

    diff_file_file(path, atf::fs::path(buf));

    ::unlink(buf);
}

void
atf_check::print_file(const atf::fs::path &p)
    const
{
    std::ifstream f(p.c_str());
    f >> std::noskipws;
    std::istream_iterator< char > begin(f), end;
    std::ostream_iterator< char > out(std::cerr);
    std::copy(begin, end, out);
}

bool
atf_check::run_status_check(const atf::check::check_result &r)
    const
{
    int status = r.status();
    bool retval = true;

    if (m_status_check == SC_EQUAL) {

        if (m_status_arg != status) {
            std::cerr << "Fail: expected exit status " << m_status_arg
                      << ", but got " << status << std::endl;
            retval = false;
        }

    } else if (m_status_check == SC_NOT_EQUAL) {

        if (m_status_arg == status) {
            std::cerr << "Fail: expected exit status other than "
                      << m_status_arg << std::endl;
            retval = false;
        }
    }

    if (retval == false) {
        std::cerr << "Command's stdout:" << std::endl;
        print_file(r.stdout_path());
        std::cerr << std::endl;

        std::cerr << "Command's stderr:" << std::endl;
        print_file(r.stderr_path());
        std::cerr << std::endl;
    }

    return retval;
}

bool
atf_check::run_output_check(const atf::check::check_result &r,
                            const std::string &stdxxx)
    const
{
    atf::fs::path path("/");
    std::string arg;
    output_check_t check;

    if (stdxxx == "stdout") {

        path = r.stdout_path();
        arg = m_stdout_arg.c_str();
        check = m_stdout_check;

    } else if (stdxxx == "stderr") {

        path = r.stderr_path();
        arg = m_stderr_arg.c_str();
        check = m_stderr_check;

    } else
        UNREACHABLE;


    if (check == OC_EMPTY) {

        if (!file_empty(path)) {
            std::cerr << "Fail: command's " << stdxxx
                      << " was not empty" << std::endl;
            diff_file_file(atf::fs::path("/dev/null"), path);

            return false;
        }

    } else if (check == OC_FILE) {

        if (atf::io::cmp_file_file(path, atf::fs::path(arg)) != 0) {
            std::cerr << "Fail: command's " << stdxxx
                      << " and file '" << arg
                      << "' differ" << std::endl;
            diff_file_file(path, atf::fs::path(arg));
            
            return false;
        }

    } else if (check == OC_INLINE) {

        if (atf::io::cmp_file_str(path, arg) != 0) {
            std::cerr << "Fail: command's " << stdxxx << " and '"
                      << arg << "' differ" << std::endl;
            diff_file_str(path, arg);

            return false;
        }

    } else if (check == OC_SAVE) {

        std::ifstream ifs(path.c_str(), std::fstream::binary);
        ifs >> std::noskipws;
        std::istream_iterator< char > begin(ifs), end;

        std::ofstream ofs(arg.c_str(), std::fstream::binary
                                     | std::fstream::trunc);
        std::ostream_iterator <char> obegin(ofs);

        std::copy(begin, end, obegin);
    }
    
    return true;
}


std::string
atf_check::specific_args(void)
    const
{
    return "<command>";
}

atf_check::options_set
atf_check::specific_options(void)
    const
{
    using atf::application::option;
    options_set opts;

    opts.insert(option('s', "qual:value", "Handle status. Qualifier "
                           "must be one of: ignore eq:<num> ne:<num>"));
    opts.insert(option('o', "action:arg", "Handle stdout. Action must be "
               "one of: empty ignore file:<path> inline:<val> save:<path>"));
    opts.insert(option('e', "action:arg", "Handle stderr. Action must be "
               "one of: empty ignore file:<path> inline:<val> save:<path>"));

    return opts;
}

void
atf_check::process_option_s(const std::string& arg)
{
    using atf::application::usage_error;


    if (arg == "ignore") {
        m_status_check = SC_IGNORE;
        return;
    }

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "eq")
        m_status_check = SC_EQUAL;
    else if (action == "ne")
        m_status_check = SC_NOT_EQUAL;
    else
        throw usage_error("Invalid value for -s option");


    std::string value = arg.substr(pos + 1);


    try {
        m_status_arg = atf::text::to_type< unsigned int >(value);
    } catch (std::runtime_error) {
        throw usage_error("Invalid value for -s option; must be an "
                          "integer in range 0-255");
    }

    if ((m_status_arg < 0) || (m_status_arg > 255))
        throw usage_error("Invalid value for -s option; must be an "
                          "integer in range 0-255");

}

void
atf_check::process_option_o(const std::string& arg)
{
    using atf::application::usage_error;

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "empty")
        m_stdout_check = OC_EMPTY;
    else if (action == "ignore")
        m_stdout_check = OC_IGNORE;
    else if (action == "save")
        m_stdout_check = OC_SAVE;
    else if (action == "inline")
        m_stdout_check = OC_INLINE;
    else if (action == "file")
        m_stdout_check = OC_FILE;
    else
        throw usage_error("Invalid value for -o option");

    m_stdout_arg = arg.substr(pos + 1);
}

void
atf_check::process_option_e(const std::string& arg)
{
    using atf::application::usage_error;

    std::string::size_type pos = arg.find(':');
    std::string action = arg.substr(0, pos);

    if (action == "empty")
        m_stderr_check = OC_EMPTY;
    else if (action == "ignore")
        m_stderr_check = OC_IGNORE;
    else if (action == "save")
        m_stderr_check = OC_SAVE;
    else if (action == "inline")
        m_stderr_check = OC_INLINE;
    else if (action == "file")
        m_stderr_check = OC_FILE;
    else
        throw usage_error("Invalid value for -e option");

    m_stderr_arg = arg.substr(pos + 1);
}

void
atf_check::process_option(int ch, const char* arg)
{
    switch (ch) {
    case 's':
        process_option_s(arg);
        break;

    case 'o':
        process_option_o(arg);
        break;

    case 'e':
        process_option_e(arg);
        break;

    default:
        UNREACHABLE;
    }
}

int
atf_check::main(void)
{
    if (m_argc < 1)
        throw atf::application::usage_error("No command specified");

    int status = EXIT_FAILURE;
    
    std::cout << "Checking command [" << m_argv[0] << "]" << std::endl;

    atf::check::check_result r(m_argv[0]);

    if ((run_status_check(r) == false) ||
        (run_output_check(r, "stderr") == false) ||
        (run_output_check(r, "stdout") == false))
        status = EXIT_FAILURE;
    else
        status = EXIT_SUCCESS;

    return status;
}

int
main(int argc, char* const* argv)
{
    return atf_check().run(argc, argv);
}