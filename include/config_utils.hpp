/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:20:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/23 17:34:38 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <iostream>
#include <sstream>
#include <vector>

void error_exit(const std::string& msg);
bool is_valid_ip(const std::string& ip);
bool is_valid_port(const std::string& port);
void set_single_string(std::vector<std::string>& tokens,
				size_t& i, std::string& field, const std::string& directive_name);
void parse_root_wrapper(std::vector<std::string>& token, size_t& i, LocationContext& lc);
void parse_upload_store_wrapper(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);
void parse_index_wrapper(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);
void parse_allow_methods_wrapper(std::vector<std::string>& tokens, size_t& i, LocationContext& lc);

#endif
