/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:20:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/20 16:19:43 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <iostream>
#include <sstream>

void error_exit(const std::string& msg);
bool is_valid_ip(const std::string& ip);
bool is_valid_port(const std::string& port);

#endif
