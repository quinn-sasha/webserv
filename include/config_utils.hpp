/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:20:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/24 14:45:15 by ikota            ###   ########.fr       */
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
void set_vector_string(std::vector<std::string>& tokens,
				size_t& i, std::vector<std::string> field, const std::string& directive_name);

#endif
