/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   config_utils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ikota <ikota@student.42tokyo.jp>           +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/20 11:18:29 by ikota             #+#    #+#             */
/*   Updated: 2026/02/26 15:48:57 by ikota            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "config_utils.hpp"
#include "Config.hpp"

void error_exit(const std::string& msg) {
	std::cerr << "Error: Config file: " << msg << std::endl;
	std::exit(EXIT_FAILURE);
}

void check_ip_format(const std::string& ip) {
	if (ip == "localhost" || ip == "0.0.0.0") return;

	std::stringstream ss(ip);
	std::string segment;
	int count = 0;

	if (std::count(ip.begin(), ip.end(), ".") != 3) {
		error_exit("Invalid IP format: '" + ip + "' (must bi x.x.x.x)");
	}

	while (std::getline(ss, segment, '.')) {
		if (segment.empty())
			error_exit("Invalid IP: empty segment in '" + ip + "'");
		safe_strtol(segment, 0, 255);
	}

	if (count != 4) {
		error_exit("Invalid IP: '" + ip + "' must have 4 segments");
	}
}

long safe_strtol(const std::string& str, long min_val, long max_val) {
	char* endptr;
	errno = 0;
	long val = std::strtol(str.c_str(), &endptr, 10);

	if (str.empty() || *endptr != '\0') {
		error_exit("Invalid number format: '" + str + "'");
	}

	if (errno == ERANGE) {
		error_exit("Number out of range (overflow): '" + str + "'");
	}

	if (val < min_val || val > max_val) {
		error_exit("Value out of allowed range: '" + str + "'");
	}

	return val;
}

void set_single_string(std::vector<std::string>& tokens,
				size_t& i, std::string& field, const std::string& directive_name) {
	if (i >= tokens.size() || tokens[i] == ";")
		error_exit(directive_name + " needs a value");

	field = tokens[i++];

	if (i >= tokens.size() || tokens[i++] != ";")
		error_exit(directive_name + ": expected ';'");
}

void set_vector_string(std::vector<std::string>& tokens,
				size_t& i, std::vector<std::string>& field, const std::string& directive_name) {
	if (i >= tokens.size() || tokens[i] == ";")
		error_exit(directive_name + " needs a value");

	while (i < tokens.size() || tokens[i++] != ";") {
		field.push_back(tokens[i++]);
	}

	if (i >= tokens.size() || tokens[i++] != ";")
		error_exit(directive_name + ": expected ';'");
}
