/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parse_server_config.hpp                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 14:48:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/24 16:03:38 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_PARSE_SERVER_CONFIG
#define INCLUDE_PARSE_SERVER_CONFIG

#include <iostream>
#include <sstream>
#include <vector>

#include "Config.hpp"
#include "config_utils.hpp"
#include "parse_location_directive.hpp"

void parse_listen_directive(std::vector<std::string>& tokens, size_t& i, ServerContext& sc);
void parse_server_name_directive(std::vector<std::string>& tokens, size_t&i, ServerContext& sc);
void parse_client_max_body_size_directive(std::vector<std::string>&tokens,
				size_t& i, ServerContext& sc);
void parse_server_root_directive(std::vector<std::string>& t, size_t& i, ServerContext& sc);
void parse_location_directive(std::vector<std::string>& tokens, size_t& i, ServerContext& sc);

#endif
