/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_location_directive.hpp                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 14:41:49 by ikota             #+#    #+#             */
/*   Updated: 2026/02/24 15:06:13 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_PARSE_LOCATION_DIRECTIVE
#define INCLUDE_PARSE_LOCATION_DIRECTIVE

#include <iostream>
#include <sstream>
#include <vector>

#include "Config.hpp"
#include "config_utils.hpp"

void parse_root_directive(std::vector<std::string>& token, size_t& i, LocationContext& lc);
void parse_upload_store_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);
void parse_index_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);
void parse_allow_methods_directive(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);
void parse_autoindex_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc);
void parse_return_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc);
void parse_cgi_extension_directive(std::vector<std::string>& tokens, size_t&i, LocationContext& lc);

#endif
