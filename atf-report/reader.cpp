//
// Automated Testing Framework (atf)
//
// Copyright (c) 2007, 2008, 2009, 2010 The NetBSD Foundation, Inc.
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

#include <map>
#include <sstream>
#include <utility>

#include "atf-c++/parser.hpp"
#include "atf-c++/sanity.hpp"
#include "atf-c++/text.hpp"

#include "reader.hpp"

namespace impl = atf::atf_report;
#define IMPL_NAME "atf::atf_report"

// ------------------------------------------------------------------------
// Auxiliary functions.
// ------------------------------------------------------------------------

static
size_t
string_to_size_t(const std::string& str)
{
    std::istringstream ss(str);
    size_t s;
    ss >> s;

    return s;
}

// ------------------------------------------------------------------------
// The "atf_tps" auxiliary parser.
// ------------------------------------------------------------------------

namespace atf_tps {

static const atf::parser::token_type& eof_type = 0;
static const atf::parser::token_type& nl_type = 1;
static const atf::parser::token_type& text_type = 2;
static const atf::parser::token_type& colon_type = 3;
static const atf::parser::token_type& comma_type = 4;
static const atf::parser::token_type& tps_count_type = 5;
static const atf::parser::token_type& tp_start_type = 6;
static const atf::parser::token_type& tp_end_type = 7;
static const atf::parser::token_type& tc_start_type = 8;
static const atf::parser::token_type& tc_so_type = 9;
static const atf::parser::token_type& tc_se_type = 10;
static const atf::parser::token_type& tc_end_type = 11;
static const atf::parser::token_type& passed_type = 12;
static const atf::parser::token_type& failed_type = 13;
static const atf::parser::token_type& skipped_type = 14;
static const atf::parser::token_type& info_type = 16;

class tokenizer : public atf::parser::tokenizer< std::istream > {
public:
    tokenizer(std::istream& is, size_t curline) :
        atf::parser::tokenizer< std::istream >
            (is, true, eof_type, nl_type, text_type, curline)
    {
        add_delim(':', colon_type);
        add_delim(',', comma_type);
        add_keyword("tps-count", tps_count_type);
        add_keyword("tp-start", tp_start_type);
        add_keyword("tp-end", tp_end_type);
        add_keyword("tc-start", tc_start_type);
        add_keyword("tc-so", tc_so_type);
        add_keyword("tc-se", tc_se_type);
        add_keyword("tc-end", tc_end_type);
        add_keyword("passed", passed_type);
        add_keyword("failed", failed_type);
        add_keyword("skipped", skipped_type);
        add_keyword("info", info_type);
    }
};

} // namespace atf_tps

// ------------------------------------------------------------------------
// The "atf_tps_reader" class.
// ------------------------------------------------------------------------

impl::atf_tps_reader::atf_tps_reader(std::istream& is) :
    m_is(is)
{
}

impl::atf_tps_reader::~atf_tps_reader(void)
{
}

void
impl::atf_tps_reader::got_info(const std::string& what,
                               const std::string& val)
{
}

void
impl::atf_tps_reader::got_ntps(size_t ntps)
{
}

void
impl::atf_tps_reader::got_tp_start(const std::string& tp, size_t ntcs)
{
}

void
impl::atf_tps_reader::got_tp_end(const std::string& reason)
{
}

void
impl::atf_tps_reader::got_tc_start(const std::string& tcname)
{
}

void
impl::atf_tps_reader::got_tc_stdout_line(const std::string& line)
{
}

void
impl::atf_tps_reader::got_tc_stderr_line(const std::string& line)
{
}

void
impl::atf_tps_reader::got_tc_end(const atf::tests::tcr& tcr)
{
}

void
impl::atf_tps_reader::got_eof(void)
{
}

void
impl::atf_tps_reader::read_info(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    (void)p.expect(colon_type, "`:'");

    atf::parser::token t = p.expect(text_type, "info property name");
    (void)p.expect(comma_type, "`,'");
    got_info(t.text(), atf::text::trim(p.rest_of_line()));

    (void)p.expect(nl_type, "new line");
}

void
impl::atf_tps_reader::read_tp(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    atf::parser::token t = p.expect(tp_start_type,
                                    "start of test program");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test program name");
    std::string tpname = t.text();

    t = p.expect(comma_type, "`,'");

    t = p.expect(text_type, "number of test programs");
    size_t ntcs = string_to_size_t(t.text());

    t = p.expect(nl_type, "new line");

    ATF_PARSER_CALLBACK(p, got_tp_start(tpname, ntcs));

    size_t i = 0;
    while (p.good() && i < ntcs) {
        try {
            read_tc(&p);
            i++;
        } catch (const parse_error& pe) {
            p.add_error(pe);
            p.reset(nl_type);
        }
    }
    t = p.expect(tp_end_type, "end of test program");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test program name");
    if (t.text() != tpname)
        throw parse_error(t.lineno(), "Test program name used in "
                                      "terminator does not match "
                                      "opening");

    t = p.expect(nl_type, comma_type,
                 "new line or comma_type");
    std::string reason;
    if (t.type() == comma_type) {
        reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for failed test program");
        t = p.next();
    }

    ATF_PARSER_CALLBACK(p, got_tp_end(reason));
}

void
impl::atf_tps_reader::read_tc(void* pptr)
{
    using atf::parser::parse_error;
    using namespace atf_tps;
    using atf::tests::tcr;

    atf::parser::parser< tokenizer >& p =
        *reinterpret_cast< atf::parser::parser< tokenizer >* >
        (pptr);

    atf::parser::token t = p.expect(tc_start_type, "start of test case");

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test case name");
    std::string tcname = t.text();
    ATF_PARSER_CALLBACK(p, got_tc_start(tcname));

    t = p.expect(nl_type, "new line");

    t = p.expect(tc_end_type, tc_so_type, tc_se_type,
                 "end of test case or test case's stdout/stderr line");
    while (t.type() != tc_end_type &&
           (t.type() == tc_so_type || t.type() == tc_se_type)) {
        atf::parser::token t2 = t;

        t = p.expect(colon_type, "`:'");

        std::string line = p.rest_of_line();

        if (t2.type() == tc_so_type) {
            ATF_PARSER_CALLBACK(p, got_tc_stdout_line(line));
        } else {
            INV(t2.type() == tc_se_type);
            ATF_PARSER_CALLBACK(p, got_tc_stderr_line(line));
        }

        t = p.expect(nl_type, "new line");

        t = p.expect(tc_end_type, tc_so_type, tc_se_type,
                     "end of test case or test case's stdout/stderr line");
    }

    t = p.expect(colon_type, "`:'");

    t = p.expect(text_type, "test case name");
    if (t.text() != tcname)
        throw parse_error(t.lineno(),
                          "Test case name used in terminator does not "
                          "match opening");

    t = p.expect(comma_type, "`,'");

    t = p.expect(passed_type, failed_type, skipped_type,
                 "passed, failed or skipped");
    if (t.type() == passed_type) {
        ATF_PARSER_CALLBACK(p, got_tc_end(tcr(tcr::passed_state)));
    } else if (t.type() == failed_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for failed test case result");
        ATF_PARSER_CALLBACK(p, got_tc_end(tcr(tcr::failed_state, reason)));
    } else if (t.type() == skipped_type) {
        t = p.expect(comma_type, "`,'");
        std::string reason = text::trim(p.rest_of_line());
        if (reason.empty())
            throw parse_error(t.lineno(),
                              "Empty reason for skipped test case result");
        ATF_PARSER_CALLBACK(p, got_tc_end(tcr(tcr::skipped_state, reason)));
    } else
        UNREACHABLE;

    t = p.expect(nl_type, "new line");
}

void
impl::atf_tps_reader::read(void)
{
    using atf::parser::parse_error;
    using namespace atf_tps;

    std::pair< size_t, atf::parser::headers_map > hml =
        atf::parser::read_headers(m_is, 1);
    atf::parser::validate_content_type(hml.second, "application/X-atf-tps", 2);

    tokenizer tkz(m_is, hml.first);
    atf::parser::parser< tokenizer > p(tkz);

    try {
        atf::parser::token t;

        while ((t = p.expect(tps_count_type, info_type, "tps-count or info "
                             "field")).type() == info_type)
            read_info(&p);

        t = p.expect(colon_type, "`:'");

        t = p.expect(text_type, "number of test programs");
        size_t ntps = string_to_size_t(t.text());
        ATF_PARSER_CALLBACK(p, got_ntps(ntps));

        t = p.expect(nl_type, "new line");

        size_t i = 0;
        while (p.good() && i < ntps) {
            try {
                read_tp(&p);
                i++;
            } catch (const parse_error& pe) {
                p.add_error(pe);
                p.reset(nl_type);
            }
        }

        while ((t = p.expect(eof_type, info_type, "end of stream or info "
                             "field")).type() == info_type)
            read_info(&p);
        ATF_PARSER_CALLBACK(p, got_eof());
    } catch (const parse_error& pe) {
        p.add_error(pe);
        p.reset(nl_type);
    }
}