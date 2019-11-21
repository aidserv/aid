// Copyright 2016 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include "emugl/common/stringparsing.h"

#include "ShaderValidator.h"

// TODO: Improve parsing/analysis
// https://code.google.com/p/android/issues/detail?id=206951

static std::vector<std::string> parse_raw_declarations(std::string text) {
    std::string emp("");
    std::string sp(" ");

    std::string newline("\n");
    std::string scol(";");

    std::string openpar("(");
    std::string openparsp("( ");
    std::string closepar(")");
    std::string openbr("{");
    std::string closebr("}");
    std::string brscol("};");

    std::string linecomment("//");
    std::string multilinecomment_begin("/*");
    std::string multilinecomment_end("*/");

    text.insert(0, sp);

    bool err = false;
    std::string preprocessed = replace_with(newline, sp,
                               replace_with(openpar, openparsp,
                               replace_with(closebr, brscol,
                               remove_in_out(
                               remove_in_out(
                                   text,
                                   multilinecomment_begin,
                                   multilinecomment_end,
                                   &err),
                                   linecomment,
                                   newline,
                                   &err))));

    // Comment parsing is allowed to fail
    err = false;

    std::vector<std::string> decls;
    std::string no_func_impls = remove_in_out(preprocessed,
            openbr,
            closebr,
            &err);

    if (!err) {
        decls = split(scol, no_func_impls);
    }
    return decls;
}

// Validates arrangement of keywords in
// a GLSL ES variable declaration or function parameter.
// With the given parsing functions, this is only designed to pass
// dEQP-GLES2.shaders.qualification_order.*
static void validate_keywords_in_decls(const std::vector<std::string>& decls,
                                       bool* err) {
    std::vector<std::string> variance_keywords;

    variance_keywords.push_back(std::string("invariant"));

    std::vector<std::string> storage_keywords;

    storage_keywords.push_back(std::string(" const "));
    storage_keywords.push_back(std::string(" uniform "));
    storage_keywords.push_back(std::string(" attribute "));
    storage_keywords.push_back(std::string(" varying "));
    storage_keywords.push_back(std::string(" sampler "));

    std::vector<std::string> precision_keywords;

    precision_keywords.push_back(std::string(" lowp "));
    precision_keywords.push_back(std::string(" mediump "));
    precision_keywords.push_back(std::string(" highp "));
    precision_keywords.push_back(std::string(" superp "));

    std::vector<std::string> parameter_keywords;

    parameter_keywords.push_back(std::string(" in ")); // c.f., "int"
    parameter_keywords.push_back(std::string(" out "));
    parameter_keywords.push_back(std::string(" inout "));

    for (size_t i = 0; i < decls.size(); i++) {
        std::string variance, storage, precision, parameter;
        size_t storage_pos, precision_pos, parameter_pos;

        storage_pos = multi_find(decls[i], storage_keywords);
        precision_pos = multi_find(decls[i], precision_keywords);
        parameter_pos = multi_find(decls[i], parameter_keywords);

        if (storage_pos != std::string::npos &&
                precision_pos != std::string::npos) {
            if (storage_pos < precision_pos) {
                if (parameter_pos != std::string::npos) {
                    if (parameter_pos > precision_pos) {
                        *err = true;
                    }
                }
            } else {
                *err = true;
            }
        } else if (precision_pos != std::string::npos &&
                parameter_pos != std::string::npos) {
            if (parameter_pos > precision_pos) {
                *err = true;
            }
        }
    }
}

static void validate_glsles_variable_decls(
        const std::vector<std::string>& raw_decls,
        bool* err) {
    std::string openpar("(");
    std::vector<std::string> non_function_decls;
    for (size_t i = 0; i < raw_decls.size(); i++) {
        if (raw_decls[i].find(openpar) == std::string::npos) {
            non_function_decls.push_back(raw_decls[i]);
        }
    }
    validate_keywords_in_decls(non_function_decls, err);
}

static void validate_glsles_function_parameters(
        const std::vector<std::string>& raw_decls,
        bool* err) {

    bool parse_err = false;
    std::string openpar("(");
    std::string closepar(")");
    std::string comma(",");
    std::vector<std::string> func_param_decls;
    for (size_t i = 0; i < raw_decls.size(); i++) {
        if (raw_decls[i].find(openpar) != std::string::npos) {
            std::vector<std::string> func_params =
                isolate_in_out(raw_decls[i], openpar, closepar, &parse_err);
            if (func_params.size() > 0) {
                // Just use the first set of parens
                std::vector<std::string> these_params =
                    split(comma, func_params[0]);
                func_param_decls.insert(func_param_decls.end(),
                                        these_params.begin(),
                                        these_params.end());
            }
        }
    }

    validate_keywords_in_decls(func_param_decls, err);
}

bool validate_glsles_keywords(const char* src) {
    bool err = false;

    std::vector<std::string> raw_decls = parse_raw_declarations(src);

    // Liberal validation if parsing fails
    if (raw_decls.empty()) { return true; }

    validate_glsles_variable_decls(raw_decls, &err);
    validate_glsles_function_parameters(raw_decls, &err);
    return !err;
}
