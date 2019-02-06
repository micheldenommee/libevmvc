/*
    This file is part of libpq-async++
    Copyright (C) 2011-2018 Michel Denommee (and other contributing authors)
    
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _libevmvc_multipart_parser_h
#define _libevmvc_multipart_parser_h

#include "stable_headers.h"
#include "router.h"
#include "stack_debug.h"
#include "fields.h"
#include "http_param.h"
#include "response.h"

namespace evmvc { namespace _internal {

// multipart/form-data; boundary=---------------------------334625884604427441415954120

enum class multipart_parser_state
{
    init,
    headers,
    content,
    failed,
};

enum class multipart_content_type
{
    unknown,
    unset,
    form,
    file,
    mixed,
    alternative,
    digest
};

typedef struct multipart_content_t
{
    multipart_content_t()
        : type(multipart_content_type::unknown),
        name(""), headers()
    {
    }
    
    multipart_content_t(multipart_content_type ct)
        : type(ct),
        name(""), headers()
    {
    }
    
    evmvc::sp_http_param get(const evmvc::string_view& pname)
    {
        for(auto& ele : headers)
            if(strcmp(ele->name(), pname.data()) == 0)
                return ele;
        return nullptr;
    }
    
    multipart_content_type type;
    std::string name;
    evmvc::http_params headers;
    
} multipart_content;

typedef struct multipart_content_form_t
    : public multipart_content
{
    multipart_content_form_t()
        : multipart_content(multipart_content_type::form),
        param()
    {
    }

    multipart_content_form_t()
        : multipart_content(multipart_content_type::form),
        param()
    {
        this->headers = copy.headers;
    }
    
    sp_http_param param;
    
} multipart_content_form;

typedef struct multipart_content_file_t
    : public multipart_content
{
    multipart_content_file_t()
        : multipart_content(multipart_content_type::file),
        temp_path(), mime_type(""), size(0), fd(-1)
    {
    }

    multipart_content_file_t(const multipart_content& copy)
        : multipart_content(multipart_content_type::file),
        temp_path(), mime_type(""), size(0), fd(-1)
    {
        this->headers = copy.headers;
    }
    
    boost::filesystem::path temp_path;
    std::string mime_type;
    
    size_t size;
    int fd;
    
} multipart_content_file;

typedef struct multipart_content_mixed_t
    : public multipart_content
{
    multipart_content_mixed_t()
        : multipart_content(multipart_content_type::mixed),
        start_boundary(), end_boundary(), contents()
    {
    }

    multipart_content_mixed_t(const multipart_content& copy)
        : multipart_content(multipart_content_type::mixed),
        start_boundary(), end_boundary(), contents()
    {
        this->headers = copy.headers;
    }
    
    std::string start_boundary;
    std::string end_boundary;
    
    std::vector<std::shared_ptr<multipart_content>> contents;
    
} multipart_content_mixed;

typedef struct multipart_content_alternative_t
    : public multipart_content
{
    multipart_content_alternative_t()
        : multipart_content(multipart_content_type::alternative),
        start_boundary(), end_boundary(), contents()
    {
    }

    multipart_content_alternative_t(const multipart_content& copy)
        : multipart_content(multipart_content_type::alternative),
        start_boundary(), end_boundary(), contents()
    {
        this->headers = copy.headers;
    }
    
    std::string start_boundary;
    std::string end_boundary;
    
    std::vector<std::shared_ptr<multipart_content>> contents;
    
} multipart_content_alternative;

typedef struct multipart_content_digest_t
    : public multipart_content
{
    multipart_content_digest_t()
        : multipart_content(multipart_content_type::digest),
        start_boundary(), end_boundary(), contents()
    {
    }
    
    multipart_content_digest_t(const multipart_content& copy)
        : multipart_content(multipart_content_type::digest),
        start_boundary(), end_boundary(), contents()
    {
        this->headers = copy.headers;
    }
    
    std::string start_boundary;
    std::string end_boundary;
    
    std::vector<std::shared_ptr<multipart_content>> contents;
    
} multipart_content_digest;


typedef struct multipart_parser_t
{
    multipart_parser_t()
        : app(nullptr),
        state(multipart_parser_state::init),
        start_boundary(""), end_boundary(""),
        contents(),
        total_size(0), uploaded_size(0),
        buf(nullptr), current_content()
    {
    }
    
    mvc::app* app;
    
    multipart_parser_state state;
    
    std::string start_boundary;
    std::string end_boundary;
    
    std::vector<std::shared<multipart_content>> contents;
    
    uint64_t total_size;
    uint64_t uploaded_size;
    
    evbuf_t* buf;
    
    std::shared<multipart_content> current_content;
    
} multipart_parser;

bool is_multipart_data(evhtp_request_t* req, evhtp_headers_t* hdr)
{
    if(req->method != htp_method_POST)
        return false;
    
    evhtp_kv_t* header = nullptr;
    if((header = evhtp_headers_find_header(
        hdr, evmvc::to_string(evmvc::field::content_type).data()
        )
    ) != nullptr){
        std::vector<std::string> vals;
        boost::split(vals, header->val, boost::is_any_of(";"));
        for(auto& val : vals)
            if(evmvc::trim_copy(val) == "multipart/form-data")
                return true;
    }
    return false;
}

static std::string get_boundary(const std::string& hdr_val)
{
    std::vector<std::string> kvs;
    boost::split(kvs, hdr_val, boost::is_any_of(";"));
    for(auto& kv_val : kvs){
        std::vector<std::string> kv;
        auto tkv_val = evmvc::trim_copy(kv_val);
        boost::split(kv, tkv_val, boost::is_any_of("="));
        
        if(kv.size() != 2)
            continue;
        
        if(evmvc::trim_copy(kv[0]) == "boundary")
            return evmvc::trim_copy(kv[1]);
    }
    return "";
}

static std::string get_boundary(evhtp_headers_t* hdr)
{
    evhtp_kv_t* header = nullptr;
    if((header = evhtp_headers_find_header(
        hdr, evmvc::to_string(evmvc::field::content_type).data()
        )
    ) != nullptr){
        std::string val = get_boundary(header->val);
        if(!val.empty())
            return val;
    }
    
    std::cerr <<
        "Invalid multipart/form-data query, boundary wasn't found!\n";
    return "";
}

static uint64_t get_content_length(evhtp_headers_t* hdr)
{
    evhtp_kv_t* header = nullptr;
    if((header = evhtp_headers_find_header(
        hdr, evmvc::to_string(evmvc::field::content_length).data()
        )
    ) != nullptr)
        return evmvc::str_to_num<uint64_t>(header->val);
    
    return 0;
}

static bool parse_boundary_header(
    evmvc::_internal::multipart_parser* mp, char* hdr)
{
// Host: www.w3schools.com
// User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:60.0) Gecko/20100101 Firefox/60.0
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
// Accept-Language: en-US,en;q=0.8,fr-CA;q=0.5,fr;q=0.3
// Accept-Encoding: gzip, deflate, br
// Referer: https://www.w3schools.com/TAGs/tryit.asp?filename=tryhtml_form_enctype
// Content-Type: multipart/form-data; boundary=---------------------------1572261267569431463538505199
// Content-Length: 279
// Cookie: ASPSESSIONIDSSSRBABS=GFPMGCCAMPHMOPNBFBGAPCGD; ASPSESSIONIDQQQSBAAS=IGAEJFEALOACHACMNKAJAMDF; G_ENABLED_IDPS=google
// DNT: 1
// Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// 
// -----------------------------1572261267569431463538505199
// Content-Disposition: form-data; name="fname"
//
//
// -----------------------------1572261267569431463538505199
// Content-Disposition: form-data; name="lname"
//
//
// -----------------------------1572261267569431463538505199--


// The following example illustrates "multipart/form-data" encoding. 
// Suppose we have the following form:
//
//  <FORM action="http://server.com/cgi/handle"
//        enctype="multipart/form-data"
//        method="post">
//    <P>
//    What is your name? <INPUT type="text" name="submit-name"><BR>
//    What files are you sending? <INPUT type="file" name="files"><BR>
//    <INPUT type="submit" value="Send"> <INPUT type="reset">
//  </FORM>
//
// If the user enters "Larry" in the text input, and selects the 
// text file "file1.txt", the user agent might send back the following data:
//
//    Content-Type: multipart/form-data; boundary=AaB03x
//
//    --AaB03x
//    Content-Disposition: form-data; name="submit-name"
//
//    Larry
//    --AaB03x
//    Content-Disposition: form-data; name="files"; filename="file1.txt"
//    Content-Type: text/plain
//
//    ... contents of file1.txt ...
//    --AaB03x--


// If the user selected a second (image) file "file2.gif",
// the user agent might construct the parts as follows:
//
//    Content-Type: multipart/form-data; boundary=AaB03x
//
//    --AaB03x
//    Content-Disposition: form-data; name="submit-name"
//
//    Larry
//    --AaB03x
//    Content-Disposition: form-data; name="files"
//    Content-Type: multipart/mixed; boundary=BbC04y
//
//    --BbC04y
//    Content-Disposition: file; filename="file1.txt"
//    Content-Type: text/plain
//
//    ... contents of file1.txt ...
//    --BbC04y
//    Content-Disposition: file; filename="file2.gif"
//    Content-Type: image/gif
//    Content-Transfer-Encoding: binary
//
//    ...contents of file2.gif...
//    --BbC04y--
//    --AaB03x--
    
    std::string hdr_line(hdr);
    size_t col_idx = hdr_line.find_first_of(":");
    if(col_idx == std::string::npos){
        std::cerr << "Invalid boundary header format!";
        return false;
    }
    
    std::string hdr_name = hdr_line.substr(0, col_idx -1);
    
    std::string hdr_val = hdr_line.substr(col_idx +1);
    mp->current_content->params.emplace_back(
        std::make_shared<http_param>(
            hdr_name, hdr_val
        )
    );
}

bool assign_content_type(evmvc::_internal::multipart_parser* mb)
{
    auto ct = mb->current_content->get(
        evmvc::to_string(evmvc::field::content_type)
    );
    auto cd = mb->current_content->get(
        evmvc::to_string(evmvc::field::content_disposition)
    );
    
    if(ct){
        if(ct.get<std::string>().find("multipart/mixed") !=
            std::string::npos
        ){
            auto cc = std::make_shared<multipart_content_mixed>(
                mb->current_content
            );
            auto boundary = get_boundary(ct.get<std::string>());
            if(!boundary.empty()){
                cc->start_boundary = "--" + boundary;
                cc->end_boundary = "--" + boundary + "--";
            }
            mb->current_content = cc;
            ct.reset();
            
        }else if(ct.get<std::string>().find("multipart/alternative") !=
            std::string::npos
        ){
            auto cc = std::make_shared<multipart_content_alternative>(
                mb->current_content
            );
            auto boundary = get_boundary(ct.get<std::string>());
            if(!boundary.empty()){
                cc->start_boundary = "--" + boundary;
                cc->end_boundary = "--" + boundary + "--";
            }
            mb->current_content = cc;
            ct.reset();
            
        }else if(ct.get<std::string>().find("multipart/digest") !=
            std::string::npos
        ){
            auto cc = std::make_shared<multipart_content_digest>(
                mb->current_content
            );
            auto boundary = get_boundary(ct.get<std::string>());
            if(!boundary.empty()){
                cc->start_boundary = "--" + boundary;
                cc->end_boundary = "--" + boundary + "--";
            }
            mb->current_content = cc;
            ct.reset();
            
        }
        
    }
    
    
}

static evhtp_res on_read_multipart_data(
    evhtp_request_t *req, evbuf_t *buf, void *arg)
{
    auto mb = (evmvc::_internal::multipart_parser*)arg;
    if(mb->state == multipart_parser_state::failed)
        return EVHTP_RES_OK;
    
    size_t blen = evbuffer_get_length(buf);
    mb->uploaded_size += blen;
    evbuffer_add_buffer(mb->buf, buf);
    evbuffer_drain(buf, blen);
    
    int res = EVHTP_RES_OK;
    
    bool has_works = true;
    while(has_works){
        switch(mb->state){
            case multipart_parser_state::init:{
                int len;
                char* line = evbuffer_readln(
                    mb->buf, &len, EVBUFFER_EOL_CRLF
                );
                if(!line){
                    has_works = false;
                    break;
                }
                
                if(mb->start_boundary != line){
                    free(line);
                    res = EVHTP_RES_BADREQ;
                    goto cleanup;
                }
                // header boundary found
                mb->state = multipart_data_parser_state::headers;
                mb->current_content = std::make_shared<multipart_content>(
                    multipart_content_type::unset
                );
                free(line);
                break;
            }
            
            case multipart_parser_state::headers:{
                int len;
                char* line = evbuffer_readln(
                    mb->buf, &len, EVBUFFER_EOL_CRLF
                );
                if(line == nullptr){
                    has_works = false;
                    break;
                }
                
                if(len == 0){
                    // end of header part
                    free(line);
                    mb->state = multipart_parser_state::content;
                    
                    break;
                }
                
                if(!parse_boundary_header(mb, line)){
                    free(line);
                    res = EVHTP_RES_BADREQ;
                    goto cleanup;
                }
                
                free(line);
                break;
            }

            case multipart_parser_state::content:{
                
                
                break;
            }
            
        }
    }
    
cleanup:
    if(res != EVHTP_RES_OK){
        req->keepalive = 0;
        mb->state = multipart_data_parser_state::failed;
    }
    
    if(res == EVHTP_RES_BADREQ)
        send_error(req, res);
    else if(res == EVHTP_RES_BADREQ)
        send_error(req, res);
    
    return EVHTP_RES_OK;
}

static evhtp_res on_request_fini_multipart_data(
    evhtp_request_t *req, void *arg)
{
}

evhtp_res parse_multipart_data(
    evhtp_request_t* req, evhtp_headers_t* hdr, app* arg)
{
    std::string boundary = get_boundary(hdr);
    if(boundary.size() == 0)
        return EVHTP_RES_BADREQ;
    
    evmvc::_internal::multipart_parser* mb = 
        new evmvc::_internal::multipart_parser();
    
    mb->total_size = evmvc::_internal::get_content_length();
    mb->start_boundary = "--" + boundary;
    mb->end_boundary = "--" + boundary + "--";
    
    evhtp_set_hook(
        &req->hooks, evhtp_hook_on_read, 
        on_read_multipart_data, mb
    );
    evhtp_set_hook(
        &req->hooks, evhtp_hook_on_request_fini, 
        on_request_fini_multipart_data, mb
    );
    req->cbarg = mb;
    
    return EVHTP_RES_OK;
}


}}//ns evmvc::_internal
#endif //_libevmvc_multipart_parser_h
