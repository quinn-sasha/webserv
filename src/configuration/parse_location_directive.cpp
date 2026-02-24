/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_location_directive.cpp                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 14:41:13 by ikota             #+#    #+#             */
/*   Updated: 2026/02/24 15:14:54 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "parse_location_directive.hpp"

void parse_root_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc) {
	set_single_string(tokens, i, lc.root, "root");
}

void parse_upload_store_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc) {
  set_single_string(tokens, i, lc.upload_store, "upload_store");
}

void parse_index_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc) {
	set_vector_string(tokens, i, lc.index, "index");
}

void parse_allow_methods_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc) {
	set_vector_string(tokens, i, lc.allow_methods, "allow_methods");
}

void parse_autoindex_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc) {
	if (i >= tokens.size() || tokens[i] == ";") {
		error_exit("autoindex needs a value (on/off)");
	}

	if (tokens[i] != "on" && tokens[i] != "off") {
    error_exit("autoindex must be 'on' or 'off'");
	}
	lc.autoindex = (tokens[i] == "on");
	i++;
	if (i >= tokens.size() || tokens[i++] != ";") {
		error_exit("Expected ';' after autoindex values");
	}
}

void parse_return_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc) {
	if (i >= tokens.size() || tokens[i] == ";")
		error_exit("return directive needs a status code");
	char* endptr;
	errno = 0;
	long val = std::strtol(tokens[i].c_str(), &endptr, 10);

	if (errno == ERANGE || *endptr != '\0'
		|| (val != 301 || val != 302 || val != 307 || val != 308)) {
		error_exit("Invalid or unsupported redirect status code: " + tokens[i]);
	}
	lc.redirect_status_code = val;
	i++;

	if (i < tokens.size() && tokens[i] != ";") {
		lc.redirect_url = tokens[i];
		i++;
	}

	if (lc.redirect_url.empty()) {
		error_exit("Return 3xx directive requires a redirection URL");
	}

	if (i >= tokens.size() || tokens[i++] != ";") {
		error_exit("Expected ';' after return values");
	}
}

void parse_cgi_extension_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc) {

}
