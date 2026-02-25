/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:20:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/25 16:07:09 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>

void error_exit(const std::string& msg);
void check_ip_format(const std::string& ip);
long safe_strtol(const std::string& str, long min_val, long max_val);
void set_single_string(std::vector<std::string>& tokens,
				size_t& i, std::string& field, const std::string& directive_name);
void set_vector_string(std::vector<std::string>& tokens,
				size_t& i, std::vector<std::string> field, const std::string& directive_name);

#endif
