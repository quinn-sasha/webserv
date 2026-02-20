/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:18:29 by ikota             #+#    #+#             */
/*   Updated: 2026/02/20 16:19:52 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config_utils.hpp"

void error_exit(const std::string& msg) {
	std::cerr << "Error: Config file: " << msg << std::endl;
	std::exit(1);
}

bool is_valid_ip(const std::string& ip) {
	if (ip == "localhost") return true;

	int count_dots = 0;
	for (size_t i = 0; i < ip.size(); ++i) {
		if (ip[i] == '.')
			count_dots++;
	}
	if (count_dots != 3)
		return false;

	std::stringstream ss(ip);
	std::string segment;
	int count = 0;

	while (std::getline(ss, segment, '.')) {
		if (segment.empty()) {
			return false;
		}
		errno = 0;
		char* endptr;
		long val = std::strtol(segment.c_str(), &endptr, 10);
		if (errno == ERANGE || *endptr != '\0')
			return false;
		if (val < 0 || val > 255) {
			return false;
		}
		count++;
	}
	return count == 4;
}

bool is_valid_port(const std::string& port) {
	if (port.empty()) {
		return false;
	}
	char* endptr;
	errno = 0;
	long val = std::strtol(port.c_str(), &endptr, 10);
	if (errno == ERANGE || *endptr != '\0') {
		return false;
	}
	if (val < 0 || val > 65535) {
		return false;
	}
	return true;
}
