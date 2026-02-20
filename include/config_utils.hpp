/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:20:47 by ikota             #+#    #+#             */
/*   Updated: 2026/02/20 11:20:47 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef INCLUDE_CONFIG_UTILS_HPP_
#define INCLUDE_CONFIG_UTILS_HPP_

#include <iostream>

void error_exit(const std::string& msg);
bool is_all_digits(const std::string& str);

#endif